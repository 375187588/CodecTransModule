
// CodecTransModuleDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CodecTransModule.h"
#include "CodecTransModuleDlg.h"
#include "afxdialogex.h"
#include "Utility.h"
#include "CommonDefine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavdevice\avdevice.h"
#include "libavfilter\avfilter.h"
#include "libavformat\avformat.h"
#include "libswscale\swscale.h"
#include "SDL.h"
}

#if CONFIG_AVFILTER
# include "libavfilter/avcodec.h"
# include "libavfilter/avfilter.h"
# include "libavfilter/avfiltergraph.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
//SDL刷新
//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit = 0;
int thread_pause = 0;

int sfp_refresh_thread(void *opaque){
	thread_exit = 0;
	thread_pause = 0;

	while (!thread_exit) {
		if (!thread_pause){
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		SDL_Delay(40);
	}
	thread_exit = 0;
	thread_pause = 0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////

//use GDI to paint
//set '1' to choose a type of file to play
#define LOAD_BGRA    0
#define LOAD_RGB24   0
#define LOAD_BGR24   0
#define LOAD_YUV420P 1

//Width, Height
const int screen_w = 500, screen_h = 500;
const int pixel_w = 320, pixel_h = 180;

//Bit per Pixel
#if LOAD_BGRA
const int bpp = 32;
#elif LOAD_RGB24|LOAD_BGR24
const int bpp = 24;
#elif LOAD_YUV420P
const int bpp = 12;
#endif

FILE *fp = NULL;

//Storage frame data
unsigned char buffer[pixel_w*pixel_h*bpp / 8];

unsigned char buffer_convert[pixel_w*pixel_h * 3];

#if 0 //include CommonDefine.h

//Not Efficient, Just an example
//change endian of a pixel (32bit)
void CHANGE_ENDIAN_32(unsigned char *data){
	char temp3, temp2;
	temp3 = data[3];
	temp2 = data[2];
	data[3] = data[0];
	data[2] = data[1];
	data[0] = temp3;
	data[1] = temp2;
}

//change endian of a pixel (24bit)
void CHANGE_ENDIAN_24(unsigned char *data){
	char temp2 = data[2];
	data[2] = data[0];
	data[0] = temp2;
}

//RGBA to RGB24 (or BGRA to BGR24)
void CONVERT_RGBA32toRGB24(unsigned char *image, int w, int h){
	for (int i = 0; i<h; i++)
	for (int j = 0; j<w; j++){
		memcpy(image + (i*w + j) * 3, image + (i*w + j) * 4, 3);
	}
}

//RGB24 to BGR24
void CONVERT_RGB24toBGR24(unsigned char *image, int w, int h){
	for (int i = 0; i<h; i++)
	for (int j = 0; j<w; j++){
		char temp2;
		temp2 = image[(i*w + j) * 3 + 2];
		image[(i*w + j) * 3 + 2] = image[(i*w + j) * 3 + 0];
		image[(i*w + j) * 3 + 0] = temp2;
	}
}

//Change endian of a picture
void CHANGE_ENDIAN_PIC(unsigned char *image, int w, int h, int bpp){
	unsigned char *pixeldata = NULL;
	for (int i = 0; i<h; i++)
	for (int j = 0; j<w; j++){
		pixeldata = image + (i*w + j)*bpp / 8;
		if (bpp == 32){
			CHANGE_ENDIAN_32(pixeldata);
		}
		else if (bpp == 24){
			CHANGE_ENDIAN_24(pixeldata);
		}
	}
}

inline unsigned char CONVERT_ADJUST(double tmp)
{
	return (unsigned char)((tmp >= 0 && tmp <= 255) ? tmp : (tmp < 0 ? 0 : 255));
}

//YUV420P to RGB24
void CONVERT_YUV420PtoRGB24(unsigned char* yuv_src, unsigned char* rgb_dst, int nWidth, int nHeight)
{
	unsigned char *tmpbuf = (unsigned char *)malloc(nWidth*nHeight * 3);
	unsigned char Y, U, V, R, G, B;
	unsigned char* y_planar, *u_planar, *v_planar;
	int rgb_width, u_width;
	rgb_width = nWidth * 3;
	u_width = (nWidth >> 1);
	int ypSize = nWidth * nHeight;
	int upSize = (ypSize >> 2);
	int offSet = 0;

	y_planar = yuv_src;
	u_planar = yuv_src + ypSize;
	v_planar = u_planar + upSize;

	for (int i = 0; i < nHeight; i++)
	{
		for (int j = 0; j < nWidth; j++)
		{
			// Get the Y value from the y planar
			Y = *(y_planar + nWidth * i + j);
			// Get the V value from the u planar
			offSet = (i >> 1) * (u_width)+(j >> 1);
			V = *(u_planar + offSet);
			// Get the U value from the v planar
			U = *(v_planar + offSet);

			// Cacular the R,G,B values
			// Method 1
			R = CONVERT_ADJUST((Y + (1.4075 * (V - 128))));
			G = CONVERT_ADJUST((Y - (0.3455 * (U - 128) - 0.7169 * (V - 128))));
			B = CONVERT_ADJUST((Y + (1.7790 * (U - 128))));
			/*
			// The following formulas are from MicroSoft' MSDN
			int C,D,E;
			// Method 2
			C = Y - 16;
			D = U - 128;
			E = V - 128;
			R = CONVERT_ADJUST(( 298 * C + 409 * E + 128) >> 8);
			G = CONVERT_ADJUST(( 298 * C - 100 * D - 208 * E + 128) >> 8);
			B = CONVERT_ADJUST(( 298 * C + 516 * D + 128) >> 8);
			R = ((R - 128) * .6 + 128 )>255?255:(R - 128) * .6 + 128;
			G = ((G - 128) * .6 + 128 )>255?255:(G - 128) * .6 + 128;
			B = ((B - 128) * .6 + 128 )>255?255:(B - 128) * .6 + 128;
			*/

			offSet = rgb_width * i + j * 3;

			rgb_dst[offSet] = B;
			rgb_dst[offSet + 1] = G;
			rgb_dst[offSet + 2] = R;
		}
	}
	free(tmpbuf);
}

#endif
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCodecTransModuleDlg 对话框



CCodecTransModuleDlg::CCodecTransModuleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCodecTransModuleDlg::IDD, pParent)
	, m_radioClientType(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_nMode = 1;
	m_nShowWidth = 0;
	m_nShowHeight = 0;
}

void CCodecTransModuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_CLIENT, m_radioClientType);
	DDX_Control(pDX, IDC_IPADDRESS_SERVER, m_ipCtrlServer);
}

