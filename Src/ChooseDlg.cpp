// ChooseDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "CodecTransModule.h"
#include "ChooseDlg.h"
#include "afxdialogex.h"

int g_ChooseDlg = -1;
// CChooseDlg �Ի���

IMPLEMENT_DYNAMIC(CChooseDlg, CDialogEx)

CChooseDlg::CChooseDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CChooseDlg::IDD, pParent)
	, m_radioChoose(0)
{

}

CChooseDlg::~CChooseDlg()
{
}

void CChooseDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO1, m_radioChoose);
}


BEGIN_MESSAGE_MAP(CChooseDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CChooseDlg::OnOK)
END_MESSAGE_MAP()


// CChooseDlg ��Ϣ�������

void CChooseDlg::OnOK()
{
	// TODO:  �ڴ����ר�ô����/����û���
	UpdateData();
	if (m_radioChoose == 0)
	{	//Server
		g_ChooseDlg = 0;
	}
	else
	{	//Client
		g_ChooseDlg = 1;
	}

	CDialogEx::OnOK();
}