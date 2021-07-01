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
//�����飨��¼�̿���Ϣ��
struct SuperBlock {
	uint16_t FCS = 0;              //ѭ��У����
	char Version[16];              //�ļ�ϵͳ�汾��
	uint32_t DiskSize;             //���̴�С
	uint32_t BlockSize;            //���С
	uint32_t BlockNum;             //������(=DISK_SIZE/���С)
	BlockIndex FCBBitmapOffset;    // FCB��λʾͼ�б�ʼ���
	BlockIndex DataBitmapOffset;   // Data��λʾͼ���ݿ鿪ʼ���
	BlockIndex FCBOffset;          // FCB�б�ʼ���
	BlockIndex DataOffset;         // Data���ݿ鿪ʼ���
	uint32_t FCBNum, DataBlockNum; // FCB������DataBlock����
};

struct Block {
	uint16_t FCS = 0;  //ѭ��У����
	uint16_t Size = 0; //������Ч���ݵĳ���
					   // PointerBlock    : BlockIndex[]
					   // Data		       : uint8_t[]
					   // Dir		       : FCBIndex[]
};

//�ļ����ƶΣ�һ��Block����Դ�Ŷ��FCB��
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