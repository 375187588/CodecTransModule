// TransClient.cpp : 实现文件
//

#include "stdafx.h"
#include "CodecTransModule.h"
#include "TransClient.h"
#include "afxdialogex.h"
#include "BitmapMark.h"
#include "Global\helper.h"

#define SNAP_SCREEN 1
#define WRITE_FRAME_TO_FILE 0

//inline function saveBitmap
inline bool saveBitmap(char* name, unsigned char* buf, int w, int h, int bpp)
{
	FILE* f = NULL;
	f = fopen(name, "wb");
	if (!f)
		return false;

	BITMAPFILEHEADER header;
	header.bfType = ('M' << 8) | 'B';
	header.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	header.bfSize = header.bfOffBits + w*h*bpp / 8;
	header.bfReserved1 = 0;
	header.bfReserved2 = 0;

	BITMAPINFO info;
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = w;
	info.bmiHeader.biHeight = -h;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = bpp;
	info.bmiHeader.biCompression = BI_RGB;
	info.bmiHeader.biSizeImage = 0;
	info.bmiHeader.biXPelsPerMeter = 100;
	info.bmiHeader.biYPelsPerMeter = 100;
	info.bmiHeader.biClrUsed = 0;
	info.bmiHeader.biClrImportant = 0;

	fwrite(&header, sizeof(BITMAPFILEHEADER), 1, f);
	fwrite(&info.bmiHeader, sizeof(BITMAPINFOHEADER), 1, f);
	fwrite(buf, w*h*bpp / 8, 1, f);
	fclose(f);

	return true;
}

// CTransClient 对话框

IMPLEMENT_DYNAMIC(CTransClient, CDialogEx)

CTransClient::CTransClient(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTransClient::IDD, pParent)
	, m_editServerAddr(_T("127.0.0.1"))
	, m_editServerPort(8888)
{
	m_ptrClient = NULL;
	m_bTerminal = false;
	m_pSendThread = NULL;
	m_dequeAVPacket.clear();
	video_st = NULL;
	pFormatCtx = NULL;
	picture = NULL;
	picture_buf = NULL;
	pFrame = NULL;
	buffer = NULL;
	m_frame = 0;
}

CTransClient::~CTransClient()
{

}

void CTransClient::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_IPADDR, m_editServerAddr);
	DDX_Text(pDX, IDC_EDIT_PORT, m_editServerPort);
}


BEGIN_MESSAGE_MAP(CTransClient, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_START, &CTransClient::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CTransClient::OnBnClickedButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CTransClient::OnBnClickedButtonClose)
	ON_WM_TIMER()
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CTransClient 消息处理程序

//As Client:
EnHandleResult CTransClient::OnReceive(IClient* pClient, int iLength)
{
	//::PostOnReceive(pClient->GetConnectionID(), pClient->)
	return HR_OK;
}

EnHandleResult CTransClient::OnReceive(IClient* pClient, const BYTE* pData, int iLength)
{
	::PostOnReceive(pClient->GetConnectionID(), pData, iLength);
	return HR_OK;
}

EnHandleResult CTransClient::OnSend(IClient* pClient, const BYTE* pData, int iLength)
{
	::PostOnSend(pClient->GetConnectionID(), pData, iLength);
	return HR_OK;
}

EnHandleResult CTransClient::OnClose(IClient* pClient, EnSocketOperation enOperation, int iErrorCode)
{
	::PostOnConnect(pClient->GetConnectionID(), m_editServerAddr, m_editServerPort);
	return HR_OK;
}

EnHandleResult CTransClient::OnConnect(IClient* pClient)
{
	::PostOnConnect(pClient->GetConnectionID(), m_editServerAddr, m_editServerPort);
	return HR_OK;
}

void CTransClient::snap_shot_screen()
{
	LPBITMAPINFOHEADER alpbi = NULL;

	alpbi = captureScreenFrame(0, 0, m_nScreenWidth, m_nScreenHeight, TRUE);

	if (alpbi == NULL)
		return;

	WriteFrame(alpbi, m_frame);

	CDC *pToPaint = CDC::FromHandle(::GetDC(GetDlgItem(IDC_STATIC_SHOW)->m_hWnd));
	pToPaint->SetStretchBltMode(HALFTONE);
	CRect tmpRect;
	GetDlgItem(IDC_STATIC_SHOW)->GetClientRect(&tmpRect);

	LPBITMAPINFO tmpBmp = (LPBITMAPINFO)alpbi;
	CDC MemDC;
	MemDC.CreateCompatibleDC(NULL);
	CBitmap memBmp;
	memBmp.CreateCompatibleBitmap(pToPaint, m_nScreenWidth, m_nScreenHeight);
	CBitmap *pOldBit = MemDC.SelectObject(&memBmp);
	//设置DC拉伸、缩放模式
	MemDC.SetStretchBltMode(HALFTONE);

	StretchDIBits(MemDC.m_hDC, 0, 0, m_nScreenWidth, m_nScreenHeight, 0, 0, m_nScreenWidth, m_nScreenHeight, alpbi, tmpBmp, DIB_RGB_COLORS, SRCCOPY);
	pToPaint->StretchBlt(tmpRect.left, tmpRect.top, tmpRect.Width(), tmpRect.Height(), &MemDC, 0, 0, m_nScreenWidth, m_nScreenHeight, SRCCOPY);

	MemDC.SelectObject(pOldBit);
	memBmp.DeleteObject();
	MemDC.DeleteDC();
	::ReleaseDC(NULL, pToPaint->m_hDC);

	GlobalFree(alpbi);
}