BEGIN_MESSAGE_MAP(CCodecTransModuleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CCodecTransModuleDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_CONVERT2YUV, &CCodecTransModuleDlg::OnBnClickedButtonConvert2yuv)
	ON_BN_CLICKED(IDC_BUTTON_SNAPSCREEN, &CCodecTransModuleDlg::OnBnClickedButtonSnapscreen)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CCodecTransModuleDlg::OnBnClickedButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_RECV, &CCodecTransModuleDlg::OnBnClickedButtonRecv)
	ON_BN_CLICKED(IDC_BUTTON_CHOOSEFILE, &CCodecTransModuleDlg::OnBnClickedButtonChoosefile)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CCodecTransModuleDlg::OnBnClickedButtonPlay)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_RADIO_CLIENT, &CCodecTransModuleDlg::OnBnClickedRadioClient)
	ON_BN_CLICKED(IDC_RADIO_SERVER, &CCodecTransModuleDlg::OnBnClickedRadioClient)
	ON_BN_CLICKED(IDC_BUTTON_DECODESEND, &CCodecTransModuleDlg::OnBnClickedButtonDecodesend)
	ON_BN_CLICKED(IDC_BUTTON_RECVPLAY, &CCodecTransModuleDlg::OnBnClickedButtonRecvplay)
END_MESSAGE_MAP()


// CCodecTransModuleDlg 消息处理程序

BOOL CCodecTransModuleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	InitFFmpeg();
	InitCtrl();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCodecTransModuleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CCodecTransModuleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CCodecTransModuleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCodecTransModuleDlg::InitCtrl()
{
	m_ipCtrlServer.SetWindowText(L"127.0.0.1");
	SetDlgItemText(IDC_EDIT_SENDFILE, L"D:\\Works\\编解码传输调研\\CodecTransModule\\Src\\Titanic.mkv");
	RECT rect;
	(GetDlgItem(IDC_STATIC_SHOW))->GetWindowRect(&rect);
	//ScreenToClient(&rect);
	m_nShowWidth = rect.right - rect.left;
	m_nShowHeight = rect.bottom - rect.top;
	OnBnClickedRadioClient();
	if (m_nMode)
	{
	}
}

void CCodecTransModuleDlg::OnBnClickedButtonOpen()
{
	CString fileName, strFilePath;

	//过滤
	LPCTSTR strFilter = L"mp4文件(*.mp4)|*.mp4|avi文件(*.avi)|*.avi|yuv文件(*.yuv)|*.yuv|所有文件(*.*)|*.*||";
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, NULL, 0);

	const int c_cMaxFiles = 100;
	const int c_cbBuffSize = (c_cMaxFiles * (MAX_PATH + 1)) + 1;
	fileDlg.GetOFN().lpstrFile = fileName.GetBuffer(c_cbBuffSize);
	fileDlg.GetOFN().nMaxFile = c_cbBuffSize;
	fileDlg.DoModal();

	fileName.ReleaseBuffer();
	strFilePath = fileDlg.GetPathName();
	SetDlgItemText(IDC_EDIT_FILEPATH, strFilePath);
}


