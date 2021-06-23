#define _CRT_SECURE_NO_WARNINGS
#include "FileSystem.h"
#include "Driver.h"

#define FILE_CONTROL_BLOCK_SIZE 128u
#define MAX_POINTER ((Super.BlockSize-sizeof(Block))/sizeof(BlockIndex)+10)
#define MAX_BLOCK_SPACE (Super.BlockSize-sizeof(Block))
#define MAX(x,y) (((x)>(y))?(x):(y))


BiSet::BiSet(uint32_t size)
{
	this->SizeOfBool = size;
	this->SizeOfByte = (int)ceil(size / 8);
	if (size % 8 != 0) {
		this->SizeOfByte++;
	}
	this->data = (uint8_t*)calloc(this->SizeOfByte, sizeof(uint8_t));
}
bool BiSet::get(uint32_t index) {
	return bool(data[index / 8] & (1 << (index % 8)));
}
void BiSet::set(uint32_t index, bool val) {
	if (val == true) {
		this->data[index / 8] = this->data[index / 8] | (1 << (index % 8));
	}
	else {
		uint8_t mask = ~(uint8_t)(1 << (index % 8));
		this->data[index / 8] = this->data[index / 8] & mask;
	}
}
BiSet::~BiSet()
{
	free(this->data);
}

FileControlBlock FileControlBlock::operator=(FileControlBlock& val)
{
	memcpy(this, &val, sizeof(FileControlBlock));
	return *this;

}
FileControlBlock::FileControlBlock()
{

}
FileControlBlock::FileControlBlock(enum FileType t, const char* name, uint8_t AccessMode, FCBIndex parent)
{
	strcpy(Name, name);
	this->Type = t;
	time(&this->CreateTime);
	this->Size = 0;
	this->Parent = parent;
}



SuperBlock Super;
BiSet* FCBBitMap, * DataBitMap;
bool isMounted = false;


BlockIndex getEmptyBlock() {
	if (isMounted == false) {
		printf("Mount a disk first.\n");
		return -1;
	}
	else {
		for (size_t i = 0; i < DataBitMap->SizeOfByte; ++i)
		{
			if (DataBitMap->data[i] != 0xFF) {
				for (int j = 0; j < 8; ++j) {
					if (DataBitMap->get(i * 8 + j) == false && i * 8 + j < DataBitMap->SizeOfBool) {
						return i * 8 + j;
					}
				}
			}
		}
		printf("Disk Full!\n");
		return -1;
	}
}

FCBIndex getEmptyFCB() {
	if (isMounted == false) {
		printf("Mount a disk first.\n");
		return -1;
	}
	else {
		for (size_t i = 0; i < FCBBitMap->SizeOfByte; ++i)
		{
			if (FCBBitMap->data[i] != 0xFF) {
				for (int j = 0; j < 8; ++j) {
					if (FCBBitMap->get(i * 8 + j) == false && i * 8 + j < FCBBitMap->SizeOfBool) {
						return i * 8 + j;
					}
				}
			}
		}
		printf("Disk Full!\n");
		return -1;
	}
}


void LoadFCB(FCBIndex index, FileControlBlock* buff) {
	ReadDisk((uint8_t*)buff, Super.FCBOffset * Super.BlockSize + index * FILE_CONTROL_BLOCK_SIZE, sizeof(FileControlBlock));
	return;
}
void StoreFCB(FCBIndex index, FileControlBlock* buff) {
	WriteDisk((uint8_t*)buff, Super.FCBOffset * Super.BlockSize + index * FILE_CONTROL_BLOCK_SIZE, sizeof(FileControlBlock));
	return;
}
void LoadBlock(BlockIndex index, uint8_t* buff) {
	ReadDisk(buff, (index + Super.DataOffset) * Super.BlockSize, Super.BlockSize);
}
void StoreBlock(BlockIndex index, uint8_t* buff) {
	WriteDisk(buff, (index + Super.DataOffset) * Super.BlockSize, Super.BlockSize);
}
void MakeDirBlock(FCBIndex firstIndex, uint8_t* buff) {
	((Block*)buff)->Size = sizeof(FCBIndex);
	*(FCBIndex*)(buff + sizeof(Block)) = firstIndex;
	for (size_t i = sizeof(Block) + sizeof(FCBIndex); i < Super.BlockSize; i += sizeof(FCBIndex))
	{
		*(FCBIndex*)(buff + i) = -1;
	}
}
inline uint16_t& BlockSize(uint8_t* blockBuff) {
	return ((Block*)blockBuff)->Size;
}
inline uint16_t& BlockFCS(uint8_t* blockBuff) {
	return ((Block*)blockBuff)->FCS;
}

