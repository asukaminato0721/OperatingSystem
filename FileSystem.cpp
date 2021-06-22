#define _CRT_SECURE_NO_WARNINGS
#include "FileSystem.h"
#include "Driver.h"

#define FILE_CONTROL_BLOCK_SIZE 128u



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
		this->data[index / 8] = this->data[index / 8] | (val << (index % 8));
	}
	else {
		this->data[index / 8] = this->data[index / 8] & (val << (index % 8));
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

bool AppendNewBlock(FileControlBlock* fcb, uint8_t* blockBuff) {

	for (int i = 0; i < 10; i++) {
		if (fcb->DirectBlock[i] == -1) {
			BlockIndex blockIndex = getEmptyBlock();
			DataBitMap->set(blockIndex, true);
			StoreBlock(blockIndex, blockBuff);//写入数据库
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

void PrintInfo() {
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
	printf("Directory\n");
	printf("Name        : %s\n", reader.fcb.Name);
	printf("Create Time : %s\n", ctime(&reader.fcb.CreateTime));
	printf("Read Time   : %s\n", ctime(&reader.fcb.ReadTime));
	printf("Modify Time : %s\n", ctime(&reader.fcb.ModifyTime));
	printf("Parent      : %d\n", reader.fcb.Parent);
	printf("------------------------------------------------\n");
	printf("%12s | %12s | %6s | % 10s\n", "Name", "Size", "FCB", "Physical Address");
	printf("------------------------------------------------\n");
	uint64_t pos = sizeof(block) + sizeof(FCBIndex);//跳过第一个（自己）
	FileControlBlock fcb;
	while (reader.ReadNextBlock(buff, &block) == true)
	{
		for (; pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
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
		pos = sizeof(block);
	}
PrintDir_end:
	printf("================================================\n\n");
	free(buff);
}

bool CreateDirectory(string name, FCBIndex parent) {
	if (name.length() > 30) {
		printf("Directory Name Too long!\n");
		return false;
	}

	//构建目录文件的DataBlock
	uint8_t* BlockBuff = (uint8_t*)malloc(Super.BlockSize);
	//构建目录块
	MakeDirBlock(parent, BlockBuff);
	BlockIndex StorgeLocation = getEmptyBlock();
	DataBitMap->set(StorgeLocation, true);																	//设置FCB的bitmap
	StoreBlock(StorgeLocation, BlockBuff);																	//写入DirectoryBlock


	//构建目录FCB
	FileControlBlock DirFCB(FileType::Directory, name.c_str(), (uint8_t)Access::Read | (uint8_t)Access::Write | (uint8_t)Access::Delete, parent);
	DirFCB.DirectBlock[0] = StorgeLocation;																			//设置dataBlock
	FCBIndex newFcbIndex = getEmptyFCB();
	FCBBitMap->set(newFcbIndex, true);																				//设置DataBlock的bitmap
	StoreFCB(newFcbIndex, &DirFCB);


	//读取父目录,并在其列表中添加本文件夹的FCBid
	FileControlBlock ParentFCB;
	LoadFCB(parent, &ParentFCB);
	LoadBlock(ParentFCB.DirectBlock[0], BlockBuff);//BlockBuff存放子文件的FCBIndex
	//TODO : 无法正确的检查重名。需要遍历所有块以检查重名
	//TODO : 无法建立二级pointer索引
	//检查0-9块
	for (size_t i = 0; i < 10; i++)
	{
		//块已存在->找空位
		if (ParentFCB.DirectBlock[i] != -1) {
			LoadBlock(ParentFCB.DirectBlock[i], BlockBuff);//BlockBuff存放子文件的FCBIndex
			for (int pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
				FCBIndex fcbIndex = *(FCBIndex*)(BlockBuff + pos);
				if (fcbIndex != -1) {
					//检查重名
					FileControlBlock NameCheck;
					LoadFCB(fcbIndex, &NameCheck);
					if (strcmp(NameCheck.Name, name.c_str()) == 0) {
						printf("Error!Name duplicate!\n");
						goto CreateDirectory_end;
					}
				}
				else {
					*(FCBIndex*)(BlockBuff + pos) = newFcbIndex;
					StoreBlock(ParentFCB.DirectBlock[0], BlockBuff);
					goto CreateDirectory_end;
				}
			}
		}
		else {
			//块不存在->新建块
			//创建新的块
			((Block*)BlockBuff)->Size = 0;
			*(FCBIndex*)(BlockBuff + sizeof(Block)) = newFcbIndex;
			for (size_t i = sizeof(Block); i < Super.BlockSize; i += sizeof(FCBIndex))
			{
				*(FCBIndex*)(BlockBuff + i) = -1;
			}
			BlockIndex StorgeLocation = getEmptyBlock();
			DataBitMap->set(StorgeLocation, true);			//设置FCB的bitmap
			ParentFCB.DirectBlock[i] = StorgeLocation;
			StoreBlock(StorgeLocation, BlockBuff);			//写入DirectoryBlock
			StoreFCB(parent, &ParentFCB);
		}
	}
	//
CreateDirectory_end:
	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//写入FCB的bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//写入DataBlock的bitmap
	free(BlockBuff);
	return true;
}

bool CreateFile(string name, FCBIndex dir) {
	//构建Data块
	uint8_t* BlockBuff = (uint8_t*)malloc(Super.BlockSize);
	BlockIndex blockIndex = getEmptyBlock();
	DataBitMap->set(blockIndex, true);
	StoreBlock(blockIndex, BlockBuff);
	//构建FCB块
	FileControlBlock fcb(FileType::File, name.c_str(), (uint8_t)Access::Read | (uint8_t)Access::Write | (uint8_t)Access::Delete, dir);
	fcb.DirectBlock[0] = blockIndex;
	FCBIndex fcbIndex = getEmptyFCB();
	FCBBitMap->set(fcbIndex, true);
	StoreFCB(fcbIndex, &fcb);
	//写入父目录
	LoadFCB(dir, &fcb);
	BlockIndex readBlockIndex;
	DataReader reader(dir);
	while (reader.ReadNextBlock(BlockBuff, &readBlockIndex))
	{
		for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
			if (*(FCBIndex*)(BlockBuff + pos) == -1) {
				*(FCBIndex*)(BlockBuff + pos) = fcbIndex;
				StoreBlock(readBlockIndex, BlockBuff);
				goto CreateFile_end;
			}
		}
	}
	//所有表都满了，新增新表
	MakeDirBlock(fcbIndex, BlockBuff);//制作空白dir块,其第一项为文件的FCB
	AppendNewBlock(&fcb, BlockBuff);//将dir块连接至父目录的fcb中
	//TODO：未完成加入新的索引表（当前前一个索引表完全填满时，无法自动切换至下一张）

CreateFile_end:
	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//写入FCB的bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//写入DataBlock的bitmap
	free(BlockBuff);
	return true;
}

bool DeleteFile(FCBIndex file) {
	FileControlBlock thisFile;
	LoadFCB(file, &thisFile);
	return true;
}

