#ifndef SR_RDT_RECEIVER_H
#define SR_RDT_RECEIVER_H
#include "RdtReceiver.h"
class SRRdtReceiver :public RdtReceiver
{
private:
	int NextSeqNum;					// �ڴ��յ�����һ���������
	Packet AckPkt;					//ȷ�ϱ���
	int base;						//�����
	int N;							//���ڴ�С
	Message* MsgWaiting;			//�ѽ��ղ��ȴ��ϴ�Ӧ�ò�����ݰ�
	int win_ptr;					//�����б�ǣ������ȡ��ָ�������������һ��δ�����õ�λ��
	int* record;
public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();

public:

	void receive(Packet &packet);	//���ձ��ģ�����NetworkService����
};

#endif
#pragma once
