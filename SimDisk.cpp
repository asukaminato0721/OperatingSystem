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
	CreateDirectory("Dir1", 0);
	CreateDirectory("Dir2", 0);
	CreateDirectory("Dir3", 0);
	CreateFile("Data1", 0);
	CreateFile("Data2", 0);
	for (size_t i = 0; i < 5000; i++)
	{
		CreateDirectory((string("Test") + to_string(i)).c_str(), 1);
	}
	PrintDir(0);
	PrintInfo();

	std::cout << "Hello World!\n";


}
