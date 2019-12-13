#ifndef SR_RDT_SENDER_H
#define SR_RDT_SENDER_H
#include "RdtSender.h"
class SRRdtSender :public RdtSender
{
private:
	int NextSeqNum;					// 下一个发送序号
	int N;							//窗口长度
	int base;						//基序号
	Packet* packetWaitingAck;		//已发送并等待Ack的数据包
	int win_ptr;					//数组中标记，方便存取，指向的是数组中下一个未被利用的位置
	bool WaitingState;

public:
	bool getWaitingState();
	bool send(Message &message);						//发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(Packet &ackPkt);						//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);					//Timeout handler，将被NetworkServiceSimulator调用

public:
	SRRdtSender();
	virtual ~SRRdtSender();
};

#endif
