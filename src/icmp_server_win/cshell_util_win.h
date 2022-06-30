#pragma once
#include <Windows.h>
#include <functional>
#include <VersionHelpers.h>

class cshell_util
{
public:
	cshell_util();
	~cshell_util();

	int create_shell();
	int write_shell_cmd(char* data,int size);

	//send cb
	std::function<int(short cmd, char* data, int size)> m_send_cb = nullptr;

	std::function<int(int acp,char* data, int size)> m_read_result_cb = nullptr;

private:
	static DWORD WINAPI read_cmd_result_cb(LPVOID lparam);

	int create_shell_for_win7();
	int create_shell_for_win8_or_greater();

	int m_running = 0;              //���б�־
	int m_win8_or_greater = 0;      //win8������ϵͳ

	HWND m_h_console = nullptr;     //cmd���ھ��
	HANDLE m_std_input = nullptr;   //cmd��׼������
	HANDLE m_std_out = nullptr;     //cmd��׼������
	HANDLE m_std_error = nullptr;   //cmd��׼������Ϣ���

	HANDLE m_read_pipe = nullptr;   //�ܵ������
	HANDLE m_write_pipe = nullptr;  //�ܵ�д���

	HANDLE m_cmd = nullptr;         //cmd���̾��
	HANDLE m_thd = nullptr;         //�߳̾��
};

