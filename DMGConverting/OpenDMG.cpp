#include "OpenDMG.hpp"

_mishblk parseMISHBLOCK(_mishblk input) {
	// input.Signature = convert_int(input.Signature);
	// if (input.Signature != 0x6D697368) {errno = EINVAL;}
	if (input.Signature != 0x6873696D) {errno = EINVAL;}
	// input.Version = convert_int(input.Version);
	// input.FirstSectorNumber = convert_int64(input.FirstSectorNumber);
	// input.SectorCount = convert_int64(input.SectorCount);
	// input.DataStart = convert_int64(input.DataStart);
	// input.DecompressedBufferRequested = convert_int(input.DecompressedBufferRequested);
	// input.BlocksDescriptor = convert_int(input.BlocksDescriptor);
	// input.ChecksumType = convert_int(input.ChecksumType);
	// input.Checksum = convert_int(input.Checksum);
	input.BlocksRunCount = convert_int(input.BlocksRunCount);
	return input; 
}

_Kolyblck parseKOLYBLOCK(_Kolyblck input) {
	// input.Signature = convert_int(input.Signature);
	// if(input.Signature != 0x6B6F6C79) {errno = EINVAL; }
	if(input.Signature != 0x796C6F6B) {errno = EINVAL; }
	// input.Version = convert_int(input.Version);
	// input.HeaderSize = convert_int(input.HeaderSize);
	// input.Flags = convert_int(input.Flags);
	// input.RunningDataForkOffset = convert_int64(input.RunningDataForkOffset);
	input.DataForkOffset = convert_int64(input.DataForkOffset);
	input.DataForkLength = convert_int64(input.DataForkLength);
	input.RsrcForkOffset = convert_int64(input.RsrcForkOffset);
	input.RsrcForkLength = convert_int64(input.RsrcForkLength);
	// input.SegmentNumber = convert_int(input.SegmentNumber);
	// input.SegmentCount = convert_int(input.SegmentCount);
	// input.DataForkChecksumType = convert_int(input.DataForkChecksumType);
	// input.DataForkChecksum = convert_int(input.DataForkChecksum);
	input.XMLOffset = convert_int64(input.XMLOffset);
	input.XMLLength = convert_int64(input.XMLLength);
	// input.MasterChecksumType = convert_int(input.MasterChecksumType);
	// input.MasterChecksum = convert_int(input.MasterChecksum);
	// input.ImageVariant = convert_int(input.ImageVariant);
	// input.SectorCount = convert_int64(input.SectorCount);
	return input; 
}

