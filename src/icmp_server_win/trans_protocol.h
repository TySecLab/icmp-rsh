#ifndef _TRANS_PROTOCOL_H
#define _TRANS_PROTOCOL_H

enum MY_TRANS_CMD
{
	CLIENT_REQ_SHAKE_HANDS,  //client��������
	SERVER_REP_SHAKE_HANDS,  //server���ֻظ�
	CLIENT_ACK_SHAKE_HANDS,  //client����ȷ��

	CLIENT_REQ_COMMAND,      //client����
	SERVER_REP_RESULT,       //server�ظ����
};

#define MAX_PACK_SIZE     1350  //��������ݴ�С
#define DEFAULT_SEND_PACK 4     //Ĭ�Ϸ�������

#pragma pack(1)
typedef struct _MY_TRANS_HEADER
{
	short  pack_size;              //��С = header + data
	short  sid;                    //�Ự��־
	short  cmd;                    //����
	short  max_send_pack;          //һ���Է�����
	short  send_pack_seq;          //���Ͱ����: �� 1 ��ʼ
	short  recv_pack_ack;          //���հ�ȷ�Ϻ�: �� 1 ��ʼ
	short  pack_count;             //�����
	time_t time_stamp;             //ʱ���
	//char   data[MAX_PACK_SIZE];  //����
}MY_TRANS_HEADER;
#pragma pack()

#endif // !_TRANS_PROTOCOL_H
