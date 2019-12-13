#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

//======================全局变量====================


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

		//每个分组需要拥有自己的定时器
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck[win_ptr-1].seqnum);			//启动发送方定时器
			
		pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[win_ptr - 1]);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方

		//输出滑动窗口内数据
		int seq_temp = this->NextSeqNum;
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
	
			this->WaitingState = true;
		}

		return true;
	}

}

void SRRdtSender::receive(Packet &ackPkt) {

	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	
	if (checkSum == ackPkt.checksum && ackPkt.acknum < this->NextSeqNum && ackPkt.acknum >= this->base) //收到ack且分组序号在窗口内
	{
		int pos = 0;
		int next_pos;

		printf("期待的ack序号：%d\n", this->packetWaitingAck[0].seqnum);
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		
		

		//在滑动窗口中为已收到ack的位置做标记
		pos = ackPkt.acknum - this->base;
		
		//关闭相应的计时器,放在这里是为了防止重复关闭计时器
		if (this->packetWaitingAck[pos].acknum == -1)//未收到确认
			pns->stopTimer(SENDER, this->packetWaitingAck[pos].seqnum);

		this->packetWaitingAck[pos].acknum = 1;

		
		

		if (pos == 0)//即该分组序号为send_base,此时需移动滑动窗口
		{
			printf("\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~``\n");
			printf("ack list: %d %d %d %d", this->packetWaitingAck[0].acknum, this->packetWaitingAck[1].acknum, this->packetWaitingAck[2].acknum, this->packetWaitingAck[3].acknum);
			printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~``\n\n");

			//找到最小序号未被确认处
			for (next_pos = 0; next_pos < 4; next_pos++)
			{
				if (this->packetWaitingAck[next_pos].acknum != 1)
					break;
			}

			//将数组元素前移（伪队列操作）
			for (int j = 0; j < next_pos; j++) {
				memset(this->packetWaitingAck, 0, sizeof(Packet));
				for (int i = 1; i < 4; i++)
					this->packetWaitingAck[i - 1] = this->packetWaitingAck[i];

				this->win_ptr -= 1;
				this->base += 1;
				this->packetWaitingAck[3].acknum = -1;
			}
			
			//前移操作后，若之前为满窗口则会出现空缺
			if (this->WaitingState == true)//满窗口状态接收确认
			{
				this->WaitingState = false;

			}
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
		printf("ack编号：%d  ， 期待编号范围：%d - %d\n", ackPkt.acknum, this->packetWaitingAck[0].seqnum,this->packetWaitingAck[3].seqnum);

	}

}

void SRRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	int pos = seqNum - this->base;
	pUtils->printPacket("发送方定时器时间到，重发窗口内等待ack的报文", this->packetWaitingAck[pos]);
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck[pos]);			//重新发送数据包



}