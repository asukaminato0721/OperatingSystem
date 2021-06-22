#define _CRT_SECURE_NO_WARNINGS
#include "FileSystem.h"
#include "Driver.h"

#define FILE_CONTROL_BLOCK_SIZE 128u


FCBIndex WorkDirectoryIndex;

BiSet::BiSet(uint32_t size)
{
	this->SizeOfBool = size;
	this->SizeOfByte = size / 8;
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

FileControlBlock::FileControlBlock(FileType t, char* name, uint8_t AccessMode)
{
	strcpy(Name, name);
	this->Type = t;
	this->CreateTime = clock();
	this->Size = 0;
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
	WorkDirectoryIndex = index;
	return;
}
void WriteFCB(FCBIndex index, FileControlBlock* buff) {
	WriteDisk((uint8_t*)buff, Super.FCBOffset * Super.BlockSize + index * FILE_CONTROL_BLOCK_SIZE, sizeof(FileControlBlock));
	return;
}
void LoadBlock(BlockIndex index, uint8_t* buff) {
	ReadDisk(buff, (index + Super.DataOffset) * Super.BlockSize, Super.BlockSize);
}
void WriteBlock(BlockIndex index, uint8_t* buff) {
	WriteDisk(buff, (index + Super.DataOffset) * Super.BlockSize, Super.BlockSize);
}

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

	sizeof(FileControlBlock);
	Super.FCBBitmapOffset = 1;
	Super.DataBitmapOffset = 1 + FCBBitmapBlock;
	Super.FCBOffset = Super.DataBitmapOffset + DataBitmapBlock;
	Super.DataOffset = Super.FCBOffset + FCBBlockNum;
	WriteDisk((uint8_t*)&Super, 0, sizeof(Super));															//写入SuperBlock



	FCBBitMap = new BiSet(Super.FCBNum);																	//初始化FCB的bitmap
	DataBitMap = new BiSet(Super.DataBlockNum);																//初始化Data的bitmap


	isMounted = true;


	//构建根目录块
	uint8_t* DirBlock = (uint8_t*)malloc(Super.BlockSize);
	((Block*)DirBlock)->Size = 0;
	for (size_t i = sizeof(Block); i < Super.BlockSize; i += sizeof(FCBIndex))
	{
		*(FCBIndex*)(DirBlock + i) = -1;
	}
	BlockIndex StorgeLocation = getEmptyBlock();
	DataBitMap->set(StorgeLocation, true);																	//设置FCB的bitmap
	WriteBlock(StorgeLocation, DirBlock);																	//写入DirectoryBlock
	free(DirBlock);

	//构建根目录FCB
	FileControlBlock root(FileType::Directory, (char*)"Root", (uint8_t)Access::None);
	strcpy(root.Name, "Root");
	root.AccessMode = (uint8_t)Access::None;
	root.CreateTime = clock();

	root.Size = 0;
	root.DirectBlock[0] = StorgeLocation;																	//设置存储块号
	FCBBitMap->set(0, true);																				//设置DataBlock的bitmap
	//WriteDisk((uint8_t*)&root, Super.BlockSize * (Super.FCBOffset + 0), sizeof(root));						
	WriteFCB(0, &root);																						//root的FCB.id = 0

	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//写入FCB的bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//写入DataBlock的bitmap
}

void Login(string userName, string password) {

}

void PrintInfo() {
	printf("Disk Size                = %8.2lf MByte\n", Super.DiskSize * 1.0 / (1 << 20));
	printf("Block Size               = %8d Bytes\n", Super.BlockSize);
	printf("Block Count              = %8d \n", Super.BlockNum);
	printf("\n");

	printf("Super Count              = %8d\n", 1);
	printf("FCB Bitmap Block Count   = %8d\n", Super.DataBitmapOffset - Super.FCBBitmapOffset);
	printf("Data Bitmap Block Count  = %8d\n", Super.FCBOffset - Super.DataBitmapOffset);
	printf("FCB Block Count          = %8d\n", Super.DataOffset - Super.FCBOffset);
	printf("    FCB Count            = %8d\n", Super.FCBNum);
	printf("Data Block Count         = %8d\n", Super.DataBlockNum);
}

