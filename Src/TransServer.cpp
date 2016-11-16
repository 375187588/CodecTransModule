// TransServer.cpp : 实现文件
//

#include "stdafx.h"
#include "CodecTransModule.h"
#include "TransServer.h"
#include "afxdialogex.h"

#define SCREEN_SHOT	100
#define DECODE_PACKET 101
const int g_useFormatCtx = 0;
// CTransServer 对话框

IMPLEMENT_DYNAMIC(CTransServer, CDialogEx)

CTransServer::CTransServer(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTransServer::IDD, pParent)
	, m_editDenyAddr(_T("127.0.0.1"))
	, m_editPort(8888)
{
	m_nScreenWidth = 0;
	m_nScreenHeight = 0;
	m_nShowWidth = 0;
	m_nShowHeight = 0;
	m_frame = 0;
	m_dequeAVPacket.clear();
	m_pDecodeContext = NULL;
	m_pSWS = NULL;
	m_nFrameIndex = 0;
	m_bTerminal = false;
}

CTransServer::~CTransServer()
{
}

void CTransServer::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_IPADDR, m_editDenyAddr);
	DDX_Text(pDX, IDC_EDIT_PORT, m_editPort);
}


BEGIN_MESSAGE_MAP(CTransServer, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_START, &CTransServer::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_RECV, &CTransServer::OnBnClickedButtonRecv)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CTransServer::OnBnClickedButtonClose)
	ON_WM_TIMER()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

BOOL CTransServer::OnInitDialog()
{
	__super::OnInitDialog();
	SetWindowText(L"Server");
	//Get Screen Size
	CRect rect;
	GetDesktopWindow()->GetWindowRect(rect);
	m_nScreenWidth = rect.Width();
	m_nScreenHeight = rect.Height();

	GetDlgItem(IDC_STATIC_SHOW)->GetClientRect(rect);
	m_nShowWidth = rect.Width();
	m_nShowHeight = rect.Height();
	m_dcShow = ::GetDC(GetDlgItem(IDC_STATIC_SHOW)->GetSafeHwnd());

	return TRUE;
}

// CTransServer 消息处理程序

//As Server:
EnHandleResult CTransServer::OnReceive(CONNID dwConnID, int iLength)
{
	return HR_OK;
}

EnHandleResult CTransServer::OnReceive(CONNID dwConnID, const BYTE* pData, int iLength)
{
	AVPacket packet;
	av_init_packet(&packet);
	int nRet = av_new_packet(&packet, iLength);

	memcpy(packet.data, pData, iLength);
	packet.dts = packet.pts = m_nFrameIndex++;
	packet.stream_index = 0;
	packet.flags = 1;
	packet.stream_index = 0;

	m_cs.Lock();
	if (m_dequeAVPacket.size() < MAX_BUFFER_SIZE)
		m_dequeAVPacket.push_back(packet);
	m_cs.Unlock();

	const char* pFeedback = "recv data";
	if (m_ptrServer->Send(dwConnID, (BYTE*)pFeedback, strlen(pFeedback)))
		return HR_OK;
	else
		return HR_ERROR;
}

EnHandleResult CTransServer::OnSend(CONNID dwConnID, const BYTE* pData, int iLength)
{
	::PostOnSend(dwConnID, pData, iLength);
	return HR_OK;
}

EnHandleResult CTransServer::OnPrepareListen(SOCKET soListen)
{
	return HR_OK;
}

EnHandleResult CTransServer::OnAccept(CONNID dwConnID, SOCKET soClient)
{
	BOOL bPass = TRUE;
	TCHAR szAddress[40];
	int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
	USHORT usPort;

	m_ptrServer->GetRemoteAddress(dwConnID, szAddress, iAddressLen, usPort);
	
	if (!m_editDenyAddr.IsEmpty())
	{
		if (m_editDenyAddr.CompareNoCase(szAddress) == 0)
			bPass = FALSE;
	}

	::PostOnAccept(dwConnID, szAddress, usPort, bPass);
	return HR_OK;
}

EnHandleResult CTransServer::OnShutdown()
{
	::PostOnShutdown();
	return HR_OK;
}

EnHandleResult CTransServer::OnClose(CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
	iErrorCode == SE_OK ? ::PostOnClose(dwConnID) :
		::PostOnError(dwConnID, enOperation, iErrorCode);
	
	return HR_OK;
}