void CCodecTransModuleDlg::OnBnClickedButtonConvert2yuv()
{
	CString strFilePath;
	GetDlgItemText(IDC_EDIT_FILEPATH, strFilePath);
	
	CString strOutput = strFilePath.Mid(0, strFilePath.ReverseFind('.') + 1) + L"yuv";
	CFile fileWrite;
	BOOL bOpen = fileWrite.Open(strOutput, CFile::modeCreate | CFile::modeWrite);

	AVFormatContext* pAVFormatContext = NULL;
	int len = WideCharToMultiByte(CP_ACP, 0, strFilePath, -1, NULL, 0, NULL, NULL);
	char *pStrSrc = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, strFilePath, -1, pStrSrc, len, NULL, NULL);

	AVInputFormat* pAVInputFormat = NULL;
	AVDictionary* pAVDict = NULL;
	AVCodecContext* pAVCodecContext = NULL;

	pAVFormatContext = avformat_alloc_context();
	if (0 != avformat_open_input(&pAVFormatContext, pStrSrc, NULL, NULL))
	{
		OutputDebugString(_T("call avformat_open_input fail!\n"));
		return;
	}
	if (avformat_find_stream_info(pAVFormatContext, NULL) < 0)
	{	
		OutputDebugString(_T("call avformat_find_stream_info fail!\n"));
		return;
	}
	int nVideoType = -1;
	for (unsigned int i = 0; i < pAVFormatContext->nb_streams; i++)
	{
		if (pAVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			nVideoType = i;
			break;
		}
	}
	AVCodecContext* pCodecCtx = pAVFormatContext->streams[nVideoType]->codec;
	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL){
		OutputDebugString(L"Codec not found.\n");
		return ;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0){
		OutputDebugString(L"Could not open codec.\n");
		return ;
	}

	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameYUV = av_frame_alloc();
	uint8_t* out_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));

	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	AVPacket* pPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	av_dump_format(pAVFormatContext, 0, pStrSrc, 0);
	printf("-------------------------------------------------\n");
	struct SwsContext* img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	int ret, nGotPicture, nYSize, nWriteCount = 0;
	while (av_read_frame(pAVFormatContext, pPacket) >= 0){
		if (pPacket->stream_index == nVideoType){
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &nGotPicture, pPacket);
			if (ret < 0){
				printf("Decode Error.\n");
				return ;
			}
			if (nGotPicture){
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);

				nYSize = pCodecCtx->width*pCodecCtx->height;
				fileWrite.Write(pFrameYUV->data[0], nYSize);
				fileWrite.Write(pFrameYUV->data[1], nYSize/4);
				fileWrite.Write(pFrameYUV->data[2], nYSize/4);

				OutputDebugString(L"Succeed to decode 1 frame!\n");
				nWriteCount++;
			}
		}
		av_free_packet(pPacket);
	}
	printf("write count is %d\n", nWriteCount);
	//flush decoder
	//FIX: Flush Frames remained in Codec

	while (1) {
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &nGotPicture, pPacket);
		if (ret < 0)
			break;
		if (!nGotPicture)
			break;
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
			pFrameYUV->data, pFrameYUV->linesize);

		int y_size = pCodecCtx->width*pCodecCtx->height;
		fileWrite.Write(pFrameYUV->data[0], nYSize);
		fileWrite.Write(pFrameYUV->data[1], nYSize / 4);
		fileWrite.Write(pFrameYUV->data[2], nYSize / 4);

		OutputDebugString(L"Flush Decoder: Succeed to decode 1 frame!\n");
		nWriteCount++;
	}
	printf("write count is %d\n", nWriteCount);
	sws_freeContext(img_convert_ctx);

	//fclose(fp_yuv);
	fileWrite.Close();

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pAVFormatContext);
}

void CCodecTransModuleDlg::InitFFmpeg()
{
	avcodec_register_all();
	avdevice_register_all();
	avfilter_register_all();
	av_register_all();
	avformat_network_init();
}

void CCodecTransModuleDlg::UninitFFmpeg()
{
	avformat_network_deinit();
}

void CCodecTransModuleDlg::InitSDL()
{
}

void CCodecTransModuleDlg::UninitSDL()
{
}

