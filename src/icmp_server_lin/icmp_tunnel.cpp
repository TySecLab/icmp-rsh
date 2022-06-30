#include "icmp_tunnel.h"

CIcmp::CIcmp()
{
    m_init = socket_env_init();
}

CIcmp::~CIcmp()
{
    if (m_init)
    {
        socket_env_free();
    }
}

bool CIcmp::socket_env_init()
{
#ifdef _WIN32
    WSADATA wsaData;
    int n_ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    return n_ret == 0 ? true : false;
#else
    if (setuid(0) == 0)
        //ignore SIGPIPE 
        return signal(SIGPIPE, SIG_IGN) == SIG_ERR ? false : true;
    else
        return false;
#endif
}

void CIcmp::socket_env_free()
{
#ifdef _WIN32
    WSACleanup();
#endif // _WIN32
}

void CIcmp::socket_clsoe(MY_SOCKET& fd)
{
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif // _WIN32
    fd = -1;
}

MY_SOCKET CIcmp::create_raw_socket(int protocol)
{
    MY_SOCKET fd = socket(AF_INET, SOCK_RAW, protocol);
    return fd;
}

u_short CIcmp::checksum(unsigned short* addr, int len)
{
    int             nleft = len;
    int             sum = 0;
    unsigned short* w = addr;
    unsigned short  answer = 0;

    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    /* 4mop up an odd byte, if necessary */
    if (nleft == 1)
    {
        *(unsigned char*)(&answer) = *(unsigned char*)w;
        sum += answer;
    }

    /* 4add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);                 /* add carry */
    answer = ~sum;                      /* truncate to 16 bits */
    return(answer);
}
