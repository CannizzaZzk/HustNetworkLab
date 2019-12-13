#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"


SRRdtReceiver::SRRdtReceiver() :NextSeqNum(0),base(0),N(4)
{
	AckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	AckPkt.checksum = 0;
	AckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		AckPkt.payload[i] = '.';
	}
	AckPkt.checksum = pUtils->calculateCheckSum(AckPkt);

	MsgWaiting = new Message[N];
	win_ptr = 0;

	record = new int[4];
	memset(record, 0, 4 * sizeof(int));

}


SRRdtReceiver::~SRRdtReceiver()
{

}

void SRRdtReceiver::receive(Packet &packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);

	//���Ӧ����λ��
	int count_slide = 0;

	//У�����ȷ�����ڻ�������֮��
	if (checkSum == packet.checksum && packet.seqnum < this->base + this->N && packet.seqnum >= this->base) {
		pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);
		
		//�жϸñ����Ƿ��յ���
		int pos = packet.seqnum - this->base;

		if (record[pos] == 0)//�ñ��Ļ�δ���յ���
		{
			//ȡ��Message����������շ�����
			memcpy(MsgWaiting[pos].data, packet.payload, sizeof(packet.payload));
			this->record[pos] = 1;

			count_slide = 0;//��ʼ���ϴ��ݴ���������֮ǰ������������ʼ��

			if (pos == 0)//������ŵ���rcv_base
			{
				//�����յ����������ϴ���
				for (int i = 0; i < 4; i++)
				{
					if (this->record[i] == 1)
					{
						//��Ӧ�ò㴫������
						pns->delivertoAppLayer(RECEIVER, this->MsgWaiting[i]);
						

						//�Լ���������
						count_slide += 1;
						
					}
					else {
						break;
					}
				}
			}

			while (count_slide != 0)
			{
				for (int j = 1; j < 4; j++)
				{
					this->MsgWaiting[j - 1] = this->MsgWaiting[j];
					this->record[j - 1] = this->record[j];
				}
				this->record[3] = 0;
				memset(this->MsgWaiting+3, 0, sizeof(Message));
				//���������ƶ�
				this->base += 1;
				this->NextSeqNum += 1;

				count_slide -= 1;
			}
		}	

		//���ͷ�����ȷ��
		AckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
		AckPkt.checksum = pUtils->calculateCheckSum(AckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", AckPkt);
		pns->sendToNetworkLayer(SENDER, AckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�

		printf("\n\n####################################\n");
		printf("���շ�������� %d\n", this->base);
		printf("record���飺%d  %d  %d  %d\n", this->record[0], this->record[1], this->record[2], this->record[3]);
		printf("####################################\n\n");

	}
	else if (checkSum == packet.checksum && packet.seqnum < this->base  && packet.seqnum >= this->base - this->N)
	{
		//���ͷ�����ȷ��
		AckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
		AckPkt.checksum = pUtils->calculateCheckSum(AckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", AckPkt);
		pns->sendToNetworkLayer(SENDER, AckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);
		}
		else {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,������Ų���", packet);
		}
		pUtils->printPacket("���շ����·����ϴε�ȷ�ϱ���", AckPkt);
		pns->sendToNetworkLayer(SENDER, AckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢���ϴε�ȷ�ϱ���

	}
}