bool AppendNewBlock(FileControlBlock* fcb, FCBIndex fcbIndex, uint8_t* blockBuff) {

	for (int i = 0; i < 10; i++) {
		if (fcb->DirectBlock[i] == -1) {
			BlockIndex blockIndex = getEmptyBlock();
			DataBitMap->set(blockIndex, true);
			StoreBlock(blockIndex, blockBuff);//写入数据块
			fcb->DirectBlock[i] = blockIndex;//更新FCB
			goto AppendNewBlock_end;
		}
	}
	if (fcb->Pointer != -1) {
		uint8_t* pointerBuff = (uint8_t*)malloc(Super.BlockSize);
		LoadBlock(fcb->Pointer, pointerBuff);
		for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(BlockIndex)) {
			if (*(BlockIndex*)(pointerBuff + pos) == -1) {
				BlockIndex blockIndex = getEmptyBlock();
				DataBitMap->set(blockIndex, true);
				StoreBlock(blockIndex, blockBuff);//写入数据块
				*(BlockIndex*)(pointerBuff + pos) = blockIndex;
				StoreBlock(fcb->Pointer, pointerBuff);//更新pointer块
				free(pointerBuff);
				goto AppendNewBlock_end;
			}
		}
		printf("ERROR:Append Block Failed! No Pointer remain.\n");
		return false;
	}
AppendNewBlock_end:
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//写入DataBlock的bitmap
	StoreFCB(fcbIndex, fcb);
	return true;
}

class DataReader
{
public:
	DataReader(FCBIndex FcbIndex)
	{
		this->PointerPage = (uint8_t*)malloc(Super.BlockSize);
		LoadFCB(FcbIndex, &fcb);

	}
	~DataReader()
	{
		free(PointerPage);
	}
	bool ReadNextBlock(uint8_t* buff, BlockIndex* block) {
		if (blockIndex < 10) {
			if (fcb.DirectBlock[blockIndex] == -1) {
				return false;
			}
			else {
				LoadBlock(fcb.DirectBlock[blockIndex], buff);
				*block = fcb.DirectBlock[blockIndex];
				blockIndex++;
				return true;
			}
		}
		else {
			if (fcb.Pointer == -1) {
				return false;
			}
			else {
				BlockIndex secondPosIndex = blockIndex % 10;
				if (secondPosIndex * sizeof(BlockIndex) + sizeof(Block) >= Super.BlockSize) {
					return false;
				}
				BlockIndex SecDataPointer;
				ReadDisk((uint8_t*)&SecDataPointer, (Super.DataOffset + fcb.Pointer) * Super.BlockSize + secondPosIndex * sizeof(FCBIndex), sizeof(FCBIndex));
				if (SecDataPointer == -1) {
					return false;
				}
				else {
					LoadBlock(SecDataPointer, buff);
					*block = SecDataPointer;
					blockIndex++;
					return true;
				}
			}
		}
	}

	FileControlBlock fcb;
	BlockIndex blockIndex = 0;
	uint8_t* PointerPage = nullptr;

};

class BlockManager
{
public:

	BlockManager()
	{
		BlockBuff = (uint8_t*)malloc(Super.BlockSize);
	}

	void load(BlockIndex index)
	{
		LoadBlock(index, BlockBuff);
		this->Fcs = ((Block*)BlockBuff)->FCS;
		this->Size = ((Block*)BlockBuff)->Size;
	}
	void makeBuffer() {
		((Block*)this->BlockBuff)->FCS = Fcs;
		((Block*)this->BlockBuff)->Size = Size;
	}
	~BlockManager()
	{
		free(BlockBuff);
	}

