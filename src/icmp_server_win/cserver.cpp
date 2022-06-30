#include "cserver.h"
#include <WS2tcpip.h>
#include <mstcpip.h>

CServer::CServer()
{
	if (m_init)
	{
		if (get_local_ip() < 0)
		{
			return;
		}

		srand(time(NULL));
		m_srv_fd = create_raw_socket();

		sockaddr_in sin = { 0 };
		sin.sin_family = AF_INET;
		if (inet_pton(AF_INET, m_local_ip, &sin.sin_addr) <= 0)
		{
			printf("[x]inet_pton error.\n");
			return;
		}

		int ret = bind(m_srv_fd, (sockaddr*)&sin, sizeof(sin));
		if (ret < 0)
		{
			printf("[x]bind error.\n");
			return;
		}

		RCVALL_VALUE emRcvallValue = RCVALL_IPLEVEL;
		DWORD dwBytesReturned = 0;
		if (SOCKET_ERROR == WSAIoctl(m_srv_fd, SIO_RCVALL, &emRcvallValue, sizeof(emRcvallValue), NULL, 0, &dwBytesReturned, NULL, NULL))
		{
			return;
		}

		srv_recv();
	}
}

CServer::~CServer()
{
	if (m_session != nullptr)
	{
		delete m_session;
	}
}

int CServer::srv_recv()
{
	char buff[1500] = { 0 };
	sockaddr_in sin = { 0 };

	while (m_rcv_running)
	{
		memset(buff,0,sizeof(buff));
		memset(&sin, 0, sizeof(sin));
		int len = sizeof(sin);

		int rcv_size = recvfrom(m_srv_fd, buff, sizeof(buff), 0, (sockaddr*)&sin, &len);
		if (rcv_size <= 0)
		{
			//error
			printf("[x]recvfrom exit.\n");
			m_rcv_running = 0;
			break;
		}
		else
		{
			printf("[!]recvfrom size:%d\n", rcv_size);

			//解包数据
			resolve_pack(&sin, buff, rcv_size);
		}	
	}
	return 0;
}

int CServer::srv_send(sockaddr_in* sin, char* data, int size)
{
	m_send_lock.lock();

	int len = sizeof(icmphdr) + size;
	char* p_datagram = new char[len]();

	icmphdr* p_icmphdr = (icmphdr*)p_datagram;
	p_icmphdr->type = 0;
	p_icmphdr->code = 0;
	p_icmphdr->un.echo.id = ICMP_ID;
	p_icmphdr->un.echo.sequence = 1;

	char* p_data = p_datagram + sizeof(icmphdr);
	memcpy(p_data, data, size);

	p_icmphdr->checksum = checksum((u_short*)p_datagram, len);

	int sock_len = sizeof(sockaddr_in);
	int ret = sendto(m_srv_fd, p_datagram, len, 0, (sockaddr*)sin, sock_len);

	delete[] p_datagram;
	m_send_lock.unlock();
	return ret;
}

int CServer::resolve_pack(sockaddr_in* sin, char* data, int size)
{
	//size check
	if (size < sizeof(iphdr) + sizeof(icmphdr) + sizeof(MY_TRANS_HEADER))
	{
		return -1;
	}

	icmphdr* p_hdr = (icmphdr*)(data + sizeof(iphdr));
	if (p_hdr->type != 8)
	{
		return -1;
	}

	//user data
	char* p_user_data = data + sizeof(iphdr) + sizeof(icmphdr);

	//trans header
	MY_TRANS_HEADER* p_header = (MY_TRANS_HEADER*)p_user_data;

	//client握手请求
	if (p_header->cmd == CLIENT_REQ_SHAKE_HANDS)
	{
		printf("[!]client shake hands req.\n");

		//server握手回复
		p_header->cmd = SERVER_REP_SHAKE_HANDS;
		p_header->pack_size = sizeof(MY_TRANS_HEADER);
		p_header->sid = rand();
		srv_send(sin,p_user_data, p_header->pack_size);
	}
	//client握手确认
	else if (p_header->cmd == CLIENT_ACK_SHAKE_HANDS)
	{
		printf("[!]client shake hands ack.\n");

		//生成会话
		m_session = new CSession;

		//设置会话属性
		m_session->m_sid = p_header->sid;
		memcpy(&m_session->m_sin, sin, sizeof(sockaddr_in));
		m_session->m_send_cb = std::bind(&CServer::srv_send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		m_session->m_delete_cb = std::bind(&CServer::remove_session, this, std::placeholders::_1);

		//保存会话到容器
		m_map_session[m_session->m_sid] = m_session;
	}
	//交互数据
	else if(p_header->cmd == CLIENT_REQ_COMMAND)
	{
		//发送到对应会话
		auto iter = m_map_session.find(p_header->sid);
		if(iter != m_map_session.end())
		{
			std::string tmp(p_user_data, p_header->pack_size);
			iter->second->m_list_recv_pack.push_back(tmp);
		}
	}
	return 0;
}

int CServer::remove_session(short sid)
{
	//找到对应会话
	auto iter = m_map_session.find(sid);
	if (iter != m_map_session.end())
	{
		delete iter->second;
		m_map_session.erase(sid);
	}
	return 0;
}

int CServer::get_local_ip()
{
	char host_name[256] = { 0 };
	if (gethostname(host_name, 256) == SOCKET_ERROR)
	{
		return -1;
	}
	struct hostent* phe = gethostbyname(host_name);
	if (phe == NULL)
	{
		return -1;
	}
	if (phe->h_addr_list[0] == 0)
	{
		return -1;
	}

	struct in_addr addr;
	memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr));
	char* ip = inet_ntoa(addr);
	if (ip == NULL)
	{
		return -1;
	}

	strcpy(m_local_ip, ip);
	return 0;
}
