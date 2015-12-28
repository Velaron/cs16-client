//===== Copyright ?1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "stdafx.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#ifdef _DEBUG
#include <string.h>
#endif
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#define DebugBreak() __builtin_trap()
#endif

#include <math.h>
#include "MemPool.h"

CMemoryPool::CMemoryPool(int blockSize, int numElements)
{
	_blocksPerBlob = numElements;
	_blockSize = blockSize;
	_numBlobs = 0;
	_numElements = 0;

	AddNewBlob();

	_peakAlloc = 0;
	_blocksAllocated = 0;
}

CMemoryPool::~CMemoryPool(void)
{
	for (int i = 0; i < _numBlobs; i++)
		free(_memBlob[i]);
}

void *CMemoryPool::Alloc(unsigned int amount)
{
	void *returnBlock;

	if (amount > (unsigned int)_blockSize)
		return NULL;

	_blocksAllocated++;
	_peakAlloc = max(_peakAlloc, _blocksAllocated);

	if (_blocksAllocated >= _numElements)
		AddNewBlob();

	if (!_headOfFreeList)
		DebugBreak();

	returnBlock = _headOfFreeList;
	_headOfFreeList = *((void **)_headOfFreeList);

	return returnBlock;
}

void CMemoryPool::Free(void *memblock)
{
	if (!memblock)
		return;

#ifdef _DEBUG
	memset(memblock, 0xDD, _blockSize);
#endif
	_blocksAllocated--;
	*((void **)memblock) = _headOfFreeList;
	_headOfFreeList = memblock;
}

void CMemoryPool::AddNewBlob(void)
{
	int sizeMultiplier = pow(2.0, _numBlobs);
	int nElements = _blocksPerBlob * sizeMultiplier;
	int blobSize = nElements * _blockSize;

	_memBlob[_numBlobs] = malloc(blobSize);

	if (!_memBlob[_numBlobs])
		DebugBreak();

	_headOfFreeList = _memBlob[_numBlobs];

	if (!_headOfFreeList)
		DebugBreak();

	void **newBlob = (void **)_headOfFreeList;

	for (int j = 0; j < nElements - 1; j++)
	{
		newBlob[0] = (char *)newBlob + _blockSize;
		newBlob = (void **)newBlob[0];
	}

	newBlob[0] = NULL;

	_numElements += nElements;
	_numBlobs++;

	if (_numBlobs >= MAX_BLOBS - 1)
		DebugBreak();
}