void CTransServer::OnBnClickedButtonStart()
{
	if (m_ptrServer == NULL)
	{
		m_ptrServer = HP_Create_TcpServer(this);
		m_ptrServer->SetSocketBufferSize(100 * 1024);
	}
	if (m_ptrServer)
		m_ptrServer->Start(L"127.0.0.1", m_editPort);
}


void CTransServer::OnBnClickedButtonRecv()
{
	if (InitOutputFile())
	{
		//SetTimer(SCREEN_SHOT, 100, NULL);
		SetTimer(DECODE_PACKET, /*20*/1, NULL);
		//AfxBeginThread(RendThread, this);
	}
	m_nFrameIndex = 0;
}


void CTransServer::OnBnClickedButtonClose()
{
	HP_Destroy_TcpServer(m_ptrServer);

	//free buffer
	KillTimer(SCREEN_SHOT);
	KillTimer(DECODE_PACKET);
	CloseCaptureScreen();
}

//////////////////////////////////////////////////////////////////
//try another way
BOOL CTransServer::InitOutputFile()
{
	av_register_all();
	AVOutputFormat* fmt;
	AVCodec* pCodec;

	const char* out_file = "server.avi";
	if (g_useFormatCtx == 1)
	{	//not use now, only for client, save for corp with client
		pFormatCtx = avformat_alloc_context();
		fmt = av_guess_format(NULL, out_file, NULL);
		pFormatCtx->oformat = fmt;

		//注意输出路径
		if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0)
		{
			AfxMessageBox(L"打开文件失败");
			return FALSE;
		}

		video_st = avformat_new_stream(pFormatCtx, NULL);
		if (video_st == NULL)
		{
			return FALSE;
		}
		pCodecCtx = video_st->codec;
		pCodecCtx->codec_id = AV_CODEC_ID_MPEG4;

		pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
		pCodecCtx->width = m_nScreenWidth;
		pCodecCtx->height = m_nScreenHeight;
		pCodecCtx->time_base.num = 1;
		pCodecCtx->time_base.den = 8;
		pCodecCtx->bit_rate = 3000000;
		pCodecCtx->global_quality = 300;
		pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
		pCodecCtx->gop_size = 1;

		//输出格式信息
		av_dump_format(pFormatCtx, 0, out_file, 1);
		pCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
		if (!pCodec)
		{
			AfxMessageBox(TEXT("没有找到合适的编码器！\n"));
			return FALSE;
		}
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		{
			AfxMessageBox(L"编码器打开失败！\n");
			return FALSE;
		}
		//写文件头
		avformat_write_header(pFormatCtx, NULL);

		picture = av_frame_alloc();
		int nSize = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
		picture_buf = new uint8_t[nSize];
		avpicture_fill((AVPicture*)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

		pFrame = av_frame_alloc();
		m_numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
		buffer = new uint8_t[m_numBytes];

		int y_size = pCodecCtx->width * pCodecCtx->height;
		av_new_packet(&pkt, y_size * 3);

		img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB32, pCodecCtx->width,
			pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	}

	///decode part
	AVCodec* pCodecDe = avcodec_find_decoder(AV_CODEC_ID_MPEG4);
	if (pCodecDe == NULL)
	{
		AfxMessageBox(L"创建解码器失败！");
		return FALSE;
	}
	m_pDecodeContext = avcodec_alloc_context3(pCodecDe);
	if (avcodec_open2(m_pDecodeContext, pCodecDe, NULL) < 0)
	{
		AfxMessageBox(L"打开解码器失败!");
		return FALSE;
	}

	return TRUE;
}

BOOL CTransServer::CloseCaptureScreen()
{
	BOOL bRet = TRUE;
	//Flush Encoder
	if (g_useFormatCtx == 1)
	{
		if (pFormatCtx != NULL)
		{
			int ret = flush_encoder(pFormatCtx, 0);
			if (ret < 0) {
				printf("Flushing encoder failed\n");
				bRet = FALSE;
			}
			else
			{
				//写文件尾
				av_write_trailer(pFormatCtx);
			}
		}
		//clear
		if (video_st)
		{
			avcodec_close(video_st->codec);
		}
		if (pFormatCtx != NULL)
		{
			avio_close(pFormatCtx->pb);
			avformat_free_context(pFormatCtx);
		}
		pFormatCtx = NULL;

		av_free(picture);
		delete[] picture_buf;
		av_free(pFrame);
		delete[] buffer;

	}

	return bRet;
}

