#pragma once
#include <fstream>
#include <zlib.h>
#include <bzlib.h>
#include <lzfse.h>
#include <iostream>
#include <assert.h>
#include "base64.hpp"
#include "adc.hpp"
#include "convert.hpp"


#define CHUNKSIZE 0x100000
#define DECODESIZE 0x100000

#define BLOCKCHUNKENTRYSIZE 0x28

enum MountType {
	none,
	hfs, 
	apfs
};

struct _Kolyblck {
	uint32_t Signature, Version, HeaderSize, Flags; 
	uint64_t RunningDataForkOffset, DataForkOffset, DataForkLength, RsrcForkOffset, RsrcForkLength;
	uint32_t SegmentNumber, SegmentCount, SegmentID[4], DataForkChecksumType, DataForkChecksumBits, DataForkChecksum[32];
	uint64_t XMLOffset, XMLLength; 
	char Reserved1[120];
	uint32_t MasterChecksumType, MasterChecksumBits, MasterChecksum[32];
	uint32_t ImageVariant;
	uint64_t SectorCount;
	uint32_t Reserved2[3];
}; 

struct _mishblk {
	uint32_t Signature, Version;
	uint64_t FirstSectorNumber, SectorCount;
	uint64_t DataStart;
	uint32_t DecompressedBufferRequested, BlocksDescriptor;
	char Reserved[24];
	uint32_t ChecksumType, ChecksumBits, Checksum[32];
	uint32_t BlocksRunCount;
	char* Data;
}; 

struct _mishblk_data {
	uint32_t EntryType;         // Compression type used or entry type (see next table)
	uint32_t Comment;           // "+beg" or "+end", if EntryType is comment (0x7FFFFFFE). Else reserved.
	uint64_t SectorNumber;      // Start sector of this chunk
	uint64_t SectorCount;       // Number of sectors in this chunk
	uint64_t CompressedOffset;  // Start of chunk in data fork
	uint64_t CompressedLength;  
};

#define zlib 0x80000005
#define bzlib2 0x80000006
#define lzfse 0x80000007
#define adc 0x80000004
#define uncompressed 0x00000001
#define ignore_1 0x00000000
#define ignore_2 0x00000002
#define last 0xFFFFFFFF 

_mishblk parseMISHBLOCK(_mishblk input); 
_Kolyblck parseKOLYBLOCK(_Kolyblck input); 
_mishblk_data parseSectorInfo(char* input); 

int readDMG(FILE* File, FILE* Output, MountType &type);  