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

_mishblk_data parseSectorInfo(char* input) {
	_mishblk_data output; 
	output.EntryType = convert_char4((unsigned char*)input);
	output.Comment = convert_char4((unsigned char*)input + 8);				//var ignorēt
	output.SectorCount = convert_char8((unsigned char*)input + 16); 		//out_size, vajag reizināt ar 0x200
	output.CompressedOffset = convert_char8((unsigned char*)input + 24);
	output.CompressedLength = convert_char8((unsigned char*)input + 32);
	return output; 
}

int readDMG(FILE* File, FILE* Output, MountType &type) {
	char *plist, *data_end, *data_begin, *partname_begin, *partname_end; 
	char *sector_data_buffer = (char*)malloc(0x28); 
	Bytef *tmp, *otmp, *dtmp; 
	int partnum = 0, i = 0, extractPart = 0; 
	unsigned int data_size = 0; 
	z_stream z;
	bz_stream bz; 
	struct _mishblk *parts = NULL;
	uint64_t out_size, in_offs_add, add_offs, to_read, to_write, chunk;
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
		if (errno == EINVAL) {goto exit; return -1;}
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
			if (!parts) {goto exit; return -1;}
			memcpy(&parts[i], &mishblk, 0xD8);
			parts[i].Data = (char *)malloc(mishblk.BlocksRunCount * 0x28);
			memcpy(parts[i].Data, mish_begin + 0xCC, mishblk.BlocksRunCount * 0x28);
		}
	}
	else if (kolyblock.XMLOffset && kolyblock.XMLLength) {
		plist = (char *)malloc(kolyblock.XMLLength);
		fseeko(File, kolyblock.XMLOffset, SEEK_SET);
		fread(plist, kolyblock.XMLLength, 1, File);
		char *_blkx_begin = strstr(plist, "<key>blkx</key>") + 15;
		unsigned int blkx_size = strstr(_blkx_begin, "</array>") - _blkx_begin;
		data_begin = (char *)malloc(blkx_size);
		memcpy(data_begin, _blkx_begin, blkx_size);
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
			if (!parts) {goto exit; return -1; }
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
			partname_begin = strstr(data_begin, "<key>Name</key>") + 28;						// partition names
			partname_end = strstr(partname_begin, "<"); 
			char* partname = (char*)malloc(partname_end - partname_begin);
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
			free(partname); 
		}
	}
	else {std::cout << "Nav kolybloka! Fails ir sabojāts, vai tam šis rīks nav nepieciešams!\n"; goto exit; return -1;}

	if (!extractPart) {type = none;}
	tmp = (Bytef *)malloc(CHUNKSIZE);
	otmp = (Bytef *)malloc(CHUNKSIZE);
	dtmp = (Bytef *)malloc(CHUNKSIZE);
	lzfse_out = (uint8_t *)malloc(lzfse_outsize);
	z.zalloc = (alloc_func) 0;
	z.zfree = (free_func) 0;
	z.opaque = (voidpf) 0;
	bz.bzalloc = NULL;
	bz.bzfree = NULL;
	bz.opaque = NULL;
	int err; 
	_mishblk_data sector_data; 
	sector_data.CompressedOffset = in_offs_add = kolyblock.DataForkOffset; 
	for (int i = extractPart ? extractPart : 0; i < extractPart ? extractPart : partnum && sector_data.CompressedOffset <= kolyblock.DataForkLength-kolyblock.DataForkOffset; i++) {
		fflush(stdout); 
		add_offs = in_offs_add; 
		for (int offset = 0; sector_data.EntryType != last && offset < parts[i].BlocksRunCount; offset++) {
			memcpy(sector_data_buffer, parts[i].Data + (offset * 0x28), 0x28); 
			sector_data = parseSectorInfo(sector_data_buffer);
			in_offs_add = add_offs + sector_data.CompressedOffset + sector_data.CompressedLength;
			switch (sector_data.EntryType) {
				case zlib:
					inflateInit(&z);
					fseeko(File, sector_data.CompressedOffset + add_offs, SEEK_SET);
					to_read = sector_data.CompressedLength;
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
							if (err == Z_MEM_ERROR) {goto exit; return -1; }
							to_write = CHUNKSIZE - z.avail_out;
							fwrite(otmp, 1, to_write, Output); 
						} while (!z.avail_out);
					} while (err != Z_STREAM_END);
					inflateEnd(&z);
					break; 
				case bzlib2: 
					BZ2_bzDecompressInit(&bz, 0, 0);
					fseeko(File, sector_data.CompressedOffset + add_offs, SEEK_SET);
					to_read = sector_data.CompressedLength;
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
				case lzfse: 
					fseeko(File, sector_data.CompressedOffset + add_offs, SEEK_SET);
					if (sector_data.CompressedLength) {
						to_read = fread(tmp, 1, to_read > CHUNKSIZE ? CHUNKSIZE : sector_data.CompressedLength, File);
						while (true) {
							to_write = lzfse_decode_buffer(lzfse_out, lzfse_outsize, tmp, to_read, NULL);
							if (to_write == lzfse_outsize) {
								lzfse_outsize <<= 1;
								lzfse_out = (uint8_t *) realloc(lzfse_out, lzfse_outsize);
							} 
							else {break;}
						}
						fwrite(lzfse_out, 1, to_write, Output); 
					}
					break; 
				case adc: 
					fseeko(File, sector_data.CompressedOffset + add_offs, SEEK_SET);
					to_read = sector_data.CompressedLength;
					while (to_read) {
						chunk = to_read > CHUNKSIZE ? CHUNKSIZE : to_read;
						to_write = fread(tmp, 1, chunk, File);
						int bytes_written;	  
						int read_from_input = adc_decompress(to_write, tmp, CHUNKSIZE, dtmp, &bytes_written);
						fwrite(dtmp, 1, bytes_written, Output);
						to_read -= read_from_input;
					}
					break; 
				case uncompressed: 
					fseeko(File, sector_data.CompressedOffset + add_offs, SEEK_SET);
					to_read = sector_data.CompressedLength;
					while (to_read) {
						chunk = to_read > CHUNKSIZE ? CHUNKSIZE : to_read;
						fread(tmp, 1, chunk, File);
						fwrite(tmp, 1, chunk, Output);
						to_read -= chunk;
					}
					break; 
				case ignore_1: case ignore_2: 
					memset(tmp, 0, CHUNKSIZE);
					to_write = sector_data.SectorCount * 0x200;
					while (to_write) {
						chunk = to_write > CHUNKSIZE ? CHUNKSIZE : to_write; 
						if (fwrite(tmp, 1, chunk, Output) != chunk) {goto exit; return -1; }
						to_write -= chunk;
					}
					break; 
				case last: 
					if (!sector_data.CompressedOffset && partnum > i+1) {
						if (convert_char8((unsigned char *)parts[i+1].Data + 24)) {
							in_offs_add = kolyblock.DataForkOffset;
						}
					}
					else {
						in_offs_add = kolyblock.DataForkOffset;
					}
					break; 
			}
		}
		if (extractPart) {break; }
	}
	goto exit; 
	return 0; 
	exit: 
		#define del(x) if(x) {free(x);}
		del(lzfse_out);
		del(tmp); 
		del(otmp); 
		del(dtmp); 
		for (int i = 0; i < partnum; i++) {
			del(parts[i].Data); 
		}
		del(sector_data_buffer); 
		del(data_begin); 
		del(parts); 
		del(plist); 
		if (File)
			fclose(File);
		if (Output)
			fclose(Output);	
}