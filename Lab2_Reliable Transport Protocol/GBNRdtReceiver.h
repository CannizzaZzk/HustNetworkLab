#ifndef GBN_RDT_RECEIVER_H
#define GBN_RDT_RECEIVER_H
#include "RdtReceiver.h"
class GBNRdtReceiver :public RdtReceiver
{
private:
	int NextSeqNum;	// 期待收到的下一个报文序号
	Packet AckPkt;	//确认报文

public:
	GBNRdtReceiver();
	virtual ~GBNRdtReceiver();

public:

	void receive(Packet &packet);	//接收报文，将被NetworkService调用
};

#endif
