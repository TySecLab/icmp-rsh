#pragma once
#include <list>
#include <string>
#include <thread>
#include <mutex>
#include <functional>
#include "trans_protocol.h"
#include "cshell_util_win.h"
#include "cape.h"

class CSession
{
public:
	CSession();
	~CSession();

	//»á»°id
	short m_sid;

	//shell
	cshell_util* m_shell = nullptr;

	//list recv pack
	std::list<std::string> m_list_recv_pack;

	//list send data
	std::list<std::string> m_list_send_pack;

	//send cb
	std::function<int (sockaddr_in* sin, char* data, int size)> m_send_cb = nullptr;

	//send cb
	std::function<int(short sid)> m_delete_cb = nullptr;

	//sockaddr
	sockaddr_in m_sin = { 0 };

private:
	//cape key
	char m_key[32] = "A565EEB5780142AF28F4F7083829FFA";

	//cape
	Cape* m_cape = nullptr;

	//thread
	std::thread* m_thd_resolve_recv = nullptr;

	//thread
	std::thread* m_thd_resolve_send = nullptr;

	//thread run flag
	int m_thd_running = 1;

	//string data buff
	std::string m_str_buff;

	//resolve recv
	static void tfun_resolve_recv_cb(void* param);

	//resolve send
	static void tfun_resolve_send_cb(void* param);

	//recv pack
	int recv_pack(char* pack, int size);

	//send pack
	int send_pack(char* data, int size);

	//append pack list send
	int slice_data_to_pack(short cmd, char* data, int size);
};

