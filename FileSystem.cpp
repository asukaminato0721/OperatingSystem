#define _CRT_SECURE_NO_WARNINGS
#include "FileSystem.h"
#include "Driver.h"
#include "Crypto.h"


//Global Vars
SuperBlock Super;

class BiSet {
public:
	BiSet(uint32_t size) {
		this->SizeOfBool = size;
		this->SizeOfByte = (int)ceil(size / 8);
		if (size % 8 != 0) {
			this->SizeOfByte++;
		}
		this->data = (uint8_t*)calloc(this->SizeOfByte, sizeof(uint8_t));
	}
	bool get(uint32_t index) { return bool(data[index / 8] & (1 << (index % 8))); }
	void set(uint32_t index, bool val) {
		if (val == true) {
			this->data[index / 8] = this->data[index / 8] | (1 << (index % 8));
		}
		else {
			uint8_t mask = ~(uint8_t)(1 << (index % 8));
			this->data[index / 8] = this->data[index / 8] & mask;
		}
	}
	~BiSet() { free(this->data); }
	uint32_t SizeOfBool;
	uint32_t SizeOfByte;
	uint8_t* data = nullptr;
}*FCBBitMap, * DataBitMap;


//Tool Functions

FileControlBlock::FileControlBlock() {}
FileControlBlock::FileControlBlock(enum FileType t, const char* name, uint8_t AccessMode, FCBIndex parent) {
	strcpy(Name, name);
	this->Type = t;
	time(&this->CreateTime);
	this->Size = 0;
	this->Parent = parent;
	this->AccessMode = AccessMode;
}

