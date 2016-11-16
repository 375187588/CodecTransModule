#pragma once

class CVideoFileOpr
{
public:
	CVideoFileOpr();
	virtual ~CVideoFileOpr();

public:
	int ConverFile2YUV(CString strFilePath);
};

