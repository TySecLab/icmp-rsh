#include "cclient.h"

CClient::CClient(char* ip)
{
	m_clt_fd = create_raw_socket();
	m_sin.sin_family = AF_INET;
	if (inet_pton(AF_INET, ip, &m_sin.sin_addr) <= 0)
	{
		return;
	}
	else
	{
		//请求登录
		req_login();

		//RCVALL_VALUE emRcvallValue = RCVALL_IPLEVEL;
		//DWORD dwBytesReturned = 0;
		//if (SOCKET_ERROR == WSAIoctl(m_clt_fd, SIO_RCVALL, &emRcvallValue, sizeof(emRcvallValue), NULL, 0, &dwBytesReturned, NULL, NULL))
		//{
		//	int n = WSAGetLastError();
		//	return;
		//}

		//cape
		m_cape = new Cape((unsigned char*)m_key, 32, 36);

		//recv
		m_thd_recv = new std::thread(tfun_recv_cb, this);
		m_thd_recv->join();
	}
}

CClient::~CClient()
{
	m_rcv_running = 0;
	socket_clsoe(m_clt_fd);

	delete m_thd_recv;
	delete m_thd_input;
}

int CClient::req_login()
{
	MY_TRANS_HEADER header = { 0 };
	header.cmd = CLIENT_REQ_SHAKE_HANDS;
	header.pack_size = sizeof(MY_TRANS_HEADER);
	return clt_send(&m_sin, (char*)&header, sizeof(MY_TRANS_HEADER));
}

int CClient::clt_send(sockaddr_in* sin, char* data, int size)
{
	m_send_lock.lock();

	int len = sizeof(icmphdr) + size;
	char* p_datagram = new char[len]();

	icmphdr* p_icmphdr = (icmphdr*)p_datagram;

	p_icmphdr->type = 8;
	p_icmphdr->code = 0;
	p_icmphdr->un.echo.id = ICMP_ID;
	p_icmphdr->un.echo.sequence = 1;

	char* p_data = p_datagram + sizeof(icmphdr);
	memcpy(p_data, data, size);

	p_icmphdr->checksum = checksum((u_short*)p_datagram, len);

	int n_sock_len = sizeof(sockaddr_in);
	int ret = sendto(m_clt_fd, p_datagram, len, 0, (sockaddr*)sin, n_sock_len);

	delete[] p_datagram;
	m_send_lock.unlock();

	return ret;
}

void CClient::tfun_recv_cb(void* param)
{
	CClient* p_this = (CClient*)param;
	char buff[1500] = { 0 };
	sockaddr_in sin = { 0 };

	while (p_this->m_rcv_running)
	{
		memset(buff, 0, sizeof(buff));
		memset(&sin, 0, sizeof(sin));
		socklen_t len = sizeof(sin);

		int rcv_size = recvfrom(p_this->m_clt_fd, buff, sizeof(buff), 0, (sockaddr*)&sin, &len);
		if (rcv_size <= 0)
		{
			//error
			printf("[x]recvfrom exit.\n");
			p_this->m_rcv_running = 0;
			break;
		}
		else
		{
			//printf("[!]recvfrom size:%d\n", rcv_size);

			//解包数据
			p_this->resolve_pack(&sin, buff, rcv_size);
		}
	}
}

int CClient::resolve_pack(sockaddr_in* sin, char* data, int size)
{
	//size check
	if (size < sizeof(iphdr) + sizeof(icmphdr) + sizeof(MY_TRANS_HEADER))
	{
		return -1;
	}

	icmphdr* p_hdr = (icmphdr*)(data + sizeof(iphdr));
	//if (p_hdr->type != 0)
	//{
	//	return -1;
	//}

	//user data
	char* p_pack_data = data + sizeof(iphdr) + sizeof(icmphdr);

	//trans header
	MY_TRANS_HEADER* p_header = (MY_TRANS_HEADER*)p_pack_data;

	//client握手请求
	if (p_header->cmd == SERVER_REP_SHAKE_HANDS)
	{
		if (m_sid == 0)
		{
			printf("[!]connection successful.\n");
			printf("[!]current session id: %d.\n\n", p_header->sid);

			//保存sid/sockaddr
			m_sid = p_header->sid;
			memcpy(&m_sin, sin, sizeof(m_sin));

			//server握手回复
			p_header->cmd = CLIENT_ACK_SHAKE_HANDS;
			p_header->pack_size = sizeof(MY_TRANS_HEADER);

			//发送确认包
			clt_send(sin, p_pack_data, p_header->pack_size);

			//创建输入线程
			m_thd_input = new std::thread(tfun_get_input_cb, this);
			m_thd_input->detach();
		}		
	}
	
	//交互数据
	else if(p_header->cmd == SERVER_REP_RESULT)
	{
		if (p_header->sid == m_sid)
		{
			char* p_user_data = p_pack_data + sizeof(MY_TRANS_HEADER);
			int data_size = p_header->pack_size - sizeof(MY_TRANS_HEADER);

			char* dec_buff = new char[data_size]();
			m_cape->decrypt((unsigned char*)p_user_data, (unsigned char*)dec_buff, data_size);

			printf("%s", dec_buff);
			delete[] dec_buff;
		}	
	}
	return 0;
}

void CClient::tfun_get_input_cb(void* param)
{
	CClient* p_this = (CClient*)param;

	//获取输入
	char input[1024] = { 0 };
	int loop = 0;
	while (p_this->m_rcv_running)
	{
		std::cin.getline(input, sizeof(input) - 1);
		if (strlen(input) > 0)
		{
			if (strcmp(input, "exit") == 0)
			{
				loop = 1;
			}
			else
			{
				strcat(input, "\n");
			}

			//加密数据
			char* enc_buff = new char[strlen(input) + 1]();
			p_this->m_cape->encrypt((unsigned char*)input, strlen(input), (unsigned char*)enc_buff, rand());

			p_this->pack_send(CLIENT_REQ_COMMAND, enc_buff, strlen(input) + 1);
			delete[] enc_buff;

			if (loop)
			{
				//exit
				p_this->m_rcv_running = 0;
				break;
			}
		}
	}
}

int CClient::pack_send(short cmd, char* data, int size)
{
	char* pack = new char[sizeof(MY_TRANS_HEADER) + size]();

	MY_TRANS_HEADER* p_hdr = (MY_TRANS_HEADER*)pack;
	p_hdr->cmd = cmd;
	p_hdr->pack_size = sizeof(MY_TRANS_HEADER) + size;
	p_hdr->sid = m_sid;

	memcpy(pack + sizeof(MY_TRANS_HEADER), data, size);

	clt_send(&m_sin, pack, p_hdr->pack_size);

	delete[] pack;
	return 0;
}