	uint8_t* BlockBuff = nullptr;
	uint16_t Fcs;
	uint16_t Size;
};
class PointerBlockManager :public BlockManager
{
public:
	void write(BlockIndex index)
	{
		Block b;
		for (size_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(BlockIndex))
		{
			if (*(BlockIndex*)(BlockBuff + pos) != -1) {
				b.Size++;
			}
		}
		b.FCS = 0;
		memcpy(BlockBuff, &b, sizeof(Block));
		StoreBlock(index, BlockBuff);
	}
	BlockIndex* begin() {
		return (BlockIndex*)(this->BlockBuff + sizeof(Block));
	}
	BlockIndex* end() {
		return (BlockIndex*)(this->BlockBuff + Super.BlockSize);
	}
	BlockIndex* loc(uint64_t index) {
		return (BlockIndex*)(this->BlockBuff + sizeof(Block) + (index * sizeof(BlockIndex)));
	}
};
class DirBlockManager :public BlockManager
{
public:
	void write(BlockIndex index)
	{
		Block b;
		for (size_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex))
		{
			if (*(FCBIndex*)(BlockBuff + pos) != -1) {
				b.Size++;
			}
		}
		b.FCS = 0;
		memcpy(BlockBuff, &b, sizeof(Block));
		StoreBlock(index, BlockBuff);
	}
	FCBIndex* begin() {
		return (FCBIndex*)(this->BlockBuff + sizeof(Block));
	}
	FCBIndex* end() {
		return (FCBIndex*)(this->BlockBuff + Super.BlockSize);
	}
	FCBIndex* loc(uint64_t index) {
		return (FCBIndex*)(this->BlockBuff + sizeof(Block) + (index * sizeof(FCBIndex)));
	}
};
class DataBlockManager :public BlockManager
{
public:
	void write(BlockIndex index, uint16_t size)
	{
		Block b;
		b.Size = size;
		b.FCS = 0;
		memcpy(BlockBuff, &b, sizeof(Block));
		StoreBlock(index, BlockBuff);

	}
	uint8_t* begin() {
		return (uint8_t*)(this->BlockBuff + sizeof(Block));
	}
	uint8_t* end() {
		return (uint8_t*)(this->BlockBuff + Super.BlockSize);
	}
};

class FCBPointerManager
{
public:
	FCBPointerManager() {
		Pointer = (uint8_t*)malloc(Super.BlockSize);
	}
	~FCBPointerManager()
	{
		free(Pointer);
	}

