#pragma once
#include <cstdint>
#include <string>


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
* FCBS 1024Block
*
*/

class BiSet
{
public:
	BiSet(uint32_t size);
	bool get(uint32_t index);
	void set(uint32_t index, bool val);
	~BiSet();
	uint32_t SizeOfBool;
	uint32_t SizeOfByte;
	uint8_t* data = nullptr;
};

enum class Access :uint8_t
{
	None = 0,
	Read = 1 << 0,
	Write = 1 << 1,
	Delete = 1 << 2,
};

enum class FileType :uint8_t
{
	File, Directory
};
//�����飨��¼�̿���Ϣ��
struct SuperBlock
{
	uint16_t FCS = 0;								//ѭ��У����

	uint32_t DiskSize;								//���̴�С
	uint32_t BlockSize;								//���С
	uint32_t BlockNum;								//������(=DISK_SIZE/���С)
	BlockIndex FCBBitmapOffset;						//FCB��λʾͼ�б�ʼ���
	BlockIndex DataBitmapOffset;					//Data��λʾͼ���ݿ鿪ʼ���
	BlockIndex FCBOffset;					     	//FCB�б�ʼ���
	BlockIndex DataOffset;							//Data���ݿ鿪ʼ���
	uint32_t FCBNum, DataBlockNum;				    //FCB������DataBlock����
};

//FCB��bitmap
extern BiSet* FCBBitMap;
//Data���ݿ��bitmap
extern BiSet* DataBitMap;

struct Block {
	uint16_t FCS = 0;					//ѭ��У����
	uint16_t Size;						//������Ч���ݵĳ���
	uint8_t data[];

	//PointerBlock : BlockIndex[]
	//Data		   : uint8_t[]
	//Dir		   : FCBIndex[]
};

//�ļ����ƶΣ�һ��Block����Դ�Ŷ��FCB��
struct FileControlBlock {
	FileControlBlock operator=(FileControlBlock& val);
	FileControlBlock();
	FileControlBlock(FileType t, char* name, uint8_t AccessMode);


	char Name[32];
	enum FileType Type;
	uint32_t Size;
	time_t CreateTime, ModifyTime, ReadTime;
	uint8_t AccessMode;
	BlockIndex DirectBlock[10] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
	BlockIndex Pointer = -1;

	uint8_t __padding[14];
};


void FormatDisk(uint32_t blocksize = 1 << 10, uint32_t FCBBlockNum = 0);
void Login(string userName, string password);
void PrintInfo();
void PrintDir(FCBIndex dir);
bool CreateFile(string name, FCBIndex parent);
bool DeleteFile();
bool CreateDirectory(string name, FCBIndex parent);

