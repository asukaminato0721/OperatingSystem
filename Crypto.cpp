#include "Crypto.h"
#define CRC_16_POLYNOMIALS            0x8005

uint16_t Cal_XOR_ErrorDetection(uint8_t* data, uint32_t lenOfBytes) {
	register uint16_t result = 0x00;
	for (size_t i = 0; i < lenOfBytes; i += sizeof(uint32_t))
	{
		result = result ^ *(uint32_t*)(data + i);
	}
	return result;
};

bool Chk_XOR_ErrorDetection(uint8_t* data, uint16_t lenOfBytes, uint16_t FCS) {

	return (Cal_XOR_ErrorDetection(data, lenOfBytes) == FCS);
};