	void Load(FCBIndex index) {
		LoadFCB(index, &FCB);
		for (size_t i = 0; i < 10; i++)
		{
			BlockList.push_back(FCB.DirectBlock[i]);
		}
		if (FCB.Pointer != -1) {
			LoadBlock(FCB.Pointer, Pointer);
			for (auto iter = (BlockIndex*)(Pointer + sizeof(Block)); iter < (BlockIndex*)(Pointer + Super.BlockSize); iter++)
			{
				BlockList.push_back(*iter);
			}
		}
	}
	//获取本文件的第index个块
	BlockIndex GetBlockIndex(uint32_t index) {
		if (index < 10) {
			return FCB.DirectBlock[index];
		}
		else if (FCB.Pointer == -1) {
			return -1;
		}
		else {
			return *((BlockIndex*)(Pointer + sizeof(Block)) + (index % 10));
		}
	}
	//增加新的块
	BlockIndex AddNewBlock(uint8_t* blockBuff) {
		BlockIndex blockIndex = -1;
		for (int i = 0; i < 10; i++) {
			if (FCB.DirectBlock[i] == -1) {
				blockIndex = getEmptyBlock();
				DataBitMap->set(blockIndex, true);
				StoreBlock(blockIndex, blockBuff);//写入数据块
				FCB.DirectBlock[i] = blockIndex;//更新FCB
				goto AppendNewBlock_end;
			}
		}
		if (FCB.Pointer != -1) {
			uint8_t* pointerBuff = (uint8_t*)malloc(Super.BlockSize);
			LoadBlock(FCB.Pointer, pointerBuff);
			for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(BlockIndex)) {
				if (*(BlockIndex*)(pointerBuff + pos) == -1) {
					blockIndex = getEmptyBlock();
					DataBitMap->set(blockIndex, true);
					StoreBlock(blockIndex, blockBuff);//写入数据块
					*(BlockIndex*)(pointerBuff + pos) = blockIndex;
					StoreBlock(FCB.Pointer, pointerBuff);//更新pointer块
					free(pointerBuff);
					goto AppendNewBlock_end;
				}
			}
			printf("ERROR:Append Block Failed! No Pointer remain.\n");
			return blockIndex;
		}
	AppendNewBlock_end:
		WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//写入DataBlock的bitmap
		StoreFCB(Self_Index, &FCB);
		return true;
	}
	//将第index页读入内存
	BlockIndex ReadPage(uint32_t index, uint8_t* blockBuff) {
		if (index < 10) {
			if (FCB.DirectBlock[index] != -1) {
				LoadBlock(FCB.DirectBlock[index], blockBuff);
				return FCB.DirectBlock[index];
			}
			else {
				return -1;
			}
		}
		else {
			auto blockId = *((BlockIndex*)(Pointer + sizeof(Block)) + (index % 10));
			if (blockId != -1) {
				LoadBlock(blockId, blockBuff);
				return blockId;
			}
			else {
				return -1;
			}
		}
	}

	FileControlBlock FCB;
	uint8_t* Pointer;
	FCBIndex Self_Index = -1;
	vector<BlockIndex> BlockList;

};

void LoadDisk() {}

void FormatDisk(uint32_t blocksize, uint32_t FCBBlockNum) {
	auto size = getDiskSize();

	//构建Super块
	Super.DiskSize = size;
	Super.BlockSize = blocksize;
	Super.BlockNum = Super.DiskSize / Super.BlockSize;
	if (FCBBlockNum == 0) {
		FCBBlockNum = (Super.BlockNum / 4) / (Super.BlockSize / FILE_CONTROL_BLOCK_SIZE);
	}

	Super.FCBNum = FCBBlockNum * blocksize / FILE_CONTROL_BLOCK_SIZE;


	uint32_t FCBBitmapBlock = (uint32_t)ceil(Super.FCBNum / (8.0 * blocksize));						//FCB位示图所占Blcok数
	uint32_t RemainBlock = Super.BlockNum - 1 - FCBBitmapBlock - FCBBlockNum;						//剩余的block数 = 总block数 - super - FCB位示图 - [Data位示图] - FCBBlockNum - [Data]
	uint32_t DataBitmapBlock = (uint32_t)ceil(RemainBlock * 1.0 / (1 + 8 * blocksize));				//位示图占x个block，data区有`blocksize`*8*x个DataBlock。所以x=Remian/(1+8*blocksize)
	Super.DataBlockNum = RemainBlock - DataBitmapBlock;
	Super.FCBBitmapOffset = 1;
	Super.DataBitmapOffset = 1 + FCBBitmapBlock;
	Super.FCBOffset = Super.DataBitmapOffset + DataBitmapBlock;
	Super.DataOffset = Super.FCBOffset + FCBBlockNum;
	WriteDisk((uint8_t*)&Super, 0, sizeof(Super));															//写入SuperBlock

	FCBBitMap = new BiSet(Super.FCBNum);																	//初始化FCB的bitmap
	DataBitMap = new BiSet(Super.DataBlockNum);																//初始化Data的bitmap

	isMounted = true;


	//空目录不需要数据块
	////构建根目录数据块
	//uint8_t* DirDataBlock = (uint8_t*)malloc(Super.BlockSize);
	//MakeDirBlock(0, DirDataBlock);
	//BlockIndex StorgeLocation = getEmptyBlock();
	//DataBitMap->set(StorgeLocation, true);																	//设置FCB的bitmap
	//StoreBlock(StorgeLocation, DirDataBlock);																//写入DirectoryBlock
	//free(DirDataBlock);

	//构建根目录FCB
	FileControlBlock root(FileType::Directory, "Root", (uint8_t)Access::None, 0);
	strcpy(root.Name, "Root");
	root.AccessMode = (uint8_t)Access::None;
	time(&root.CreateTime);

	root.Size = 0;
	//空目录暂无数据块
	//root.DirectBlock[0] = StorgeLocation;																	//设置存储块号
	FCBBitMap->set(0, true);																				//设置DataBlock的bitmap
	//WriteDisk((uint8_t*)&root, Super.BlockSize * (Super.FCBOffset + 0), sizeof(root));						
	StoreFCB(0, &root);																						//root的FCB.id = 0

	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//写入FCB的bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//写入DataBlock的bitmap
}

