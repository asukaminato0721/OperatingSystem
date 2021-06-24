#include "Driver.h"

//uint8_t *Disk;
fstream Disk;
#define Size (200 * 1 << 20) 





bool initDisk() {
	Disk.open("./SimDisk.disk", ios::in | ios::out | ios::binary);
	return true;
	//Disk = (uint8_t *)malloc(sizeof(uint8_t) * Size);
	//if (Disk == NULL) {
	//    return false;
	//}
	//return true;
}

bool ReadDisk(uint8_t* buff, uint64_t pos, uint32_t len) {
	Disk.seekp(pos);
	Disk.read((char*)buff, len);
	return true;
	//memcpy(buff, Disk + pos, len);
	//return true;
}

bool WriteDisk(uint8_t* buff, uint64_t pos, uint32_t len) {
	Disk.seekg(pos);
	Disk.write((char*)buff, len);
	return true;
	//if (pos + len > Size) {
	//	cout << "Driver Error!\nPos=" << pos << ",len=" << len << "Not Writeable!" << endl;
	//	return false;
	//}
	//memcpy(Disk + pos, buff, len);
	//return true;
}

uint32_t getDiskSize() { return Size; }