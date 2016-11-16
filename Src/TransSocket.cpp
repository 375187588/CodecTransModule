// TransSocket.cpp : 实现文件
//

#include "stdafx.h"
#include "CodecTransModule.h"
#include "TransSocket.h"


// CTransSocket

CTransSocket::CTransSocket()
{
	m_nType = 0;
}

CTransSocket::~CTransSocket()
{
}

//直接接收文件
BOOL CTransSocket::GetFileFromRemoteSender(CString fName)
{
	CSocket sockSrvr;
	sockSrvr.Create(PRE_AGREED_PORT);	// Creates our server socket
	sockSrvr.Listen();					// Start listening for the client at PORT
	CSocket sockConnection;
	sockSrvr.Accept(sockConnection);	// Use another CSocket to accept the connection

	CFile destFile;
	CFileException fe;
	BOOL bOpenFile;
	if (!(bOpenFile = destFile.Open(fName, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, &fe)))
	{
		TCHAR szCause[256];
		fe.GetErrorMessage(szCause, 255);
		TRACE("GetFileFromRemoteSender enccountered an error while opening the local file\n"
			"\t File Name = %s\n\t Cause = %d\n\t m_iosError = %d\n",
			fe.m_strFileName, szCause, fe.m_cause, fe.m_lOsError);
	}
	char szRecv[256] = { 0 };
	char szContent[RECV_BUFFER_SIZE] = { 0 };
	int nRecvSize = sockConnection.Receive(szRecv, sizeof(szRecv));
	int nFileSize = 0;
	sscanf(szRecv, "%s %d", szContent, &nFileSize);
	int nLeftSize = nFileSize;
	sockConnection.Send("send please..", strlen("send please.."));
	while (nLeftSize > 0)
	{
		memset(szContent, 0, RECV_BUFFER_SIZE);
		nRecvSize = sockConnection.Receive(szContent, RECV_BUFFER_SIZE);
		destFile.Write(szContent, nRecvSize);
		nLeftSize -= nRecvSize;
		//sockConnection.Send("send please..", strlen("send please.."));
	}

	sockConnection.Close();
	sockSrvr.Close();
	destFile.Close();

	CString str(szRecv);
	AfxMessageBox(str);

	return TRUE;
}

//直接发送文件
BOOL CTransSocket::SendFileToRemoteReceiver(CString strIP, CString fName)
{
	int len = WideCharToMultiByte(CP_ACP, 0, fName, -1, NULL, 0, NULL, NULL);
	char *pFileName = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, fName, -1, pFileName, len, NULL, NULL);

	CSocket sockClient;
	sockClient.Create();
	sockClient.Connect(strIP, PRE_AGREED_PORT);

	CFile srcFile;
	CFileException fe;
	BOOL bFileOpen = FALSE;
	if (!(bFileOpen = srcFile.Open(fName, CFile::modeRead | CFile::typeBinary, &fe)))
	{
		TCHAR strCause[256];
		fe.GetErrorMessage(strCause, 255);
		TRACE("SendFileToRemoteRecipient encountered an error while opening the local file\n"
			"\tFile name = %s\n\tCause = %s\n\tm_cause = %d\n\tm_IOsError = %d\n",
			fe.m_strFileName, strCause, fe.m_cause, fe.m_lOsError);
		return FALSE;
	}
	ULONGLONG ullFileLength = srcFile.GetLength();
	int nSend, nRecv = 0;
	char szSend[256] = { 0 };
	memset(szSend, 0, 256);
	sprintf(szSend, "%s %d", pFileName, ullFileLength);
	nSend = sockClient.Send(szSend, strlen(szSend));

	char szContent[SEND_BUFFER_SIZE] = { 0 };
	nRecv = sockClient.Receive(szContent, RECV_BUFFER_SIZE);
	
	ULONGLONG ullLeftSize = ullFileLength;
	while (ullLeftSize > 0)
	{
		memset(szContent, 0, SEND_BUFFER_SIZE);
		nSend = srcFile.Read(szContent, SEND_BUFFER_SIZE);
		nSend = sockClient.Send(szContent, nSend);
		ullLeftSize -= nSend;
	}

	sockClient.Close();
	srcFile.Close();
	delete pFileName;
	return TRUE;
}

BOOL CTransSocket::RecvAVPacket(void* pPacket)
{
	if (pPacket == NULL)
		return FALSE;
	CSocket sockSrvr;
	sockSrvr.Create(PRE_AGREED_PORT);
	sockSrvr.Listen();
	CSocket sockConnection;
	sockSrvr.Accept(sockConnection);

	int nRecv = sockConnection.Receive(pPacket, RECV_BUFFER_SIZE);
	sockConnection.Close();
	sockSrvr.Close();

	return TRUE;
}

BOOL CTransSocket::SendAVPacket(CString strIPAddress, void* pPacket, int nSendSize)
{
	CSocket sockClient;
	sockClient.Create();
	sockClient.Connect(strIPAddress, PRE_AGREED_PORT);

	if (pPacket == NULL)
		return FALSE;
	if (nSendSize <= 0)
		nSendSize = SEND_BUFFER_SIZE;
	int nSend = sockClient.Send(pPacket, nSendSize);
	sockClient.Close();
	return TRUE;
}

