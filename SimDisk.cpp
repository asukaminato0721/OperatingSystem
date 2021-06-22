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
	PrintInfo();
	PrintDir(0);
	CreateDirectory("Alice", 0);
	CreateDirectory("Bob", 0);
	CreateDirectory("Jec", 0);
	CreateDirectory("Jec", 0);
	for (size_t i = 0; i < 2000; i++)
	{
		CreateDirectory((string("Test") + to_string(i)).c_str(), 1);
	}
	PrintDir(0);
	PrintDir(1);

	std::cout << "Hello World!\n";


}
