#include<Winsock2.h>
#include<string>
#include<stdio.h>
#include<direct.h>
#include<process.h>
#include<iostream>
#include<fstream>
#include<map>
#include<conio.h>
using namespace std;
#pragma comment(lib,"Ws2_32.lib")
#pragma warning(disable:4996)
//====================全局常量======================
const int PORT = 12280;
const int BUF_LEN = 10240;
const int MIN_LEN = 128;


//====================函数声明======================
int response_404(SOCKET sAccept);//设置404响应信息 
int response_200(SOCKET sAccept, long file_len, string content_type); //设置200响应报文信息
int file_response(SOCKET sAccept, FILE *file, long file_len);  //响应资源
int file_404_page(SOCKET sAccept); 



unsigned int _stdcall thread_net(PVOID pM) {
	SOCKET sAccept = (SOCKET)pM;
	char recv_buf[BUF_LEN];
	char real_Path[_MAX_PATH];
	int ret;
	string  method, url, content_type;
	memset(recv_buf, 0, sizeof(recv_buf));
	fd_set my_set;
	timeval time_out;
	FD_ZERO(&my_set);
	FD_SET(sAccept, &my_set);
	if (select(sAccept + 1, &my_set, NULL, NULL, &time_out) != 1)
	{
		Sleep(200);
	}

	ret = recv(sAccept, recv_buf, sizeof(recv_buf), 0);
	if (ret == SOCKET_ERROR)   //接收错误
	{
		cout << "接收数据失败:" << WSAGetLastError() << "\n" << endl;
		closesocket(sAccept);
		return -1;
	}
	else cout << "请求报文:\n" << recv_buf << endl; //接收成功，打印请求报文

	string receive(recv_buf);
	int pos = receive.find(" ");
	method = receive.substr(0, pos); //找到请求的方法
	cout << "请求的方法为:" << method << endl;
	string _str = receive.substr(pos + 1, receive.length());
	pos = _str.find(" ");
	url = _str.substr(0, pos); //找到请求的url
	cout << "请求url:" << url << endl;
	pos = url.find(".");
	content_type = url.substr(pos + 1, url.length() - 1);//请求文件类型
	cout << "请求文件类型:" << content_type << endl;

	_getcwd(real_Path, _MAX_PATH);
	string path(real_Path);
	path.append(url.replace(0, 1, "\\"));
	cout << "请求的文件在本地的路径:" << path << endl;
	// 打开本地路径下的文件
	cout << path.c_str() << endl;
	FILE *resource = fopen(path.c_str(), "rb");
	// 没有该文件则发送一个简单的404-file not found的html页面，并断开本次连接
	if (resource == NULL)
	{
		response_404(sAccept);
		// 如果method是GET，则发送自定义的file not found页面
		if (0 == stricmp(method.c_str(), "GET"))
			file_404_page(sAccept);
		closesocket(sAccept); //释放连接套接字，结束与该客户的通信
		cout<<"请求文件不存在...\nclose ok.\n"<<endl;
		return -1;
	}

	// 求出文件长度，记得重置文件指针到文件头
	fseek(resource, 0, SEEK_SET);
	fseek(resource, 0, SEEK_END);
	long flen = ftell(resource);
	cout << "请求文件长度为: " << flen << endl;;
	fseek(resource, 0, SEEK_SET);
	
	// 发送200的响应报文
	response_200(sAccept, flen, content_type);

	// 如果是GET方法则发送请求的资源
	if (0 == stricmp(method.c_str(), "GET"))
	{
		if (0 == file_response(sAccept, resource, flen))
			cout << "文件发送成功...\n\n\n" << endl;
		else
			cout<<"文件发送失败...\n\n\n"<<endl;
	}
	fclose(resource);
	closesocket(sAccept); //释放连接套接字，结束与该客户的通信
	return 0;
}

int file_404_page(SOCKET sAccept) {
	string str,send_buf;
	ifstream fileRead("404.TXT");
	if (fileRead.is_open()) {
		while (getline(fileRead, str)) {
			send_buf.append(str);
		}
		fileRead.close();
	}
	send(sAccept, send_buf.data(), send_buf.length(), 0);
	return 0;
}

/*
	响应资源
*/
int file_response(SOCKET sAccept, FILE *file, long file_len) {
	char *send_buf = new char[file_len + 1];
	memset(send_buf, 0, file_len + 1);       //缓存清0
	fread(send_buf, sizeof(char), file_len, file);

	if (SOCKET_ERROR == send(sAccept, send_buf, file_len, 0))
	{
		cout << "send() Failed:\n" << WSAGetLastError() << endl;;
		return -1;
	}
	return 0;
}