BOOL CTransClient::WriteFrame(LPBITMAPINFOHEADER bmp, int pts)
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
		return FALSE;
	}
	if (got_picture == 1)
	{
		pkt.stream_index = video_st->index;
		//push in packet deque
		CSingleLock lock(&m_cs, TRUE);
		if (m_dequeAVPacket.size() < MAX_BUFFER_SIZE)
		{
			m_dequeAVPacket.push_back(pkt);
		}
		//now we write temporary
#if WRITE_FRAME_TO_FILE
		ret = av_write_frame(pFormatCtx, &pkt);
#endif
		lock.Unlock();
	}
	return ret > 0 ? TRUE : FALSE;
}

BOOL CTransClient::InitOutputFile()
{
	av_register_all();
	AVOutputFormat* fmt;
	AVCodec* pCodec;

	const char* out_file = "client.avi";

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


LPBITMAPINFOHEADER CTransClient::captureScreenFrame(int left, int top, int width, int height, int tempDisableRect)
{
	HDC hScreenDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);

	HDC hMemDC = ::CreateCompatibleDC(hScreenDC);
	HBITMAP hbm;
	hbm = CreateCompatibleBitmap(hScreenDC, width, height);
	HBITMAP oldbm = (HBITMAP)SelectObject(hMemDC, hbm);
	BitBlt(hMemDC, 0, 0, width, height, hScreenDC, left, top, SRCCOPY | CAPTUREBLT);

	//add tip text
	SYSTEMTIME tm;
	GetLocalTime(&tm);
	CString str;
	str.Format(_T("%2d:%02d:%02d:%03d"), tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);

	BitmapMark mark(hbm, hMemDC);
	mark.addText(str, 0, 0);

	str.Format(_T("%lld"), m_frame);
	m_frame++;
	mark.addText(str, 10, 10);
	//add end


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

HANDLE CTransClient::Bitmap2Dib(HBITMAP hbitmap, UINT bits)
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

UINT CTransClient::TransThread(LPVOID lpParam)
{
	if (lpParam != NULL)
		((CTransClient*)lpParam)->TransAVPacket();
	return 0;
}

void CTransClient::TransAVPacket()
{
	DWORD dwErr = 0;
	while (!m_bTerminal)
	{	//check the deque exists data, if exists then render
		CSingleLock lock(&m_cs, TRUE);
		if (m_dequeAVPacket.size() > 0)
		{
			AVPacket* pPacket = static_cast<AVPacket*>(&m_dequeAVPacket.at(0));
			stDataLength stDataLen;
			stDataLen.nDataLength = pPacket->size;
			//Trans avFrame To Server
			BOOL bSend;
			if(m_ptrClient != NULL)
			{
				bSend = m_ptrClient->Send((BYTE*)pPacket->data, pPacket->size, 0);
				dwErr = GetLastError();
				if (bSend == FALSE)
				{
					CString strError;
					strError.Format(L"error number is : %d\n", dwErr);
					OutputDebugString(strError);
				}
			}

			av_free_packet(pPacket);
			m_dequeAVPacket.pop_front();
		}
		lock.Unlock();
		Sleep(1);//Sleep(50); 
	}
}

void CTransClient::OnBnClickedButtonStart()
{
	//client start socket
	if (!m_ptrClient->Start(m_editServerAddr, m_editServerPort))
		AfxMessageBox(L"connect server fail..");
}

void CTransClient::OnBnClickedButtonSend()
{
	if (InitOutputFile())
	{
		SetTimer(SNAP_SCREEN, 10, NULL);
	}
	// trans thread
	m_pSendThread = AfxBeginThread(TransThread, (LPVOID)this);
}

void CTransClient::OnBnClickedButtonClose()
{
	if (m_ptrClient)
	{
		m_ptrClient->Stop();
		//HP_Destroy_TcpClient(m_ptrClient);
	}

	CSingleLock lock(&m_cs, TRUE);
	while (m_dequeAVPacket.size() > 0)
	{
		AVPacket* packet = static_cast<AVPacket*>(&m_dequeAVPacket.at(0));
		m_dequeAVPacket.pop_front();
	}
	lock.Unlock();
}

BOOL CTransClient::CloseCaptureScreen()
{
	BOOL bRet = TRUE;
#if WRITE_FRAME_TO_FILE
	//Flush Encoder
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
#endif
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
	if (picture != NULL)
		av_free(picture);

	if (picture_buf != NULL)
		delete[] picture_buf;
	
	if (pFrame != NULL)
		av_free(pFrame);

	if (buffer != NULL)
		delete[] buffer;
	
	return bRet;
}


BOOL CTransClient::OnInitDialog()
{
	__super::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetWindowText(L"Client");
	UpdateData(FALSE);

	//Get Screen Size
	CRect rect;
	GetDesktopWindow()->GetWindowRect(rect);
	m_nScreenWidth = rect.Width();
	m_nScreenHeight = rect.Height();

	m_dcShow = *(GetDlgItem(IDC_STATIC_SHOW)->GetDC());
	
	GetDlgItem(IDC_STATIC_SHOW)->GetClientRect(&rect);
	m_nShowWidth = rect.Width();
	m_nShowHeight = rect.Height();
	
	//init network para
	{
		if (m_ptrClient == NULL)
		{
			m_ptrClient = HP_Create_TcpClient(this);
			m_ptrClient->SetSocketBufferSize(100 * 1024);
		}
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}


void CTransClient::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case SNAP_SCREEN:
		snap_shot_screen();
		break;
	default:
		break;
	}
	__super::OnTimer(nIDEvent);
}

void CTransClient::OnClose()
{
	m_bTerminal = true;

	//KillTimer(SNAP_SCREEN);
	CloseCaptureScreen();
	OnBnClickedButtonClose();
	__super::OnClose();
}