#define _CRT_SECURE_NO_WARNINGS
#include "FileSystem.h"
#include "Driver.h"
using namespace std;

// TODO �� ɾ���ļ�ʱ����Ŀ¼��Ŀ¼�ļ����ջ�

BiSet::BiSet(uint32_t size) {
    this->SizeOfBool = size;
    this->SizeOfByte = (int)ceil(size / 8);
    if (size % 8 != 0) {
        this->SizeOfByte++;
    }
    this->data = (uint8_t *)calloc(this->SizeOfByte, sizeof(uint8_t));
}
bool BiSet::get(uint32_t index) { return bool(data[index / 8] & (1 << (index % 8))); }
void BiSet::set(uint32_t index, bool val) {
    if (val == true) {
        this->data[index / 8] = this->data[index / 8] | (1 << (index % 8));
    } else {
        uint8_t mask = ~(uint8_t)(1 << (index % 8));
        this->data[index / 8] = this->data[index / 8] & mask;
    }
}
BiSet::~BiSet() { free(this->data); }

FileControlBlock FileControlBlock::operator=(FileControlBlock &val) {
    memcpy(this, &val, sizeof(FileControlBlock));
    return *this;
}
FileControlBlock::FileControlBlock() {}
FileControlBlock::FileControlBlock(enum FileType t, const char *name, uint8_t AccessMode, FCBIndex parent) {
    strcpy(Name, name);
    this->Type = t;
    time(&this->CreateTime);
    this->Size = 0;
    this->Parent = parent;
}

SuperBlock Super;

BiSet *FCBBitMap, *DataBitMap;
bool isMounted = false;

