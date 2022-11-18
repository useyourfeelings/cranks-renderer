#include<iostream>
#include<cstddef>
#include"memory.h"

MemoryBlock::MemoryBlock():
	align(alignof(std::max_align_t)),
	blockSize(524288) // 1024 * 256 = 262144
{
}

MemoryBlock::~MemoryBlock() {
	for (auto& block : freeBlocks)
		_aligned_free(block.first);

	for (auto& block : busyBlocks)
		_aligned_free(block.first);

	_aligned_free(currentBlock);
}

void MemoryBlock::Reset() {
	currentBlockPos = 0;
	freeBlocks.splice(freeBlocks.end(), busyBlocks); // fast
	//freeBlocks.splice(freeBlocks.begin(), busyBlocks); // slow

	//std::cout << "freeBlocks = " << freeBlocks.size() << std::endl;
}

void* MemoryBlock::Alloc(size_t nBytes) {
	nBytes = (nBytes + align - 1) & ~(align - 1); // 取ceiling

	//std::cout << "Alloc = " << nBytes << std::endl;

	if (currentBlockPos + nBytes > currentBlockSize) { // test current
		if (currentBlock) { // push current
			busyBlocks.push_back(std::make_pair(currentBlock, currentBlockSize));
			currentBlock = nullptr;
		}

		for (auto itr = freeBlocks.begin(); itr != freeBlocks.end(); ++itr) {
			if (itr->second >= nBytes) { // reuse
				void* m = itr->first;
				currentBlock = itr->first;
				currentBlockSize = itr->second;
				freeBlocks.erase(itr);
				break;
			}
		}

		if (!currentBlock) { // real new block
			currentBlockSize = std::max(nBytes, blockSize);
			currentBlock = (uint8_t*)(_aligned_malloc(currentBlockSize, 64));
			//std::cout << "Alloc real = " << nBytes << std::endl;
		}

		currentBlockPos = 0;
	} 

	void* start = currentBlock + currentBlockPos;
	currentBlockPos += nBytes;

	return start;
}

//template <typename T>
//T *Alloc() {
//	//std::cout << sizeof(T);
//	return Alloc(sizeof(T));
//}

