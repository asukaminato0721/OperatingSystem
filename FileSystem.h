#pragma once
#include <cstdint>
#include <string>
#include <vector>


#define FILE_CONTROL_BLOCK_SIZE 128u
#define MAX_POINTER ((Super.BlockSize - sizeof(Block)) / sizeof(BlockIndex) + 10)
#define MAX_BLOCK_SPACE (Super.BlockSize - sizeof(Block))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define VERSION_STRING "SimDisk V1.0"

using namespace std;

typedef int32_t BlockIndex;
typedef int32_t FCBIndex;

/*
 * |--------------------------------Disk-------------------------------|
 * |---Super Block---|--Bitmap--|---FCBS---|---------Data Block--------|
 *
 * [FCBS] : |FileControlBlock1|FileControlBlock2|FileControlBlock3|...
 *
 *
 * super Block 1 Block
 * Bitmap
 * FCBS
 *
 */



class Access {
public:
	static uint8_t  None;
	static uint8_t	Read;
	static uint8_t	Write;
	static uint8_t	Delete;
	static uint8_t	All;
};


enum class FileType : uint8_t { File, Directory };
//超级块（记录盘块信息）
struct SuperBlock {
	uint16_t FCS = 0;               //循环校验码
	char Version[16];				//文件系统版本号
	uint32_t DiskSize;				//磁盘大小
	uint32_t BlockSize;             //块大小
	uint32_t BlockNum;              //块数量(=DISK_SIZE/块大小)
	BlockIndex FCBBitmapOffset;     // FCB的位示图列表开始块号
	BlockIndex DataBitmapOffset;    // Data的位示图数据块开始块号
	BlockIndex FCBOffset;           // FCB列表开始块号
	BlockIndex DataOffset;          // Data数据块开始块号
	uint32_t FCBNum, DataBlockNum;  // FCB数量和DataBlock数量
};



struct Block {
	uint16_t FCS = 0;  //循环校验码
	uint16_t Size = 0; //块内有效数据的长度

	// PointerBlock : BlockIndex[]
	// Data		   : uint8_t[]
	// Dir		   : FCBIndex[]
};


//文件控制段（一个Block里可以存放多个FCB）
struct FileControlBlock {
	FileControlBlock();
	FileControlBlock(enum FileType t, const char* name, uint8_t AccessMode, FCBIndex parent);

	char Name[32];
	enum FileType Type;
	uint32_t Size;
	time_t CreateTime, ModifyTime, ReadTime;
	uint8_t AccessMode;
	FCBIndex Parent;
	BlockIndex DirectBlock[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	BlockIndex Pointer = -1;

	uint8_t __padding[6];
};

//// FCB的bitmap
//extern BiSet* FCBBitMap;
//// Data数据块的bitmap
//extern BiSet* DataBitMap;
//超级块信息
extern SuperBlock Super;


bool LoadDisk();
void FormatDisk(uint32_t blocksize = 1 << 10, uint32_t FCBBlockNum = 0);
void PrintDiskInfo();


FCBIndex CreateDirectory(const string& name, FCBIndex parent);
FCBIndex CreateFile(const string& name, FCBIndex dir);


void PrintDir(FCBIndex dir);
void PrintInfo(FCBIndex file);


FCBIndex Create(const string& name, FCBIndex dir, enum FileType t);
int64_t ReadFile(FCBIndex file, int64_t pos, int64_t len, uint8_t* buff);
int64_t WriteFile(FCBIndex file, int64_t pos, int64_t len, const uint8_t* buff);
bool DeleteFile(FCBIndex file);
bool RenameFile(const string& newName, FCBIndex file);
bool ChangeAccessMode(FCBIndex file, uint8_t newMode);

FCBIndex Find(FCBIndex dir, const string& filename);
void FileInfo(FCBIndex file, FileControlBlock* fcb);
vector<FCBIndex> GetChildren(FCBIndex dir);