#define _CRT_SECURE_NO_WARNINGS
#include "FileSystem.h"
#include "Driver.h"

#define FILE_CONTROL_BLOCK_SIZE 128u
#define MAX_POINTER ((Super.BlockSize-sizeof(Block))/sizeof(BlockIndex)+10)
#define MAX_BLOCK_SPACE (Super.BlockSize-sizeof(Block))


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

bool AppendNewBlock(FileControlBlock* fcb, FCBIndex fcbIndex, uint8_t* blockBuff) {

	for (int i = 0; i < 10; i++) {
		if (fcb->DirectBlock[i] == -1) {
			BlockIndex blockIndex = getEmptyBlock();
			DataBitMap->set(blockIndex, true);
			StoreBlock(blockIndex, blockBuff);//д�����ݿ�
			fcb->DirectBlock[i] = blockIndex;//����FCB
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
				StoreBlock(blockIndex, blockBuff);//д�����ݿ�
				*(BlockIndex*)(pointerBuff + pos) = blockIndex;
				StoreBlock(fcb->Pointer, pointerBuff);//����pointer��
				free(pointerBuff);
				goto AppendNewBlock_end;
			}
		}
		printf("ERROR:Append Block Failed! No Pointer remain.\n");
		return false;
	}
AppendNewBlock_end:
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//д��DataBlock��bitmap
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
	void Load(FCBIndex index) {

		LoadFCB(index, &fcb);
		if (fcb.Pointer != -1) {
			Pointer.load(fcb.Pointer);
		}
	}

	BlockIndex getBlockIndex(uint32_t index) {
		if (index < 10) {
			return fcb.DirectBlock[index];
		}
		else if (fcb.Pointer == -1) {
			return -1;
		}
		else {
			return *Pointer.loc(index % 10);
		}
	}

	BlockIndex AddNewBlock(uint8_t* blockBuff) {
		BlockIndex blockIndex = -1;
		for (int i = 0; i < 10; i++) {
			if (fcb.DirectBlock[i] == -1) {
				blockIndex = getEmptyBlock();
				DataBitMap->set(blockIndex, true);
				StoreBlock(blockIndex, blockBuff);//д�����ݿ�
				fcb.DirectBlock[i] = blockIndex;//����FCB
				goto AppendNewBlock_end;
			}
		}
		if (fcb.Pointer != -1) {
			uint8_t* pointerBuff = (uint8_t*)malloc(Super.BlockSize);
			LoadBlock(fcb.Pointer, pointerBuff);
			for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(BlockIndex)) {
				if (*(BlockIndex*)(pointerBuff + pos) == -1) {
					blockIndex = getEmptyBlock();
					DataBitMap->set(blockIndex, true);
					StoreBlock(blockIndex, blockBuff);//д�����ݿ�
					*(BlockIndex*)(pointerBuff + pos) = blockIndex;
					StoreBlock(fcb.Pointer, pointerBuff);//����pointer��
					free(pointerBuff);
					goto AppendNewBlock_end;
				}
			}
			printf("ERROR:Append Block Failed! No Pointer remain.\n");
			return blockIndex;
		}
	AppendNewBlock_end:
		WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//д��DataBlock��bitmap
		StoreFCB(Self_Index, &fcb);
		return true;
	}
	FileControlBlock fcb;
	PointerBlockManager Pointer;
	FCBIndex Self_Index = -1;


};

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
	Super.FCBBitmapOffset = 1;
	Super.DataBitmapOffset = 1 + FCBBitmapBlock;
	Super.FCBOffset = Super.DataBitmapOffset + DataBitmapBlock;
	Super.DataOffset = Super.FCBOffset + FCBBlockNum;
	WriteDisk((uint8_t*)&Super, 0, sizeof(Super));															//д��SuperBlock

	FCBBitMap = new BiSet(Super.FCBNum);																	//��ʼ��FCB��bitmap
	DataBitMap = new BiSet(Super.DataBlockNum);																//��ʼ��Data��bitmap

	isMounted = true;


	//��Ŀ¼����Ҫ���ݿ�
	////������Ŀ¼���ݿ�
	//uint8_t* DirDataBlock = (uint8_t*)malloc(Super.BlockSize);
	//MakeDirBlock(0, DirDataBlock);
	//BlockIndex StorgeLocation = getEmptyBlock();
	//DataBitMap->set(StorgeLocation, true);																	//����FCB��bitmap
	//StoreBlock(StorgeLocation, DirDataBlock);																//д��DirectoryBlock
	//free(DirDataBlock);

	//������Ŀ¼FCB
	FileControlBlock root(FileType::Directory, "Root", (uint8_t)Access::None, 0);
	strcpy(root.Name, "Root");
	root.AccessMode = (uint8_t)Access::None;
	time(&root.CreateTime);

	root.Size = 0;
	//��Ŀ¼�������ݿ�
	//root.DirectBlock[0] = StorgeLocation;																	//���ô洢���
	FCBBitMap->set(0, true);																				//����DataBlock��bitmap
	//WriteDisk((uint8_t*)&root, Super.BlockSize * (Super.FCBOffset + 0), sizeof(root));						
	StoreFCB(0, &root);																						//root��FCB.id = 0

	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//д��FCB��bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//д��DataBlock��bitmap
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

