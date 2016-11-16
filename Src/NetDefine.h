#pragma once
//define some trans common data struct

typedef struct _PACK_DATA_{
	unsigned long nSeqNo;		//package number
	unsigned long nPackSize;	//package size
	void* pData;				//package data
}stPackData;