void PrintDir(FCBIndex dir) {
	FileControlBlock fcb;
	LoadFCB(dir, &fcb);
	uint8_t* buff = (uint8_t*)malloc(Super.BlockSize);
	printf("================================================\n");
	printf("Directory : %s\n", fcb.Name);
	printf("------------------------------------------------\n");
	printf("%12s | %13s | % 10s\n", "Name", "Size", "Physical Address");
	printf("------------------------------------------------\n");
	bool isNull = true;
	for (size_t i = 0; i < 10; i++)
	{
		BlockIndex TableBlock = fcb.DirectBlock[i];
		if (TableBlock == -1) {
			break;
		}
		else {
			sizeof(SuperBlock);
			ReadDisk(buff, (Super.DataOffset + TableBlock) * Super.BlockSize, Super.BlockSize);
			for (size_t pos = sizeof(Block) + sizeof(FCBIndex); pos < Super.BlockSize; pos += sizeof(FCBIndex))
			{
				FCBIndex fcbIndex = *(FCBIndex*)(buff + pos);
				if (fcbIndex == -1) {
					break;
				}
				else {
					FileControlBlock fcb;
					LoadFCB(fcbIndex, &fcb);
					if (fcb.Type == FileType::Directory) {
						printf("%12s | %13s | ", fcb.Name, "Directory");
						for (int i = 0; i < 10 && fcb.DirectBlock[i] != -1; i++)
						{
							printf("%10u", fcb.DirectBlock[i]);
						}
						printf("\n");
					}
					else if (fcb.Type == FileType::File) {
						printf("%12s | %8dBytes | % 10u\n", fcb.Name, fcb.Size, fcb.DirectBlock[0]);
					}
					isNull = false;
				}
			}
		}
	}
	if (isNull) {
		printf("            Directory Empty             \n");
	}
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
	((Block*)BlockBuff)->Size = 0;

	*(FCBIndex*)(BlockBuff + sizeof(Block)) = WorkDirectoryIndex;//上级目录
	for (size_t i = sizeof(Block) + sizeof(Block); i < Super.BlockSize; i += sizeof(FCBIndex))
	{
		*(FCBIndex*)(BlockBuff + i) = -1;//初始化下级目录
	}
	BlockIndex StorgeLocation = getEmptyBlock();
	DataBitMap->set(StorgeLocation, true);																	//设置FCB的bitmap
	WriteBlock(StorgeLocation, BlockBuff);																	//写入DirectoryBlock


	//构建目录FCB
	FileControlBlock DirFCB(FileType::Directory, (char*)name.c_str(), (uint8_t)Access::Read | (uint8_t)Access::Write | (uint8_t)Access::Delete);
	DirFCB.DirectBlock[0] = StorgeLocation;																			//设置dataBlock
	FCBIndex newFcbIndex = getEmptyFCB();
	FCBBitMap->set(newFcbIndex, true);																				//设置DataBlock的bitmap
	WriteFCB(newFcbIndex, &DirFCB);


	//读取父目录,并在其列表中添加本文件夹的FCBid
	FileControlBlock ParentFCB;
	LoadFCB(parent, &ParentFCB);
	LoadBlock(ParentFCB.DirectBlock[0], BlockBuff);//BlockBuff存放子文件的FCBIndex

	//检查0号块
	LoadBlock(ParentFCB.DirectBlock[0], BlockBuff);//BlockBuff存放了父目录的FCBIndex 和 子文件的FCBIndex
	for (int pos = sizeof(Block) + sizeof(FCBIndex); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
		FCBIndex fcbIndex = *(FCBIndex*)(BlockBuff + pos);
		if (fcbIndex != -1) {
			//检查重名
			FileControlBlock NameCheck;
			LoadFCB(fcbIndex, &NameCheck);
			if (strcmp(NameCheck.Name, name.c_str()) == 0) {
				printf("Error!Name duplicate!\n");
				goto end;
			}
		}
		else {
			*(FCBIndex*)(BlockBuff + pos) = newFcbIndex;
			WriteBlock(ParentFCB.DirectBlock[0], BlockBuff);
			goto end;
		}
	}
	//检查1-9块
	for (size_t i = 1; i < 10; i++)
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
						goto end;
					}
				}
				else {
					*(FCBIndex*)(BlockBuff + pos) = newFcbIndex;
					WriteBlock(ParentFCB.DirectBlock[0], BlockBuff);
					goto end;
				}
			}
		}
		else {//块不存在->新建块
			//创建新的块
			((Block*)BlockBuff)->Size = 0;
			*(FCBIndex*)(BlockBuff + sizeof(Block)) = newFcbIndex;
			for (size_t i = sizeof(Block); i < Super.BlockSize; i += sizeof(FCBIndex))
			{
				*(FCBIndex*)(BlockBuff + i) = -1;
			}
			BlockIndex StorgeLocation = getEmptyBlock();
			DataBitMap->set(StorgeLocation, true);																	//设置FCB的bitmap
			ParentFCB.DirectBlock[i] = StorgeLocation;
			WriteBlock(StorgeLocation, BlockBuff);			//写入DirectoryBlock
			WriteFCB(parent, &ParentFCB);
		}
	}

end:
	free(BlockBuff);
	return true;
}

bool CreateFile(string name, FCBIndex parent) {
	return false;
}

bool DeleteFile() {
	return false;
}

