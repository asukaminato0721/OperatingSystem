#define _CRT_SECURE_NO_WARNINGS

#include <bits/stdc++.h>
#include <iostream>

#include "Driver.h"
#include "FileSystem.h"
using namespace std;

#define TestSize 3000

int main() {
	uint8_t testBuff[TestSize];
	uint8_t testBuff2[TestSize];
	ifstream fs("C:\\Users\\Azura\\Desktop\\VNS\\VNS - PURE.cpp");
	fs.read((char*)testBuff, 10);
	fs.read((char*)testBuff, TestSize);


	initDisk();
	FormatDisk(1024);
	auto dir1 = CreateDirectory("Dir1", 0);
	auto dir2 = Create("Dir2", 0, FileType::Directory);
	auto file1 = Create("File1", 0, FileType::File);
	auto file2 = Create("File1", 0, FileType::File);

	WriteFile(file1, 0, 2000, testBuff);


	ReadFile(file1, 0, 2000, testBuff2);

	PrintDir(0);
	PrintDiskInfo();
	PrintFileInfo(file1);
	PrintFileInfo(0);

}
