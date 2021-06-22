#define _CRT_SECURE_NO_WARNINGS


#include <iostream>
#include <bits/stdc++.h>

#include "Driver.h"
#include "FileSystem.h"
using namespace std;

int main()
{
	initDisk();
	FormatDisk(1024);
	auto dir1 = CreateDirectory("Dir1", 0);
	auto dir2 = CreateDirectory("Dir2", 0);
	auto dir3 = CreateDirectory("Dir3", dir2);
	auto dir4 = CreateDirectory("Dir4", dir3);
	CreateFile("Data1", 0);
	auto data2 = CreateFile("Data2", dir1);

	//for (size_t i = 0; i < 5000; i++)
	//{
	//	CreateDirectory((string("Test") + to_string(i)).c_str(), 1);
	//}

	PrintInfo();
	printf("\n\n");
	PrintDir(0);
	PrintDir(dir2);
	PrintDir(dir3);
	PrintDir(dir4);

	DeleteFile(data2);


	PrintDir(0);
	printf("\n\n");

	PrintInfo();
	printf("\n\n");


	std::cout << "Hello World!\n";


}
