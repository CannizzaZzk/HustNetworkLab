#pragma once
#include "stdafx.h"
#include "Global.h"
#include "TCPRdtReceiver.h"

// 构造函数，初始化确认报文
TCPRdtReceiver::TCPRdtReceiver()
{
	expectSequenceNumberRcvd = 0;
	// 确认号
	//初始状态下，上次发送的确认包的确认序号为7，使得当第一个接受的数据包出错时该确认报文的确认号为7
	lastAckPkt.acknum = Configuration::SEQNUM_MAX - 1;
	// 校验和
	lastAckPkt.checksum = 0;
	// 序号
	lastAckPkt.seqnum = -1;	//忽略该字段。
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';// 报文内容
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

// 析构函数，没有动态内存的分配
TCPRdtReceiver::~TCPRdtReceiver()
{
	/*空函数体*/
}

// 接收报文，将被NetworkService调用
void TCPRdtReceiver::receive(Packet &packet)
{
	int checkSum = pUtils->calculateCheckSum(packet);	// 校验和
	// 如果收到的报文序号等于接收方期待收到的报文序号，且校验和正确
	if (expectSequenceNumberRcvd == packet.seqnum && checkSum == packet.checksum) {
		pUtils->printPacket("接收方 收到 正确报文", packet);
		// 取出Message，向上递交给应用层
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);	// 报文交付
		lastAckPkt.acknum = packet.seqnum;	// 确认号
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方 发送 确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	// 确认报文的发送
		// 通过与序号最大值取余实现序列有界
		expectSequenceNumberRcvd = (expectSequenceNumberRcvd + 1) % Configuration::SEQNUM_MAX;
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("接收方 收到 校验错误报文", packet);
		}
		else {
			pUtils->printPacket("接收方 收到 序号错误报文", packet);
		}
		pUtils->printPacket("接收方 重发 确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	// 上次确认报文的重发
	}
	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
}