#pragma once
#include <cstdint>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <iostream>
#include <vector>

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
	BlockIndex FCBBitmapOffset;						//FCB��λʾͼ�б���ʼ���
	BlockIndex DataBitmapOffset;					//Data��λʾͼ���ݿ鿪ʼ���
	BlockIndex FCBOffset;					     	//FCB�б���ʼ���
	BlockIndex DataOffset;							//Data���ݿ鿪ʼ���
	uint32_t FCBNum, DataBlockNum;				    //FCB������DataBlock����
};

//FCB��bitmap
extern BiSet* FCBBitMap;
//Data���ݿ��bitmap
extern BiSet* DataBitMap;

struct Block {
	uint16_t FCS = 0;					//ѭ��У����
	uint16_t Size = 0;					//������Ч���ݵĳ���

	//PointerBlock : BlockIndex[]
	//Data		   : uint8_t[]
	//Dir		   : FCBIndex[]
};

struct DataBlock :Block
{
	uint8_t data[];
};
struct PointerBlock :Block
{
	BlockIndex data[];
};
struct DirBlock :Block
{
	FCBIndex data[];
};

//�ļ����ƶΣ�һ��Block����Դ�Ŷ��FCB��
struct FileControlBlock {
	FileControlBlock operator=(FileControlBlock& val);
	FileControlBlock();
	FileControlBlock(enum FileType t, const char* name, uint8_t AccessMode, FCBIndex parent);


	char Name[32];
	enum FileType Type;
	uint32_t Size;
	time_t CreateTime, ModifyTime, ReadTime;
	uint8_t AccessMode;
	FCBIndex Parent;
	BlockIndex DirectBlock[10] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
	BlockIndex Pointer = -1;

	uint8_t __padding[6];
};


void Login(string userName, string password);



void Load();


void PrintDiskInfo();

void FormatDisk(uint32_t blocksize = 1 << 10, uint32_t FCBBlockNum = 0);

FCBIndex CreateDirectory(string name, FCBIndex parent);
FCBIndex CreateFile(string name, FCBIndex dir);
bool DeleteFile(FCBIndex file);
void PrintDir(FCBIndex dir);
void PrintFileInfo(FCBIndex file);


/// <summary>
/// ��ѯĳĿ¼�µ��ļ�FCB��
/// </summary>
/// <param name="dir">������Ŀ¼</param>
/// <param name="filename">�����ҵ��ļ���</param>
/// <returns>���ҵ���FCB�ţ����δ�鵽��dir����Ŀ¼���򷵻�-1</returns>
FCBIndex Find(FCBIndex dir, string filename);


/// <summary>
/// ��ȡ�ļ�
/// </summary>
/// <param name="file">�ļ�FCB���</param>
/// <param name="pos">��ȡ��ʼ��</param>
/// <param name="buff">������</param>
/// <param name="len">��ȡ����</param>
/// <returns>��������ȡλ�õ���һλ</returns>
uint64_t ReadFile(FCBIndex file, uint64_t pos, uint64_t len, uint8_t* buff);

/// <summary>
/// д���ļ�
/// </summary>
/// <param name="file">�ļ�FCB���</param>
/// <param name="pos">д����ʼ��</param>
/// <param name="buff">������</param>
/// <param name="len">д�볤��</param>
/// <returns>�������д��λ�õ���һλ</returns>
uint64_t WriteFile(FCBIndex file, uint64_t pos, uint64_t len, uint8_t* buff);



uint64_t FileInfo(FCBIndex file, FileControlBlock* fcb);