#pragma once


// CChooseDlg �Ի���

class CChooseDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CChooseDlg)

public:
	CChooseDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CChooseDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	int m_radioChoose;
	afx_msg void OnBnClickedOk();
};
