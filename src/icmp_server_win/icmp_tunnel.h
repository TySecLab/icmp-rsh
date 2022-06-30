#pragma once
//for windows
#ifdef _WIN32
#include <WinSock2.h>
#include <cstdint>
#pragma comment(lib, "ws2_32.lib") 
#pragma warning(disable:4996)
typedef SOCKET MY_SOCKET;

#pragma pack(1)
//ip header
struct iphdr
{
    unsigned char ipv4_ver_hl;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};

//icmp header
struct icmphdr
{
    uint8_t type;		/* message type */
    uint8_t code;		/* type sub-code */
    uint16_t checksum;
    union
    {
        struct
        {
            uint16_t	id;
            uint16_t	sequence;
        } echo;			/* echo datagram */
        uint32_t	gateway;	/* gateway address */
        struct
        {
            uint16_t	__glibc_reserved;
            uint16_t	mtu;
        } frag;			/* path mtu discovery */
    } un;
};
#pragma pack()

//for linux
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
typedef int MY_SOCKET;
#endif

#define ICMP_ID  65535

class CIcmp
{
public:
    CIcmp();
    ~CIcmp();

protected:
    //init flag
    bool m_init = false;

    //socket env init
    bool socket_env_init();

    //socket env free
    void socket_env_free();

    //close socket 
    void socket_clsoe(MY_SOCKET& fd);

    //create raw socket for icmp
    MY_SOCKET create_raw_socket(int protocol = IPPROTO_ICMP);

    //This function is taken from the public domain version of Ping
    u_short checksum(unsigned short* addr, int len);
};
