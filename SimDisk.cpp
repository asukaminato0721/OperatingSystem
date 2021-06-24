#define _CRT_SECURE_NO_WARNINGS

#include <bits/stdc++.h>
#include <iostream>

#include "Driver.h"
#include "FileSystem.h"
using namespace std;

inline uint16_t &xxx(uint8_t *blockBuff) { return ((Block *)blockBuff)->Size; }

int main() {
    initDisk();
    FormatDisk(1024);
    auto dir1 = CreateDirectory("Dir1", 0);
    auto dir2 = CreateDirectory("Dir2", 0);
    auto dir3 = CreateDirectory("Dir3", dir2);
    auto dir4 = CreateDirectory("Dir4", dir3);
    CreateFile("File1", 0);
    CreateFile("File2", 0);
    CreateFile("File3", 0);
    CreateFile("File4", 0);
    CreateFile("File5", 0);
    CreateFile("File6", 0);
    CreateFile("File7", 0);
    CreateFile("File8", 0);
    auto data2 = CreateFile("Data2", dir1);

    // for (size_t i = 0; i < 5000; i++)
    //{
    //	CreateDirectory((string("Test") + to_string(i)).c_str(), 1);
    //}
    uint8_t *buff = (uint8_t *)malloc(2000);

    PrintDiskInfo();
    printf("\n\n");
    PrintDir(0);

    DeleteFile(data2);

    PrintDir(0);
    PrintDir(dir1);
    printf("\n\n");

    PrintDiskInfo();
    printf("\n\n");
    printf("\n\n");
    printf("\n\n");
    printf("\n\n");
    printf("\n\n");
    printf("\n\n");
    auto file = CreateFile("TestFile", 0);
    PrintFileInfo(file);

    WriteFile(file, 0, 2000, (uint8_t *)"hello world!");
    WriteFile(file, 1010, 20, (uint8_t *)"hello world!");
    PrintFileInfo(file);

    ReadFile(file, 1010, 20, buff);
    printf("########################\n%s\n########################\n", buff);

    // printf("pos = %lld\n", WriteFile(file, 0, 1021, (uint8_t*)"hello world!"));
    // WriteFile(file, 0, 128, (uint8_t*)"hello world!");
    PrintFileInfo(file);

    PrintDir(0);
    PrintDiskInfo();
}
