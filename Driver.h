#pragma once
#include <cstdint>
#include <fstream>

using namespace std;

bool initDisk();
void DisMountDisk();


bool ReadDisk(uint8_t *buff, uint64_t pos, uint32_t len);
bool WriteDisk(uint8_t *buff, uint64_t pos, uint32_t len);
uint32_t getDiskSize();
