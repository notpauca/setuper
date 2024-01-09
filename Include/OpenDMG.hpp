#pragma once
#include <fstream>
#include <zlib.h>
#include <bzlib.h>
#include <iostream>
#include <assert.h>
#include "base64.hpp"
#include "adc.hpp"
#include "convert.hpp"

#define CHUNKSIZE 0x100000
#define DECODESIZE 0x100000

#define bool short
#define true 1
#define false 0

struct _Kolyblck {
    uint32_t Signature;
    uint32_t Version;
    uint32_t HeaderSize;
    uint32_t Flags;
    uint64_t RunningDataForkOffset;
    uint64_t DataForkOffset;
    uint64_t DataForkLength;
    uint64_t RsrcForkOffset;
    uint64_t RsrcForkLength;
    uint32_t SegmentNumber;
    uint32_t SegmentCount;
    uint32_t SegmentID1;
    uint32_t SegmentID2;
    uint32_t SegmentID3;
    uint32_t SegmentID4;
    uint32_t DataForkChecksumType;
    uint32_t Reserved1;
    uint32_t DataForkChecksum;
    uint32_t Reserved2;
    char Reserved3[120];
    uint64_t XMLOffset;
    uint64_t XMLLength;
    char Reserved4[120];
    uint32_t MasterChecksumType;
    uint32_t Reserved5;
    uint32_t MasterChecksum;
    uint32_t Reserved6;
    char Reserved7[120];
    uint32_t ImageVariant;
    uint64_t SectorCount;
    char Reserved8[12];
}; 

struct _mishblk {
	uint32_t BlocksSignature;
	uint32_t InfoVersion;
	uint64_t FirstSectorNumber;
	uint64_t SectorCount;
	uint64_t DataStart;
	uint32_t DecompressedBufferRequested;
	uint32_t BlocksDescriptor;
	char Reserved1[24];
	uint32_t ChecksumType;
	uint32_t Reserved2;
	uint32_t Checksum;
	uint32_t Reserved3;
	char Reserved4[120];
	uint32_t BlocksRunCount;
	char *Data;
}; 

_mishblk parseMISHBLOCK(_mishblk input); 
_Kolyblck parseKOLYBLOCK(_Kolyblck input); 
int readDMG(FILE* File, FILE* Output); 