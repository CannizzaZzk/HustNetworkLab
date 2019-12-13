#include "stdafx.h"
#include "Global.h"
#include "TCPRdtSender.h"

TCPRdtSender::TCPRdtSender()
{
	expectSequenceNumberSend = 0;		// 下一个发送序号 
	waitingState = false;				// 是否处于等待Ack的状态
	// 该变量由发送报文的函数维护
	pPacketWaitingAck = new Packet[Configuration::SEQNUM_MAX];

	// 以下变量用于维护滑动窗口；
	// 当滑动窗口连续时，base < nowlMax <= rMax;
	// 当滑动窗口不连续时，base < rMax，0 < nowlMax < base;
	// 滑动窗口序列 从base开始, base、base+1、... 、0、... 、lMax-1
	windowN = Configuration::WINDOW_N;	// 滑动窗口大小
	base = 0;							// 当前窗口的序号的基址，由receive函数维护
	rMax = Configuration::SEQNUM_MAX;	// 窗口序号上界，即最大序号为rMax-1
	nowlMax = windowN;					// 滑动窗口右边界，最大为rMax，最小为1,由receive函数维护
	countACK = 0;						// 重复ack计数
}

TCPRdtSender::~TCPRdtSender()
{
	if (pPacketWaitingAck) {
		delete[] pPacketWaitingAck;
		pPacketWaitingAck = 0;
	}
}

/*
getWaitingState 函数返回 RdtSender 是否处于等待状态.
对于 StopWait 协议，当发送方等待上层发送的 Packet的确认时，getWaitingState 函数应该返回 true；
对于 GBN 协议，当发送方的发送窗口满了时getWaitingState 函数应该返回 true。
模拟网络环境需要调用RdtSender 的这个方法来判断是否需要将应用层下来的数据递交给 Rdt，
当 getWaitingState 返回 true 时，应用层不会有数据下来。
*/
bool TCPRdtSender::getWaitingState()
{
	return waitingState;
}

