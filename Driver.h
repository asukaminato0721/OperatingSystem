#pragma once

#include <bits/stdc++.h>
using namespace std;

bool initDisk();
bool ReadDisk(uint8_t* buff, uint64_t pos, uint32_t len);
bool WriteDisk(uint8_t* buff, uint64_t pos, uint32_t len);
uint32_t getDiskSize();
