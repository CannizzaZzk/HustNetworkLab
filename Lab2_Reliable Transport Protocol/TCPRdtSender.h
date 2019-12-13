#include "stdafx.h"
#include "RdtSender.h"

class TCPRdtSender :public RdtSender {
private:
	int expectSequenceNumberSend;	// 下一个发送序号 
	bool waitingState;				// 是否处于等待Ack的状态
	Packet *pPacketWaitingAck;		// 已发送并等待Ack的数据包
	// 以下变量用于维护滑动窗口；
	// 当滑动窗口连续时，base < nowlMax = rMax;
	// 当滑动窗口不连续时，base < rMax，0 < nowlMax < base;
	// 滑动窗口序列 从base开始, base、base+1、... 、0、... 、lMax-1
	int windowN;					// 滑动窗口大小
	int base;						// 当前窗口的序号的基址
	int rMax;						// 窗口序号上界，即最大序号为rMax-1
	int nowlMax;					// 滑动窗口右边界，最大为rMax，最小为1

	int countACK;					// 重复ack计数

public:
	bool getWaitingState();
	bool send(Message &message);
	void receive(Packet &ackPkt);	//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);//Timeout handler，将被NetworkServiceSimulator调用

	TCPRdtSender();
	virtual ~TCPRdtSender();
};