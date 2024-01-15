#pragma once
#include <fstream>
#include <zlib.h>
#include <bzlib.h>
#include <iostream>
#include <assert.h>
#include "base64.hpp"
#include "adc.hpp"
#include "convert.hpp"
#include <zip.h>

#define CHUNKSIZE 0x100000
#define DECODESIZE 0x100000

struct _Kolyblck {
    uint32_t Signature, Version, HeaderSize, Flags; 
    uint64_t RunningDataForkOffset, DataForkOffset, DataForkLength, RsrcForkOffset, RsrcForkLength;
    uint32_t SegmentNumber, SegmentCount, SegmentID1, SegmentID2, segmentID3, SegmentID4, DataForkChecksumType, Reserved1, DataForkChecksum, Reserved2;
    char Reserved3[120];
    uint64_t XMLOffset, XMLLength; 
    char Reserved4[120];
    uint32_t MasterChecksumType, Reserved5, MasterChecksum, Reserved6;
    char Reserved7[120];
    uint32_t ImageVariant;
    uint64_t SectorCount;
    char Reserved8[12];
}; 

struct _mishblk {
	uint32_t Signature, Version;
	uint64_t FirstSectorNumber, SectorCount;
	uint64_t DataStart;
	uint32_t DecompressedBufferRequested, BlocksDescriptor;
	char Reserved1[24];
	uint32_t ChecksumType, Reserved2, Checksum, Reserved3;
	char Reserved4[120];
	uint32_t BlocksRunCount;
	char *Data;
}; 

_mishblk parseMISHBLOCK(_mishblk input); 
_Kolyblck parseKOLYBLOCK(_Kolyblck input); 
int readDMG(FILE* File, FILE* Output); 