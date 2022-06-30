#pragma once
#include "trans_protocol.h"
#include <mutex>
#include "cape.h"

class CClient : public CIcmp
{
public:
	CClient(char* ip);
	~CClient();

private:
	MY_SOCKET m_clt_fd = -1;
	std::mutex m_send_lock;
	sockaddr_in m_sin = { 0 };
	int m_rcv_running = 1;
	std::thread* m_thd_recv = nullptr;
	std::thread* m_thd_input = nullptr;
	short m_sid = 0;
	Cape* m_cape = nullptr;
	char m_key[32] = "A565EEB5780142AF28F4F7083829FFA";

private:
	int req_login();
	int clt_send(sockaddr_in* sin, char* data, int size);
	static void tfun_recv_cb(void* param);
	int resolve_pack(sockaddr_in* sin, char* data, int size);
	static void tfun_get_input_cb(void* param);
	int pack_send(short cmd,char* data, int size);
};