FCBIndex CreateDirectory(string name, FCBIndex parent) {
	if (name.length() > 30) {
		printf("Directory Name Too long!\n");
		return -1;
	}

	//����Ŀ¼�ļ���DataBlock
	uint8_t* BlockBuff = (uint8_t*)malloc(Super.BlockSize);

	////����Ŀ¼��
	//MakeDirBlock(parent, BlockBuff);
	//BlockIndex StorgeLocation = getEmptyBlock();
	//DataBitMap->set(StorgeLocation, true);																	//����FCB��bitmap
	//StoreBlock(StorgeLocation, BlockBuff);																	//д��DirectoryBlock


	//����Ŀ¼FCB
	FileControlBlock DirFCB(FileType::Directory, name.c_str(), (uint8_t)Access::Read | (uint8_t)Access::Write | (uint8_t)Access::Delete, parent);
	//DirFCB.DirectBlock[0] = StorgeLocation;																			//����dataBlock
	FCBIndex newFcbIndex = getEmptyFCB();
	FCBBitMap->set(newFcbIndex, true);																				//����DataBlock��bitmap
	StoreFCB(newFcbIndex, &DirFCB);


	//��ȡ��Ŀ¼,�������б�����ӱ��ļ��е�FCBid
	FileControlBlock ParentFCB;
	LoadFCB(parent, &ParentFCB);
	LoadBlock(ParentFCB.DirectBlock[0], BlockBuff);//BlockBuff������ļ���FCBIndex
	//TODO : �޷���ȷ�ļ����������Ҫ�������п��Լ������
	//TODO : �޷���������pointer����
	//���0-9��
	for (size_t i = 0; i < 10; i++)
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
			//�鲻����->�½���
			//�����µĿ�
			((Block*)BlockBuff)->Size = 0;
			for (size_t i = sizeof(Block); i < Super.BlockSize; i += sizeof(FCBIndex))
			{
				*(FCBIndex*)(BlockBuff + i) = -1;
			}
			*(FCBIndex*)(BlockBuff + sizeof(Block)) = newFcbIndex;
			BlockIndex StorgeLocation = getEmptyBlock();
			DataBitMap->set(StorgeLocation, true);			//����FCB��bitmap
			ParentFCB.DirectBlock[i] = StorgeLocation;
			StoreBlock(StorgeLocation, BlockBuff);			//д��DirectoryBlock
			StoreFCB(parent, &ParentFCB);
			goto CreateDirectory_end;
		}
	}
	//
CreateDirectory_end:
	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//д��FCB��bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//д��DataBlock��bitmap
	free(BlockBuff);
	return newFcbIndex;
}

