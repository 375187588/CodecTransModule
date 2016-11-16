#pragma once
#include <vector>

class Utility
{
public:
	Utility();
	~Utility();

public:
	static BOOL Init();
	static BOOL Uninit();

	static char* CStringWToChar(CString string);

private:
	static std::vector<char*> m_vecPtr;
};

