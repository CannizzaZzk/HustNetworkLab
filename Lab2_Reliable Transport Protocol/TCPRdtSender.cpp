#include "stdafx.h"
#include "Global.h"
#include "TCPRdtSender.h"

TCPRdtSender::TCPRdtSender()
{
	expectSequenceNumberSend = 0;		// ��һ��������� 
	waitingState = false;				// �Ƿ��ڵȴ�Ack��״̬
	// �ñ����ɷ��ͱ��ĵĺ���ά��
	pPacketWaitingAck = new Packet[Configuration::SEQNUM_MAX];

	// ���±�������ά���������ڣ�
	// ��������������ʱ��base < nowlMax <= rMax;
	// ���������ڲ�����ʱ��base < rMax��0 < nowlMax < base;
	// ������������ ��base��ʼ, base��base+1��... ��0��... ��lMax-1
	windowN = Configuration::WINDOW_N;	// �������ڴ�С
	base = 0;							// ��ǰ���ڵ���ŵĻ�ַ����receive����ά��
	rMax = Configuration::SEQNUM_MAX;	// ��������Ͻ磬��������ΪrMax-1
	nowlMax = windowN;					// ���������ұ߽磬���ΪrMax����СΪ1,��receive����ά��
	countACK = 0;						// �ظ�ack����
}

TCPRdtSender::~TCPRdtSender()
{
	if (pPacketWaitingAck) {
		delete[] pPacketWaitingAck;
		pPacketWaitingAck = 0;
	}
}

/*
getWaitingState �������� RdtSender �Ƿ��ڵȴ�״̬.
���� StopWait Э�飬�����ͷ��ȴ��ϲ㷢�͵� Packet��ȷ��ʱ��getWaitingState ����Ӧ�÷��� true��
���� GBN Э�飬�����ͷ��ķ��ʹ�������ʱgetWaitingState ����Ӧ�÷��� true��
ģ�����绷����Ҫ����RdtSender ������������ж��Ƿ���Ҫ��Ӧ�ò����������ݵݽ��� Rdt��
�� getWaitingState ���� true ʱ��Ӧ�ò㲻��������������
*/
bool TCPRdtSender::getWaitingState()
{
	return waitingState;
}

//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;
//�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
bool TCPRdtSender::send(Message &message)
{
	if (waitingState)	//���ͷ����ڵȴ�ȷ��״̬
		return false;
	// ��ǰ����ķ���
	Packet *pPacketDel = &pPacketWaitingAck[expectSequenceNumberSend];
	// ��ţ���һ�����͵���ţ���ʼΪ0
	pPacketDel->seqnum = expectSequenceNumberSend;
	// ȷ�Ϻ�
	pPacketDel->acknum = -1;	// ���Ը��ֶ�
	// У���
	pPacketDel->checksum = 0;

	memcpy(pPacketDel->payload, message.data, sizeof(message.data));
	pPacketDel->checksum = pUtils->calculateCheckSum(*pPacketDel);
	pUtils->printPacket("���ͷ� ���� ԭʼ����", *pPacketDel);
	// ���ڵĵ�һ�����ͱ�����Ҫ������ʱ�����ü�ʱ���ı����base
	if (base == expectSequenceNumberSend) {
		pns->startTimer(SENDER, Configuration::TIME_OUT, base);
		// cout << endl;
	}
	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	pns->sendToNetworkLayer(RECEIVER, *pPacketDel);
	expectSequenceNumberSend++;
	// �п��ܻ����nowlMax == rMax����expectSequenceNumberSend == rMax�����
	if (nowlMax == rMax && expectSequenceNumberSend == nowlMax) {
		// ������������µĴ�������������expectSequenceNumberSend��
		// Ϊ�˷��㴦����Ϊ��������´����������б��Ľ��룬���Բ����и�����
	}
	else
		expectSequenceNumberSend %= rMax;
	// ������
	if (base < nowlMax) {	// ��������û�зֶ�
		// �ڴ������û�г������ڴ�С
		if (expectSequenceNumberSend == nowlMax) // �ӵ�����
			waitingState = true;
	}
	else {
		// ��һ����ű�baseС���ҵ��˱߽�
		if (expectSequenceNumberSend < base && expectSequenceNumberSend >= nowlMax)
			waitingState = true;
	}
	// �����ǰ��������
	cout << "���ͷ� ���� ԭʼ���ĺ� �������ڣ�";
	cout << "| >";
	for (int i = base; i <= rMax; i++) {
		// �����ڷֶ����
		if (nowlMax < base && i == rMax)
			i = 0;
		if (i == nowlMax) {
			// ������
			if (expectSequenceNumberSend == nowlMax)
				cout << " <";
			cout << " |" << endl;
			break;
		}
		if (i == expectSequenceNumberSend)
			cout << " <";
		cout << " " << i;
	}

	//��������������ļ���
	cout.rdbuf(foutTCP.rdbuf());	// �����������Ϊ����TCPЭ�鴰�ڵ��ļ�
	cout << "���ͷ� ���� ԭʼ���ĺ� �������ڣ�";
	cout << "| >";
	for (int i = base; i <= rMax; i++) {
		// �����ڷֶ����
		if (nowlMax < base && i == rMax)
			i = 0;
		if (i == nowlMax) {
			// ������
			if (expectSequenceNumberSend == nowlMax)
				cout << " <";
			cout << " |" << endl;
			break;
		}
		if (i == expectSequenceNumberSend)
			cout << " <";
		cout << " " << i;
	}
	cout.rdbuf(backup);	// ���������Ϊ����̨

	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
	return true;
}

