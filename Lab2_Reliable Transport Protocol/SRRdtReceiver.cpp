#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"


SRRdtReceiver::SRRdtReceiver() :NextSeqNum(0),base(0),N(4)
{
	AckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	AckPkt.checksum = 0;
	AckPkt.seqnum = -1;	//忽略该字段
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
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//标记应滑动位数
	int count_slide = 0;

	//校验和正确，且在滑动窗口之内
	if (checkSum == packet.checksum && packet.seqnum < this->base + this->N && packet.seqnum >= this->base) {
		pUtils->printPacket("接收方正确收到发送方的报文", packet);
		
		//判断该报文是否被收到过
		int pos = packet.seqnum - this->base;

		if (record[pos] == 0)//该报文还未被收到过
		{
			//取出Message，缓存进接收方窗口
			memcpy(MsgWaiting[pos].data, packet.payload, sizeof(packet.payload));
			this->record[pos] = 1;

			count_slide = 0;//开始向上传递窗口内数据之前，将计数器初始化

			if (pos == 0)//报文序号等于rcv_base
			{
				//将已收到的数据向上传递
				for (int i = 0; i < 4; i++)
				{
					if (this->record[i] == 1)
					{
						//向应用层传输数据
						pns->delivertoAppLayer(RECEIVER, this->MsgWaiting[i]);
						

						//对计数器操作
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
				//滑动窗口移动
				this->base += 1;
				this->NextSeqNum += 1;

				count_slide -= 1;
			}
		}	

		//向发送方返回确认
		AckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		AckPkt.checksum = pUtils->calculateCheckSum(AckPkt);
		pUtils->printPacket("接收方发送确认报文", AckPkt);
		pns->sendToNetworkLayer(SENDER, AckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方

		printf("\n\n####################################\n");
		printf("接收方窗口起点 %d\n", this->base);
		printf("record数组：%d  %d  %d  %d\n", this->record[0], this->record[1], this->record[2], this->record[3]);
		printf("####################################\n\n");

	}
	else if (checkSum == packet.checksum && packet.seqnum < this->base  && packet.seqnum >= this->base - this->N)
	{
		//向发送方返回确认
		AckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		AckPkt.checksum = pUtils->calculateCheckSum(AckPkt);
		pUtils->printPacket("接收方发送确认报文", AckPkt);
		pns->sendToNetworkLayer(SENDER, AckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
		}
		else {
			pUtils->printPacket("接收方没有正确收到发送方的报文,报文序号不对", packet);
		}
		pUtils->printPacket("接收方重新发送上次的确认报文", AckPkt);
		pns->sendToNetworkLayer(SENDER, AckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送上次的确认报文

	}
}