#ifndef SR_RDT_SENDER_H
#define SR_RDT_SENDER_H
#include "RdtSender.h"
class SRRdtSender :public RdtSender
{
private:
	int NextSeqNum;					// ��һ���������
	int N;							//���ڳ���
	int base;						//�����
	Packet* packetWaitingAck;		//�ѷ��Ͳ��ȴ�Ack�����ݰ�
	int win_ptr;					//�����б�ǣ������ȡ��ָ�������������һ��δ�����õ�λ��
	bool WaitingState;

public:
	bool getWaitingState();
	bool send(Message &message);						//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(Packet &ackPkt);						//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkServiceSimulator����

public:
	SRRdtSender();
	virtual ~SRRdtSender();
};

#endif
