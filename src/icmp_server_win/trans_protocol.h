#ifndef _TRANS_PROTOCOL_H
#define _TRANS_PROTOCOL_H

enum MY_TRANS_CMD
{
	CLIENT_REQ_SHAKE_HANDS,  //client握手请求
	SERVER_REP_SHAKE_HANDS,  //server握手回复
	CLIENT_ACK_SHAKE_HANDS,  //client握手确认

	CLIENT_REQ_COMMAND,      //client命令
	SERVER_REP_RESULT,       //server回复结果
};

#define MAX_PACK_SIZE     1350  //包最大数据大小
#define DEFAULT_SEND_PACK 4     //默认发送数量

#pragma pack(1)
typedef struct _MY_TRANS_HEADER
{
	short  pack_size;              //大小 = header + data
	short  sid;                    //会话标志
	short  cmd;                    //命令
	short  max_send_pack;          //一次性发包数
	short  send_pack_seq;          //发送包序号: 从 1 开始
	short  recv_pack_ack;          //接收包确认号: 从 1 开始
	short  pack_count;             //封包数
	time_t time_stamp;             //时间戳
	//char   data[MAX_PACK_SIZE];  //数据
}MY_TRANS_HEADER;
#pragma pack()

#endif // !_TRANS_PROTOCOL_H
