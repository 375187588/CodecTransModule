#pragma once
#include "HPSocket\HPSocket.h"
#include "CommonDefine.h"
#include <deque>
// CTransClient 对话框

class CTransClient : public CDialogEx, public CTcpClientListener
{
	DECLARE_DYNAMIC(CTransClient)

public:
	CTransClient(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CTransClient();

// 对话框数据
	enum { IDD = IDD_DIALOG_CLIENT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonSend();
	afx_msg void OnBnClickedButtonClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
private:
	//As Client:
	virtual EnHandleResult OnReceive(IClient* pClient, int iLength);
	virtual EnHandleResult OnReceive(IClient* pClient, const BYTE* pData, int iLength);
	virtual EnHandleResult OnSend(IClient* pClient, const BYTE* pData, int iLength);
	virtual EnHandleResult OnClose(IClient* pClient, EnSocketOperation enOperation, int iErrorCode);
	virtual EnHandleResult OnConnect(IClient* pClient);

	void snap_shot_screen();

	BOOL InitOutputFile();
	BOOL CloseCaptureScreen();
	BOOL WriteFrame(LPBITMAPINFOHEADER bmp, int pts);
	LPBITMAPINFOHEADER captureScreenFrame(int left, int top, int width, int height, int tempDisableRect);
	HANDLE Bitmap2Dib(HBITMAP hbitmap, UINT bits);

	static UINT TransThread(LPVOID lpParam);
	void TransAVPacket();

private:
	HDC m_dcShow;
	HDC m_dcMem;
	HDC m_dcSrc;
	int m_nScreenWidth;
	int m_nScreenHeight;
	int m_nShowWidth;
	int m_nShowHeight;
	CString m_editServerAddr;
	UINT m_editServerPort;
	CTcpClientPtr m_ptrClient;
	std::deque<stScreenBuffer>	m_dequeScreenBuffer;
	std::deque<AVFrame*>			m_dequeAVFrame;
	bool m_bTerminal;				//Terminal to show
	CWinThread*		m_pSendThread;
	//
	int m_numBytes;
	int m_frame;
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
};