/*
	发送200的响应信息
*/
int response_200(SOCKET sAccept, long file_len, string type) {
	string send_buf = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nServer:Net_Server\r\nContent-Length: +"+ to_string(file_len) +"\r\n";

	//判断传输文件类型
	if (type == "png")
		send_buf += "Content-Type: image/png;charset=UTF-8\r\n";
	else if (type == "html")
		send_buf += "Content-Type: text/html;charset=UTF-8\r\n";
	else if (type == "jpg")
		send_buf += "Content-Type: image/jpeg;charset=UTF-8\r\n";
	else if (type == "mp4")
		send_buf += "Content-Type: video/mpeg4;charset=UTF-8\r\n";
	else if (type == "txt")
		send_buf += "Content-Type: text/plain;charset=UTF-8\r\n";
	else if (type == "xml")
		send_buf += "Content-Type: application/xml;charset=UTF-8\r\n";
	send(sAccept, send_buf.data(), send_buf.length(), 0);
	cout << send_buf.data();

	cout << send_buf.c_str() << endl;
	send_buf = "\r\n";
	cout << send_buf.data();
	send(sAccept, send_buf.data(), send_buf.length(), 0);
	cout << send_buf.data();
	return 0;
}

/*
	发送404的响应信息
*/
int response_404(SOCKET sAccept){
	string send_buf = "HTTP/1.1 404 NOT FOUND\r\nConnection: keep-alive\r\nServer:Net_Server\r\nContent-Type: text/html\r\n\r\n";
	send(sAccept, send_buf.data(), send_buf.length(), 0);
	return 0;
}

/*
	主函数
*/
int main() {
	
	WSADATA wData;
	SOCKET sock_Listen, sAccept; //服务器监听套接字
	struct sockaddr_in server, client; //服务器端套接字和客户端套接字地址
	int len;
	

	cout << "1-启动服务器" << endl;
	cout << "2-关闭服务器" << endl;
	//cout << "3-退出" << endl;
	int select;	
	while (1) {
		scanf("%d", &select);
		switch (select)
		{
			case 1: {
				if (WSAStartup(MAKEWORD(2, 2), &wData) != 0)
				{
					cout << "Failed to load Winsock....\n" << endl;
					return -1;
				}
				cout << "Winsock load success......\n" << endl;

				sock_Listen = socket(AF_INET, SOCK_STREAM, 0);
				if (sock_Listen == INVALID_SOCKET)
				{
					cout << "socket_Listen Failed....\n" << endl;
					return -1;
				}
				cout << "socket_listen start success......\n" << endl;

				cout << "手动配置？(Y or N)" << endl;
				char choice;
				int port;
				string addr;
				cin >> choice;
				if (choice == 'N') {
					server.sin_family = AF_INET;
					server.sin_port = htons(PORT);               
					server.sin_addr.s_addr = htonl(INADDR_ANY);   
				}
				else
				{
					cout << "端口号：";
					cin >> port;
					cout << "\n" << "IP:" << endl;
					cin >> addr;
					server.sin_family = AF_INET;
					server.sin_port = htons(port);               
					server.sin_addr.s_addr = inet_addr(addr.data());   
					cout << "=========" << endl;
					cout << addr.data()<< endl;
					cout << port << endl;
					cout << "==========" << endl;
				}

				if (bind(sock_Listen, (LPSOCKADDR)&server, sizeof(server)) == SOCKET_ERROR)
				{
					cout << "bind Failed....\n" << endl;
					return -1;
				}
				cout << "bind start success\n" << endl;

				if (listen(sock_Listen, 5) == SOCKET_ERROR)
				{
					cout << "listen Failed....\n" << endl;
					return -1;
				}
				cout << "listen start success......\n\n\n" << endl;
				cout << "-------------------------------------------\n" << endl;
				u_long blockMode = 1;
				if (ioctlsocket(sock_Listen, FIONBIO, &blockMode) == SOCKET_ERROR) {
					cout << "ioctlsocket() for new session failed with error!"<<endl;
					return -1;
				}
				while (1) {
					if (kbhit()) {
						break;
					}
					len = sizeof(client);
					sAccept = accept(sock_Listen, (struct sockaddr *)&client, &len);
					if (sAccept == INVALID_SOCKET)
					{
						if (WSAGetLastError() == 10035)
						{
							continue;
						}
						printf("accept() Failed:%d\n", WSAGetLastError());
						break;
					}
					string clientAddress; //clientAddress是个空字符串， clientAddress.empty() is true
					sockaddr_in clientAddr;
					int nameLen, rtn;

					nameLen = sizeof(clientAddr);
					rtn = getsockname(sAccept, (LPSOCKADDR)&clientAddr, &nameLen);
					if (rtn != SOCKET_ERROR) {
						clientAddress += inet_ntoa(clientAddr.sin_addr);
						cout << "Client-IP:" << clientAddress << endl;
						unsigned short uIPPort = clientAddr.sin_port;
						cout << "Client-Port:" << htons(client.sin_port) << endl;
					}
					cout << endl;
					if (sAccept == INVALID_SOCKET)
					{
						cout << "accept Failed....\n" << endl;
						break;
					}
					_beginthreadex(NULL, 0, thread_net, (SOCKET *)sAccept, 0, NULL);
				}
			}
			case 2: {
				/*cout << "已经关闭" << endl;*/
				if(sock_Listen != NULL) 
					closesocket(sock_Listen);
				cout << "已经关闭" << endl<<endl;

				cout << "1-启动服务器" << endl;
				cout << "2-关闭服务器" << endl;
				break;
			}
		}
	}
	WSACleanup();
	return 0;
}