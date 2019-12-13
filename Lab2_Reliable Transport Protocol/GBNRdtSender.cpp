#include "stdafx.h"
#include "Global.h"
#include "GBNRdtSender.h"

//======================ȫ�ֱ���====================
int stop_time_num = 0;

//=================================================

GBNRdtSender::GBNRdtSender() :NextSeqNum(0),N(4),base(0),WaitingState(false)
{
	packetWaitingAck = new Packet[N];
	win_ptr = 0;
}


bool GBNRdtSender::getWaitingState() {
	
	return WaitingState;
}

GBNRdtSender::~GBNRdtSender()
{
	delete packetWaitingAck;
}


bool GBNRdtSender::send(Message &message) {
	
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

		//���������³��ֵȴ�ack�����ݰ��򼤻��ʱ��
		if (this->base == this->NextSeqNum - 1)
		{
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[0].seqnum);			//�������ͷ���ʱ��
			stop_time_num = this->packetWaitingAck[0].seqnum;
		}
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[win_ptr-1]);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		
		int seq_temp = 0;
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
			printf("test");
			this->WaitingState = true;
		}

		return true;
	}

}

void GBNRdtSender::receive(Packet &ackPkt) {
	
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//���У�����ȷ������ȷ�����=���ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum && ackPkt.acknum == this->packetWaitingAck[0].seqnum) {
		
		
		printf("�ڴ���ack��ţ�%d\n", this->packetWaitingAck[0].seqnum);
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		this->base = ackPkt.acknum + 1;
		
		if (this->WaitingState == true)//������״̬����ȷ��
		{
			this->WaitingState = false;

		}

		if (this->base == this->NextSeqNum)//������û�еȴ�ack��Packet
			pns->stopTimer(SENDER, stop_time_num);
	
		//������Ԫ��ǰ��һλ��α���в�����
		memset(this->packetWaitingAck, 0, sizeof(Packet));
		for (int i = 1; i < 10; i++)
			this->packetWaitingAck[i - 1] = this->packetWaitingAck[i];
		
		this->win_ptr -= 1;

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
	else if (checkSum == ackPkt.checksum && ackPkt.acknum > this->packetWaitingAck[0].seqnum && this->win_ptr!=0)
	{
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		this->base = ackPkt.acknum + 1;

		if (this->base == this->NextSeqNum)//������û�еȴ�ack��Packet
			pns->stopTimer(SENDER, stop_time_num);
		
		if (this->WaitingState == true)//������״̬����ȷ��
		{
			this->WaitingState = false;

		}
		
		for(int j = 0;j<(ackPkt.acknum - this->packetWaitingAck[0].seqnum +1);j++)
		{
			//������Ԫ��ǰ��һλ��α���в�����
			memset(this->packetWaitingAck, 0, sizeof(Packet));
			for (int i = 1; i < 10; i++)
				this->packetWaitingAck[i - 1] = this->packetWaitingAck[i];

			this->win_ptr -= 1;
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
		printf("ack��ţ�%d  �� �ڴ���ţ�%d\n", ackPkt.acknum, this->packetWaitingAck[0].seqnum);

	}
	
}

void GBNRdtSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط������ڵȴ�ack�ı���", this->packetWaitingAck[0]);
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[0].seqnum);			//�����������ͷ���ʱ��
	stop_time_num = this->packetWaitingAck[0].seqnum;
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[0]);			//���·������ݰ�

	for (int i = 1; i < win_ptr; i++)
	{
		pUtils->printPacket("\n\n", this->packetWaitingAck[i]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[i]);
	}

}