BlockIndex getEmptyBlock() {
    if (isMounted == false) {
        printf("Mount a disk first.\n");
        return -1;
    } else {
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
}

FCBIndex getEmptyFCB() {
    if (isMounted == false) {
        printf("Mount a disk first.\n");
        return -1;
    } else {
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
}

void LoadFCB(FCBIndex index, FileControlBlock *buff) {
    ReadDisk((uint8_t *)buff, Super.FCBOffset * Super.BlockSize + index * FILE_CONTROL_BLOCK_SIZE, sizeof(FileControlBlock));
    return;
}
void StoreFCB(FCBIndex index, FileControlBlock *buff) {
    WriteDisk((uint8_t *)buff, Super.FCBOffset * Super.BlockSize + index * FILE_CONTROL_BLOCK_SIZE, sizeof(FileControlBlock));
    return;
}
void LoadBlock(BlockIndex index, uint8_t *buff) { ReadDisk(buff, (index + Super.DataOffset) * Super.BlockSize, Super.BlockSize); }
void StoreBlock(BlockIndex index, uint8_t *buff) { WriteDisk(buff, (index + Super.DataOffset) * Super.BlockSize, Super.BlockSize); }

void MakeDirBlock(FCBIndex firstIndex, uint8_t *buff) {
    ((Block *)buff)->Size = sizeof(FCBIndex);
    *(FCBIndex *)(buff + sizeof(Block)) = firstIndex;
    for (size_t i = sizeof(Block) + sizeof(FCBIndex); i < Super.BlockSize; i += sizeof(FCBIndex)) {
        *(FCBIndex *)(buff + i) = -1;
    }
}
inline uint16_t &BlockSize(uint8_t *blockBuff) { return ((Block *)blockBuff)->Size; }
inline uint16_t &BlockFCS(uint8_t *blockBuff) { return ((Block *)blockBuff)->FCS; }

class FilePointerReader {
  public:
    FilePointerReader() { Pointer = (uint8_t *)malloc(Super.BlockSize); }
    ~FilePointerReader() { free(Pointer); }

    void Load(FCBIndex index) {
        LoadFCB(index, &FCB);
        for (size_t i = 0; i < 10; i++) {
            BlockList.push_back(FCB.DirectBlock[i]);
        }
        if (FCB.Pointer != -1) {
            LoadBlock(FCB.Pointer, Pointer);
            for (auto iter = (BlockIndex *)(Pointer + sizeof(Block)); iter < (BlockIndex *)(Pointer + Super.BlockSize); iter++) {
                BlockList.push_back(*iter);
            }
        }
    }
    //��ȡ���ļ��ĵ�index����
    BlockIndex GetBlockIndex(uint32_t index) {
        if (index < 10) {
            return FCB.DirectBlock[index];
        } else if (FCB.Pointer == -1) {
            return -1;
        } else {
            return *((BlockIndex *)(Pointer + sizeof(Block)) + (index % 10));
        }
    }
    //�����µĿ�
    BlockIndex AddNewBlock(uint8_t *blockBuff) {
        BlockIndex blockIndex = -1;
        for (int i = 0; i < 10; i++) {
            if (FCB.DirectBlock[i] == -1) {
                blockIndex = getEmptyBlock();
                DataBitMap->set(blockIndex, true);
                StoreBlock(blockIndex, blockBuff); //д�����ݿ�
                FCB.DirectBlock[i] = blockIndex;   //����FCB
                goto AppendNewBlock_end;
            }
        }
        if (FCB.Pointer != -1) {
            uint8_t *pointerBuff = (uint8_t *)malloc(Super.BlockSize);
            LoadBlock(FCB.Pointer, pointerBuff);
            for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(BlockIndex)) {
                if (*(BlockIndex *)(pointerBuff + pos) == -1) {
                    blockIndex = getEmptyBlock();
                    DataBitMap->set(blockIndex, true);
                    StoreBlock(blockIndex, blockBuff); //д�����ݿ�
                    *(BlockIndex *)(pointerBuff + pos) = blockIndex;
                    StoreBlock(FCB.Pointer, pointerBuff); //����pointer��
                    free(pointerBuff);
                    goto AppendNewBlock_end;
                }
            }
            printf("ERROR:Append Block Failed! No Pointer remain.\n");
            return blockIndex;
        }
    AppendNewBlock_end:
        WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte); //д��DataBlock��bitmap
        StoreFCB(Self_Index, &FCB);
        return true;
    }
    //����indexҳ�����ڴ�
    BlockIndex ReadPage(uint32_t index, uint8_t *blockBuff) {
        if (index < 10) {
            if (FCB.DirectBlock[index] != -1) {
                LoadBlock(FCB.DirectBlock[index], blockBuff);
                return FCB.DirectBlock[index];
            } else {
                return -1;
            }
        } else {
            auto blockId = *((BlockIndex *)(Pointer + sizeof(Block)) + (index % 10));
            if (blockId != -1) {
                LoadBlock(blockId, blockBuff);
                return blockId;
            } else {
                return -1;
            }
        }
    }

    FileControlBlock FCB;
    uint8_t *Pointer;
    FCBIndex Self_Index = -1;
    vector<BlockIndex> BlockList;
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

    uint32_t FCBBitmapBlock = (uint32_t)ceil(Super.FCBNum / (8.0 * blocksize));         // FCBλʾͼ��ռBlcok��
    uint32_t RemainBlock = Super.BlockNum - 1 - FCBBitmapBlock - FCBBlockNum;           //ʣ���block�� = ��block�� - super - FCBλʾͼ - [Dataλʾͼ] - FCBBlockNum - [Data]
    uint32_t DataBitmapBlock = (uint32_t)ceil(RemainBlock * 1.0 / (1 + 8 * blocksize)); //λʾͼռx��block��data����`blocksize`*8*x��DataBlock������x=Remian/(1+8*blocksize)
    Super.DataBlockNum = RemainBlock - DataBitmapBlock;
    Super.FCBBitmapOffset = 1;
    Super.DataBitmapOffset = 1 + FCBBitmapBlock;
    Super.FCBOffset = Super.DataBitmapOffset + DataBitmapBlock;
    Super.DataOffset = Super.FCBOffset + FCBBlockNum;
    WriteDisk((uint8_t *)&Super, 0, sizeof(Super)); //д��SuperBlock

    FCBBitMap = new BiSet(Super.FCBNum);        //��ʼ��FCB��bitmap
    DataBitMap = new BiSet(Super.DataBlockNum); //��ʼ��Data��bitmap

    isMounted = true;

    //��Ŀ¼����Ҫ���ݿ�
    ////������Ŀ¼���ݿ�
    // uint8_t* DirDataBlock = (uint8_t*)malloc(Super.BlockSize);
    // MakeDirBlock(0, DirDataBlock);
    // BlockIndex StorgeLocation = getEmptyBlock();
    // DataBitMap->set(StorgeLocation, true);																	//����FCB��bitmap
    // StoreBlock(StorgeLocation, DirDataBlock);																//д��DirectoryBlock
    // free(DirDataBlock);

    //������Ŀ¼FCB
    FileControlBlock root(FileType::Directory, "Root", (uint8_t)Access::None, 0);
    strcpy(root.Name, "Root");
    root.AccessMode = (uint8_t)Access::None;
    time(&root.CreateTime);

    root.Size = 0;
    //��Ŀ¼�������ݿ�
    // root.DirectBlock[0] = StorgeLocation;																	//���ô洢���?
    FCBBitMap->set(0, true); //����DataBlock��bitmap
    // WriteDisk((uint8_t*)&root, Super.BlockSize * (Super.FCBOffset + 0), sizeof(root));
    StoreFCB(0, &root); // root��FCB.id = 0

    WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);    //д��FCB��bitmap
    WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte); //д��DataBlock��bitmap
}

