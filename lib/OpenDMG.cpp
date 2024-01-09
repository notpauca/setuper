#include "OpenDMG.hpp"

_mishblk parseMISHBLOCK(_mishblk input) {
	input.BlocksSignature = convert_int(input.BlocksSignature);
	if (input.BlocksSignature != 0x6D697368) {errno = 22;}
	input.InfoVersion = convert_int(input.InfoVersion);
	input.FirstSectorNumber = convert_int64(input.FirstSectorNumber);
	input.SectorCount = convert_int64(input.SectorCount);
	input.DataStart = convert_int64(input.DataStart);
	input.DecompressedBufferRequested = convert_int(input.DecompressedBufferRequested);
	input.BlocksDescriptor = convert_int(input.BlocksDescriptor);
	input.ChecksumType = convert_int(input.ChecksumType);
	input.Checksum = convert_int(input.Checksum);
	input.BlocksRunCount = convert_int(input.BlocksRunCount);
	return input; 
}

_Kolyblck parseKOLYBLOCK(_Kolyblck input) {
	input.Signature = convert_int(input.Signature);
    if(input.Signature != 0x6b6f6c79) {errno = 22; }
    input.Version = convert_int(input.Version);
    input.HeaderSize = convert_int(input.HeaderSize);
    input.Flags = convert_int(input.Flags);
    input.RunningDataForkOffset = convert_int64(input.RunningDataForkOffset);
    input.DataForkOffset = convert_int64(input.DataForkOffset);
    input.DataForkLength = convert_int64(input.DataForkLength);
    input.RsrcForkOffset = convert_int64(input.RsrcForkOffset);
    input.RsrcForkLength = convert_int64(input.RsrcForkLength);
    input.SegmentNumber = convert_int(input.SegmentNumber);
    input.SegmentCount = convert_int(input.SegmentCount);
    input.DataForkChecksumType = convert_int(input.DataForkChecksumType);
    input.DataForkChecksum = convert_int(input.DataForkChecksum);
    input.XMLOffset = convert_int64(input.XMLOffset);
    input.XMLLength = convert_int64(input.XMLLength);
    input.MasterChecksumType = convert_int(input.MasterChecksumType);
    input.MasterChecksum = convert_int(input.MasterChecksum);
    input.ImageVariant = convert_int(input.ImageVariant);
    input.SectorCount = convert_int64(input.SectorCount);
	return input; 
}

