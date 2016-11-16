#pragma once
#include "HPSocket\HPSocket.h"
#include "Global\helper.h"
#include "CommonDefine.h"
#include <deque>

// CTransServer 对话框
class CTransServer : public CDialogEx, public CTcpServerListener
{
	DECLARE_DYNAMIC(CTransServer)

public:
	CTransServer(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CTransServer();

// 对话框数据
	enum { IDD = IDD_DIALOG_SERVER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonRecv();
	afx_msg void OnBnClickedButtonClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();

private:
	//As Server:
	virtual EnHandleResult OnReceive(CONNID dwConnID, int iLength);
	virtual EnHandleResult OnReceive(CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnSend(CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnPrepareListen(SOCKET soListen);
	virtual EnHandleResult OnAccept(CONNID dwConnID, SOCKET soClient);
	virtual EnHandleResult OnShutdown();
	virtual EnHandleResult OnClose(CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);
protected:
	BOOL InitOutputFile();
	void CaptureScreen();
	BOOL CloseCaptureScreen();

	BOOL WriteFrame(LPBITMAPINFOHEADER bmp, int pts);
	LPBITMAPINFOHEADER captureScreenFrame(int left, int top, int width, int height, int tempDisableRect);
	HANDLE Bitmap2Dib(HBITMAP hbitmap, UINT bits);

	void DecodePacket();

	static UINT RendThread(LPVOID lpParam);
	void RendAVPacket();

private:
	CString m_editDenyAddr;
	UINT m_editPort;
	CTcpServerPtr	m_ptrServer;

	int m_nScreenWidth;
	int m_nScreenHeight;
	int m_nShowWidth;
	int m_nShowHeight;

	int m_numBytes;
	int m_frame;
	HDC m_dcShow;
	AVCodecContext* pCodecCtx;
	AVPacket pkt;
	AVStream* video_st;
	AVFormatContext* pFormatCtx;
	AVFrame *pFrame;
	AVFrame* picture;
	uint8_t* picture_buf;
	uint8_t* buffer;
	SwsContext *img_convert_ctx;
	std::deque<AVPacket> m_dequeAVPacket;

	//decode
	AVCodecContext* m_pDecodeContext;
	CCriticalSection m_cs;
	SwsContext*		m_pSWS;
	int m_nFrameIndex;
	bool m_bTerminal;				//Terminal to show
};
