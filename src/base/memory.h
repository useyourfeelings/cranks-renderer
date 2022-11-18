#ifndef MEMORY_H
#define MEMORY_H

#include<list>

#define MB_ALLOC(mb, T) new (mb.Alloc(sizeof(T))) T

class alignas(64) MemoryBlock {
public:
	MemoryBlock();
	~MemoryBlock();

	void Reset();
	void* Alloc(size_t nBytes);

	//template <typename T>
	//T *Alloc() {
	//	//std::cout << sizeof(T);
	//	return Alloc(sizeof(T));
	//}

private:

	int align;
	size_t blockSize;
	std::list<std::pair<uint8_t*, size_t>> freeBlocks, busyBlocks;

	size_t currentBlockSize = 0;
	size_t currentBlockPos = 0;
	uint8_t* currentBlock = nullptr;
};


#endif