void Login(string userName, string password) {}

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
    for (size_t i = 0; i < FCBBitMap->SizeOfBool; i++) {
        if (FCBBitMap->get(i) == false) {
            freeFCB++;
        } else {
            usedFCB++;
        }
    }
    for (size_t i = 0; i < DataBitMap->SizeOfBool; i++) {
        if (DataBitMap->get(i) == false) {
            freeBlock++;
        } else {
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
    uint8_t *blockBuff = (uint8_t *)malloc(Super.BlockSize);
    BlockIndex block = 0;
    printf("================================================\n");
    printf("%12s | %12s | %6s | % 10s\n", "Name", "Size", "FCB", "Physical Address");
    FileControlBlock DirFCB, childs;
    LoadFCB(dir, &DirFCB);
    for (size_t i = 0; i < 10 && DirFCB.DirectBlock[i] != -1; i++) {
        LoadBlock(DirFCB.DirectBlock[i], blockBuff);
        for (FCBIndex *iter = (FCBIndex *)(blockBuff + sizeof(Block)); iter < (FCBIndex *)(blockBuff + Super.BlockSize); iter++) {
            if (*iter == -1) {
                continue;
            }
            LoadFCB(*iter, &childs);
            if (DirFCB.Type == FileType::Directory) {
                printf("%12s | %12s | %6d | ", childs.Name, "<Dir>", *iter);
            } else {
                printf("%12s | %12d | %6d | ", childs.Name, childs.Size, *iter);
            }
            for (int i = 0; i < 10 && childs.DirectBlock[i] != -1; i++) {
                printf("%X ", childs.DirectBlock[i]);
            }
            printf("\n");
        }
    }
PrintDir_end:
    printf("================================================\n\n");
    free(blockBuff);
}

void PrintFileInfo(FCBIndex file) {
    FileControlBlock fcb;
    LoadFCB(file, &fcb);
    printf("================================================\n");
    printf("Name        : %s\n", fcb.Name);
    if (fcb.Type == FileType::Directory) {
        printf("Type        : Directory\n");
    } else {
        printf("Type        : File\n");
        printf("Size        : %d Bytes\n", fcb.Size);
    }
    printf("Create Time : %s\n", ctime(&fcb.CreateTime));
    printf("Read Time   : %s\n", ctime(&fcb.ReadTime));
    printf("Modify Time : %s\n", ctime(&fcb.ModifyTime));
    printf("Parent      : %d\n", fcb.Parent);
    printf("================================================\n");
}

FCBIndex Find(FCBIndex dir, string filename) {
    FileControlBlock fcb, result;
    LoadFCB(dir, &fcb);
    if (fcb.Type == FileType::File) {
        return -1;
    } else {
        uint8_t *blockBuff = (uint8_t *)malloc(Super.BlockSize);
        for (size_t i = 0; i < 10; i++) {
            if (fcb.DirectBlock[i] != -1) {
                LoadBlock(fcb.DirectBlock[i], blockBuff);
                for (int pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
                    FCBIndex fcbIndex = *(FCBIndex *)(blockBuff + pos);
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

void GetFCB(FCBIndex file, FileControlBlock *fcb) { LoadFCB(file, fcb); }

FCBIndex Create(string name, FCBIndex dir, enum FileType t) {
    if (name.length() > 30) {
        printf("Directory Name Too long!\n");
        return -1;
    }
    uint8_t *BlockBuff = (uint8_t *)malloc(Super.BlockSize);
    //����Data��!���ļ�������Data��

    //����FCB��
    FileControlBlock fcb(t, name.c_str(), (uint8_t)Access::Read | (uint8_t)Access::Write | (uint8_t)Access::Delete, dir);
    // FCB.DirectBlock[0] = blockIndex;
    FCBIndex fcbIndex = getEmptyFCB();
    FCBBitMap->set(fcbIndex, true);
    StoreFCB(fcbIndex, &fcb);
    //д�븸Ŀ¼(��fcbIndexд�븸Ŀ¼)
    FileControlBlock dirFCB, NameCheck;
    LoadFCB(dir, &dirFCB);
    for (size_t i = 0; i < 10; i++) {
        if (dirFCB.DirectBlock[i] == -1) { //�����µ�Ŀ¼ҳ
            MakeDirBlock(fcbIndex, BlockBuff);
            BlockIndex newDirPage = getEmptyBlock();
            DataBitMap->set(newDirPage, true);
            dirFCB.DirectBlock[i] = newDirPage;
            StoreBlock(newDirPage, BlockBuff);
            goto CreateFile_end;
        } else { //���Լ�������Ŀ¼ҳ��˳��������
            LoadBlock(dirFCB.DirectBlock[i], BlockBuff);
            for (FCBIndex *iter = (FCBIndex *)(BlockBuff + sizeof(Block)); iter < (FCBIndex *)(BlockBuff + Super.BlockSize); iter++) {
                if (*iter == -1) {
                    *iter = fcbIndex;
                    BlockSize(BlockBuff) += sizeof(FCBIndex);
                    StoreBlock(dirFCB.DirectBlock[i], BlockBuff);
                    goto CreateFile_end;
                } else {
                    LoadFCB(*iter, &NameCheck);
                    if (strcmp(NameCheck.Name, name.c_str()) == 0) {
                        printf("Error:File name duplicate ,%s is already existed!\n", NameCheck.Name);
                        goto CreateFile_end;
                    }
                }
            }
        }
    }

CreateFile_end:
    StoreFCB(dir, &dirFCB);
    WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);    //д��FCB��bitmap
    WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte); //д��DataBlock��bitmap
    free(BlockBuff);
    return fcbIndex;
}

vector<FCBIndex> GetChildren(FCBIndex dir) {
    vector<FCBIndex> ret;
    uint8_t *blockBuff = (uint8_t *)malloc(Super.BlockSize);
    BlockIndex block = 0;
    FileControlBlock DirFCB, childs;
    LoadFCB(dir, &DirFCB);
    for (size_t i = 0; i < 10 && DirFCB.DirectBlock[i] != -1; i++) {
        LoadBlock(DirFCB.DirectBlock[i], blockBuff);
        for (FCBIndex *iter = (FCBIndex *)(blockBuff + sizeof(Block)); iter < (FCBIndex *)(blockBuff + Super.BlockSize); iter++) {
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

FCBIndex CreateDirectory(string name, FCBIndex dir) { return Create(name, dir, FileType::Directory); }

FCBIndex CreateFile(string name, FCBIndex dir) { return Create(name, dir, FileType::File); }

bool DeleteFile(FCBIndex file) {
    FileControlBlock thisFile;
    LoadFCB(file, &thisFile);
    uint8_t *blockBuff = (uint8_t *)malloc(Super.BlockSize);
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
                if (*(BlockIndex *)(blockBuff + pos) != -1) {
                    blocks.push_back(*(BlockIndex *)(blockBuff + pos));
                }
            }
        }
        //ɾ���ļ����ݿ�
        for (size_t i = 0; i < blocks.size(); i++) {
            DataBitMap->set(blocks[i], false);
        }
        //ɾ��FCB
        FCBBitMap->set(file, false);
    } else if (thisFile.Type == FileType::Directory) {
        for (int i = 0; i <= 10; ++i) {
            if (thisFile.DirectBlock[i] != -1) {
                LoadBlock(thisFile.DirectBlock[i], blockBuff);
                for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
                    if (*(FCBIndex *)(blockBuff + pos) != -1) {
                        DeleteFile(*(FCBIndex *)(blockBuff + pos));
                    }
                }
            }
        }
    }
    //ɾ����Ŀ¼�ļ�¼
    FCBIndex parentIndex = thisFile.Parent;
    FileControlBlock parentFCB;
    LoadFCB(parentIndex, &parentFCB);
    for (size_t i = 0; i < 10 && parentFCB.DirectBlock[i] != -1; i++) {
        LoadBlock(parentFCB.DirectBlock[i], blockBuff);
        for (uint64_t pos = sizeof(Block); pos < Super.BlockSize; pos += sizeof(FCBIndex)) {
            if (*(FCBIndex *)(blockBuff + pos) == file) {
                *(FCBIndex *)(blockBuff + pos) = -1;
                StoreBlock(parentFCB.DirectBlock[i], blockBuff);
                // TEST �� �����Ŀ¼���?�գ���ɾ����Ŀ¼��
                for (uint64_t pos2 = sizeof(Block); pos2 < Super.BlockSize; pos2 += sizeof(FCBIndex)) {
                    if (*(FCBIndex *)(blockBuff + pos2) != -1) {
                        goto DeleteFile_end;
                    }
                }
                //ɾ���յ�Ŀ¼��
                DataBitMap->set(parentFCB.DirectBlock[i], false);
                parentFCB.DirectBlock[i] = -1;
                // DirectBlock���ջ�
                for (int j = i + 1; j < 10; j++) {
                    parentFCB.DirectBlock[j - 1] = parentFCB.DirectBlock[j];
                }
                parentFCB.DirectBlock[9] = -1;
                goto DeleteFile_end;
            }
        }
    }

DeleteFile_end:
    WriteDisk(FCBBitMap->data, Super.BlockSize * Super.FCBBitmapOffset, FCBBitMap->SizeOfByte);    //д��FCB��bitmap
    WriteDisk(DataBitMap->data, Super.BlockSize * Super.DataBitmapOffset, DataBitMap->SizeOfByte); //д��DataBlock��bitmap

    free(blockBuff);
    return false;
}

int64_t WriteFile(FCBIndex file, int64_t pos, int64_t len, uint8_t *buff) {
    FileControlBlock ThisFileFCB;
    LoadFCB(file, &ThisFileFCB);
    if (pos > ThisFileFCB.Size) {
        printf("Error:pos = %lld is larger than file's size = %lu.\n", pos, ThisFileFCB.Size);
        return -1;
    } else if (pos < 0) {
        printf("Error:pos = %lld should larger than or equal to 0.\n", pos);
        return -1;
    } else if (len < 0) {
        printf("Error:len = %lld should larger than or equal to 0.\n", len);
        return -1;
    }
    uint64_t inBlockPos = pos;
    uint64_t StartPos = 0;
    uint8_t *blockBuff = (uint8_t *)malloc(Super.BlockSize);

    FilePointerReader PointerBlock;
    PointerBlock.Load(file);

    PointerBlock.FCB.Size = MAX(PointerBlock.FCB.Size, pos + len);

    for (size_t PageNum = 0; PageNum < MAX_POINTER; PageNum++) {
        BlockIndex block = PointerBlock.ReadPage(PageNum, blockBuff);
        if (block == -1) { //д��������µĿ�?
            goto WriteFile_CreateNew;
        } else if (inBlockPos < MAX_BLOCK_SPACE) {     //д������ǵ�ǰ��?
            if (inBlockPos + len <= MAX_BLOCK_SPACE) { //�ڿ�϶�д���
                memcpy(blockBuff + inBlockPos, buff, len);
                BlockSize(blockBuff) = MAX(BlockSize(blockBuff), inBlockPos + len);
                StoreBlock(block, blockBuff);
                goto WriteFile_end;
            } else { //�治��->������϶
                memcpy(blockBuff + inBlockPos, buff, MAX_BLOCK_SPACE - inBlockPos);
                BlockSize(blockBuff) = MAX_BLOCK_SPACE;
                StoreBlock(block, blockBuff);
                StartPos += MAX_BLOCK_SPACE - inBlockPos; //д��MAX_BLOCK_SPACE - inBlockPos���ֽ�
                goto WriteFile_CreateNew;
            }

        } else { //д����㻹�ں���?
            inBlockPos -= BlockSize(blockBuff);
        }
    }

WriteFile_CreateNew:
    //��buff��StartPos ~ len֮��Ĳ���д���¿�?

    while (StartPos < len) {
        if (len - StartPos >= MAX_BLOCK_SPACE) { //д������
            // pointerBlock.FCB.Size += MAX_BLOCK_SPACE;
            BlockSize(blockBuff) = MAX_BLOCK_SPACE;
            memcpy(blockBuff + sizeof(Block), buff + StartPos, MAX_BLOCK_SPACE);
            PointerBlock.AddNewBlock(blockBuff);
            StartPos += MAX_BLOCK_SPACE;
        } else { //��ʣ�ռ�
            // pointerBlock.FCB.Size += (len - StartPos);
            BlockSize(blockBuff) = len;
            memcpy(blockBuff + sizeof(Block), buff + StartPos, len - StartPos);
            PointerBlock.AddNewBlock(blockBuff);
            StartPos += (len - StartPos);
        }
    }

WriteFile_end:
    free(blockBuff);
    StoreFCB(file, &PointerBlock.FCB);
    return pos + len;
}

int64_t ReadFile(FCBIndex file, int64_t pos, int64_t len, uint8_t *buff) {
    FileControlBlock ThisFileFCB;
    LoadFCB(file, &ThisFileFCB);
    if (pos + len > ThisFileFCB.Size) {
        printf("Error:pos+len = %ld is larger than file's size = %lu.\n", pos + len, ThisFileFCB.Size);
        return -1;
    } else if (pos < 0) {
        printf("Error:pos = %ld should larger than or equal to 0.\n", pos);
        return -1;
    } else if (len < 0) {
        printf("Error:len = %ld should larger than or equal to 0.\n", len);
        return -1;
    }
    uint64_t inBlockPos = pos;
    uint64_t BytesToBeRead = len;
    uint8_t *blockBuff = (uint8_t *)malloc(Super.BlockSize);

    FilePointerReader pointerBlock;
    pointerBlock.Load(file);
    size_t PageNum;
    for (PageNum = 0; PageNum < MAX_POINTER; PageNum++) {
        BlockIndex block = pointerBlock.ReadPage(PageNum, blockBuff);

        if (inBlockPos < MAX_BLOCK_SPACE) { //�Ӵ˿鿪ʼ��
            goto ReadFile_StartToRead;
        } else { //��ȡ��㻹�ں���?
            inBlockPos -= BlockSize(blockBuff);
        }
    }

ReadFile_StartToRead:
    //���ļ��ĵ�PageNum��inBlockOffset�ֽڿ�ʼ����ȡBytesToBeRead�ֽڵ�����

    for (; BytesToBeRead > 0; ++PageNum) {
        BlockIndex block = pointerBlock.ReadPage(PageNum, blockBuff);
        if (BytesToBeRead >= BlockSize(blockBuff)) { //��������
            memcpy(buff + (len - BytesToBeRead), blockBuff + sizeof(Block), BlockSize(blockBuff));
            BytesToBeRead -= BlockSize(blockBuff);
        } else { //��ȡһ����
            memcpy(buff + (len - BytesToBeRead), blockBuff + sizeof(Block), BytesToBeRead);
            BytesToBeRead -= BytesToBeRead;
        }
    }

ReadFile_end:
    free(blockBuff);
    StoreFCB(file, &pointerBlock.FCB);
    return pos + len;
}