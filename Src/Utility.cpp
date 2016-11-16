#include "stdafx.h"
#include "Utility.h"

std::vector<char*> Utility::m_vecPtr;

Utility::Utility()
{
}


Utility::~Utility()
{
}

BOOL Utility::Init()
{
	m_vecPtr.clear();
	return TRUE;
}

BOOL Utility::Uninit()
{
	for (size_t i = 0; i < m_vecPtr.size(); i++)
	{
		char* ptr = m_vecPtr.at(i);
		delete ptr;
	}
	m_vecPtr.clear();
	return TRUE;
}

char* Utility::CStringWToChar(CString string)
{
	int len = WideCharToMultiByte(CP_ACP, 0, string, -1, NULL, 0, NULL, NULL);
	char *pStr = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, string, -1, pStr, len, NULL, NULL);
	return pStr;
}