#pragma once

class BitmapMark
{
public:

	BitmapMark(HBITMAP, HDC);
	~BitmapMark();

public:

	void addText(CString str, int _x, int _y);

private:

	HFONT m_hFont;
	HFONT m_hOldFont;
	HFONT m_hInitFont;
	COLORREF m_foreColor;
	COLORREF m_bkColor;
	bool m_isTransparant;
	HBITMAP m_hBmp;
	HBITMAP	m_hOldBmp;
	HDC	m_hMemDC;
};