void CCodecTransModuleDlg::OnBnClickedButtonSnapscreen()
{
	CBitmap memBitmap, *pOldBitmap;
	CWindowDC dc(GetDesktopWindow());
	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	int nWidth, nHeight;
	/* nWidth=GetSystemMetrics(SM_CXSCREEN);
	nHeight=GetSystemMetrics(SM_CYSCREEN);*/
	CRect rect;
	GetDesktopWindow()->GetWindowRect(rect);
	ClientToScreen(&rect);
	nWidth = rect.Width();
	nHeight = rect.Height();
	memBitmap.CreateCompatibleBitmap(&dc, nWidth, nHeight);
	pOldBitmap = memDC.SelectObject(&memBitmap);
	// memDC.StretchBlt(0,0,rect.Width(),rect.Height(),&dc,0,0,rect.Width(),rect.Height(),SRCCOPY);
	memDC.BitBlt(0, 0, nWidth, nHeight, &dc, 0, 0, SRCCOPY);

	BITMAPFILEHEADER BMFhead;
	BITMAPINFOHEADER BMIhead;
	BMFhead.bfReserved1 = 0;
	BMFhead.bfReserved2 = 0;
	BMFhead.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	BMFhead.bfSize = BMFhead.bfOffBits + nWidth*nHeight * 4;
	BMFhead.bfType = 0x4d42;

	BMIhead.biBitCount = 32;
	BMIhead.biClrImportant = 0;
	BMIhead.biClrUsed = 0;
	BMIhead.biSizeImage = 0;
	BMIhead.biXPelsPerMeter = 0;
	BMIhead.biYPelsPerMeter = 0;
	BMIhead.biCompression = BI_RGB;
	BMIhead.biHeight = nHeight;
	BMIhead.biPlanes = 1;
	BMIhead.biSize = sizeof(BITMAPINFOHEADER);
	BMIhead.biWidth = nWidth;

	DWORD dwSize = nWidth*nHeight * 4;
	BYTE *pData = new BYTE[dwSize];
	GetDIBits(memDC.m_hDC, (HBITMAP)memBitmap.m_hObject, 0, nHeight, pData, (LPBITMAPINFO)&BMIhead, DIB_RGB_COLORS);

	CFile file;
	file.Open(L"test.bmp", CFile::modeCreate | CFile::modeWrite);
	file.Write(&BMFhead, sizeof(BITMAPFILEHEADER));
	file.Write(&BMIhead, sizeof(BITMAPINFOHEADER));
	file.Write(pData, dwSize);
	file.Close();
	memDC.SelectObject(pOldBitmap);
}

void  CCodecTransModuleDlg::SetShowMode(int nMode)
{
	if (nMode > -1 && nMode < 1)
		m_nMode = nMode;
}

UINT CCodecTransModuleDlg::SendFileThread(LPVOID lParam)
{
	CTransSocket::SendFileToRemoteReceiver(_T("127.0.0.1"), _T("Hello_标清.mp4"));
	return 0;
}

UINT CCodecTransModuleDlg::RecvFileThread(LPVOID lParam)
{
	CTransSocket::GetFileFromRemoteSender(_T("Hello.mp4"));
	return 0;
}

void CCodecTransModuleDlg::OnBnClickedButtonSend()
{
	AfxBeginThread(SendFileThread, NULL);
}

void CCodecTransModuleDlg::OnBnClickedButtonRecv()
{
	AfxBeginThread(RecvFileThread, NULL);
}

void CCodecTransModuleDlg::OnBnClickedButtonChoosefile()
{
	CString fileName, strFilePath;

	LPCTSTR strFilter = L"所有文件(*.*)|*.*||";
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, NULL, 0);

	const int c_cMaxFiles = 100;
	const int c_cbBuffSize = (c_cMaxFiles * (MAX_PATH + 1)) + 1;
	fileDlg.GetOFN().lpstrFile = fileName.GetBuffer(c_cbBuffSize);
	fileDlg.GetOFN().nMaxFile = c_cbBuffSize;
	fileDlg.DoModal();

	fileName.ReleaseBuffer();
	strFilePath = fileDlg.GetPathName();
	SetDlgItemText(IDC_EDIT_SENDFILE, strFilePath);
}

void CCodecTransModuleDlg::OnBnClickedButtonPlay()
{
	fp = fopen("test_yuv420p_320x180.yuv", "rb+");
	HWND hwnd = GetDlgItem(IDC_STATIC_SHOW)->GetSafeHwnd();
	Render(hwnd);
}

