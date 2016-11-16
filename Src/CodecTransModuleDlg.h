
// CodecTransModuleDlg.h : ͷ�ļ�
//

#pragma once

#include "TransSocket.h"
#include "afxcmn.h"
// CCodecTransModuleDlg �Ի���
class CCodecTransModuleDlg : public CDialogEx
{
// ����
public:
	CCodecTransModuleDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_CODECTRANSMODULE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
protected:
	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonConvert2yuv();
	afx_msg void OnBnClickedButtonConver2rgb();
	afx_msg void OnBnClickedButtonSnapscreen();
	afx_msg void OnBnClickedButtonSend();
	afx_msg void OnBnClickedButtonRecv();
	afx_msg void OnBnClickedButtonChoosefile();
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnClose();
	afx_msg void OnBnClickedRadioClient();
	afx_msg void OnBnClickedButtonDecodesend();
	afx_msg void OnBnClickedButtonRecvplay();

public:
	static UINT SendFileThread(LPVOID lParam);
	static UINT RecvFileThread(LPVOID lParam);

	void  SetShowMode(int nMode = 1);

private:
	void InitCtrl();

	void InitFFmpeg();
	void UninitFFmpeg();

	void InitSDL();
	void UninitSDL();

	//GDI Render
	bool Render(HWND hwnd);
	bool RenderPacket(HWND hwnd, char* buf, char* buffer_convert, int nPixWidth, int nPixHeight);

	//decode media file then trans Frame data
	void DecodeTransYUVFrame(CString strFilePath);

	//
	void RecvFramePlay();

private:
	int m_nMode; //Server or Client, default is server(1)
	int m_nShowWidth;
	int m_nShowHeight;

	CTransSocket	m_transSock;
public:
	int m_radioClientType;
	CIPAddressCtrl m_ipCtrlServer;
};
