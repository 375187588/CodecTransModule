#pragma once
// CTransSocket 命令目标
#define PRE_AGREED_PORT		8686
#define TRANS_PACKET_PORT   8888
#define SEND_BUFFER_SIZE	4096
#define RECV_BUFFER_SIZE	4096
class CTransSocket : public CSocket
{
public:
	CTransSocket();
	virtual ~CTransSocket();

public:
	//Server接收文件并保存
	static BOOL GetFileFromRemoteSender(CString strName);
	//Client读文件并发送
	static BOOL SendFileToRemoteReceiver(CString strIP, CString strName);

	//Server接收数据 -- 主要想来接收AVPacket
	static BOOL RecvAVPacket(void* pPacket);
	//Client发送数据 -- 主要想来发送AVPacket
	static BOOL SendAVPacket(CString strIPAddress, void* pPacket, int nSendSize);

	//Init Uninit
	void InitSocket(UINT nPort, int nSocketType, CString strSocketAddress = L"");	//strSocketAddress为空则为Server
	void UninitSocket();

	//Send/Recv notifition to continue Send/Recv
	BOOL SendToContinue();
	BOOL RecvToContinue();
	//Send/Recv end notifition.
	BOOL SendEndInfo();
	BOOL RecEndInfo();
	//send/recv data length before send/recv data in case recver don't know recv the data length.
	int SendDataLength(int nSize);
	int RecvDataLength(int& nSize);
	//Send  Recv
	int SendData(void* pData, int nSendSize);
	int RecvData(void* pData, int nRecvSize);
private:
	//client
	void Connect(CString strIPAddress, int nPort);
	//server 
	void Listen();
	void Accept();
	//both
	void Bind(CString strIPAddress, int nPort); 
	void Send();
	void Receive();

private:
	CSocket m_socket;
	CSocket m_socketConn;
	int		m_nType;	//Client(0) or Server (1)
};