bool CCodecTransModuleDlg::Render(HWND hwnd)
{
	HDC hdc = *(GetDlgItem(IDC_STATIC_SHOW)->GetDC());
	while (1)
	{
		//Read Pixel Data
		if (fread(buffer, 1, pixel_w*pixel_h*bpp / 8, fp) != pixel_w*pixel_h*bpp / 8){
			// Loop
			fseek(fp, 0, SEEK_SET);
			fread(buffer, 1, pixel_w*pixel_h*bpp / 8, fp);
			//return true;
		}

		//HDC hdc = GetDC(hwnd);

		//Note:
		//Big Endian or Small Endian?
		//ARGB order:high bit -> low bit.
		//ARGB Format Big Endian (low address save high MSB, here is A) in memory : A|R|G|B
		//ARGB Format Little Endian (low address save low MSB, here is B) in memory : B|G|R|A

		//Microsoft Windows is Little Endian
		//So we must change the order
#if LOAD_BGRA
		CONVERT_RGBA32toRGB24(buffer, pixel_w, pixel_h);
		//we don't need to change endian
		//Because input BGR24 pixel data(B|G|R) is same as RGB in Little Endian (B|G|R)
#elif LOAD_RGB24
		//Change to Little Endian
		CHANGE_ENDIAN_PIC(buffer, pixel_w, pixel_h, 24);
#elif LOAD_BGR24
		//In fact we don't need to do anything.
		//Because input BGR24 pixel data(B|G|R) is same as RGB in Little Endian (B|G|R)
		//CONVERT_RGB24toBGR24(buffer,pixel_w,pixel_h);
		//CHANGE_ENDIAN_PIC(buffer,pixel_w,pixel_h,24);
#elif LOAD_YUV420P
		//YUV Need to Convert to RGB first
		//YUV420P to RGB24
		CONVERT_YUV420PtoRGB24(buffer, buffer_convert, pixel_w, pixel_h);
		//Change to Little Endian
		CHANGE_ENDIAN_PIC(buffer_convert, pixel_w, pixel_h, 24);
#endif

		//BMP Header
		BITMAPINFO m_bmphdr = { 0 };
		DWORD dwBmpHdr = sizeof(BITMAPINFO);
		//24bit
		m_bmphdr.bmiHeader.biBitCount = 24;
		m_bmphdr.bmiHeader.biClrImportant = 0;
		m_bmphdr.bmiHeader.biSize = dwBmpHdr;
		m_bmphdr.bmiHeader.biSizeImage = 0;
		m_bmphdr.bmiHeader.biWidth = pixel_w;
		//Notice: BMP storage pixel data in opposite direction of Y-axis (from bottom to top).
		//So we must set reverse biHeight to show image correctly.
		m_bmphdr.bmiHeader.biHeight = -pixel_h;
		m_bmphdr.bmiHeader.biXPelsPerMeter = 0;
		m_bmphdr.bmiHeader.biYPelsPerMeter = 0;
		m_bmphdr.bmiHeader.biClrUsed = 0;
		m_bmphdr.bmiHeader.biPlanes = 1;
		m_bmphdr.bmiHeader.biCompression = BI_RGB;
		//Draw data
#if LOAD_YUV420P
		//YUV420P data convert to another buffer
		int nResult = StretchDIBits(hdc,
			0, 0,
			m_nShowWidth, m_nShowHeight,//screen_w, screen_h,
			0, 0,
			pixel_w, pixel_h,
			buffer_convert,
			&m_bmphdr,
			DIB_RGB_COLORS,
			SRCCOPY);
#else
		//Draw data
		int nResult = StretchDIBits(hdc,
			0, 0,
			screen_w, screen_h,
			0, 0,
			pixel_w, pixel_h,
			buffer,
			&m_bmphdr,
			DIB_RGB_COLORS,
			SRCCOPY);
#endif
	
		Sleep(40);
	}
	::ReleaseDC(hwnd, hdc);

	return true;
}

bool CCodecTransModuleDlg::RenderPacket(HWND hwnd, char* buf, char* buffer_convert, int nPixWidth, int nPixHeight)
{
	HDC hdc = *(GetDlgItem(IDC_STATIC_SHOW)->GetDC());
	//Render
	{
#if LOAD_BGRA
		CONVERT_RGBA32toRGB24(buffer, nPixWidth, nPixHeight);
		//we don't need to change endian
		//Because input BGR24 pixel data(B|G|R) is same as RGB in Little Endian (B|G|R)
#elif LOAD_RGB24
		//Change to Little Endian
		CHANGE_ENDIAN_PIC(buffer, nPixWidth, nPixHeight, 24);
#elif LOAD_BGR24
		//In fact we don't need to do anything.
		//Because input BGR24 pixel data(B|G|R) is same as RGB in Little Endian (B|G|R)
		//CONVERT_RGB24toBGR24(buffer,pixel_w,pixel_h);
		//CHANGE_ENDIAN_PIC(buffer,pixel_w,pixel_h,24);
#elif LOAD_YUV420P
		//YUV Need to Convert to RGB first
		//YUV420P to RGB24
		CONVERT_YUV420PtoRGB24((unsigned char*)buf, (unsigned char*)buffer_convert, nPixWidth, nPixHeight);
		//Change to Little Endian
		CHANGE_ENDIAN_PIC((unsigned char*)buffer_convert, nPixWidth, nPixHeight, 24);
#endif
		//BMP Header
		BITMAPINFO m_bmphdr = { 0 };
		DWORD dwBmpHdr = sizeof(BITMAPINFO);
		//24bit
		m_bmphdr.bmiHeader.biBitCount = 24;
		m_bmphdr.bmiHeader.biClrImportant = 0;
		m_bmphdr.bmiHeader.biSize = dwBmpHdr;
		m_bmphdr.bmiHeader.biSizeImage = 0;
		m_bmphdr.bmiHeader.biWidth = nPixWidth;
		//Notice: BMP storage pixel data in opposite direction of Y-axis (from bottom to top).
		//So we must set reverse biHeight to show image correctly.
		m_bmphdr.bmiHeader.biHeight = -nPixHeight;
		m_bmphdr.bmiHeader.biXPelsPerMeter = 0;
		m_bmphdr.bmiHeader.biYPelsPerMeter = 0;
		m_bmphdr.bmiHeader.biClrUsed = 0;
		m_bmphdr.bmiHeader.biPlanes = 1;
		m_bmphdr.bmiHeader.biCompression = BI_RGB;
		//Draw Data
#if LOAD_YUV420P
		//YUV420 convert to another data
		int nResult = StretchDIBits(hdc,
			0, 0,
			m_nShowWidth, m_nShowHeight,
			0, 0,
			nPixWidth, nPixHeight,
			buffer_convert,
			&m_bmphdr,
			DIB_RGB_COLORS,
			SRCCOPY);
#else
		//Draw data
		int nResult = StretchDIBits(hdc,
			0, 0,
			screen_w, screen_h,
			0, 0,
			pixel_w, pixel_h,
			buffer,
			&m_bmphdr,
			DIB_RGB_COLORS,
			SRCCOPY);
#endif
		//no sleep 40
	}
	::ReleaseDC(hwnd, hdc);
	return true;
}

