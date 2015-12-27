//===== Copyright ?1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//===========================================================================//

#ifndef MEMPOOL_H
#define MEMPOOL_H

#ifdef _WIN32
#pragma once
#endif

class CMemoryPool
{
public:
	CMemoryPool(int blockSize, int numElements);
	~CMemoryPool(void);

public:
	void *Alloc(unsigned int amount);
	void Free(void *memblock);
	int Count(void) { return _blocksAllocated; }
	void AddNewBlob(void);

public:
	enum
	{
		MAX_BLOBS = 16
	};

	int _blockSize;
	int _blocksPerBlob;
	int _numElements;
	void *_memBlob[MAX_BLOBS];
	void *_headOfFreeList;
	int _numBlobs;
	int _peakAlloc;
	int _blocksAllocated;
};

#endif