int readDMG(FILE* File, FILE* Output) {
	char *plist, *blkx, *partname_begin, *partname_end, *data_end, *data_begin; 
	Bytef *tmp, *otmp, *dtmp; 
	unsigned long long total_written = 0;
    int partnum = 0, i = 0; 
	unsigned int data_size = 0; 
	z_stream z;
	bz_stream bz; 
	struct _mishblk *parts = NULL;
	uint64_t out_offs = 0, out_size = 0, in_offs = 0, in_size = 0, in_offs_add = 0, add_offs = 0, to_read = 0, to_write = 0, chunk = 0;
    _Kolyblck kolyblock; 
    fseek(File, -0x200, 2); 
    fread(&kolyblock, 0x200, 1, File);
    kolyblock = parseKOLYBLOCK(kolyblock); 
	if (errno == EINVAL) {return -1; }
    if (kolyblock.XMLOffset && kolyblock.XMLLength) {
		plist = (char *)malloc(kolyblock.XMLLength + 1);
		plist[kolyblock.XMLLength] = '\0';
        fseeko(File, kolyblock.XMLOffset, 0);
        fread(plist, kolyblock.XMLLength, 1, File);
        char *_blkx_begin = strstr(plist, "<key>blkx</key>");
        unsigned int blkx_size = strstr(_blkx_begin, "</array>") - _blkx_begin;
		blkx = (char *)malloc(blkx_size + 1);
        memcpy(blkx, _blkx_begin, blkx_size);
        blkx[blkx_size] = '\0'; 
		data_begin = blkx; 
        while (true) {
            unsigned int tmplen; 
            data_begin = strstr(data_begin, "<data>");
            if (!data_begin) {break;}
            data_begin += 6;
			data_end = strstr(data_begin, "</data>");
			if (!data_end) {break; }
			data_size = data_end - data_begin;
			i = ++partnum;
			parts = (struct _mishblk *)realloc(parts, (partnum + 1) * 0xD8);
			if (!parts) {std::cerr << "nodalījumu mainīgais neeksistē, kautkas nav kārtībā! "; return -1;}
            char *base64data = (char *)malloc(data_size + 1);
            base64data[data_size] = '\0';
            memcpy(base64data, data_begin, data_size);
            cleanup_base64(base64data, data_size); 
            decode_base64(base64data, strlen(base64data), base64data, &tmplen);
			memset(&parts[i], 0, 0xD8);
			memcpy(&parts[i], base64data, 0xCC);
            parts[i] = parseMISHBLOCK(parts[i]);
            parts[i].Data = (char *)malloc(parts[i].BlocksRunCount * 0x28); 
            memcpy(parts[i].Data, base64data + 0xCC, parts[i].BlocksRunCount * 0x28);
            free(base64data); 
            char *partname_begin = strstr(data_begin, "<key>Name</key>");
			partname_begin = strstr(partname_begin, "<key>Name</key>") + strlen("<key>Name</key>");
            char *partname_end = strstr(partname_begin, "</string>");
            char partname[255] = ""; 
			memcpy(partname, partname_begin, partname_end - partname_begin); 
        }
    }
    else if (kolyblock.RsrcForkOffset != 0 && kolyblock.RsrcForkLength != 0) {
        char* plist = (char *)malloc(kolyblock.RsrcForkLength);
        fseeko(File, kolyblock.RsrcForkOffset, 2); 
        fread(plist, kolyblock.RsrcForkLength, 1, File); 
        struct _mishblk mishblk; 
        int next_mishblk = 0; 
        char *mish_begin = plist + 0x104; 
        while (true) {
            mish_begin += next_mishblk; 
            if (mish_begin - plist + 0xCC > kolyblock.RsrcForkLength) {break;} 
			memcpy(&mishblk, 0, 0xD8);
			memcpy(&mishblk, mish_begin, 0xCC);
            mishblk = parseMISHBLOCK(mishblk);
            next_mishblk = 0xCC + 0x28 * mishblk.BlocksRunCount + 0x04;
            int i = ++partnum;  
            struct _mishblk *parts = (_mishblk *)realloc(parts, partnum * sizeof(_mishblk));
			if (!parts) {std::cerr << "nodalījumu mainīgais neeksistē, kautkas nav kārtībā! "; return -1;}
            memcpy(&parts[i], &mishblk, 0xD8);
			parts[i].Data = (char *)malloc(mishblk.BlocksRunCount * 0x28);
            memcpy(parts[i].Data, mish_begin + 0xCC, mishblk.BlocksRunCount * 0x28);
        }
    }
    else {std::cerr << "DMG failā neatrodas bloku informācija, tādēļ tas ir sabojāts. "; return -1; }

	char reserved[5] = "    ";
	unsigned int block_type, dw_reserved; 
	in_offs = add_offs = in_offs_add = kolyblock.DataForkOffset; 
	tmp = (Bytef *) malloc(0x100000);
	otmp = (Bytef *) malloc(0x100000);
	dtmp = (Bytef *) malloc(0x100000);
	z.zalloc = (alloc_func) 0;
	z.zfree = (free_func) 0;
	z.opaque = (voidpf) 0;
	bz.bzalloc = NULL;
	bz.bzfree = NULL;
	bz.opaque = NULL;
	int err = 0; 
	unsigned int offset;
	for (int i = 0; i < partnum && in_offs <= kolyblock.DataForkLength-kolyblock.DataForkOffset; i++) {
		fflush(stdout); 
		offset = 0; 
		add_offs = in_offs_add; 
		block_type = 0; 
		// unsigned long bi = 0; // ig it's binary. idk though. 
		while (block_type != 0xffffffff && offset < parts[i].BlocksRunCount * 0x28) {
			block_type = convert_char4((unsigned char *)parts[i].Data + offset);
			dw_reserved = convert_char4((unsigned char *)parts[i].Data + offset + 4);
			memcpy(&reserved, parts[i].Data + offset + 4, 4);
			out_offs = convert_char8((unsigned char *)parts[i].Data + offset + 8) * 0x200;
			out_size = convert_char8((unsigned char *)parts[i].Data + offset + 16) * 0x200;
			in_offs = convert_char8((unsigned char *)parts[i].Data + offset + 24);
			in_size = convert_char8((unsigned char *)parts[i].Data + offset + 32);
			if (block_type != 0xffffffff) {in_offs_add = add_offs + in_offs + in_size;}
			if (block_type == 0x80000005) {			// zlib data
				if (inflateInit(&z)) {std::cout << "kautkas nav kārtībā. Kas? idfk"; return -1; }
				fseeko(File, in_offs + add_offs, 0);
				to_read = in_size;
				do {
					if (!to_read)
						break;
					if (to_read > 0x100000)
						chunk = 0x100000;
					else
						chunk = to_read;
					z.avail_in = fread(tmp, 1, chunk, File);
					if (!z.avail_in) {break;} 
					to_read -= z.avail_in;
					z.next_in = tmp;
					do {
						z.avail_out = CHUNKSIZE;
						z.next_out = otmp;
						int err = inflate(&z, Z_NO_FLUSH);
						assert(err != Z_STREAM_ERROR);	/* state not clobbered */
						switch (err) {
						case 2:
							err = -3;	/* and fall through */
						case -3:
						case -4: return 1;
						}
						to_write = CHUNKSIZE - z.avail_out;
						fwrite(otmp, 1, to_write, Output); 
						total_written += to_write;
					} while (z.avail_out == 0);
				} while (err != Z_STREAM_END);

				(void)inflateEnd(&z);
			} 
			else if (block_type == 0x80000006) {	// bzlib2 data
				fseeko(File, in_offs + add_offs, 0);
				to_read = in_size;
				do {
					if (!to_read)
						break;
					if (to_read > CHUNKSIZE)
						chunk = CHUNKSIZE;
					else
						chunk = to_read;
					bz.avail_in = fread(tmp, 1, chunk, File);
					if (bz.avail_in == 0)
						break;
					to_read -= bz.avail_in;
					bz.next_in = (char *)tmp;
					do {
						bz.avail_out = CHUNKSIZE;
						bz.next_out = (char *)otmp;
						err = BZ2_bzDecompress(&bz);
						to_write = CHUNKSIZE - bz.avail_out;
						fwrite(otmp, 1, to_write, Output); 
					} while (bz.avail_out == 0);
				} while (err != BZ_STREAM_END);

				(void)BZ2_bzDecompressEnd(&bz);
			} 
			else if (block_type == 0x80000004) {	// ADC data
				fseeko(File, in_offs + add_offs, 0);
				to_read = in_size;
				while (to_read > 0) {
					chunk = to_read > CHUNKSIZE ? CHUNKSIZE : to_read;
					to_write = fread(tmp, 1, chunk, File);
					int bytes_written;
					int read_from_input = adc_decompress(to_write, tmp, 0x100000, dtmp, &bytes_written);
					fwrite(dtmp, 1, bytes_written, Output);
					to_read -= read_from_input;
				}
			} 
			else if (block_type == 0x00000001) {	// raw data
				fseeko(File, in_offs + add_offs, 0);
				to_read = in_size;
				while (to_read > 0) {
					if (to_read > CHUNKSIZE)
						chunk = CHUNKSIZE;
					else
						chunk = to_read;
					to_write = fread(tmp, 1, chunk, File);
					fwrite(tmp, 1, chunk, Output);
					to_read -= chunk;
				}
			} 
			else if (block_type == 0x00000000 || block_type == 0x00000002) {	// Zero-fill or Ignored block
				memset(tmp, 0, CHUNKSIZE);
				to_write = out_size;
				while (to_write > 0) {
					if (to_write > CHUNKSIZE)
						chunk = CHUNKSIZE;
					else
						chunk = to_write;
					if (fwrite(tmp, 1, chunk, Output) != chunk) {
						return 1; 
					}
					total_written += chunk;
					to_write -= chunk;
				}
			}
			else if (block_type == 0xffffffff) {	// last blxx entry
				if (in_offs == 0 && partnum > i+1) {
					if (convert_char8((unsigned char *)parts[i+1].Data + 24) != 0)
						in_offs_add = kolyblock.DataForkOffset;
				} else
					in_offs_add = kolyblock.DataForkOffset;

			}
			offset += 0x28;
		}
	}
    
	#define delete(x) if(x) {free(x);}

	delete(tmp); 
	delete(otmp); 
	delete(dtmp); 
	for (int i = 0; i < partnum; i++) {
		delete(parts[i].Data); 
	}
	partnum = 0; 
	delete(parts); 
	delete(plist); 
	delete(blkx); 
	if (File)
		fclose(File);
	if (Output)
		fclose(Output);	
	return 0; 
}