void CCodecTransModuleDlg::DecodeTransYUVFrame(CString strFilePath)
{
#if 1
	FILE* fp = fopen("localT.yuv", "wb+");
	//change to char*
	int len = WideCharToMultiByte(CP_ACP, 0, strFilePath, -1, NULL, 0, NULL, NULL);
	char *pStrSrc = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, strFilePath, -1, pStrSrc, len, NULL, NULL);

	AVFormatContext* pAVFormatContext = NULL;
	AVInputFormat* pAVInputFormat = NULL;
	AVDictionary* pAVDict = NULL;
	AVCodecContext* pAVCodecContext = NULL;

	pAVFormatContext = avformat_alloc_context();
	if (0 != avformat_open_input(&pAVFormatContext, pStrSrc, NULL, NULL))
	{
		OutputDebugString(_T("call avformat_open_input fail!\n"));
		return;
	}
	if (avformat_find_stream_info(pAVFormatContext, NULL) < 0)
	{
		OutputDebugString(_T("call avformat_find_stream_info fail!\n"));
		return;
	}
	int nVideoType = -1;
	for (unsigned int i = 0; i < pAVFormatContext->nb_streams; i++)
	{
		if (pAVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			nVideoType = i;
			break;
		}
	}
	AVCodecContext* pCodecCtx = pAVFormatContext->streams[nVideoType]->codec;
	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL){
		OutputDebugString(L"Codec not found.\n");
		return;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0){
		OutputDebugString(L"Could not open codec.\n");
		return;
	}

	//get frame data...
	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameYUV = av_frame_alloc();
	uint8_t* out_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));

	avpicture_fill((AVPicture*)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	AVPacket* pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
	//Output Info
	printf("--------------- File Information ----------------\n");
	av_dump_format(pAVFormatContext, 0, pStrSrc, 0);
	printf("-------------------------------------------------\n");
	struct SwsContext* img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	int ret, nGotPicture, nShowSize, nSend = 0;
	CString strLog;
	//Send Each Packet Size
	nShowSize = pCodecCtx->width*pCodecCtx->height;
	char* pTransSize = new char[100];
	memset(pTransSize, 0, 100);
	sprintf(pTransSize, "Width: %d Height: %d", pCodecCtx->width, pCodecCtx->height);
	OutputDebugString(L"Send Data Length..\n");
	//m_transSock.RecvToContinue();
	nSend = m_transSock.SendDataLength(strlen(pTransSize));
	OutputDebugString(L"Send Data ..\n");
	//m_transSock.RecvToContinue();
	nSend = m_transSock.SendData(pTransSize, strlen(pTransSize));
	//m_transSock.SendToContinue();
	int nRecv = 0;
	//OutputDebugString(L"recv data length.\n");
	//nSend = m_transSock.RecvDataLength(nRecv);
	//strLog.Format(L"data length is %d, recv data.\n", nRecv);
	//OutputDebugString(strLog);
	//nSend = m_transSock.RecvData(pTransSize, nSend);
	OutputDebugString(L"block to recv server notify which to continue to send.\n");
	m_transSock.RecvToContinue();
	//CString strLog;
	OutputDebugString(L"Entry while loop.\n");
	int nSendCount = 0;
	HWND hwnd = GetDlgItem(IDC_STATIC_SHOW)->GetSafeHwnd();
	int nLineSize = pCodecCtx->width * pCodecCtx->height;
	char* buffer_convert = new char[nLineSize * 3];
	while (av_read_frame(pAVFormatContext, pPacket) >= 0)
	{
		if (pPacket->stream_index == nVideoType)
		{
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &nGotPicture, pPacket);
			if (ret < 0)
			{
				OutputDebugString(L"Decode Error.\n");
				return;
			}
			if (nGotPicture && (pFrame->pict_type == AV_PICTURE_TYPE_I || pFrame->pict_type == AV_PICTURE_TYPE_P))
			{
				memset(buffer_convert, 0, nLineSize * 3);
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);

				nShowSize = pCodecCtx->width*pCodecCtx->height;
				//recv server ready info
				OutputDebugString(L"recv server continue info, 1.\n");
				if (m_transSock.RecvToContinue())
				{
					OutputDebugString(L"recved server continue info, 1.\n");
					strLog.Format(L"Send DataLength, %d\n", nShowSize);
					OutputDebugString(strLog);
					m_transSock.SendDataLength(nShowSize);
					strLog.Format(L"Send Y Data.\n", nShowSize);
					OutputDebugString(strLog);
					if (m_transSock.RecvToContinue())
					{
						nSend = m_transSock.SendData((void*)pFrameYUV->data[0], nShowSize);
						fwrite(pFrameYUV->data[0], nShowSize, 1, fp);
					}
				}
				//recv server ready info
				OutputDebugString(L"recv server continue info, 2.\n");
				if (m_transSock.RecvToContinue())
				{
					OutputDebugString(L"recved server continue info, 2.\n");

					strLog.Format(L"Send DataLength, %d\n", nShowSize/4);
					OutputDebugString(strLog);
					m_transSock.SendDataLength(nShowSize / 4);
					
					strLog.Format(L"Send U Data.\n", nShowSize);
					OutputDebugString(strLog);
					if (m_transSock.RecvToContinue())
					{
						nSend = m_transSock.SendData((void*)pFrameYUV->data[1], nShowSize / 4);
						fwrite(pFrameYUV->data[1], nShowSize / 4, 1, fp);
					}
				}
				//recv server ready info
				OutputDebugString(L"recv server continue info, 3.\n");
				if (m_transSock.RecvToContinue())
				{
					OutputDebugString(L"recved server continue info, 3.\n");

					strLog.Format(L"Send DataLength, %d\n", nShowSize / 4);
					OutputDebugString(strLog);
					m_transSock.SendDataLength(nShowSize / 4);

					strLog.Format(L"Send V Data.\n", nShowSize);
					OutputDebugString(strLog);
					if (m_transSock.RecvToContinue())
					{
						nSend = m_transSock.SendData((void*)pFrameYUV->data[2], nShowSize / 4);
						fwrite(pFrameYUV->data[2], nShowSize / 4, 1, fp);
					}
				}
				
				strLog.Format(L"Send Count:%d.\n", ++nSendCount);
				OutputDebugString(strLog);
				RenderPacket(hwnd, (char*)pFrameYUV->data, (char*)buffer_convert, pCodecCtx->width, pCodecCtx->height);
				Sleep(40);
			}
		}
		av_free_packet(pPacket);
	}
	
	//flush frames remained in codec
	while (1) {
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &nGotPicture, pPacket);
		if (ret < 0)
			break;
		if (!nGotPicture)
			break;
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
			pFrameYUV->data, pFrameYUV->linesize);

		int linesize = pCodecCtx->width*pCodecCtx->height;
		m_transSock.SendDataLength(nShowSize);
		nSend = m_transSock.SendData((void*)pFrameYUV->data[0], nShowSize);
		m_transSock.SendDataLength(nShowSize/4);
		nSend = m_transSock.SendData((void*)pFrameYUV->data[1], nShowSize / 4);
		m_transSock.SendDataLength(nShowSize/4);
		nSend = m_transSock.SendData((void*)pFrameYUV->data[2], nShowSize / 4);
		
		OutputDebugString(L"decode and trans 1 frame!\n");
	}

	m_transSock.SendEndInfo();
	m_transSock.UninitSocket();

	delete[] buffer_convert;
	fclose(fp);
	av_free_packet(pPacket);
	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);
	av_free(out_buffer);
	avformat_free_context(pAVFormatContext);
	delete[] pTransSize;
