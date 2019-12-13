#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

//======================ȫ�ֱ���====================


//=================================================

SRRdtSender::SRRdtSender() :NextSeqNum(0), N(4), base(0), WaitingState(false)
{
	packetWaitingAck = new Packet[N];
	win_ptr = 0;
}


bool SRRdtSender::getWaitingState() {

	return WaitingState;
}

SRRdtSender::~SRRdtSender()
{
}


bool SRRdtSender::send(Message &message) {

	if (this->NextSeqNum >= this->base + this->N)			//�������������
	{
		this->WaitingState = true;

		return false;
	}
	else
	{

		printf("/*********************************\n");
		printf("��������\n");
		printf("/*********************************\n");

		//�������͵����ݴ�����봰��
		this->packetWaitingAck[win_ptr].acknum = -1; //���Ը��ֶ�
		this->packetWaitingAck[win_ptr].seqnum = this->NextSeqNum;
		this->packetWaitingAck[win_ptr].checksum = 0;
		memcpy(this->packetWaitingAck[win_ptr].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[win_ptr].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[win_ptr]);
		pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck[win_ptr]);

		//����win_ptr��ֵʹ������ָ���λ
		win_ptr += 1;
		//����NextSeqNum��ֵ
		NextSeqNum += 1;

		//ÿ��������Ҫӵ���Լ��Ķ�ʱ��
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[win_ptr-1].seqnum);			//�������ͷ���ʱ��
			
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[win_ptr - 1]);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�

		//�����������������
		int seq_temp = this->NextSeqNum;
		printf("============================================\n");
		printf("���������ڣ�");
		for (int j = 0; j < this->win_ptr; j++)
		{
			printf("	%d", this->packetWaitingAck[j].seqnum);
			seq_temp = this->packetWaitingAck[j].seqnum;
		}
		printf("	>%d<", ++seq_temp);
		for (int j = this->win_ptr + 1; j < 4; j++)
		{
			printf("	%d", ++seq_temp);
		}
		printf("	\n");
		printf("============================================\n");
		printf("win_ptr = %d\n", this->win_ptr);

		if (this->win_ptr == 4)
		{
			//�������ж�
	
			this->WaitingState = true;
		}

		return true;
	}

}

void SRRdtSender::receive(Packet &ackPkt) {

	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	
	if (checkSum == ackPkt.checksum && ackPkt.acknum < this->NextSeqNum && ackPkt.acknum >= this->base) //�յ�ack�ҷ�������ڴ�����
	{
		int pos = 0;
		int next_pos;

		printf("�ڴ���ack��ţ�%d\n", this->packetWaitingAck[0].seqnum);
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		
		

		//�ڻ���������Ϊ���յ�ack��λ�������
		pos = ackPkt.acknum - this->base;
		
		//�ر���Ӧ�ļ�ʱ��,����������Ϊ�˷�ֹ�ظ��رռ�ʱ��
		if (this->packetWaitingAck[pos].acknum == -1)//δ�յ�ȷ��
			pns->stopTimer(SENDER, this->packetWaitingAck[pos].seqnum);

		this->packetWaitingAck[pos].acknum = 1;

		
		

		if (pos == 0)//���÷������Ϊsend_base,��ʱ���ƶ���������
		{
			printf("\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~``\n");
			printf("ack list: %d %d %d %d", this->packetWaitingAck[0].acknum, this->packetWaitingAck[1].acknum, this->packetWaitingAck[2].acknum, this->packetWaitingAck[3].acknum);
			printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~``\n\n");

			//�ҵ���С���δ��ȷ�ϴ�
			for (next_pos = 0; next_pos < 4; next_pos++)
			{
				if (this->packetWaitingAck[next_pos].acknum != 1)
					break;
			}

			//������Ԫ��ǰ�ƣ�α���в�����
			for (int j = 0; j < next_pos; j++) {
				memset(this->packetWaitingAck, 0, sizeof(Packet));
				for (int i = 1; i < 4; i++)
					this->packetWaitingAck[i - 1] = this->packetWaitingAck[i];

				this->win_ptr -= 1;
				this->base += 1;
				this->packetWaitingAck[3].acknum = -1;
			}
			
			//ǰ�Ʋ�������֮ǰΪ�����������ֿ�ȱ
			if (this->WaitingState == true)//������״̬����ȷ��
			{
				this->WaitingState = false;

			}
		}

		//��ʾ�����������������
		int seq_temp = ackPkt.acknum;
		printf("============================================\n");
		printf("���������ڣ�");
		for (int j = 0; j < this->win_ptr; j++)
		{
			printf("	%d", this->packetWaitingAck[j].seqnum);
			seq_temp = this->packetWaitingAck[j].seqnum;
		}
		printf("	>%d<", ++seq_temp);
		for (int j = this->win_ptr + 1; j < 4; j++)
		{
			printf("	%d", ++seq_temp);
		}
		printf("	\n");
		printf("============================================\n");

		printf("\n===========================================\n");
		printf("ricieve ack sucessfully, win_ptr = %d\n", this->win_ptr);
		printf("base = %d, nestseq = %d\n", this->base, this->NextSeqNum);
		if (this->WaitingState == false)
			printf("WaitingState = false\n");
		else
			printf("WaitingState = true\n");
		printf("===========================================\n");
	}
	else {
		printf("δ�յ��ڴ���ȷ��\n");
		//ȷ�ϴ���������в���
		printf("ack��ţ�%d  �� �ڴ���ŷ�Χ��%d - %d\n", ackPkt.acknum, this->packetWaitingAck[0].seqnum,this->packetWaitingAck[3].seqnum);

	}

}

void SRRdtSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	int pos = seqNum - this->base;
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط������ڵȴ�ack�ı���", this->packetWaitingAck[pos]);
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[pos]);			//���·������ݰ�



}