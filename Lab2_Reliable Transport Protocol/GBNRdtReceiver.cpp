#include "stdafx.h"
#include "Global.h"
#include "GBNRdtReceiver.h"
int diliver_count = 0;

GBNRdtReceiver::GBNRdtReceiver() :NextSeqNum(0)
{
	AckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	AckPkt.checksum = 0;
	AckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		AckPkt.payload[i] = '.';
	}
	AckPkt.checksum = pUtils->calculateCheckSum(AckPkt);
}


GBNRdtReceiver::~GBNRdtReceiver()
{
}

void GBNRdtReceiver::receive(Packet &packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);

	//���У�����ȷ��ͬʱ�յ����ĵ���ŵ��ڽ��շ��ڴ��յ��ı������һ��
	if (checkSum == packet.checksum && this->NextSeqNum == packet.seqnum) {
		pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);

		//ȡ��Message�����ϵݽ���Ӧ�ò�
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);
		printf("\n\n\n############################\n");
		printf("���ϵݽ����ݰ���ţ�%d\n", packet.seqnum);
		printf("���ݵ� %d �����ݰ�", ++diliver_count);
		printf("\n############################\n\n\n");

		AckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
		AckPkt.checksum = pUtils->calculateCheckSum(AckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", AckPkt);
		pns->sendToNetworkLayer(SENDER, AckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�

		this->NextSeqNum += 1; 
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