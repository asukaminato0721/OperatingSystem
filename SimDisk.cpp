#define _CRT_SECURE_NO_WARNINGS

#include <bits/stdc++.h>
#include <iostream>

#include "Driver.h"
#include "FileSystem.h"
using namespace std;

#define TestSize 3000
uint8_t buff[TestSize];
char input[1 << 20];
FCBIndex workDir = 0;


int __main() {
	initDisk();
	if (LoadDisk() == false) {
		FormatDisk(4 * 1 << 10);
	}


	PrintDiskInfo();
	while (true)
	{
		cout << "Input : ";
		cin.getline(input, 1024);
		//cout << "in = " << input;


		if (memcmp(input, "exit", 4) == 0) {
			DisMount();
			return 0;
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
			auto  newWD = Find(workDir, newdir);
			FileControlBlock fcb;
			FileInfo(newWD, &fcb);

			if (newWD == -1) {
				cout << "No such Dir" << endl;
				continue;
			}
			else if (fcb.Type == FileType::File) {
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
			uint8_t* inFileBuff = (uint8_t*)malloc(fcb.Size);
			ReadFile(fcbID, 0, fcb.Size, inFileBuff);
			inFileBuff[fcb.Size + 1] = 0;
			cout << inFileBuff << endl;
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
			}if (input[nameEnd + 3] == 'd') {
				acc = acc | Access::Delete;
			}


			ChangeAccessMode(Find(workDir, fileName), acc);
			cout << endl;
		}

	}
}