#endif
}

void CCodecTransModuleDlg::OnClose()
{
	//execute uninit when close window
	UninitSDL();
	UninitFFmpeg();

	m_transSock.Close();

	CDialogEx::OnClose();
}

void CCodecTransModuleDlg::OnBnClickedRadioClient()
{
	UpdateData(TRUE);
	switch (m_radioClientType)
	{
	case 0:
		GetDlgItem(IDC_EDIT_SENDFILE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_SEND)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_CHOOSEFILE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_DECODESEND)->EnableWindow(TRUE);
		
		GetDlgItem(IDC_EDIT_RECVFILE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_RECV)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_RECVPLAY)->EnableWindow(FALSE);
		break;
	case 1:
		GetDlgItem(IDC_EDIT_SENDFILE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_SEND)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_CHOOSEFILE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_DECODESEND)->EnableWindow(FALSE);

		GetDlgItem(IDC_EDIT_RECVFILE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_RECV)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_RECVPLAY)->EnableWindow(TRUE);
		break;
	default:
		break;
	}
}

void CCodecTransModuleDlg::OnBnClickedButtonDecodesend()
{
	if (m_ipCtrlServer.IsBlank())
		m_transSock.InitSocket(TRANS_PACKET_PORT, 0, L"127.0.0.1");
	else
	{
		//DWORD dwAddr;
		//m_ipCtrlServer.GetAddress(dwAddr);
		//unsigned char *pIP;
		//pIP = (unsigned  char*)&dwAddr;
		//strIP.Format(L"%u.%u.%u.%u", *(pIP + 3), *(pIP + 2), *(pIP + 1), *pIP);
		CString strIP;
		m_ipCtrlServer.GetWindowText(strIP);
		m_transSock.InitSocket(TRANS_PACKET_PORT, 0, strIP);
	}
	CString strFilePath;
	GetDlgItemText(IDC_EDIT_SENDFILE, strFilePath);
	DecodeTransYUVFrame(strFilePath);
}

