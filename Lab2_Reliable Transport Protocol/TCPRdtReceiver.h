#include "stdafx.h"
#include "RdtReceiver.h"

class TCPRdtReceiver :public RdtReceiver {
private:
	int expectSequenceNumberRcvd;	// �ڴ��յ�����һ���������
	Packet lastAckPkt;				//�ϴη��͵�ȷ�ϱ���
public:
	TCPRdtReceiver();
	virtual ~TCPRdtReceiver();
	void receive(Packet &packet);	//���ձ��ģ�����NetworkService����
};