int readDMG(FILE* File, FILE* Output, MountType &type) {
	char *plist, *blkx, *data_end, *data_begin, *partname_begin, *partname_end; 
	Bytef *tmp, *otmp, *dtmp; 
	int partnum = 0, i = 0, extractPart = 0; 
	unsigned int data_size = 0; 
	z_stream z;
	bz_stream bz; 
	struct _mishblk *parts = NULL;
	uint64_t out_size, CompressedOffset, CompressedLength, in_offs_add, add_offs, to_read, to_write, chunk;
	_Kolyblck kolyblock; 
	size_t lzfse_outsize = 4 * CHUNKSIZE;
	uint8_t *lzfse_out = NULL;
	fseeko(File, 0, SEEK_SET); 
	fread(&kolyblock, 0x200, 1, File);
	errno = 0; 
	kolyblock = parseKOLYBLOCK(kolyblock); 
	if (errno == EINVAL) {
		fseeko(File, -0x200, SEEK_END); 
		fread(&kolyblock, 0x200, 1, File);
		errno = 0; 
		kolyblock = parseKOLYBLOCK(kolyblock); 
		if (errno == EINVAL) {std::cout << "rip\n"; return -1;}
	}
	if (kolyblock.RsrcForkOffset && kolyblock.RsrcForkLength) {
		char* plist = (char *)malloc(kolyblock.RsrcForkLength);
		fseeko(File, kolyblock.RsrcForkOffset, SEEK_END); 
		fread(plist, kolyblock.RsrcForkLength, 1, File); 
		struct _mishblk mishblk; 
		int next_mishblk = 0; 
		char *mish_begin = plist + 0x104; 
		while (true) {
			mish_begin += next_mishblk; 
			if (mish_begin - plist + 0xCC > kolyblock.RsrcForkLength) {break; } 
			memcpy(&mishblk, 0, 0xD8);
			memcpy(&mishblk, mish_begin, 0xCC);
			mishblk = parseMISHBLOCK(mishblk);
			next_mishblk = 0xD0 + 0x28 * mishblk.BlocksRunCount;
			i = ++partnum;  
			struct _mishblk *parts = (_mishblk *)realloc(parts, partnum * 0xD8);
			if (!parts) {return -1;}
			memcpy(&parts[i], &mishblk, 0xD8);
			parts[i].Data = (char *)malloc(mishblk.BlocksRunCount * 0x28);
			memcpy(parts[i].Data, mish_begin + 0xCC, mishblk.BlocksRunCount * 0x28);
		}
	}
	else if (kolyblock.XMLOffset && kolyblock.XMLLength) {
		plist = (char *)malloc(kolyblock.XMLLength);
		fseeko(File, kolyblock.XMLOffset, SEEK_SET);
		fread(plist, kolyblock.XMLLength, 1, File);
		char *_blkx_begin = strstr(plist, "<key>blkx</key>");
		unsigned int blkx_size = strstr(_blkx_begin, "</array>") - _blkx_begin;
		blkx = (char *)malloc(blkx_size);
		memcpy(blkx, _blkx_begin, blkx_size);
		data_begin = blkx; 
		while (true) {
			unsigned int tmplen; 
			data_begin = strstr(data_begin, "<data>");
			if (!data_begin) {break; }
			data_begin += 6;
			data_end = strstr(data_begin, "</data>");
			if (!data_end) {break; }
			data_size = data_end - data_begin;
			i = ++partnum;
			parts = (struct _mishblk *)realloc(parts, (partnum + 1) * 0xD8);
			if (!parts) {return -1; }
			char *base64data = (char *)malloc(data_size);
			memcpy(base64data, data_begin, data_size);
			cleanup_base64(base64data, data_size); 
			decode_base64(base64data, strlen(base64data), base64data, &tmplen);
			memset(&parts[i], 0, 0xD8);
			memcpy(&parts[i], base64data, 0xCC);
			parts[i] = parseMISHBLOCK(parts[i]);
			parts[i].Data = (char *)malloc(parts[i].BlocksRunCount * 0x28); 
			memcpy(parts[i].Data, base64data + 0xCC, parts[i].BlocksRunCount * 0x28);
			free(base64data); 
			partname_begin = strstr(data_begin, "<key>Name</key>");						// partition names
			partname_begin = strstr(partname_begin, "<key>Name</key>") + 16;
			partname_begin = strstr(partname_begin, "<string>");
			partname_end = strstr(partname_begin, "</string>"); 
			char partname[partname_end - partname_begin]; 
			memcpy(partname, partname_begin, partname_end - partname_begin); 
			if (type==none) {
				if (strstr(partname, "HFS")) {
					extractPart = i; 
					type = hfs; 
				}
				else if (strstr(partname, "APFS")) {
					extractPart = i; 
					type = apfs; 
				}
			}
		}
	}
	else {std::cout << "File doesn't have a kolyblock\n"; return -1;}

	if (!extractPart) {std::cout << "not ok\n"; type = none;}; 
	unsigned int EntryType, offset; 
	CompressedOffset = in_offs_add = kolyblock.DataForkOffset; 
	tmp = (Bytef *)malloc(CHUNKSIZE);
	otmp = (Bytef *)malloc(CHUNKSIZE);
	dtmp = (Bytef *)malloc(CHUNKSIZE);
	z.zalloc = (alloc_func) 0;
	z.zfree = (free_func) 0;
	z.opaque = (voidpf) 0;
	bz.bzalloc = NULL;
	bz.bzfree = NULL;
	bz.opaque = NULL;
	int err; 
	lzfse_out = (uint8_t *)malloc(lzfse_outsize);
	for (i = 0; i < partnum && CompressedOffset <= kolyblock.DataForkLength-kolyblock.DataForkOffset; i++) {
		fflush(stdout); 
		if (extractPart) {i = extractPart; }
		offset = EntryType = 0; 
		add_offs = in_offs_add; 
		while (EntryType != 0xFFFFFFFF && offset < parts[i].BlocksRunCount * 40) {
			EntryType = convert_char4((unsigned char *)parts[i].Data + offset);
			out_size = convert_char8((unsigned char *)parts[i].Data + offset + 16) * 0x200;
			CompressedOffset  = convert_char8((unsigned char *)parts[i].Data + offset + 24);
			CompressedLength = convert_char8((unsigned char *)parts[i].Data + offset + 32);
			in_offs_add = add_offs + CompressedOffset + CompressedLength;
			switch (EntryType) {
				case 0x80000005: //zlib
					inflateInit(&z);
					fseeko(File, CompressedOffset + add_offs, SEEK_SET);
					to_read = CompressedLength;
					do {
						if (!to_read) {break; }
						chunk = to_read > CHUNKSIZE ? CHUNKSIZE : to_read;
						z.avail_in = fread(tmp, 1, chunk, File);
						if (!z.avail_in) {break;} 
						to_read -= z.avail_in;
						z.next_in = tmp;
						do {
							z.avail_out = CHUNKSIZE;
							z.next_out = otmp;
							err = inflate(&z, Z_NO_FLUSH);
							if (err == Z_MEM_ERROR) {return -1; }
							to_write = CHUNKSIZE - z.avail_out;
							fwrite(otmp, 1, to_write, Output); 
						} while (!z.avail_out);
					} while (err != Z_STREAM_END);
					inflateEnd(&z);
					break; 
				case 0x80000006: //bzlib2
					BZ2_bzDecompressInit(&bz, 0, 0);
					fseeko(File, CompressedOffset + add_offs, SEEK_SET);
					to_read = CompressedLength;
					do {
						if (!to_read) {break; }
						chunk = to_read > CHUNKSIZE ? CHUNKSIZE : to_read;
						bz.avail_in = fread(tmp, 1, chunk, File);
						if (!bz.avail_in) {break; }
						to_read -= bz.avail_in;
						bz.next_in = (char *)tmp;
						do {
							bz.avail_out = CHUNKSIZE;
							bz.next_out = (char *)otmp;
							err = BZ2_bzDecompress(&bz);
							to_write = CHUNKSIZE - bz.avail_out;
							fwrite(otmp, 1, to_write, Output); 
						} while (!bz.avail_out);
					} while (err != BZ_STREAM_END);
					BZ2_bzDecompressEnd(&bz);
					break; 
				case 0x80000007: //LZFSE
					fseeko(File, CompressedOffset + add_offs, SEEK_SET);
					to_read = fread(tmp, 1, to_read, File);
					if (to_read) {
						while (true) {
							to_write = lzfse_decode_buffer(lzfse_out, lzfse_outsize, tmp, to_read, nullptr);
							if (to_write == lzfse_outsize) {
								lzfse_outsize <<= 1;
								lzfse_out = (uint8_t *) realloc(lzfse_out, lzfse_outsize);
							} 
							else {break;}
						}
						fwrite(lzfse_out, 1, to_write, Output); 
					}
					break; 
				case 0x80000004: //ADC
					fseeko(File, CompressedOffset + add_offs, SEEK_SET);
					to_read = CompressedLength;
					while (to_read) {
						chunk = to_read > CHUNKSIZE ? CHUNKSIZE : to_read;
						to_write = fread(tmp, 1, chunk, File);
						int bytes_written;	  
						int read_from_input = adc_decompress(to_write, tmp, CHUNKSIZE, dtmp, &bytes_written);
						fwrite(dtmp, 1, bytes_written, Output);
						to_read -= read_from_input;
					}
					break; 
				case 0x00000001: //uncompressed
					fseeko(File, CompressedOffset + add_offs, SEEK_SET);
					to_read = CompressedLength;
					while (to_read) {
						chunk = to_read > CHUNKSIZE ? CHUNKSIZE : to_read;
						fread(tmp, 1, chunk, File);
						fwrite(tmp, 1, chunk, Output);
						to_read -= chunk;
					}
					break; 
				case 0x00000000: case 0x00000002: //ignore
					memset(tmp, 0, CHUNKSIZE);
					to_write = out_size;
					while (to_write) {
						chunk = to_write > CHUNKSIZE ? CHUNKSIZE : to_write; 
						if (fwrite(tmp, 1, chunk, Output) != chunk) {return -1; }
						to_write -= chunk;
					}
					break; 
				case 0xFFFFFFFF: //pēdējais bloks
					if (!CompressedOffset && partnum > i+1) {
						if (convert_char8((unsigned char *)parts[i+1].Data + 24)) {
							in_offs_add = kolyblock.DataForkOffset;
						}
					}
					else {
						in_offs_add = kolyblock.DataForkOffset;
					}
					break; 
			}
			offset += 0x28;
		}
		if (extractPart) {break; }
	}

	#define del(x) if(x) {free(x);}

	del(lzfse_out);
	del(tmp); 
	del(otmp); 
	del(dtmp); 
	for (int i = 0; i < partnum; i++) {
		del(parts[i].Data); 
	}
	del(parts); 
	del(plist); 
	del(blkx); 
	if (File)
		fclose(File);
	if (Output)
		fclose(Output);	
	return 0; 
}