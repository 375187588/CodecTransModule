#pragma once
class CBmpScreen
{
public:
	CBmpScreen();
	virtual ~CBmpScreen();

	static void SnapScreen(CString strFileName = L"Test.bmp");
};

