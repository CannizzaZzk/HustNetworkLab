#include "stdafx.h"
#include "RdtReceiver.h"

class TCPRdtReceiver :public RdtReceiver {
private:
	int expectSequenceNumberRcvd;	// 期待收到的下一个报文序号
	Packet lastAckPkt;				//上次发送的确认报文
public:
	TCPRdtReceiver();
	virtual ~TCPRdtReceiver();
	void receive(Packet &packet);	//接收报文，将被NetworkService调用
};
