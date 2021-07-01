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

class Access {
public:
	static constexpr uint8_t None = 0;
	static constexpr uint8_t Read = 1 << 1;
	static constexpr uint8_t Write = 1 << 2;
	static constexpr uint8_t Delete = 1 << 3;
	static constexpr uint8_t All = 0xFF;
};

enum class FileType : uint8_t { File, Directory };
//超级块（记录盘块信息）
struct SuperBlock {
	uint16_t FCS = 0;              //循环校验码
	char Version[16];              //文件系统版本号
	uint32_t DiskSize;             //磁盘大小
	uint32_t BlockSize;            //块大小
	uint32_t BlockNum;             //块数量(=DISK_SIZE/块大小)
	BlockIndex FCBBitmapOffset;    // FCB的位示图列表开始块号
	BlockIndex DataBitmapOffset;   // Data的位示图数据块开始块号
	BlockIndex FCBOffset;          // FCB列表开始块号
	BlockIndex DataOffset;         // Data数据块开始块号
	uint32_t FCBNum, DataBlockNum; // FCB数量和DataBlock数量
};

struct Block {
	uint16_t FCS = 0;  //循环校验码
	uint16_t Size = 0; //块内有效数据的长度
					   // PointerBlock    : BlockIndex[]
					   // Data		       : uint8_t[]
					   // Dir		       : FCBIndex[]
};

//文件控制段（一个Block里可以存放多个FCB）
struct FileControlBlock {
	FileControlBlock();
	FileControlBlock(enum FileType t, const char* name, uint8_t AccessMode, FCBIndex parent);

	char Name[32];
	enum FileType Type;
	uint32_t Size;
	time_t CreateTime = 0, ModifyTime = 0, ReadTime = 0;
	uint8_t AccessMode;
	FCBIndex Parent;
	BlockIndex DirectBlock[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	BlockIndex Pointer = -1;

	uint8_t __padding[6];
};

void FormatDisk(uint32_t blocksize = 1 << 10, uint32_t FCBBlockNum = 0);

bool fs_init();
bool fs_Destruction();

bool CheckDisk();

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