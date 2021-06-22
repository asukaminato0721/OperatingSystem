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

	//����Super��
	Super.DiskSize = size;
	Super.BlockSize = blocksize;
	Super.BlockNum = Super.DiskSize / Super.BlockSize;
	if (FCBBlockNum == 0) {
		FCBBlockNum = (Super.BlockNum / 4) / (Super.BlockSize / FILE_CONTROL_BLOCK_SIZE);
	}

	Super.FCBNum = FCBBlockNum * blocksize / FILE_CONTROL_BLOCK_SIZE;


	uint32_t FCBBitmapBlock = (uint32_t)ceil(Super.FCBNum / (8.0 * blocksize));						//FCBλʾͼ��ռBlcok��
	uint32_t RemainBlock = Super.BlockNum - 1 - FCBBitmapBlock - FCBBlockNum;						//ʣ���block�� = ��block�� - super - FCBλʾͼ - [Dataλʾͼ] - FCBBlockNum - [Data]
	uint32_t DataBitmapBlock = (uint32_t)ceil(RemainBlock * 1.0 / (1 + 8 * blocksize));				//λʾͼռx��block��data����`blocksize`*8*x��DataBlock������x=Remian/(1+8*blocksize)
	Super.DataBlockNum = RemainBlock - DataBitmapBlock;

	sizeof(FileControlBlock);
	Super.FCBBitmapOffset = 1;
	Super.DataBitmapOffset = 1 + FCBBitmapBlock;
	Super.FCBOffset = Super.DataBitmapOffset + DataBitmapBlock;
	Super.DataOffset = Super.FCBOffset + FCBBlockNum;
	WriteDisk((uint8_t*)&Super, 0, sizeof(Super));															//д��SuperBlock



	FCBBitMap = new BiSet(Super.FCBNum);																	//��ʼ��FCB��bitmap
	DataBitMap = new BiSet(Super.DataBlockNum);																//��ʼ��Data��bitmap


	isMounted = true;


	//������Ŀ¼��
	uint8_t* DirBlock = (uint8_t*)malloc(Super.BlockSize);
	((Block*)DirBlock)->Size = 0;
	for (size_t i = sizeof(Block); i < Super.BlockSize; i += sizeof(FCBIndex))
	{
		*(FCBIndex*)(DirBlock + i) = -1;
	}
	BlockIndex StorgeLocation = getEmptyBlock();
	DataBitMap->set(StorgeLocation, true);																	//����FCB��bitmap
	WriteBlock(StorgeLocation, DirBlock);																	//д��DirectoryBlock
	free(DirBlock);

	//������Ŀ¼FCB
	FileControlBlock root(FileType::Directory, (char*)"Root", (uint8_t)Access::None);
	strcpy(root.Name, "Root");
	root.AccessMode = (uint8_t)Access::None;
	root.CreateTime = clock();

	root.Size = 0;
	root.DirectBlock[0] = StorgeLocation;																	//���ô洢���
	FCBBitMap->set(0, true);																				//����DataBlock��bitmap
	//WriteDisk((uint8_t*)&root, Super.BlockSize * (Super.FCBOffset + 0), sizeof(root));						
	WriteFCB(0, &root);																						//root��FCB.id = 0

	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//д��FCB��bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//д��DataBlock��bitmap
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

	//����Ŀ¼�ļ���DataBlock
	uint8_t* BlockBuff = (uint8_t*)malloc(Super.BlockSize);
	//����Ŀ¼��
	((Block*)BlockBuff)->Size = 0;

	*(FCBIndex*)(BlockBuff + sizeof(Block)) = WorkDirectoryIndex;//�ϼ�Ŀ¼
	for (size_t i = sizeof(Block) + sizeof(Block); i < Super.BlockSize; i += sizeof(FCBIndex))
	{
		*(FCBIndex*)(BlockBuff + i) = -1;//��ʼ���¼�Ŀ¼
	}
	BlockIndex StorgeLocation = getEmptyBlock();
	DataBitMap->set(StorgeLocation, true);																	//����FCB��bitmap
	WriteBlock(StorgeLocation, BlockBuff);																	//д��DirectoryBlock


	//����Ŀ¼FCB
	FileControlBlock DirFCB(FileType::Directory, (char*)name.c_str(), (uint8_t)Access::Read | (uint8_t)Access::Write | (uint8_t)Access::Delete);
	DirFCB.DirectBlock[0] = StorgeLocation;																			//����dataBlock
	FCBIndex newFcbIndex = getEmptyFCB();
	FCBBitMap->set(newFcbIndex, true);																				//����DataBlock��bitmap
	WriteFCB(newFcbIndex, &DirFCB);


	//��ȡ��Ŀ¼,�������б�����ӱ��ļ��е�FCBid
	FileControlBlock ParentFCB;
	LoadFCB(parent, &ParentFCB);
	LoadBlock(ParentFCB.DirectBlock[0], BlockBuff);//BlockBuff������ļ���FCBIndex

	//���0�ſ�
	LoadBlock(ParentFCB.DirectBlock[0], BlockBuff);//BlockBuff����˸�Ŀ¼��FCBIndex �� ���ļ���FCBIndex
	for (int pos = sizeof(Block) + sizeof(FCBIndex); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
		FCBIndex fcbIndex = *(FCBIndex*)(BlockBuff + pos);
		if (fcbIndex != -1) {
			//�������
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
	//���1-9��
	for (size_t i = 1; i < 10; i++)
	{
		//���Ѵ���->�ҿ�λ
		if (ParentFCB.DirectBlock[i] != -1) {
			LoadBlock(ParentFCB.DirectBlock[i], BlockBuff);//BlockBuff������ļ���FCBIndex
			for (int pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
				FCBIndex fcbIndex = *(FCBIndex*)(BlockBuff + pos);
				if (fcbIndex != -1) {
					//�������
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
		else {//�鲻����->�½���
			//�����µĿ�
			((Block*)BlockBuff)->Size = 0;
			*(FCBIndex*)(BlockBuff + sizeof(Block)) = newFcbIndex;
			for (size_t i = sizeof(Block); i < Super.BlockSize; i += sizeof(FCBIndex))
			{
				*(FCBIndex*)(BlockBuff + i) = -1;
			}
			BlockIndex StorgeLocation = getEmptyBlock();
			DataBitMap->set(StorgeLocation, true);																	//����FCB��bitmap
			ParentFCB.DirectBlock[i] = StorgeLocation;
			WriteBlock(StorgeLocation, BlockBuff);			//д��DirectoryBlock
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

