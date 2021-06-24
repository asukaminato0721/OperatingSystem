#include "Driver.h"

uint8_t *Disk;
size_t Size = 200 * 1 << 20; // 30MByte

bool initDisk() {
    Disk = (uint8_t *)malloc(sizeof(uint8_t) * Size);
    if (Disk == NULL) {
        return false;
    }
    return true;
}

bool ReadDisk(uint8_t *buff, uint64_t pos, uint32_t len) {
    /*if (pos + len > Size) {
            cout << "Driver Error!\nPos=" << pos << ",len=" << len << "Not Readable!" << endl;
            return false;
    }*/
    memcpy(buff, Disk + pos, len);
    return true;
}

bool WriteDisk(uint8_t *buff, uint64_t pos, uint32_t len) {
    if (pos + len > Size) {
        cout << "Driver Error!\nPos=" << pos << ",len=" << len << "Not Writeable!" << endl;
        return false;
    }
    memcpy(Disk + pos, buff, len);
    return true;
}

uint32_t getDiskSize() { return Size; }