//发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;
//如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
bool TCPRdtSender::send(Message &message)
{
	if (waitingState)	//发送方处于等待确认状态
		return false;
	// 当前处理的分组
	Packet *pPacketDel = &pPacketWaitingAck[expectSequenceNumberSend];
	// 序号；下一个发送的序号，初始为0
	pPacketDel->seqnum = expectSequenceNumberSend;
	// 确认号
	pPacketDel->acknum = -1;	// 忽略该字段
	// 校验和
	pPacketDel->checksum = 0;

	memcpy(pPacketDel->payload, message.data, sizeof(message.data));
	pPacketDel->checksum = pUtils->calculateCheckSum(*pPacketDel);
	pUtils->printPacket("发送方 发送 原始报文", *pPacketDel);
	// 窗口的第一个发送报文需要开启定时器，该计时器的编号是base
	if (base == expectSequenceNumberSend) {
		pns->startTimer(SENDER, Configuration::TIME_OUT, base);
		// cout << endl;
	}
	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	pns->sendToNetworkLayer(RECEIVER, *pPacketDel);
	expectSequenceNumberSend++;
	// 有可能会出现nowlMax == rMax，而expectSequenceNumberSend == rMax的情况
	if (nowlMax == rMax && expectSequenceNumberSend == nowlMax) {
		// 这种特殊情况下的窗口满，不处理expectSequenceNumberSend，
		// 为了方便处理，因为这种情况下窗口满不会有报文进入，所以不会有副作用
	}
	else
		expectSequenceNumberSend %= rMax;
	// 处理窗口
	if (base < nowlMax) {	// 滑动窗口没有分段
		// 期待的序号没有超出窗口大小
		if (expectSequenceNumberSend == nowlMax) // 加到上限
			waitingState = true;
	}
	else {
		// 下一个序号比base小，且到了边界
		if (expectSequenceNumberSend < base && expectSequenceNumberSend >= nowlMax)
			waitingState = true;
	}
	// 输出当前滑动窗口
	cout << "发送方 发送 原始报文后 滑动窗口：";
	cout << "| >";
	for (int i = base; i <= rMax; i++) {
		// 处理窗口分段情况
		if (nowlMax < base && i == rMax)
			i = 0;
		if (i == nowlMax) {
			// 窗口满
			if (expectSequenceNumberSend == nowlMax)
				cout << " <";
			cout << " |" << endl;
			break;
		}
		if (i == expectSequenceNumberSend)
			cout << " <";
		cout << " " << i;
	}

	//滑动窗口输出到文件中
	cout.rdbuf(foutTCP.rdbuf());	// 将输出流设置为保存TCP协议窗口的文件
	cout << "发送方 发送 原始报文后 滑动窗口：";
	cout << "| >";
	for (int i = base; i <= rMax; i++) {
		// 处理窗口分段情况
		if (nowlMax < base && i == rMax)
			i = 0;
		if (i == nowlMax) {
			// 窗口满
			if (expectSequenceNumberSend == nowlMax)
				cout << " <";
			cout << " |" << endl;
			break;
		}
		if (i == expectSequenceNumberSend)
			cout << " <";
		cout << " " << i;
	}
	cout.rdbuf(backup);	// 将输出流设为控制台

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
	// 如果有报文未确认
	if (expectSequenceNumberSend != base) {
		bool inWindow = false;	// 用于标志确认号是否在窗口内
		bool reSend = false;	// 用于标志是否需要重发报文
		int ackPktNum = ackPkt.acknum;	// 当前报文的确认号
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		// 窗口分段
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
			// 有可能会出现nowlMax == rMax，而expectSequenceNumberSend == rMax的情况
			// 这种情况下expectSequenceNumberSend没有取余，所以不会变成0
			if (ackPktNum >= base && ackPktNum < expectSequenceNumberSend)
				inWindow = true;
		}
		// 如果该确认号在窗口中
		if (inWindow) {
			// 采用累计确认的方式
			// 校验和正确
			if (checkSum == ackPkt.checksum) {
				countACK = 0;	// 重置
				pns->stopTimer(SENDER, pPacketWaitingAck[base].seqnum);
				pUtils->printPacket("发送方 收到 确认报文 报文正确", ackPkt);
				// 窗口移动
				base = ackPktNum + 1;
				base %= rMax;
				nowlMax = base + windowN;
				if (nowlMax == rMax) {
					/*刚好等于右边界这种情况下不分段窗口*/
				}
				else {
					// nowlMax取值范围1~rMax
					nowlMax %= rMax;
					// 处理了之前nowlMax == rMax，且expectSequenceNumberSend == nowlMax的情况
					expectSequenceNumberSend %= rMax;
				}
				// 窗口不为空，重启定时器
				if (base != expectSequenceNumberSend)
					pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketWaitingAck[base].seqnum);
				// 窗口未满
				waitingState = false;
			}
			else {
				pUtils->printPacket("发送方 收到 确认报文 校验和错误", ackPkt);
				// reSend = true;	// 需要重发报文
			}
		}
		else { // ack没有命中
			// 处理ackPktNum 比base小1的情况
			if ((base + rMax - 1) % rMax == ackPktNum) {
				cout << "发送方 收到 确认报文 " << "确认号" << ackPktNum << " 请求重传分组" << base;
				countACK++;
				cout << " 请求次数 " << countACK;
				if (countACK == 3) {
					cout << " 已达到三次重复确认报文";
					reSend = true;
				}
			}
			else {
				cout << "发送方 收到 确认报文 没有该确认号";
			}
			pUtils->printPacket("", ackPkt);
		}
		if (reSend) {
			countACK = 0;	// 重置
			cout << "！！！" << endl << "快速重传！" << endl;
			cout.rdbuf(foutTCP.rdbuf());	// 将输出流设置为保存TCP协议窗口的文件
			cout << "快速重传！" << endl;
			cout.rdbuf(backup);	// 将输出流设为控制台

			pns->stopTimer(SENDER, pPacketWaitingAck[base].seqnum);
			cout << endl;
			pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketWaitingAck[base].seqnum);
			for (int i = base; i <= rMax; i++) {
				if (nowlMax < base && i == rMax)
					i = 0;
				if (i == expectSequenceNumberSend)
					break;
				cout << "发送方 重发 报文" << i;
				pUtils->printPacket("", pPacketWaitingAck[i]);
				pns->sendToNetworkLayer(RECEIVER, pPacketWaitingAck[i]);

				cout.rdbuf(foutTCP.rdbuf());	// 将输出流设置为保存TCP协议窗口的文件
				cout << "发送方 重发 报文" << i << endl;
				cout.rdbuf(backup);	// 将输出流设为控制台
			}
		}
		// 输出当前滑动窗口
		cout << "发送方 处理 响应报文后 滑动窗口：";
		cout << "| >";
		for (int i = base; i <= rMax; i++) {
			// 处理窗口分段情况
			if (nowlMax < base && i == rMax)
				i = 0;
			if (i == nowlMax) {
				// 窗口满
				if (expectSequenceNumberSend == nowlMax)
					cout << " <";
				cout << " |" << endl;
				break;
			}
			if (i == expectSequenceNumberSend)
				cout << " <";
			cout << " " << i;
		}

		//滑动窗口输出到文件中
		cout.rdbuf(foutTCP.rdbuf());	// 将输出流设置为保存TCP协议窗口的文件
		cout << "发送方 处理 响应报文后 滑动窗口：";
		cout << "| >";
		for (int i = base; i <= rMax; i++) {
			// 处理窗口分段情况
			if (nowlMax < base && i == rMax)
				i = 0;
			if (i == nowlMax) {
				// 窗口满
				if (expectSequenceNumberSend == nowlMax)
					cout << " <";
				cout << " |" << endl;
				break;
			}
			if (i == expectSequenceNumberSend)
				cout << " <";
			cout << " " << i;
		}
		cout.rdbuf(backup);	// 将输出流设为控制台

		if (reSend) system("pause"); // 用于快速重传的暂停
	}
	else {
		cout << "发送方 收到 确认报文 窗口为空 不处理";
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
	cout << "发送方 定时器 超时：" << endl;
	cout << "发送方 重发 最早发送且未确认的报文：" << endl;
	countACK = 0;	// 重置
	pns->stopTimer(SENDER, pPacketWaitingAck[base].seqnum);
	cout << endl;
	pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketWaitingAck[base].seqnum);
	cout << "发送方 重发 报文" << base;
	pUtils->printPacket("", pPacketWaitingAck[base]);
	pns->sendToNetworkLayer(RECEIVER, pPacketWaitingAck[base]);
	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
}