void Login(string userName, string password) {

}

void PrintDiskInfo() {
	printf("================================================\n");
	printf("Disk Info\n");
	printf("------------------------------------------------\n");
	printf("Basic info.\n");
	printf("    Disk Size             = %8.2lf MByte\n", Super.DiskSize * 1.0 / (1 << 20));
	printf("    Block Size            = %8d Bytes\n", Super.BlockSize);
	printf("    Total                 = %8d Blocks\n", Super.BlockNum);
	printf("------------------------------------------------\n");
	printf("Format info.\n");
	printf("    Super                 = %8d Blocks\n", 1);
	printf("    FCB Bitmap            = %8d Blocks\n", Super.DataBitmapOffset - Super.FCBBitmapOffset);
	printf("    Data Bitmap           = %8d Blocks\n", Super.FCBOffset - Super.DataBitmapOffset);
	printf("    FCB                   = %8d Blocks\n", Super.DataOffset - Super.FCBOffset);
	printf("                          = %8d pics\n", Super.FCBNum);
	printf("    Data                  = %8d Blocks\n", Super.DataBlockNum);
	printf("------------------------------------------------\n");
	printf("Usage info.\n");
	uint32_t freeFCB = 0, freeBlock = 0, usedFCB = 0, usedBlock = 0;
	for (size_t i = 0; i < FCBBitMap->SizeOfBool; i++)
	{
		if (FCBBitMap->get(i) == false) {
			freeFCB++;
		}
		else {
			usedFCB++;
		}
	}
	for (size_t i = 0; i < DataBitMap->SizeOfBool; i++)
	{
		if (DataBitMap->get(i) == false) {
			freeBlock++;
		}
		else {
			usedBlock++;
		}
	}
	printf("    FCB Total            = %8d pics\n", freeFCB + usedFCB);
	printf("        Available        = %8d pics\n", freeFCB);
	printf("        Used             = %8d pics\n", usedFCB);
	printf("    Block Total          = %8d Blocks\n", freeBlock + usedBlock);
	printf("        Available        = %8d Blocks\n", freeBlock);
	printf("        Used             = %8d Blocks\n", usedBlock);

	printf("================================================\n");

}

void PrintDir(FCBIndex dir) {
	DataReader reader(dir);
	uint8_t* buff = (uint8_t*)malloc(Super.BlockSize);
	BlockIndex block = 0;
	printf("================================================\n");
	printf("%12s | %12s | %6s | % 10s\n", "Name", "Size", "FCB", "Physical Address");
	FileControlBlock fcb;
	while (reader.ReadNextBlock(buff, &block) == true)
	{
		for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
			if (*(FCBIndex*)(buff + pos) == -1) {
				continue;
			}
			LoadFCB(*(FCBIndex*)(buff + pos), &fcb);
			if (fcb.Type == FileType::Directory) {
				printf("%12s | %12s | %6d | ", fcb.Name, "<Dir>", *(FCBIndex*)(buff + pos));
			}
			else {
				printf("%12s | %12d | %6d | ", fcb.Name, fcb.Size, *(FCBIndex*)(buff + pos));
			}
			for (int i = 0; i < 10 && fcb.DirectBlock[i] != -1; i++)
			{
				printf("0x%08X ", fcb.DirectBlock[i]);
			}
			printf("\n");
		}
	}
