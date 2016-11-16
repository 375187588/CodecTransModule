#include "stdafx.h"
#include "BitmapMark.h"
#include "wingdi.h"

BitmapMark::BitmapMark(HBITMAP bmp, HDC hdc)
{
	m_isTransparant = true;
	m_foreColor = RGB(255, 0, 0);
	m_bkColor = RGB(255, 255, 255);

	m_hBmp = bmp;
	m_hMemDC = hdc;
	
	m_hFont = CreateFont(200, 0, 0, 0, FW_BOLD, 1, 0, 0, DEFAULT_CHARSET, 
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"»ªÎÄ¿¬Ìå");
	m_hInitFont = m_hFont;

	m_hOldFont = (HFONT)SelectObject(m_hMemDC, (HFONT)m_hFont);
	m_hOldBmp = (HBITMAP)SelectObject(m_hMemDC, m_hBmp);

	SetBkMode(m_hMemDC, m_isTransparant ? TRANSPARENT : OPAQUE);
	SetTextColor(m_hMemDC, m_foreColor);
	::SetBkColor(m_hMemDC, m_bkColor);
}

BitmapMark::~BitmapMark()
{
	SelectObject(m_hMemDC, m_hOldBmp);
	SelectObject(m_hMemDC, m_hOldFont);
	DeleteObject(m_hInitFont);
}

void BitmapMark::addText(CString str, int _x, int _y)
{
	TEXTMETRIC txtMetric = {0};
	::GetTextMetrics(m_hMemDC, &txtMetric);

	SIZE size;
	::GetTextExtentPoint(m_hMemDC, str, str.GetLength(), &size);

	BITMAP bmp;
	CBitmap* ptr = CBitmap::FromHandle(m_hBmp);
	ptr->GetBitmap(&bmp);

	int nBmpWidth = bmp.bmWidth;
	int nBmpHeight = bmp.bmHeight;
	int x = _x == 0 ? (nBmpWidth - size.cx)/2 : _x;
	int y = _y == 0 ? (nBmpHeight - size.cy)/2 : _y;

	TextOut(m_hMemDC, x, y, str, str.GetLength());
}