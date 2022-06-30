#pragma once
#include <functional>
#include <thread>

class CShellUtil
{
public:
	CShellUtil();
	~CShellUtil();

	// ‰»Î√¸¡Ó
	int write_shell_cmd(char* data,int size);

	//send cb
	std::function<int(short cmd,char* data, int size)> m_send_cb = nullptr;

private:
	//create shell
	static void create_shell(void* param);

	int m_running = 1;
	pid_t m_pid = -1;
	int m_master = -1;
	std::thread* m_thd_shell = nullptr;
};