void CTransServer::CaptureScreen()
{
	LPBITMAPINFOHEADER alpbi = NULL;

	alpbi = captureScreenFrame(0, 0, m_nScreenWidth, m_nScreenHeight, TRUE);
	WriteFrame(alpbi, m_frame);

	GlobalFree(alpbi);
}

void CTransServer::DecodePacket()
{
	AVFrame* pFrame = av_frame_alloc();
	int nGotPic = 0;
	int nRet = 0;
	m_cs.Lock();
	if (m_dequeAVPacket.size() > 0)
	{
		AVPacket* packet = static_cast<AVPacket*>(&m_dequeAVPacket.at(0));

		nRet = avcodec_decode_video2(m_pDecodeContext, pFrame, &nGotPic, packet);
		if (nRet < 0 || nGotPic == 0)
			goto free_buffer;

		AVPicture *pPic = new AVPicture;
		//把帧格式转化为RGBA格式
		AVFrameToRGBACBitmap(pFrame, pPic);

		int nRet = 0;
		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);//BITMAPINFO结构大小
		bmi.bmiHeader.biBitCount = 32;                  //32位
		bmi.bmiHeader.biCompression = BI_RGB;           //未压缩的RGB
		bmi.bmiHeader.biWidth = m_nScreenWidth;         //图片宽,原始图形是倒的,所以这里用负号进行翻转
		bmi.bmiHeader.biHeight = -m_nScreenHeight;      //图片高
		bmi.bmiHeader.biPlanes = 1;                     //调色板,因为是彩色的,所以设置为1	
		//bmi.bmiHeader.biSizeImage = pPic->linesize[0];//图片的大小可以不用

		CRect rect;
		GetClientRect(&rect);

		CDC* pDc = CDC::FromHandle(m_dcShow);
		//设置缩放模式
		SetStretchBltMode(pDc->GetSafeHdc(), HALFTONE);
		//SetBrushOrgEx(m_dcShow, 0, 0, NULL);
		//把DIB位图画到设备上显示出来
		StretchDIBits(pDc->GetSafeHdc(),    //设备上下文句柄
			0, 0, m_nShowWidth, m_nShowHeight,     //图片的大小
			0, 0, m_nScreenWidth, m_nScreenHeight,      //图片的大小
			pPic->data[0],              //RGBA位图数据
			&bmi,                       //BITMAPINFO结构指针,用于描述位图
			DIB_RGB_COLORS,             //RGB格式	
			SRCCOPY);                   //操作运算,复制
	
		avpicture_free(pPic);
		delete pPic;
	
free_buffer:

		m_dequeAVPacket.pop_front();
		av_free_packet(packet);
	}
	m_cs.Unlock();
	av_frame_free(&pFrame);
}

BOOL CTransServer::WriteFrame(LPBITMAPINFOHEADER bmp, int pts)
{
	memset(buffer, 0, m_numBytes);
	uint8_t* tmpBuf = (uint8_t*)(bmp + 1);
	for (int i = 0; i < pCodecCtx->height; i++)
	{
		memcpy(buffer + i * (pCodecCtx->width * 4), (tmpBuf + (pCodecCtx->width * 4)*(pCodecCtx->height - i - 1)), (pCodecCtx->width * 4));
	}
	int ret1 = avpicture_fill((AVPicture *)pFrame, buffer, AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);

	sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, picture->data, picture->linesize);

	//PTS
	picture->pts = pts;
	int got_picture = 0;

	//av_init_packet(&pkt);
	//编码
	AVPacket pkt;
	av_new_packet(&pkt, m_nScreenWidth * m_nScreenHeight * 3);

	int ret = avcodec_encode_video2(pCodecCtx, &pkt, picture, &got_picture);
	if (ret < 0)
	{
		AfxMessageBox(L"编码错误！\n");
		return -1;
	}
	if (got_picture == 1)
	{
		pkt.stream_index = video_st->index;
		//push in packet deque
		m_cs.Lock();
		if (m_dequeAVPacket.size() < MAX_BUFFER_SIZE)
			m_dequeAVPacket.push_back(pkt);
		m_cs.Unlock();
		
		ret = av_write_frame(pFormatCtx, &pkt);
	}
}

