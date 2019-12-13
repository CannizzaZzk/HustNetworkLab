#include "stdafx.h"
#include "Global.h"
#include "GBNRdtSender.h"

//======================全局变量====================
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
	
	if (this->NextSeqNum >= this->base + this->N)			//窗口已满的情况
	{
		this->WaitingState = true;
		return false;
	}
	else
	{
	
		printf("/*********************************\n");
		printf("发送数据\n");
		printf("/*********************************\n");
		
		//将待发送的数据打包放入窗口
		this->packetWaitingAck[win_ptr].acknum = -1; //忽略该字段
		this->packetWaitingAck[win_ptr].seqnum = this->NextSeqNum;
		this->packetWaitingAck[win_ptr].checksum = 0;
		memcpy(this->packetWaitingAck[win_ptr].payload, message.data, sizeof(message.data));
		this->packetWaitingAck[win_ptr].checksum = pUtils->calculateCheckSum(this->packetWaitingAck[win_ptr]);
		pUtils->printPacket("发送方发送报文", this->packetWaitingAck[win_ptr]);

		//更新win_ptr的值使其重新指向空位
		win_ptr += 1;
		//更新NextSeqNum的值
		NextSeqNum += 1;

		//窗口中重新出现等待ack的数据包则激活计时器
		if (this->base == this->NextSeqNum - 1)
		{
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[0].seqnum);			//启动发送方定时器
			stop_time_num = this->packetWaitingAck[0].seqnum;
		}
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[win_ptr-1]);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
		
		int seq_temp = 0;
		printf("============================================\n");
		printf("滑动窗口内：");
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
			//满窗口判断
			printf("test");
			this->WaitingState = true;
		}

		return true;
	}

}

void GBNRdtSender::receive(Packet &ackPkt) {
	
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
	if (checkSum == ackPkt.checksum && ackPkt.acknum == this->packetWaitingAck[0].seqnum) {
		
		
		printf("期待的ack序号：%d\n", this->packetWaitingAck[0].seqnum);
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		this->base = ackPkt.acknum + 1;
		
		if (this->WaitingState == true)//满窗口状态接收确认
		{
			this->WaitingState = false;

		}

		if (this->base == this->NextSeqNum)//窗口内没有等待ack的Packet
			pns->stopTimer(SENDER, stop_time_num);
	
		//将数组元素前移一位（伪队列操作）
		memset(this->packetWaitingAck, 0, sizeof(Packet));
		for (int i = 1; i < 10; i++)
			this->packetWaitingAck[i - 1] = this->packetWaitingAck[i];
		
		this->win_ptr -= 1;

		//显示滑动窗口内数据情况
		int seq_temp = ackPkt.acknum;
		printf("============================================\n");
		printf("滑动窗口内：");
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
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		this->base = ackPkt.acknum + 1;

		if (this->base == this->NextSeqNum)//窗口内没有等待ack的Packet
			pns->stopTimer(SENDER, stop_time_num);
		
		if (this->WaitingState == true)//满窗口状态接收确认
		{
			this->WaitingState = false;

		}
		
		for(int j = 0;j<(ackPkt.acknum - this->packetWaitingAck[0].seqnum +1);j++)
		{
			//将数组元素前移一位（伪队列操作）
			memset(this->packetWaitingAck, 0, sizeof(Packet));
			for (int i = 1; i < 10; i++)
				this->packetWaitingAck[i - 1] = this->packetWaitingAck[i];

			this->win_ptr -= 1;
		}
		

		//显示滑动窗口内数据情况
		int seq_temp = ackPkt.acknum;
		printf("============================================\n");
		printf("滑动窗口内：");
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
		printf("未收到期待的确认\n");
		//确认错误无需进行操作
		printf("ack编号：%d  ， 期待编号：%d\n", ackPkt.acknum, this->packetWaitingAck[0].seqnum);

	}
	
}

void GBNRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	pUtils->printPacket("发送方定时器时间到，重发窗口内等待ack的报文", this->packetWaitingAck[0]);
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[0].seqnum);			//重新启动发送方定时器
	stop_time_num = this->packetWaitingAck[0].seqnum;
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[0]);			//重新发送数据包

	for (int i = 1; i < win_ptr; i++)
	{
		pUtils->printPacket("\n\n", this->packetWaitingAck[i]);
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[i]);
	}

}