static inline BlockIndex GetEmptyBlock() {
	for (size_t i = 0; i < DataBitMap->SizeOfByte; ++i) {
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
static inline FCBIndex GetEmptyFCB() {

	for (size_t i = 0; i < FCBBitMap->SizeOfByte; ++i) {
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


static inline uint16_t& BlockSize(uint8_t* blockBuff) { return ((Block*)blockBuff)->Size; }
static inline uint16_t& BlockFCS(uint8_t* blockBuff) { return ((Block*)blockBuff)->FCS; }
static inline void LoadFCB(FCBIndex index, FileControlBlock* buff) {
	assert(index != -1);
	ReadDisk((uint8_t*)buff, (uint64_t)Super.FCBOffset * Super.BlockSize + (uint64_t)index * FILE_CONTROL_BLOCK_SIZE, FILE_CONTROL_BLOCK_SIZE);
	buff->ReadTime = time(NULL);
	return;
}
static inline void StoreFCB(FCBIndex index, FileControlBlock* buff) {
	assert(index != -1);
	WriteDisk((uint8_t*)buff, (uint64_t)Super.FCBOffset * Super.BlockSize + (uint64_t)index * FILE_CONTROL_BLOCK_SIZE, FILE_CONTROL_BLOCK_SIZE);
	return;
}
static inline void LoadBlock(BlockIndex index, uint8_t* buff) {
	assert(index != -1);
	ReadDisk(buff, ((uint64_t)index + Super.DataOffset) * Super.BlockSize, Super.BlockSize);
	if (Chk_XOR_ErrorDetection(buff + sizeof(Block), MAX_BLOCK_SPACE, BlockFCS(buff)) == false) {
		printf("Error:Error detection find a error.\n");
	}
}
static inline void StoreBlock(BlockIndex index, uint8_t* buff) {
	assert(index != -1);
	BlockFCS(buff) = Cal_XOR_ErrorDetection(buff + sizeof(Block), MAX_BLOCK_SPACE);
	WriteDisk(buff, ((uint64_t)index + Super.DataOffset) * Super.BlockSize, Super.BlockSize);
}
static inline void MakeDirBlock(FCBIndex firstIndex, uint8_t* buff) {
	((Block*)buff)->Size = sizeof(FCBIndex);
	*(FCBIndex*)(buff + sizeof(Block)) = firstIndex;
	for (size_t i = sizeof(Block) + sizeof(FCBIndex); i < Super.BlockSize; i += sizeof(FCBIndex)) {
		*(FCBIndex*)(buff + i) = -1;
	}
}


//Tool Class
class FilePointerReader {
public:
	FilePointerReader() { Pointer = (uint8_t*)malloc(Super.BlockSize); }
	~FilePointerReader() { free(Pointer); }

	void Load(FCBIndex index) {
		Self_Index = index;
		LoadFCB(index, &FCB);
		for (size_t i = 0; i < 10; i++) {
			BlockList.push_back(FCB.DirectBlock[i]);
		}
		if (FCB.Pointer != -1) {
			LoadBlock(FCB.Pointer, Pointer);
			for (auto iter = (BlockIndex*)(Pointer + sizeof(Block)); iter < (BlockIndex*)(Pointer + Super.BlockSize); iter++) {
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
			return *((BlockIndex*)(Pointer + sizeof(Block)) + (index - 10));
		}
	}
	//增加新的块
	BlockIndex AddNewBlock(uint8_t* blockBuff) {
		BlockIndex blockIndex = -1;
		for (int i = 0; i < 10; i++) {
			if (FCB.DirectBlock[i] == -1) {
				blockIndex = GetEmptyBlock();
				DataBitMap->set(blockIndex, true);
				StoreBlock(blockIndex, blockBuff); //写入数据块
				FCB.DirectBlock[i] = blockIndex;   //更新FCB
				//goto AppendNewBlock_end;
				WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte); //写入DataBlock的bitmap
				StoreFCB(Self_Index, &FCB);
				return blockIndex;
			}
		}
		//前10个块没存下
		if (FCB.Pointer == -1) {
			Pointer = (uint8_t*)malloc(Super.BlockSize);

			//制作空的Pointer
			for (BlockIndex* iter = (BlockIndex*)(Pointer + sizeof(Block)); iter < (BlockIndex*)(Pointer + Super.BlockSize); ++iter) {
				*iter = -1;
			}
			//写入Pointer块
			blockIndex = GetEmptyBlock();
			DataBitMap->set(blockIndex, true);
			StoreBlock(blockIndex, Pointer);
			//更新 FCB
			FCB.Pointer = blockIndex;
		}
		else {
			LoadBlock(FCB.Pointer, Pointer);
		}
		for (BlockIndex* iter = (BlockIndex*)(Pointer + sizeof(Block)); iter < (BlockIndex*)(Pointer + Super.BlockSize); ++iter) {
			if (*iter == -1) {
				blockIndex = GetEmptyBlock();
				DataBitMap->set(blockIndex, true);
				StoreBlock(blockIndex, blockBuff); //写入数据块
				*iter = blockIndex;
				StoreBlock(FCB.Pointer, Pointer); //更新pointer块
				//goto AppendNewBlock_end;
				WriteDisk(DataBitMap->data, (uint64_t)Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte); //写入DataBlock的bitmap
				StoreFCB(Self_Index, &FCB);
				return blockIndex;
			}
		}
		printf("ERROR:Append Block Failed! No Pointer available.\n");
		return -1;

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
			auto blockId = *((BlockIndex*)(Pointer + sizeof(Block)) + (index - 10));
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
	uint8_t* Pointer = nullptr;
	FCBIndex Self_Index = -1;
	vector<BlockIndex> BlockList;
};

//Functions
bool LoadDisk() {
	ReadDisk((uint8_t*)&Super, 0, sizeof(Super));
	if (strcmp(Super.Version, VERSION_STRING) == 0) {
		FCBBitMap = new BiSet(Super.FCBNum);        //初始化FCB的bitmap
		DataBitMap = new BiSet(Super.DataBlockNum); //初始化Data的bitmap
		ReadDisk(FCBBitMap->data, (uint64_t)Super.FCBBitmapOffset * Super.BlockSize, FCBBitMap->SizeOfByte);
		ReadDisk(DataBitMap->data, (uint64_t)Super.DataBitmapOffset * Super.BlockSize, DataBitMap->SizeOfByte);
		return true;
	}
	else {
		printf("Error:Disk Read Failed! Format the disk first\n");
		return false;
	}
}

void FormatDisk(uint32_t blocksize, uint32_t FCBBlockNum) {
	auto size = getDiskSize();

	//构建Super块
	Super.DiskSize = size;
	Super.BlockSize = blocksize;
	Super.BlockNum = Super.DiskSize / Super.BlockSize;
	strcpy(Super.Version, VERSION_STRING);
	if (FCBBlockNum == 0) {
		FCBBlockNum = (Super.BlockNum / 4) / (Super.BlockSize / FILE_CONTROL_BLOCK_SIZE);
	}

	Super.FCBNum = FCBBlockNum * blocksize / FILE_CONTROL_BLOCK_SIZE;

	uint32_t FCBBitmapBlock = (uint32_t)ceil(Super.FCBNum / (8.0 * blocksize));         // FCB位示图所占Blcok数
	uint32_t RemainBlock = Super.BlockNum - 1 - FCBBitmapBlock - FCBBlockNum;           //剩余的block数 = 总block数 - super - FCB位示图 - [Data位示图] - FCBBlockNum - [Data]
	uint32_t DataBitmapBlock = (uint32_t)ceil(RemainBlock * 1.0 / ((uint64_t)1 + 8 * blocksize)); //位示图占x个block，data区有`blocksize`*8*x个DataBlock。所以x=Remian/(1+8*blocksize)
	Super.DataBlockNum = RemainBlock - DataBitmapBlock;
	Super.FCBBitmapOffset = 1;
	Super.DataBitmapOffset = 1 + FCBBitmapBlock;
	Super.FCBOffset = Super.DataBitmapOffset + DataBitmapBlock;
	Super.DataOffset = Super.FCBOffset + FCBBlockNum;
	uint8_t* blockBuff = (uint8_t*)calloc(Super.BlockSize, sizeof(uint8_t));
	memcpy(blockBuff, &Super, sizeof(Super));
	//StoreBlock(0, blockBuff);															//写入SuperBlock
	//Super不能使用StoreBlock写入，需要手动写入
	BlockFCS(blockBuff) = Cal_XOR_ErrorDetection(blockBuff + sizeof(Block), MAX_BLOCK_SPACE);
	WriteDisk(blockBuff, 0, Super.BlockSize);


	free(blockBuff);

	FCBBitMap = new BiSet(Super.FCBNum);        //初始化FCB的bitmap
	DataBitMap = new BiSet(Super.DataBlockNum); //初始化Data的bitmap



	//空目录不需要数据块

	//构建根目录FCB
	FileControlBlock root(FileType::Directory, "Root", Access::All, 0);
	time(&root.CreateTime);

	root.Size = 0;
	FCBBitMap->set(0, true); //设置DataBlock的bitmap
	// WriteDisk((uint8_t*)&root, Super.BlockSize * (Super.FCBOffset + 0), sizeof(root));
	StoreFCB(0, &root); // root的FCB.id = 0

	WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);    //写入FCB的bitmap
	WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte); //写入DataBlock的bitmap
}

void PrintDiskInfo() {
	printf("Disk:\n");
	printf("    Disk Size     = %8.2lf MByte\n", Super.DiskSize * 1.0 / ((uint64_t)1 << 20));
	printf("    Block Size    = %8d Bytes\n", Super.BlockSize);
	printf("    Max File Size = %8.2lf KByte\n", ((((uint64_t)10 + (Super.BlockSize - sizeof(Block)) / sizeof(BlockIndex)) * Super.BlockSize)) * 1.0 / 1024);
	printf("Blocks:\n");
	printf("    Super         = %8d Blocks\n", 1);
	printf("    FCB Bitmap    = %8d Blocks\n", Super.DataBitmapOffset - Super.FCBBitmapOffset);
	printf("    Data Bitmap   = %8d Blocks\n", Super.FCBOffset - Super.DataBitmapOffset);
	printf("    FCB           = %8d Blocks\n", Super.DataOffset - Super.FCBOffset);
	printf("    Data          = %8d Blocks\n", Super.DataBlockNum);
	printf("    Total         = %8d Blocks\n", Super.BlockNum);
	printf("Usage:\n");
	uint32_t freeFCB = 0, freeBlock = 0, usedFCB = 0, usedBlock = 0;
	for (size_t i = 0; i < FCBBitMap->SizeOfBool; i++) {
		if (FCBBitMap->get(i) == false) {
			freeFCB++;
		}
		else {
			usedFCB++;
		}
	}
	for (size_t i = 0; i < DataBitMap->SizeOfBool; i++) {
		if (DataBitMap->get(i) == false) {
			freeBlock++;
		}
		else {
			usedBlock++;
		}
	}
	printf("    FCB Total     = %8d pics\n", freeFCB + usedFCB);
	printf("        Available = %8d pics\n", freeFCB);
	printf("        Used      = %8d pics\n", usedFCB);
	printf("    Block Total   = %8d Blocks\n", freeBlock + usedBlock);
	printf("        Available = %8d Blocks\n", freeBlock);
	printf("        Used      = %8d Blocks\n", usedBlock);
}

void PrintDir(FCBIndex dir) {
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSize);
	BlockIndex block = 0;
	//printf("%12s | %12s | %6s | % 10s\n", "Name", "Size", "FCB", "Physical Address");
	FileControlBlock DirFCB, childs;
	LoadFCB(dir, &DirFCB);
	for (size_t i = 0; i < 10 && DirFCB.DirectBlock[i] != -1; i++) {
		LoadBlock(DirFCB.DirectBlock[i], blockBuff);
		for (FCBIndex* iter = (FCBIndex*)(blockBuff + sizeof(Block)); iter < (FCBIndex*)(blockBuff + Super.BlockSize); iter++) {
			if (*iter == -1) {
				continue;
			}
			LoadFCB(*iter, &childs);
			if (childs.Type == FileType::Directory) {
				printf("[%d] %-12s  %18s  @ ", *iter, childs.Name, "<Dir>");
			}
			else {
				printf("[%d] %-12s  %12d Bytes  @ ", *iter, childs.Name, childs.Size);
			}
			for (int i = 0; i < 10 && childs.DirectBlock[i] != -1; i++) {
				printf("0x%X ", childs.DirectBlock[i]);
			}
			if (childs.Pointer != -1) {
				printf("...");
			}
			printf("\n");
		}
	}
	free(blockBuff);
}

void PrintInfo(FCBIndex file) {
	FileControlBlock fcb;
	LoadFCB(file, &fcb);
	printf("Name      : %s\n", fcb.Name);
	printf("Access    : ");
	if (fcb.AccessMode & Access::Read) {
		printf("r");
	}
	if (fcb.AccessMode & Access::Write) {
		printf("w");
	}
	if (fcb.AccessMode & Access::Delete) {
		printf("d");
	}
	printf("\n");

	if (fcb.Type == FileType::Directory) {
		printf("Type      : Directory\n");
	}
	else {
		printf("Type      : File\n");
		printf("Size      : %d Bytes\n", fcb.Size);
	}
	printf("Create    : %s\n", ctime(&fcb.CreateTime));
	printf("Read      : %s\n", ctime(&fcb.ReadTime));
	printf("Modify    : %s\n", ctime(&fcb.ModifyTime));
	printf("Physical Address : \n");
	FilePointerReader fpr;
	fpr.Load(file);
	for (uint32_t i = 0; i < MAX_POINTER; ++i) {
		if (fpr.GetBlockIndex(i) != -1) {
			printf("0x%X ", fpr.GetBlockIndex(i));
		}
		else {
			break;
		}
	}
	printf("\n");
}

FCBIndex CreateDirectory(const string& name, FCBIndex dir) { return Create(name, dir, FileType::Directory); }

FCBIndex CreateFile(const string& name, FCBIndex dir) { return Create(name, dir, FileType::File); }





FCBIndex Find(FCBIndex dir, const string& filename) {
	FileControlBlock fcb, result;
	LoadFCB(dir, &fcb);
	if (fcb.Type != FileType::Directory) {
		return -1;
	}
	else if (filename == "..") {
		return fcb.Parent;
	}
	else if (filename == ".") {
		return dir;
	}
	else {
		uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSize);
		for (size_t i = 0; i < 10; i++) {
			if (fcb.DirectBlock[i] != -1) {
				LoadBlock(fcb.DirectBlock[i], blockBuff);
				for (size_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
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

vector<FCBIndex> GetChildren(FCBIndex dir) {
	vector<FCBIndex> ret;
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSize);
	BlockIndex block = 0;
	FileControlBlock DirFCB, childs;
	LoadFCB(dir, &DirFCB);
	for (size_t i = 0; i < 10 && DirFCB.DirectBlock[i] != -1; i++) {
		LoadBlock(DirFCB.DirectBlock[i], blockBuff);
		for (FCBIndex* iter = (FCBIndex*)(blockBuff + sizeof(Block)); iter < (FCBIndex*)(blockBuff + Super.BlockSize); iter++) {
			if (*iter == -1) {
				continue;
			}
			ret.push_back(*iter);
		}
	}
PrintDir_end:
	free(blockBuff);
	return ret;
}

FCBIndex Create(const string& name, FCBIndex dir, enum FileType t) {
	if (name.length() > 30) {
		printf("Directory Name Too long!\n");
		return -1;
	}
	uint8_t* BlockBuff = (uint8_t*)malloc(Super.BlockSize);

	//创建新的FCB
	FileControlBlock  dirFCB;
	LoadFCB(dir, &dirFCB);
	if ((dirFCB.AccessMode & Access::Write) == false) {
		printf("Error:Access denied! Directory is not writeable.\n");
		return -1;
	}

	FileControlBlock newFileFCB(t, name.c_str(), Access::Read | Access::Write | Access::Delete, dir);
	newFileFCB.CreateTime = time(NULL);
	FCBIndex fcbIndex = GetEmptyFCB();



	//检查重名并写入父目录(将fcbIndex写入父目录)


	FileControlBlock NameCheck;
	for (size_t i = 0; i < 10; i++) {
		if (dirFCB.DirectBlock[i] == -1) { //存入新的目录页
			//真正写入FCB
			FCBBitMap->set(fcbIndex, true);
			StoreFCB(fcbIndex, &newFileFCB);

			MakeDirBlock(fcbIndex, BlockBuff);
			BlockIndex newDirPage = GetEmptyBlock();
			DataBitMap->set(newDirPage, true);
			dirFCB.DirectBlock[i] = newDirPage;
			StoreBlock(newDirPage, BlockBuff);
			goto CreateFile_end;
		}
		else { //尝试加入现有目录页，顺便检查重名
			LoadBlock(dirFCB.DirectBlock[i], BlockBuff);
			for (FCBIndex* iter = (FCBIndex*)(BlockBuff + sizeof(Block)); iter < (FCBIndex*)(BlockBuff + Super.BlockSize); iter++) {
				if (*iter == -1) {
					//真正写入FCB
					FCBBitMap->set(fcbIndex, true);
					StoreFCB(fcbIndex, &newFileFCB);

					*iter = fcbIndex;
					BlockSize(BlockBuff) += sizeof(FCBIndex);
					StoreBlock(dirFCB.DirectBlock[i], BlockBuff);
					goto CreateFile_end;
				}
				else {
					LoadFCB(*iter, &NameCheck);
					if (strcmp(NameCheck.Name, name.c_str()) == 0) {
						printf("Error:File name duplicate ,%s is already existed!\n", NameCheck.Name);
						free(BlockBuff);
						return -1;
					}
				}
			}
		}
	}

CreateFile_end:
	dirFCB.ModifyTime = time(NULL);
	dirFCB.ReadTime = time(NULL);
	StoreFCB(dir, &dirFCB);
	WriteDisk(FCBBitMap->data, (uint64_t)Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);    //写入FCB的bitmap
	WriteDisk(DataBitMap->data, (uint64_t)Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte); //写入DataBlock的bitmap
	free(BlockBuff);
	return fcbIndex;
}

void FileInfo(FCBIndex file, FileControlBlock* fcb)
{
	LoadFCB(file, fcb);
}

int64_t ReadFile(FCBIndex file, int64_t pos, int64_t len, uint8_t* buff) {
	FileControlBlock ThisFileFCB;
	LoadFCB(file, &ThisFileFCB);
	if (pos + len > ThisFileFCB.Size) {
		printf("Error:pos+len = %lld is larger than file's size = %lu.\n", pos + len, ThisFileFCB.Size);
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
	else if ((ThisFileFCB.AccessMode & Access::Read) == false) {
		printf("Error:Access denied! File is not readable.\n");
		return -1;
	}
	uint64_t inBlockPos = pos;
	uint64_t BytesToBeRead = len;
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSize);

	FilePointerReader pointerBlock;
	pointerBlock.Load(file);
	size_t PageNum;
	for (PageNum = 0; PageNum < MAX_POINTER; PageNum++) {
		BlockIndex block = pointerBlock.ReadPage(PageNum, blockBuff);

		if (inBlockPos < MAX_BLOCK_SPACE) { //从此块开始读
			goto ReadFile_StartToRead;
		}
		else { //读取起点还在后面
			inBlockPos -= BlockSize(blockBuff);
		}
	}

ReadFile_StartToRead:
	//从文件的第PageNum块inBlockOffset字节开始，读取BytesToBeRead字节的数据

	for (; BytesToBeRead > 0; ++PageNum) {
		BlockIndex block = pointerBlock.ReadPage(PageNum, blockBuff);
		if (BytesToBeRead >= BlockSize(blockBuff)) { //读完整块
			memcpy(buff + (len - BytesToBeRead), blockBuff + sizeof(Block), BlockSize(blockBuff));
			BytesToBeRead -= BlockSize(blockBuff);
		}
		else { //读取一部分
			memcpy(buff + (len - BytesToBeRead), blockBuff + sizeof(Block), BytesToBeRead);
			BytesToBeRead -= BytesToBeRead;
		}
	}

ReadFile_end:
	free(blockBuff);
	pointerBlock.FCB.ReadTime = time(NULL);
	StoreFCB(file, &pointerBlock.FCB);
	return pos + len;
}

int64_t WriteFile(FCBIndex file, int64_t pos, int64_t len, const uint8_t* buff) {
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
	else if ((ThisFileFCB.AccessMode & Access::Write) == false) {
		printf("Error:Access denied! File is not writeable.\n");
		return -1;
	}
	uint64_t inBlockPos = pos;
	uint64_t StartPos = 0;
	uint8_t* blockBuff = (uint8_t*)malloc(Super.BlockSize);

	FilePointerReader PointerBlock;
	PointerBlock.Load(file);

	PointerBlock.FCB.Size = MAX(PointerBlock.FCB.Size, pos + len);

	for (size_t PageNum = 0; PageNum < MAX_POINTER; PageNum++) {
		BlockIndex block = PointerBlock.ReadPage(PageNum, blockBuff);
		if (block == -1) { //写入起点是新的块
			goto WriteFile_CreateNew;
		}
		else if (inBlockPos < MAX_BLOCK_SPACE) {     //写入起点是当前块
			if (inBlockPos + len <= MAX_BLOCK_SPACE) { //在空隙中存下
				memcpy(blockBuff + inBlockPos, buff, len);
				BlockSize(blockBuff) = MAX(BlockSize(blockBuff), inBlockPos + len);
				StoreBlock(block, blockBuff);
				goto WriteFile_end;
			}
			else { //存不下->填满空隙
				memcpy(blockBuff + inBlockPos, buff, MAX_BLOCK_SPACE - inBlockPos);
				BlockSize(blockBuff) = MAX_BLOCK_SPACE;
				StoreBlock(block, blockBuff);
				StartPos += MAX_BLOCK_SPACE - inBlockPos; //写了MAX_BLOCK_SPACE - inBlockPos个字节
				goto WriteFile_CreateNew;
			}

		}
		else { //写入起点还在后面
			inBlockPos -= BlockSize(blockBuff);
		}
	}

WriteFile_CreateNew:
	//将buff从StartPos ~ len之间的部分写入新块

	while (StartPos < len) {
		if (len - StartPos >= MAX_BLOCK_SPACE) { //写满本块
			// pointerBlock.FCB.Size += MAX_BLOCK_SPACE;
			BlockSize(blockBuff) = MAX_BLOCK_SPACE;
			memcpy(blockBuff + sizeof(Block), buff + StartPos, MAX_BLOCK_SPACE);
			PointerBlock.AddNewBlock(blockBuff);
			StartPos += MAX_BLOCK_SPACE;
		}
		else { //还剩空间
		 // pointerBlock.FCB.Size += (len - StartPos);
			BlockSize(blockBuff) = len;
			memcpy(blockBuff + sizeof(Block), buff + StartPos, len - StartPos);
			PointerBlock.AddNewBlock(blockBuff);
			StartPos += (len - StartPos);
		}
	}

WriteFile_end:
	free(blockBuff);
	PointerBlock.FCB.ModifyTime = time(NULL);
	StoreFCB(file, &PointerBlock.FCB);
	return pos + len;
}

bool DeleteFile(FCBIndex file) {
	FileControlBlock thisFile;
	LoadFCB(file, &thisFile);
	if ((thisFile.AccessMode & Access::Delete) == false) {
		printf("Error:Access denied! File is not removable.\n");
		return false;
	}
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
		for (size_t i = 0; i < blocks.size(); i++) {
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
						DeleteFile(*(FCBIndex*)(blockBuff + pos));//删除文件夹内数据
					}
				}
				DataBitMap->set(thisFile.DirectBlock[i], false);//删除FCB
			}
		}
		FCBBitMap->set(file, false);//删除自身FCB
	}
	//删除父目录的记录
	FCBIndex parentIndex = thisFile.Parent;
	FileControlBlock parentFCB;
	LoadFCB(parentIndex, &parentFCB);
	for (size_t i = 0; i < 10 && parentFCB.DirectBlock[i] != -1; i++) {
		LoadBlock(parentFCB.DirectBlock[i], blockBuff);
		for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
			if (*(FCBIndex*)(blockBuff + pos) == file) {
				*(FCBIndex*)(blockBuff + pos) = -1;
				StoreBlock(parentFCB.DirectBlock[i], blockBuff);
				// TEST ： 如果该目录块为空，则删除此目录块
				for (uint64_t pos2 = sizeof(Block); pos2 < Super.BlockSize; pos2 += sizeof(FCBIndex)) {
					if (*(FCBIndex*)(blockBuff + pos2) != -1) {
						goto DeleteFile_end;
					}
				}
				//删除空的目录表
				DataBitMap->set(parentFCB.DirectBlock[i], false);
				parentFCB.DirectBlock[i] = -1;
				// DirectBlock紧凑化
				for (int j = i + 1; j < 10; j++) {
					parentFCB.DirectBlock[j - 1] = parentFCB.DirectBlock[j];
				}
				parentFCB.DirectBlock[9] = -1;
				goto DeleteFile_end;
			}
		}
	}

DeleteFile_end:
	WriteDisk(FCBBitMap->data, (uint64_t)Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);    //写入FCB的bitmap
	WriteDisk(DataBitMap->data, (uint64_t)Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte); //写入DataBlock的bitmap

	free(blockBuff);
	return false;
}

bool RenameFile(const string& newName, FCBIndex file)
{
	if (file == -1) {
		return false;
	}
	FileControlBlock fcb;
	LoadFCB(file, &fcb);
	strcpy(fcb.Name, newName.c_str());
	memset(fcb.Name + newName.length() + 1, 0, sizeof(FileControlBlock::Name) - newName.length() - 1);
	StoreFCB(file, &fcb);
	return true;
}

bool ChangeAccessMode(FCBIndex file, uint8_t newMode)
{
	FileControlBlock fileFCB;
	LoadFCB(file, &fileFCB);
	fileFCB.AccessMode = newMode;
	StoreFCB(file, &fileFCB);
	return true;
}