FCBIndex CreateFile(string name, FCBIndex dir) {
	if (name.length() > 30) {
		printf("Directory Name Too long!\n");
		return -1;
	}
	uint8_t* BlockBuff = (uint8_t*)malloc(Super.BlockSize);
	////����Data��!���ļ�������Data��
	//BlockIndex blockIndex = getEmptyBlock();
	//DataBitMap->set(blockIndex, true);
	//StoreBlock(blockIndex, BlockBuff);
	//����FCB��
	FileControlBlock fcb(FileType::File, name.c_str(), (uint8_t)Access::Read | (uint8_t)Access::Write | (uint8_t)Access::Delete, dir);
	//fcb.DirectBlock[0] = blockIndex;
	FCBIndex fcbIndex = getEmptyFCB();
	FCBBitMap->set(fcbIndex, true);
	StoreFCB(fcbIndex, &fcb);
	//д�븸Ŀ¼
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
	//���б����ˣ������±�
	MakeDirBlock(fcbIndex, BlockBuff);//�����հ�dir��,���һ��Ϊ�ļ���FCB
	AppendNewBlock(&fcb, dir, BlockBuff);//��dir����������Ŀ¼��fcb��
	//TODO��δ��ɼ����µ���������ǰǰһ����������ȫ����ʱ���޷��Զ��л�����һ�ţ�

CreateFile_end:
	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//д��FCB��bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//д��DataBlock��bitmap
	free(BlockBuff);
	return fcbIndex;
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
		//ɾ���ļ����ݿ�
		for (size_t i = 0; i < blocks.size(); i++)
		{
			DataBitMap->set(blocks[i], false);
		}
		//ɾ��FCB
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
	//ɾ����Ŀ¼�ļ�¼
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
				//TEST �� �����Ŀ¼��Ϊ�գ���ɾ����Ŀ¼��
				for (uint64_t pos2 = sizeof(Block); pos2 < Super.BlockSize; pos2 += sizeof(FCBIndex)) {
					if (*(FCBIndex*)(blockBuff + pos2) != -1) {
						goto DeleteFile_end;
					}
				}
				//ɾ���յ�Ŀ¼��
				DataBitMap->set(parentFCB.DirectBlock[i], false);
				parentFCB.DirectBlock[i] = -1;
				//DirectBlock���ջ�
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
	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);				//д��FCB��bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte);			//д��DataBlock��bitmap

	free(blockBuff);
	return false;
}

uint64_t WriteFile(FCBIndex file, uint64_t pos, uint64_t len, uint8_t* buff)
{
	FCBPointerManager pointManager;
	pointManager.Load(file);
	if (pos > pointManager.fcb.Size) {
		printf("Error:Pos is larger than file's size.\n");
		return -1;
	}
	uint64_t inBlockPos = pos;
	uint64_t StartPos = 0;
	DataBlockManager dbm;
	for (size_t i = 0; i < MAX_POINTER; i++)
	{
		BlockIndex block = pointManager.getBlockIndex(i);
		if (inBlockPos < Super.BlockSize - sizeof(Block)) {//�ҵ���ǰ��
			if (block != -1) {//�ڱ����ڸ�д
				dbm.load(block);
				if (inBlockPos + len <= MAX_BLOCK_SPACE) {//����ȫ��װ��
					pointManager.fcb.Size = pointManager.fcb.Size - dbm.Size + inBlockPos + len;
					memcpy(dbm.BlockBuff + sizeof(Block) + inBlockPos, buff, len);
					StartPos += len;
					dbm.write(block, inBlockPos + len);
					goto WriteFile_end;
				}
				else {//װ����
					pointManager.fcb.Size = pointManager.fcb.Size - dbm.Size + MAX_BLOCK_SPACE;
					memcpy(dbm.BlockBuff + sizeof(Block) + inBlockPos, buff, MAX_BLOCK_SPACE - inBlockPos);
					StartPos += (MAX_BLOCK_SPACE - inBlockPos);
					dbm.write(block, MAX_BLOCK_SPACE);
					break;
				}
			}
			else {//�����µĿ�
				break;
			}
		}
		else {//���������
			assert(block != -1);
			dbm.load(block);
			inBlockPos -= dbm.Size;
		}
	}

	//��ʣ�µ�len����װ���¿�


	while (StartPos < len)
	{
		if (len - StartPos >= MAX_BLOCK_SPACE) {//д������
			pointManager.fcb.Size = pointManager.fcb.Size + MAX_BLOCK_SPACE;
			dbm.Fcs = 0;
			dbm.Size = MAX_BLOCK_SPACE;
			dbm.makeBuffer();
			memcpy(dbm.BlockBuff + sizeof(Block), buff + StartPos, MAX_BLOCK_SPACE);
			pointManager.AddNewBlock(dbm.BlockBuff);
			StartPos += MAX_BLOCK_SPACE;
		}
		else {//��ʣ�ռ�
			pointManager.fcb.Size = pointManager.fcb.Size + (len - StartPos);
			dbm.Fcs = 0;
			dbm.Size = len;
			dbm.makeBuffer();
			memcpy(dbm.BlockBuff + sizeof(Block), buff + StartPos, len - StartPos);
			pointManager.AddNewBlock(dbm.BlockBuff);
			StartPos += (len - StartPos);
		}
	}

WriteFile_end:
	StoreFCB(file, &pointManager.fcb);
	return pos + len;
}


