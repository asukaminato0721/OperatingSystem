﻿#define _CRT_SECURE_NO_WARNINGS

#include <bits/stdc++.h>
#include <iostream>
#include "Driver.h"
#include "FileSystem.h"

using namespace std;

#define TestSize 3000
uint8_t buff[TestSize];
char input[1 << 20];
FCBIndex workDir = 0;



int main() {

	if (fs_init() == false) {
		cout << "Format Disk\n";
		FormatDisk(8 * 1 << 10);
	}
	PrintDiskInfo();
	//CheckDisk();


	//uint8_t buff[8 * 1 << 10];
	//for (size_t i = 0; i < (8 * 1 << 10); i++)
	//{
	//	buff[i] = rand();
	//}
	//time_t start, end;
	//start = clock();
	//for (size_t i = 0; i < 100; i++)
	//{
	//	Create("File_" + to_string(i), 0, FileType::File);
	//	auto Fileid = Find(0, "File_" + to_string(i));
	//	if (Fileid != -1) {
	//		WriteFile(Fileid, 0, rand() % (8 * 1 << 10), buff);
	//	}
	//	else {
	//		cout << "Find err" << endl;
	//	}
	//}
	//end = clock();
	//cout << end - start << "ms" << endl;






	while (true) {
		cout << "Input : ";
		cin.getline(input, 1024);
		// cout << "in = " << input;
		if (memcmp(input, "exit", 4) == 0) {
			fs_Destruction();
			return 0;
		}
		if (memcmp(input, "chkdsk", 4) == 0) {
			CheckDisk();
		}
		else if (memcmp(input, "stat", 4) == 0) {
			PrintDiskInfo();
		}
		else if (memcmp(input, "ls", 2) == 0) {
			PrintDir(workDir);
		}
		else if (memcmp(input, "cd", 2) == 0) {
			char newdir[128];
			strcpy(newdir, input + 3);
			auto newWD = Find(workDir, newdir);
			FileControlBlock fcb;

			if (newWD == -1) {
				cout << "No such Dir" << endl;
				continue;
			}
			FileInfo(newWD, &fcb);

			if (fcb.Type == FileType::File) {
				cout << newdir << " is not a Dir" << endl;
				continue;
			}
			else {
				workDir = newWD;
			}
		}
		else if (memcmp(input, "cat", 3) == 0) {
			char fileName[128];
			FileControlBlock fcb;
			strcpy(fileName, input + 4);
			auto fcbID = Find(workDir, fileName);
			if (fcbID == -1) {
				cout << "No such file" << endl;
				continue;
			}
			FileInfo(fcbID, &fcb);
			if (fcb.Type == FileType::Directory) {
				cout << fileName << " is not a file" << endl;
				continue;
			}
			uint8_t* inFileBuff = (uint8_t*)malloc(fcb.Size + 1);
			if (-1 == ReadFile(fcbID, 0, fcb.Size, inFileBuff)) {
				free(inFileBuff);
				continue;
			}
			inFileBuff[fcb.Size] = 0;
			bool isPrintable = true;
			for (size_t i = 0; i < fcb.Size; i++) {
				if ((inFileBuff[i] > 0x1f && inFileBuff[i] != 0x7f) == false && inFileBuff[i] != '\n' && inFileBuff[i] != '\r' && inFileBuff[i] != '\t' && inFileBuff[i] != '\v') {
					isPrintable = false;
					break;
				}
			}
			if (isPrintable == true) {
				cout << inFileBuff << endl;
			}
			else {
				for (size_t i = 0; i < fcb.Size; i++) {
					printf("%02X ", inFileBuff[i]);
					if ((i - 1) % 32 == 0) {
						cout << endl;
					}
				}
				cout << endl;
			}
			free(inFileBuff);

		}
		else if (memcmp(input, "touch", 5) == 0) {
			char fileName[128];
			strcpy(fileName, input + 6);
			auto fcbID = Create(fileName, workDir, FileType::File);
			cout << endl;
		}
		else if (memcmp(input, "mkdir", 5) == 0) {
			char fileName[128];
			strcpy(fileName, input + 6);
			auto fcbID = Create(fileName, workDir, FileType::Directory);
			cout << endl;
		}
		else if (memcmp(input, "rm", 2) == 0) {
			char fileName[128];
			strcpy(fileName, input + 3);
			auto fcbID = Find(workDir, fileName);
			if (fcbID == -1) {
				cout << "No such file" << endl;
				continue;
			}
			else if (fcbID == workDir) {
				FileControlBlock fcb;
				FileInfo(fcbID, &fcb);
				workDir = fcb.Parent;
			}
			DeleteFile(fcbID);

			cout << endl;
		}
		else if (memcmp(input, "infoid", 6) == 0) {
			FCBIndex file = atoi(input + 7);

			PrintInfo(file);
			cout << endl;
		}
		else if (memcmp(input, "info", 4) == 0) {
			char fileName[128];
			strcpy(fileName, input + 5);
			auto fcbID = Find(workDir, fileName);
			if (fcbID == -1) {
				cout << "No such file" << endl;
				continue;
			}
			PrintInfo(fcbID);
			cout << endl;
		}

		else if (memcmp(input, "in", 2) == 0) {
			char fileName[128];
			strcpy(fileName, input + 3);
			ifstream inFile(fileName, ios::binary);
			inFile.seekg(0, std::ios_base::end);
			uint64_t fileSize = inFile.tellg();
			inFile.seekg(0, std::ios_base::beg);
			uint8_t* inFileBuff = (uint8_t*)malloc(fileSize);
			inFile.read((char*)inFileBuff, fileSize);
			auto fileNameStr = string(fileName).substr(string(fileName).rfind('\\') + 1);
			auto newFile = Create(fileNameStr, workDir, FileType::File);
			WriteFile(newFile, 0, fileSize, inFileBuff);
			free(inFileBuff);
			inFile.close();
			cout << endl;
		}
		else if (memcmp(input, "out", 3) == 0) {
			char fileName[128];
			strcpy(fileName, input + 4);
			FileControlBlock fcb;
			auto file = Find(workDir, fileName);
			FileInfo(file, &fcb);
			uint8_t* oFileBuff = (uint8_t*)malloc(fcb.Size);
			ReadFile(file, 0, fcb.Size, oFileBuff);

			ofstream outFile(fileName, ios::binary);
			outFile.write((char*)oFileBuff, fcb.Size);

			free(oFileBuff);
			outFile.close();
			cout << endl;
		}
		else if (memcmp(input, "chmod", 5) == 0) {
			char fileName[128];
			int nameEnd = 6;
			while (input[nameEnd] != ' ') {
				nameEnd++;
			}
			memcpy(fileName, input + 6, nameEnd - 6);
			fileName[nameEnd - 6] = 0;
			uint8_t acc = 0;
			if (input[nameEnd + 1] == 'r') {
				acc = acc | Access::Read;
			}
			if (input[nameEnd + 2] == 'w') {
				acc = acc | Access::Write;
			}
			if (input[nameEnd + 3] == 'd') {
				acc = acc | Access::Delete;
			}
			ChangeAccessMode(Find(workDir, fileName), acc);
			cout << endl;
		}
	}
}