PrintDir_end:
	printf("================================================\n\n");
	free(buff);
}

void PrintFileInfo(FCBIndex file) {
	FileControlBlock fcb;
	LoadFCB(file, &fcb);
	printf("================================================\n");
	printf("Name        : %s\n", fcb.Name);
	if (fcb.Type == FileType::Directory) {
		printf("Type        : Directory\n");
	}
	else {
		printf("Type        : File\n");
		printf("Size        : %d Bytes\n", fcb.Size);

	}
	printf("Create Time : %s\n", ctime(&fcb.CreateTime));
	printf("Read Time   : %s\n", ctime(&fcb.ReadTime));
	printf("Modify Time : %s\n", ctime(&fcb.ModifyTime));
	printf("Parent      : %d\n", fcb.Parent);
	printf("================================================\n");
}

FCBIndex Find(FCBIndex dir, string filename)
{
	FileControlBlock fcb, result;
	LoadFCB(dir, &fcb);
	if (fcb.Type == FileType::File) {
		return -1;
	}
	else {
		uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSize);
		for (size_t i = 0; i < 10; i++)
		{
			if (fcb.DirectBlock[i] != -1) {
				LoadBlock(fcb.DirectBlock[i], blockBuff);
				for (int pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
					FCBIndex fcbIndex = *(FCBIndex*)(blockBuff + pos);
					if (fcbIndex != -1) {
						LoadFCB(fcbIndex, &result);
						if (strcmp(result.Name, filename.c_str()) == 0) {
							free(blockBuff);
							return fcbIndex;
						}
					}
				}
			}
		}
		free(blockBuff);
		return -1;
	}
}

FCBIndex Create(string name, FCBIndex dir, enum FileType t) {
	if (name.length() > 30) {
		printf("Directory Name Too long!\n");
		return -1;
	}
	uint8_t* BlockBuff = (uint8_t*)malloc(Super.BlockSize);
	//构建Data块!空文件不构建Data块


	//构建FCB块
	FileControlBlock fcb(t, name.c_str(), (uint8_t)Access::Read | (uint8_t)Access::Write | (uint8_t)Access::Delete, dir);
	//FCB.DirectBlock[0] = blockIndex;
	FCBIndex fcbIndex = getEmptyFCB();
	FCBBitMap->set(fcbIndex, true);
	StoreFCB(fcbIndex, &fcb);
	//写入父目录(将fcbIndex写入父目录)
	FileControlBlock dirFCB;
	LoadFCB(dir, &dirFCB);
	for (size_t i = 0; i < 10; i++)
	{
		if (dirFCB.DirectBlock[i] == -1) {//存入新的目录页
			MakeDirBlock(fcbIndex, BlockBuff);
			BlockIndex newDirPage = getEmptyBlock();
			DataBitMap->set(newDirPage, true);
			dirFCB.DirectBlock[i] = newDirPage;
			StoreBlock(newDirPage, BlockBuff);
		}
		else {//尝试加入现有目录页
			LoadBlock(dirFCB.DirectBlock[i], BlockBuff);
			for (FCBIndex* iter = (FCBIndex*)(BlockBuff + sizeof(Block)); iter < (FCBIndex*)(BlockBuff + Super.BlockSize); iter++)
			{
				if (*iter == -1) {
					*iter = fcbIndex;
					BlockSize(BlockBuff) += sizeof(FCBIndex);
					StoreBlock(dirFCB.DirectBlock[i], BlockBuff);
					goto CreateFile_end;
				}
			}
		}
	}

CreateFile_end:
	StoreFCB(dir, &dirFCB);
	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//写入FCB的bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//写入DataBlock的bitmap
	free(BlockBuff);
	return fcbIndex;
}


FCBIndex CreateDirectory(string name, FCBIndex dir) {
	return Create(name, dir, FileType::Directory);
}

FCBIndex CreateFile(string name, FCBIndex dir) {
	return Create(name, dir, FileType::File);
}




