#include "Driver.h"

fstream Disk;
#define Size (200 * 1 << 20) 

bool initDisk() {
	Disk.open("./SimDisk.disk", ios::in | ios::out | ios::binary);
	if (Disk.is_open() == false) {
		Disk.open("./SimDisk.disk", ios::out | ios::binary);
		char init[1 << 10];
		for (size_t i = 0; i < Size / (1 << 10); i ++)
		{
			Disk.write(init, (1 << 10));
		}
		Disk.close();
		Disk.open("./SimDisk.disk", ios::in | ios::out | ios::binary);
	}
	return true;
}

bool ReadDisk(uint8_t* buff, uint64_t pos, uint32_t len) {
	Disk.seekp(pos);
	Disk.read((char*)buff, len);
	return true;
}

bool WriteDisk(uint8_t* buff, uint64_t pos, uint32_t len) {
	Disk.seekg(pos);
	Disk.write((char*)buff, len);
	return true;
}

void DisMountDisk() {
	Disk.close();
}

uint32_t getDiskSize() { return Size; }