// CTransSocket 成员函数
void CTransSocket::InitSocket(UINT nPort, int nSocketType, CString strSocketAddress)
{
	if (nSocketType == 1)
	{	//Server
		m_socket.Create(nPort);
		m_socket.Listen();
		m_socket.Accept(m_socketConn);
	}
	else
	{
		m_socket.Create();
		m_socket.Connect(strSocketAddress, nPort);
	}
	m_nType = nSocketType;
}

void CTransSocket::UninitSocket()
{
	m_socket.Close();
	m_socketConn.Close();
}

BOOL CTransSocket::SendToContinue()
{
	int nSend = 0;
	const char* pNotify = "continue";
	if (m_nType == 0)
		nSend = m_socket.Send(pNotify, strlen(pNotify));
	else
		nSend = m_socketConn.Send(pNotify, strlen(pNotify));
	
	return nSend > 0 ? TRUE : FALSE;
}

BOOL CTransSocket::RecvToContinue()
{
	char szNotify[50];
	memset(szNotify, 0, sizeof(szNotify));
	int nRecv = 0;
	if (m_nType == 0)
	{
		nRecv = m_socket.Receive(szNotify, sizeof(szNotify));
	}
	else
	{
		nRecv = m_socketConn.Receive(szNotify, sizeof(szNotify));
	}
	if (strcmp(szNotify, "continue") == 0 && nRecv > 0)
		return TRUE;
	else if (strcmp(szNotify, "trans_end") == 0 && nRecv > 0)
		return FALSE;
	else
		return FALSE;
}

BOOL CTransSocket::SendEndInfo()
{
	int nSend = 0;
	const char* pNotify = "trans_end";
	if (m_nType == 0)
		nSend = m_socket.Send(pNotify, strlen(pNotify));
	else
		nSend = m_socketConn.Send(pNotify, strlen(pNotify));

	return nSend > 0 ? TRUE : FALSE;
}

//Send/Recv Data Length 
int CTransSocket::SendDataLength(int nSize)
{
	char szDataLength[50];
	memset(szDataLength, 0, sizeof(szDataLength));
	sprintf(szDataLength, "DataLength: %d", nSize);
	int nSend = 0;
	if (m_nType == 0)
		nSend = m_socket.Send(szDataLength, strlen(szDataLength));
	else
		nSend = m_socketConn.Send(szDataLength, strlen(szDataLength));
	return nSend;
}

int CTransSocket::RecvDataLength(int& nSize)
{
	char szDataLength[50];
	memset(szDataLength, 0, sizeof(szDataLength));
	int nRecv = 0;
	if (m_nType == 0)
	{
		nRecv = m_socket.Receive(szDataLength, sizeof(szDataLength));
	}
	else
	{
		nRecv = m_socketConn.Receive(szDataLength, sizeof(szDataLength));
	}
	sscanf(szDataLength, "%*s %d", &nSize);
	return nRecv;
}

//Send Recv
int CTransSocket::SendData(void* pData, int nSendSize)
{
	int nSend = 0;
	if (m_nType == 0)
	{
		int nEachSend = 0;
		while (nSend < nSendSize)
		{
			nSend = m_socket.Send((char*)pData, nSendSize);
			//nEachSend = m_socket.Send((char*)pData + nSend, SEND_BUFFER_SIZE);
			nSend += nEachSend;
		}
	}
	else
	{
		int nEachSend = 0;
		while (nSend < nSendSize)
		{
			nEachSend = m_socketConn.Send((char*)pData, nSendSize);
			//nEachSend = m_socketConn.Send((char*)pData + nSend, SEND_BUFFER_SIZE);
			nSend += nEachSend;
		}
	}
	return nSend;
}

int CTransSocket::RecvData(void* pData, int nRecvSize)
{
	int nRecv = 0;
	if (m_nType == 0)
	{
		int nEachRecv, nEachRecvExpect = 0;
		while (nRecv < nRecvSize)
		{
			nEachRecv = m_socket.Receive((char*)pData + nRecv, nRecvSize);
			//nEachRecvReal = m_socket.Receive((char*)pData + nRecv, RECV_BUFFER_SIZE);
			nRecv += nEachRecv;
		}
		//nRecv = m_socket.Receive(pData, nRecvSize);
	}
	else
	{
		int nEachRecv = 0;
		while (nRecv < nRecvSize)
		{
			//nEachRecv = m_socketConn.Receive((char*)pData + nRecv, RECV_BUFFER_SIZE);
			nEachRecv = m_socketConn.Receive((char*)pData + nRecv, nRecvSize);
			nRecv += nEachRecv;
		}
		//nRecv = m_socketConn.Receive(pData, nRecvSize);
	}
	return nRecv;
}

void CTransSocket::Bind(CString strIPAddress, int nPort)
{
	m_socket.Bind(nPort, strIPAddress);
}

//Client
void CTransSocket::Connect(CString strIPAddress, int nPort)
{
	m_socket.Connect(strIPAddress, nPort);
}

//server 
void CTransSocket::Listen()
{
	m_socket.Listen();
}
void CTransSocket::Accept()
{
	CSocket socket;
	//m_socket.Accept(&socket)
	
}