void TCPRdtSender::receive(Packet &ackPkt)
{
	// ����б���δȷ��
	if (expectSequenceNumberSend != base) {
		bool inWindow = false;	// ���ڱ�־ȷ�Ϻ��Ƿ��ڴ�����
		bool reSend = false;	// ���ڱ�־�Ƿ���Ҫ�ط�����
		int ackPktNum = ackPkt.acknum;	// ��ǰ���ĵ�ȷ�Ϻ�
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		// ���ڷֶ�
		if (nowlMax < base) {
			if (expectSequenceNumberSend < base) {
				if (ackPktNum >= base && ackPktNum < rMax)
					inWindow = true;
				else if (ackPktNum < base && ackPktNum >= 0 && ackPktNum < expectSequenceNumberSend)
					inWindow = true;
			}
			else {
				if (ackPktNum >= base && ackPktNum < expectSequenceNumberSend)
					inWindow = true;
			}
		}
		else {
			// �п��ܻ����nowlMax == rMax����expectSequenceNumberSend == rMax�����
			// ���������expectSequenceNumberSendû��ȡ�࣬���Բ�����0
			if (ackPktNum >= base && ackPktNum < expectSequenceNumberSend)
				inWindow = true;
		}
		// �����ȷ�Ϻ��ڴ�����
		if (inWindow) {
			// �����ۼ�ȷ�ϵķ�ʽ
			// У�����ȷ
			if (checkSum == ackPkt.checksum) {
				countACK = 0;	// ����
				pns->stopTimer(SENDER, pPacketWaitingAck[base].seqnum);
				pUtils->printPacket("���ͷ� �յ� ȷ�ϱ��� ������ȷ", ackPkt);
				// �����ƶ�
				base = ackPktNum + 1;
				base %= rMax;
				nowlMax = base + windowN;
				if (nowlMax == rMax) {
					/*�պõ����ұ߽���������²��ֶδ���*/
				}
				else {
					// nowlMaxȡֵ��Χ1~rMax
					nowlMax %= rMax;
					// ������֮ǰnowlMax == rMax����expectSequenceNumberSend == nowlMax�����
					expectSequenceNumberSend %= rMax;
				}
				// ���ڲ�Ϊ�գ�������ʱ��
				if (base != expectSequenceNumberSend)
					pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketWaitingAck[base].seqnum);
				// ����δ��
				waitingState = false;
			}
			else {
				pUtils->printPacket("���ͷ� �յ� ȷ�ϱ��� У��ʹ���", ackPkt);
				// reSend = true;	// ��Ҫ�ط�����
			}
		}
		else { // ackû������
			// ����ackPktNum ��baseС1�����
			if ((base + rMax - 1) % rMax == ackPktNum) {
				cout << "���ͷ� �յ� ȷ�ϱ��� " << "ȷ�Ϻ�" << ackPktNum << " �����ش�����" << base;
				countACK++;
				cout << " ������� " << countACK;
				if (countACK == 3) {
					cout << " �Ѵﵽ�����ظ�ȷ�ϱ���";
					reSend = true;
				}
			}
			else {
				cout << "���ͷ� �յ� ȷ�ϱ��� û�и�ȷ�Ϻ�";
			}
			pUtils->printPacket("", ackPkt);
		}
		if (reSend) {
			countACK = 0;	// ����
			cout << "������" << endl << "�����ش���" << endl;
			cout.rdbuf(foutTCP.rdbuf());	// �����������Ϊ����TCPЭ�鴰�ڵ��ļ�
			cout << "�����ش���" << endl;
			cout.rdbuf(backup);	// ���������Ϊ����̨

			pns->stopTimer(SENDER, pPacketWaitingAck[base].seqnum);
			cout << endl;
			pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketWaitingAck[base].seqnum);
			for (int i = base; i <= rMax; i++) {
				if (nowlMax < base && i == rMax)
					i = 0;
				if (i == expectSequenceNumberSend)
					break;
				cout << "���ͷ� �ط� ����" << i;
				pUtils->printPacket("", pPacketWaitingAck[i]);
				pns->sendToNetworkLayer(RECEIVER, pPacketWaitingAck[i]);

				cout.rdbuf(foutTCP.rdbuf());	// �����������Ϊ����TCPЭ�鴰�ڵ��ļ�
				cout << "���ͷ� �ط� ����" << i << endl;
				cout.rdbuf(backup);	// ���������Ϊ����̨
			}
		}
		// �����ǰ��������
		cout << "���ͷ� ���� ��Ӧ���ĺ� �������ڣ�";
		cout << "| >";
		for (int i = base; i <= rMax; i++) {
			// �����ڷֶ����
			if (nowlMax < base && i == rMax)
				i = 0;
			if (i == nowlMax) {
				// ������
				if (expectSequenceNumberSend == nowlMax)
					cout << " <";
				cout << " |" << endl;
				break;
			}
			if (i == expectSequenceNumberSend)
				cout << " <";
			cout << " " << i;
		}

		//��������������ļ���
		cout.rdbuf(foutTCP.rdbuf());	// �����������Ϊ����TCPЭ�鴰�ڵ��ļ�
		cout << "���ͷ� ���� ��Ӧ���ĺ� �������ڣ�";
		cout << "| >";
		for (int i = base; i <= rMax; i++) {
			// �����ڷֶ����
			if (nowlMax < base && i == rMax)
				i = 0;
			if (i == nowlMax) {
				// ������
				if (expectSequenceNumberSend == nowlMax)
					cout << " <";
				cout << " |" << endl;
				break;
			}
			if (i == expectSequenceNumberSend)
				cout << " <";
			cout << " " << i;
		}
		cout.rdbuf(backup);	// ���������Ϊ����̨

		if (reSend) system("pause"); // ���ڿ����ش�����ͣ
	}
	else {
		cout << "���ͷ� �յ� ȷ�ϱ��� ����Ϊ�� ������";
		pUtils->printPacket("", ackPkt);
	}
	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
}

void TCPRdtSender::timeoutHandler(int seqNum)
{
	cout << "���ͷ� ��ʱ�� ��ʱ��" << endl;
	cout << "���ͷ� �ط� ���緢����δȷ�ϵı��ģ�" << endl;
	countACK = 0;	// ����
	pns->stopTimer(SENDER, pPacketWaitingAck[base].seqnum);
	cout << endl;
	pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketWaitingAck[base].seqnum);
	cout << "���ͷ� �ط� ����" << base;
	pUtils->printPacket("", pPacketWaitingAck[base]);
	pns->sendToNetworkLayer(RECEIVER, pPacketWaitingAck[base]);
	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
}