LPBITMAPINFOHEADER CTransServer::captureScreenFrame(int left, int top, int width, int height, int tempDisableRect)
{
	HDC hScreenDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);

	HDC hMemDC = ::CreateCompatibleDC(hScreenDC);
	HBITMAP hbm;
	hbm = CreateCompatibleBitmap(hScreenDC, width, height);
	HBITMAP oldbm = (HBITMAP)SelectObject(hMemDC, hbm);
	BitBlt(hMemDC, 0, 0, width, height, hScreenDC, left, top, SRCCOPY | CAPTUREBLT);

	SelectObject(hMemDC, oldbm);
	LPBITMAPINFOHEADER pBM_HEADER = (LPBITMAPINFOHEADER)GlobalLock(Bitmap2Dib(hbm, 32));
	if (pBM_HEADER == NULL)
	{
		return NULL;
	}

	DeleteObject(hbm);
	DeleteDC(hMemDC);
	::ReleaseDC(NULL, hScreenDC);
	return pBM_HEADER;
}

HANDLE CTransServer::Bitmap2Dib(HBITMAP hbitmap, UINT bits)
{
	HANDLE              hdib = NULL;
	HDC                 hdc;
	BITMAP              bitmap;
	UINT                wLineLen;
	DWORD               dwSize;
	DWORD               wColSize;
	LPBITMAPINFOHEADER  lpbi;
	LPBYTE              lpBits;

	GetObject(hbitmap, sizeof(BITMAP), &bitmap);

	//
	// DWORD align the width of the DIB
	// Figure out the size of the colour table
	// Calculate the size of the DIB
	//
	wLineLen = (bitmap.bmWidth*bits + (bits - 1)) / (bits)* (bits / 8);
	wColSize = sizeof(RGBQUAD)*((bits <= 8) ? 1 << bits : 0);
	dwSize = sizeof(BITMAPINFOHEADER)+wColSize +
		(DWORD)(UINT)wLineLen*(DWORD)(UINT)bitmap.bmHeight;

	//
	// Allocate room for a DIB and set the LPBI fields
	//
	hdib = GlobalAlloc(GHND, dwSize);
	if (!hdib)
		return hdib;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib);

	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biWidth = bitmap.bmWidth;
	lpbi->biHeight = bitmap.bmHeight;
	lpbi->biPlanes = 1;
	lpbi->biBitCount = (WORD)bits;
	lpbi->biCompression = BI_RGB;
	lpbi->biSizeImage = dwSize - sizeof(BITMAPINFOHEADER)-wColSize;
	lpbi->biXPelsPerMeter = 0;
	lpbi->biYPelsPerMeter = 0;
	lpbi->biClrUsed = (bits <= 8) ? 1 << bits : 0;
	lpbi->biClrImportant = 0;

	//
	// Get the bits from the bitmap and stuff them after the LPBI
	//
	lpBits = (LPBYTE)(lpbi + 1) + wColSize;

	hdc = CreateCompatibleDC(NULL);

	GetDIBits(hdc, hbitmap, 0, bitmap.bmHeight, lpBits, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

	lpbi->biClrUsed = (bits <= 8) ? 1 << bits : 0;

	DeleteDC(hdc);
	GlobalUnlock(hdib);

	return hdib;
}

UINT CTransServer::RendThread(LPVOID lpParam)
{
	if (lpParam != NULL)
		((CTransServer*)lpParam)->RendAVPacket();
	return 0;
}

void CTransServer::RendAVPacket()
{
	while (!m_bTerminal)
	{
		DecodePacket();
		Sleep(10);
	}
}

void CTransServer::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case SCREEN_SHOT:
		CaptureScreen();
		break;
	case DECODE_PACKET:
		DecodePacket();
		break;
	default:
		break;
	}
	__super::OnTimer(nIDEvent);
}

void CTransServer::OnClose()
{
	KillTimer(SCREEN_SHOT);
	KillTimer(DECODE_PACKET);
	//free buffer
	CloseCaptureScreen();
	__super::OnClose();
}