void CCodecTransModuleDlg::OnBnClickedButtonRecvplay()
{
	CString strOutput = L"test.yuv";
	FILE* fp = NULL;
	fp = fopen("tannic.yuv", "wb+");

	m_transSock.InitSocket(TRANS_PACKET_PORT, 1);
	char* pTransSize = new char[100];
	char szRecv[100] = { 0 };
	memset(pTransSize, 0, 100);

	int nRecv = 0;
	CString strLog;
	//m_transSock.SendToContinue();
	OutputDebugString(L"RecvDataLength..\n");
	m_transSock.RecvDataLength(nRecv);
	strLog.Format(L"recv data length is :%d\n", nRecv);
	OutputDebugString(strLog);
	OutputDebugString(L"RecvData..\n");
	//m_transSock.SendToContinue();
	m_transSock.RecvData(pTransSize, nRecv);
	int nPicWidth, nPicHeight;
	sscanf(pTransSize, "%*s %d %*s %d", &nPicWidth, &nPicHeight);
	char* pTmpTransSize = pTransSize;
	delete[] pTransSize;
	pTransSize = NULL;
	strLog.Format(L"pic width is :%d, height is :%d\n", nPicWidth, nPicHeight);
	OutputDebugString(strLog);
	int nLineSize = nPicHeight * nPicWidth;
	pTransSize = new char[nLineSize * bpp / 8];
	memset(pTransSize, 0, nLineSize * bpp / 8);
	char* buffer_convert = new char[nLineSize * 3];
	memset(buffer_convert, 0, nLineSize * 3);
	HWND hwnd = GetDlgItem(IDC_STATIC_SHOW)->GetSafeHwnd();
	OutputDebugString(L"send to client continue to send.\n");
	m_transSock.SendToContinue();
	//m_transSock.SendDataLength(strlen("Send Please..."));
	//OutputDebugString(L"send to client notify.\n");
	//nRecv = m_transSock.SendData("Send Please...", strlen("Send Please..."));
	int nRecvDataSize = 0;
	int nRecvCount = 0;
	do
	{
		memset(pTransSize, 0, nLineSize * bpp / 8);
		memset(buffer_convert, 0, nLineSize * 3);
		//notify client to send
		OutputDebugString(L"Send Client to continue send data. 1\n");
		m_transSock.SendToContinue();
		//recv Y Data
		OutputDebugString(L"Recv Data Length.\n");
		m_transSock.RecvDataLength(nRecvDataSize);
		
		strLog.Format(L"Recv Data Length is %d.\n", nRecvDataSize);
		OutputDebugString(strLog);
		if (nRecvDataSize > 0)
		{
			OutputDebugString(L"recv Y data .... 1\n");
			if (!m_transSock.SendToContinue())
				break;
			nRecv = m_transSock.RecvData(pTransSize, nRecvDataSize);
			if (nRecv < 0)
				break;
			fwrite(pTransSize, nRecvDataSize, 1, fp);
		}

		//notify client to send
		OutputDebugString(L"Send Client to continue send data. 2\n");
		m_transSock.SendToContinue();
		//recv U Data
		OutputDebugString(L"Recv Data Length.\n");
		m_transSock.RecvDataLength(nRecvDataSize);

		strLog.Format(L"Recv Data Length is %d.\n", nRecvDataSize);
		OutputDebugString(strLog);
		if (nRecvDataSize > 0)
		{
			OutputDebugString(L"recv U data .... 2\n");
			if (!m_transSock.SendToContinue())
				break;
			nRecv = m_transSock.RecvData(pTransSize + nLineSize, nRecvDataSize/*nLineSize / 4*/);
			if (nRecv < 0)
				break;
			fwrite(pTransSize, nRecvDataSize, 1, fp);
		}
		//notify client to send
		OutputDebugString(L"Send Client to continue send data. 3\n");
		m_transSock.SendToContinue();
		//recv V Data
		OutputDebugString(L"Recv Data Length.\n");
		m_transSock.RecvDataLength(nRecvDataSize);

		strLog.Format(L"Recv Data Length is %d.\n", nRecvDataSize);
		OutputDebugString(strLog);
		if (nRecvDataSize > 0)
		{
			OutputDebugString(L"recv V data .... 3\n"); 
			if (!m_transSock.SendToContinue())
				break;
			nRecv = m_transSock.RecvData(pTransSize + nLineSize + nLineSize / 4, nRecvDataSize/*nLineSize / 4*/);
			if (nRecv < 0)
				break;
			fwrite(pTransSize, nRecvDataSize, 1, fp);
		}
		
		strLog.Format(L"Recv Count :%d.\n", ++nRecvCount);
		OutputDebugString(strLog);
		OutputDebugString(L"render packet...\n");
		RenderPacket(hwnd, (char*)pTransSize, (char*)buffer_convert, nPicWidth, nPicHeight);
	} while (1);

	m_transSock.UninitSocket();

	fclose(fp);

	delete[] buffer_convert;
	delete[] pTransSize;
}