bool DeleteFile(FCBIndex file) {
	FileControlBlock thisFile;
	LoadFCB(file, &thisFile);
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSize);
	if (thisFile.Type == FileType::File) {
		vector<BlockIndex> blocks;
		for (int i = 0; i <= 10; ++i) {
			if (thisFile.DirectBlock[i] != -1) {
				blocks.push_back(thisFile.DirectBlock[i]);
			}
		}
		if (thisFile.Pointer != -1) {
			blocks.push_back(thisFile.Pointer);
			LoadBlock(thisFile.Pointer, blockBuff);
			for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(BlockIndex)) {
				if (*(BlockIndex*)(blockBuff + pos) != -1) {
					blocks.push_back(*(BlockIndex*)(blockBuff + pos));
				}
			}
		}
		//删除文件数据块
		for (size_t i = 0; i < blocks.size(); i++)
		{
			DataBitMap->set(blocks[i], false);
		}
		//删除FCB
		FCBBitMap->set(file, false);
	}
	else if (thisFile.Type == FileType::Directory) {
		for (int i = 0; i <= 10; ++i) {
			if (thisFile.DirectBlock[i] != -1) {
				LoadBlock(thisFile.DirectBlock[i], blockBuff);
				for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
					if (*(FCBIndex*)(blockBuff + pos) != -1) {
						DeleteFile(*(FCBIndex*)(blockBuff + pos));
					}
				}
			}
		}
	}
	//删除父目录的记录
	FCBIndex parentIndex = thisFile.Parent;
	FileControlBlock parentFCB;
	LoadFCB(parentIndex, &parentFCB);
	for (size_t i = 0; i < 10 && parentFCB.DirectBlock[i] != -1; i++)
	{
		LoadBlock(parentFCB.DirectBlock[i], blockBuff);
		for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
			if (*(FCBIndex*)(blockBuff + pos) == file) {
				*(FCBIndex*)(blockBuff + pos) = -1;
				StoreBlock(parentFCB.DirectBlock[i], blockBuff);
				//TEST ： 如果该目录块为空，则删除此目录块
				for (uint64_t pos2 = sizeof(Block); pos2 < Super.BlockSize; pos2 += sizeof(FCBIndex)) {
					if (*(FCBIndex*)(blockBuff + pos2) != -1) {
						goto DeleteFile_end;
					}
				}
				//删除空的目录表
				DataBitMap->set(parentFCB.DirectBlock[i], false);
				parentFCB.DirectBlock[i] = -1;
				//DirectBlock紧凑化
				for (int j = i + 1; j < 10; j++)
				{
					parentFCB.DirectBlock[j - 1] = parentFCB.DirectBlock[j];
				}
				parentFCB.DirectBlock[9] = -1;
				goto DeleteFile_end;
			}
		}
	}


DeleteFile_end:
	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//写入FCB的bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//写入DataBlock的bitmap

	free(blockBuff);
	return false;
}

int64_t WriteFile(FCBIndex file, int64_t pos, int64_t len, uint8_t* buff)
{
	FileControlBlock ThisFileFCB;
	LoadFCB(file, &ThisFileFCB);
	if (pos > ThisFileFCB.Size) {
		printf("Error:pos = %lld is larger than file's size = %lu.\n", pos, ThisFileFCB.Size);
		return -1;
	}
	else if (pos < 0) {
		printf("Error:pos = %lld should larger than or equal to 0.\n", pos);
		return -1;
	}
	else if (len < 0) {
		printf("Error:len = %lld should larger than or equal to 0.\n", len);
		return -1;
	}
	uint64_t inBlockPos = pos;
	uint64_t StartPos = 0;
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSize);

	FCBPointerManager PointerBlock;
	PointerBlock.Load(file);

	PointerBlock.FCB.Size = MAX(PointerBlock.FCB.Size, pos + len);

	for (size_t PageNum = 0; PageNum < MAX_POINTER; PageNum++)
	{
		BlockIndex block = PointerBlock.ReadPage(PageNum, blockBuff);
		if (block == -1) {//写入起点是新的块
			goto WriteFile_CreateNew;
		}
		else if (inBlockPos < MAX_BLOCK_SPACE) {//写入起点是当前块
			if (inBlockPos + len <= MAX_BLOCK_SPACE) {//在空隙中存下
				memcpy(blockBuff + inBlockPos, buff, len);
				BlockSize(blockBuff) = MAX(BlockSize(blockBuff), inBlockPos + len);
				StoreBlock(block, blockBuff);
				goto WriteFile_end;
			}
			else {//存不下->填满空隙
				memcpy(blockBuff + inBlockPos, buff, MAX_BLOCK_SPACE - inBlockPos);
				BlockSize(blockBuff) = MAX_BLOCK_SPACE;
				StoreBlock(block, blockBuff);
				StartPos += MAX_BLOCK_SPACE - inBlockPos;//写了MAX_BLOCK_SPACE - inBlockPos个字节
				goto WriteFile_CreateNew;
			}

		}
		else {//写入起点还在后面
			inBlockPos -= BlockSize(blockBuff);
		}
	}

