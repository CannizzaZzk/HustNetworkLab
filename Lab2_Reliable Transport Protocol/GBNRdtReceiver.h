#ifndef GBN_RDT_RECEIVER_H
#define GBN_RDT_RECEIVER_H
#include "RdtReceiver.h"
class GBNRdtReceiver :public RdtReceiver
{
private:
	int NextSeqNum;	// �ڴ��յ�����һ���������
	Packet AckPkt;	//ȷ�ϱ���

public:
	GBNRdtReceiver();
	virtual ~GBNRdtReceiver();

public:

	void receive(Packet &packet);	//���ձ��ģ�����NetworkService����
};

#endif
