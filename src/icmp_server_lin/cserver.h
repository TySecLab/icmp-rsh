#include <map>
#include "csession.h"
#include <mutex>

class CServer : public CIcmp
{
public:
	CServer();
	~CServer();

private:
	MY_SOCKET m_srv_fd = -1;
	int m_rcv_running = 1;
	CSession* m_session = nullptr;
	std::map<short, CSession*> m_map_session;

private:
	//send lock
	std::mutex m_send_lock;

	//接收数据
	int srv_recv();

	//发送数据
	int srv_send(sockaddr_in* sin,char* data, int size);

	//数据解包
	int resolve_pack(sockaddr_in* sin,char* data,int size);

	//remove session
	int remove_session(short sid);
};