WriteFile_CreateNew:
	//将buff从StartPos ~ len之间的部分写入新块

	while (StartPos < len)
	{
		if (len - StartPos >= MAX_BLOCK_SPACE) {//写满本块
			//pointerBlock.FCB.Size += MAX_BLOCK_SPACE;
			BlockSize(blockBuff) = MAX_BLOCK_SPACE;
			memcpy(blockBuff + sizeof(Block), buff + StartPos, MAX_BLOCK_SPACE);
			PointerBlock.AddNewBlock(blockBuff);
			StartPos += MAX_BLOCK_SPACE;
		}
		else {//还剩空间
			//pointerBlock.FCB.Size += (len - StartPos);
			BlockSize(blockBuff) = len;
			memcpy(blockBuff + sizeof(Block), buff + StartPos, len - StartPos);
			PointerBlock.AddNewBlock(blockBuff);
			StartPos += (len - StartPos);
		}
	}

WriteFile_end:
	free(blockBuff);
	StoreFCB(file, &PointerBlock.FCB);
	return pos + len;
}

int64_t ReadFile(FCBIndex file, int64_t pos, int64_t len, uint8_t* buff)
{
	FileControlBlock ThisFileFCB;
	LoadFCB(file, &ThisFileFCB);
	if (pos + len > ThisFileFCB.Size) {
		printf("Error:pos+len = %ld is larger than file's size = %lu.\n", pos + len, ThisFileFCB.Size);
		return -1;
	}
	else if (pos < 0) {
		printf("Error:pos = %ld should larger than or equal to 0.\n", pos);
		return -1;
	}
	else if (len < 0) {
		printf("Error:len = %ld should larger than or equal to 0.\n", len);
		return -1;
	}
	uint64_t inBlockPos = pos;
	uint64_t BytesToBeRead = len;
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSize);

	FCBPointerManager pointerBlock;
	pointerBlock.Load(file);
	size_t PageNum;
	for (PageNum = 0; PageNum < MAX_POINTER; PageNum++)
	{
		BlockIndex block = pointerBlock.ReadPage(PageNum, blockBuff);

		if (inBlockPos < MAX_BLOCK_SPACE) {//从此块开始读
			goto ReadFile_StartToRead;
		}
		else {//读取起点还在后面
			inBlockPos -= BlockSize(blockBuff);
		}
	}

ReadFile_StartToRead:
	//从文件的第PageNum块inBlockOffset字节开始，读取BytesToBeRead字节的数据

	for (; BytesToBeRead > 0; ++PageNum)
	{
		BlockIndex block = pointerBlock.ReadPage(PageNum, blockBuff);
		if (BytesToBeRead >= BlockSize(blockBuff)) {//读完整块
			memcpy(buff + (len - BytesToBeRead), blockBuff + sizeof(Block), BlockSize(blockBuff));
			BytesToBeRead -= BlockSize(blockBuff);
		}
		else {//读取一部分
			memcpy(buff + (len - BytesToBeRead), blockBuff + sizeof(Block), BytesToBeRead);
			BytesToBeRead -= BytesToBeRead;
		}
	}

ReadFile_end:
	free(blockBuff);
	StoreFCB(file, &pointerBlock.FCB);
	return pos + len;
}