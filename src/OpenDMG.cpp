#include <fstream>
#include <zlib.h>
#include <bzlib.h>
#include <iostream>
#include <assert.h>
#include <adc.h>

#define CHUNKSIZE 0x100000
#define DECODESIZE 0x100000

#define ADC_PLAIN 0x01
#define ADC_2BYTE 0x02
#define ADC_3BYTE 0x03

#define bool short
#define true 1
#define false 0

int adc_decompress(int in_size, unsigned char *input, int avail_size, unsigned char *output, int *bytes_written)
{
	if (!in_size)
		return 0;
	bool output_full = false;
	unsigned char *inp = input;
	unsigned char *outp = output;
	int chunk_type;
	int chunk_size;
	int offset;
	int i;

	while (inp - input < in_size) {
		chunk_type = adc_chunk_type(*inp);
		switch (chunk_type) {
		case ADC_PLAIN:
			chunk_size = adc_chunk_size(*inp);
			if (outp + chunk_size - output > avail_size) {
				output_full = true;
				break;
			}
			memcpy(outp, inp + 1, chunk_size);
			inp += chunk_size + 1;
			outp += chunk_size;
			break;

		case ADC_2BYTE:
			chunk_size = adc_chunk_size(*inp);
			offset = adc_chunk_offset(inp);
			if (outp + chunk_size - output > avail_size) {
				output_full = true;
				break;
			}
			if (offset == 0) {
				memset(outp, *(outp - offset - 1), chunk_size);
				outp += chunk_size;
				inp += 2;
			} else {
				for (i = 0; i < chunk_size; i++) {
					memcpy(outp, outp - offset - 1, 1);
					outp++;
				}
				inp += 2;
			}
			break;

		case ADC_3BYTE:
			chunk_size = adc_chunk_size(*inp);
			offset = adc_chunk_offset(inp);
			if (outp + chunk_size - output > avail_size) {
				output_full = true;
				break;
			}
			if (offset == 0) {
				memset(outp, *(outp - offset - 1), chunk_size);
				outp += chunk_size;
				inp += 3;
			} else {
				for (i = 0; i < chunk_size; i++) {
					memcpy(outp, outp - offset - 1, 1);
					outp++;
				}
				inp += 3;
			}
			break;
		}
		if (output_full)
			break;
	}
	*bytes_written = outp - output;
	return inp - input;
}

int adc_chunk_type(char _byte)
{
	if (_byte & 0x80)
		return ADC_PLAIN;
	if (_byte & 0x40)
		return ADC_3BYTE;
	return ADC_2BYTE;
}

int adc_chunk_size(char _byte)
{
	switch (adc_chunk_type(_byte)) {
		case ADC_PLAIN:
		return (_byte & 0x7F) + 1;
	case ADC_2BYTE:
		return ((_byte & 0x3F) >> 2) + 3;
	case ADC_3BYTE:
		return (_byte & 0x3F) + 4;
	}
	return -1;
}

int adc_chunk_offset(unsigned char *chunk_start)
{
	unsigned char *c = chunk_start;
	switch (adc_chunk_type(*c)) {
	case ADC_PLAIN:
		return 0;
	case ADC_2BYTE:
		return ((((unsigned char)*c & 0x03)) << 8) + (unsigned char)*(c + 1);
	case ADC_3BYTE:
		return (((unsigned char)*(c + 1)) << 8) + (unsigned char)*(c + 2);
	}
	return -1;
}

bool is_base64(char c)
{
	if ((c >= 'A' && c <= 'Z') ||
	    (c >= 'a' && c <= 'z') ||
	    (c >= '0' && c <= '9') ||
	    c == '+' ||
	    c == '/' ||
	    c == '=')
		return true;
	return false;
}

int convert_int(int i)
{
	int o;
	char *p_i = (char *) &i;
	char *p_o = (char *) &o;
	p_o[0] = p_i[3];
	p_o[1] = p_i[2];
	p_o[2] = p_i[1];
	p_o[3] = p_i[0];
	return o;
}

uint64_t convert_int64(uint64_t i)
{
	uint64_t o;
	char *p_i = (char *) &i;
	char *p_o = (char *) &o;
	p_o[0] = p_i[7];
	p_o[1] = p_i[6];
	p_o[2] = p_i[5];
	p_o[3] = p_i[4];
	p_o[4] = p_i[3];
	p_o[5] = p_i[2];
	p_o[6] = p_i[1];
	p_o[7] = p_i[0];
	return o;
}

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

void cleanup_base64(char *inp, const unsigned int size)
{
	char *tinp1, *tinp2;
	unsigned int i;
	tinp1 = inp;
	tinp2 = inp;
	for (i = 0; i < size; i++) {
		if (is_base64(*tinp2)) {
			*tinp1++ = *tinp2++;
		} else {
			*tinp1 = *tinp2++;
		}
	}
	*(tinp1) = 0;
}

unsigned char decode_base64_char(const char c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	if (c >= '0' && c <= '9')
		return c - '0' + 52;
	if (c == '+')
		return 62;
	if (c == '=')
		return 0;
	return 63;
}

void decode_base64(const char *inp, unsigned int isize, char *out, unsigned int *osize)
{
	char *tinp = (char *)inp;
	char *tout;
	unsigned int i;

	*osize = isize / 4 * 3;
	if (inp != out) {
		tout = (char *)malloc(*osize);
		out = tout;
	} else {
		tout = tinp;
	}
	for (i = 0; i < (isize >> 2); i++) {
		*tout = decode_base64_char(*tinp++) << 2;
		*tout++ |= decode_base64_char(*tinp) >> 4;
		*tout = decode_base64_char(*tinp++) << 4;
		*tout++ |= decode_base64_char(*tinp) >> 2;
		*tout = decode_base64_char(*tinp++) << 6;
		*tout++ |= decode_base64_char(*tinp++);
	}
	if (*(tinp - 1) == '=')
		(*osize)--;
	if (*(tinp - 2) == '=')
		(*osize)--;
}

void fill_mishblk(char* c, struct _mishblk* m)
{
	memset(m, 0, sizeof(struct _mishblk));
	memcpy(m, c, 0xCC);
	m->BlocksSignature = convert_int(m->BlocksSignature);
	m->InfoVersion = convert_int(m->InfoVersion);
	m->FirstSectorNumber = convert_int64(m->FirstSectorNumber);
	m->SectorCount = convert_int64(m->SectorCount);
	m->DataStart = convert_int64(m->DataStart);
	m->DecompressedBufferRequested = convert_int(m->DecompressedBufferRequested);
	m->BlocksDescriptor = convert_int(m->BlocksDescriptor);
	m->ChecksumType = convert_int(m->ChecksumType);
	m->Checksum = convert_int(m->Checksum);
	m->BlocksRunCount = convert_int(m->BlocksRunCount);
}

uint32_t convert_char4(unsigned char *c)
{
	return (((uint32_t) c[0]) << 24) | (((uint32_t) c[1]) << 16) |
	(((uint32_t) c[2]) << 8) | ((uint32_t) c[3]);
}

uint64_t convert_char8(unsigned char *c)
{
	return ((uint64_t) convert_char4(c) << 32) | (convert_char4(c + 4));
}

int readDMG(FILE* File, FILE* Output) {
	char *plist, *blkx, *partname_begin, *partname_end, *data_end, *data_begin = NULL; 
	Bytef *tmp, *otmp, *dtmp = NULL; 
	unsigned long long total_written = 0;
    int partnum, i = 0; 
	unsigned int data_size; 
	z_stream z;
	bz_stream bz; 
	struct _mishblk *parts = NULL;
	uint64_t out_offs, out_size, in_offs, in_size, in_offs_add, add_offs, to_read, to_write, chunk;
    _Kolyblck result; 
    fseek(File, -0x200, 2); 
    fread(&result, 0x200, 1, File);
    result.Signature = convert_int(result.Signature);
    if(result.Signature != 0x6b6f6c79) {std::cerr << "Koly block is corrupted!\n"; return 1;}
    result.Version = convert_int(result.Version);
    result.HeaderSize = convert_int(result.HeaderSize);
    result.Flags = convert_int(result.Flags);
    result.RunningDataForkOffset = convert_int64(result.RunningDataForkOffset);
    result.DataForkOffset = convert_int64(result.DataForkOffset);
    result.DataForkLength = convert_int64(result.DataForkLength);
    result.RsrcForkOffset = convert_int64(result.RsrcForkOffset);
    result.RsrcForkLength = convert_int64(result.RsrcForkLength);
    result.SegmentNumber = convert_int(result.SegmentNumber);
    result.SegmentCount = convert_int(result.SegmentCount);
    result.DataForkChecksumType = convert_int(result.DataForkChecksumType);
    result.DataForkChecksum = convert_int(result.DataForkChecksum);
    result.XMLOffset = convert_int64(result.XMLOffset);
    result.XMLLength = convert_int64(result.XMLLength);
    result.MasterChecksumType = convert_int(result.MasterChecksumType);
    result.MasterChecksum = convert_int(result.MasterChecksum);
    result.ImageVariant = convert_int(result.ImageVariant);
    result.SectorCount = convert_int64(result.SectorCount);
    if (result.XMLOffset != 0 && result.XMLLength != 0) {
		plist = (char *)malloc(result.XMLLength + 1);
        fseeko(File, result.XMLOffset, SEEK_SET);
        fread(plist, result.XMLLength, 1, File);
		plist[result.XMLLength] = '\0';
        char *_blkx_begin = strstr(plist, "<key>blkx</key>");
        unsigned int blkx_size = strstr(_blkx_begin, "</array>") - _blkx_begin;
		blkx = (char *)malloc(blkx_size + 1);
        memcpy(blkx, _blkx_begin, blkx_size);
        blkx[blkx_size] = '\0'; 
		char* blockdata = blkx; 
        int scb = strlen("<data>"); 
		data_begin = blkx; 
        while (true) {
            unsigned int tmplen; 
            data_begin = strstr(data_begin, "<data>");
            if (!data_begin) {break;}
            data_begin += scb;
			data_end = strstr(data_begin, "</data>");
			if (!data_end) {break; }
			data_size = data_end - data_begin;
			i = partnum;
            ++partnum; 
			parts = (struct _mishblk *)realloc(parts, (partnum + 1) * sizeof(struct _mishblk));
			if (!parts) {std::cerr << "parts hasn't loaded in"; return 1;}
            char *base64data = (char *)malloc(data_size + 1);
            base64data[data_size] = '\0';					//BASE64DATA IS CORRUPTED!!!!!
            memcpy(base64data, data_begin, data_size);
			std::cout << base64data; 
            cleanup_base64(base64data, data_size); 
            decode_base64(base64data, strlen(base64data), base64data, &tmplen);
            fill_mishblk(base64data, &parts[i]);
			if (parts[i].BlocksSignature != 0x6D697368) {break;}
            parts[i].Data = (char *)malloc(parts[i].BlocksRunCount * 0x28); 
            memcpy(parts[i].Data, base64data + 0xCC, parts[i].BlocksRunCount * 0x28);
            free(base64data); 
            ++partnum; 
            char *partname_begin = strstr(data_begin, "<key>Name</key>");
			partname_begin = strstr(partname_begin, "<key>Name</key>") + strlen("<key>Name</key>");
            char *partname_end = strstr(partname_begin, "</string>");
            char partname[255] = ""; 
			memcpy(partname, partname_begin, partname_end - partname_begin); 
        }
    
    }
    else if (result.RsrcForkOffset != 0 && result.RsrcForkLength != 0) {
        char* plist = (char *)malloc(result.RsrcForkLength);
        fseeko(File, result.RsrcForkOffset, 2); 
        fread(plist, result.RsrcForkLength, 1, File); 
        partnum = 0; 
        struct _mishblk mishblk; 
        int next_mishblk = 0; 
        char *mish_begin = plist + 0x104; 
        while (true) {
            mish_begin += next_mishblk; 
            if (mish_begin - plist + 0xCC > result.RsrcForkLength) {break;} 
            fill_mishblk(mish_begin, &mishblk);
            if (mishblk.BlocksSignature != 0x6D697368) {break;}
            next_mishblk = 0xCC + 0x28 * mishblk.BlocksRunCount + 0x04;
            int i = partnum; 
            ++partnum; 
            struct _mishblk *parts = (_mishblk *)realloc(parts, partnum * sizeof(_mishblk));
			if (!parts) {std::cerr << "parts hasn't loaded in"; return 1;}
            memcpy(&parts[i], &mishblk, sizeof(struct _mishblk));
			parts[i].Data = (char *)malloc(0x28 * mishblk.BlocksRunCount);
            memcpy(parts[i].Data, mish_begin + 0xCC, 0x28 * mishblk.BlocksRunCount);
        }
    }
    else {std::cerr << "DMG failā neatrodas vajadzīgā informācija, tādēļ tas ir sabojāts. "; return 1; }

	char reserved[5] = "    ";
	unsigned int block_type, dw_reserved; 
	in_offs = add_offs = in_offs_add = result.DataForkOffset; 
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
	for (int i = 0; i < partnum && in_offs <= result.DataForkLength-result.DataForkOffset; i++) {
		fflush(stdout); 
		int offset = 0; 
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
			if (block_type == 0x80000005) {
				if (!inflateInit(&z)) {std::cout << "kautkas nav kārtībā. Kas? idfk"; break; }
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
						to_write = CHUNKSIZE - z.avail_out;
						fwrite(otmp, 1, to_write, Output); 
						total_written += to_write;
					} 
					while (z.avail_out == 0);
				} while (err != Z_STREAM_END);

				(void)inflateEnd(&z);
			} 
			else if (block_type == 0x80000006) {
				fseeko(File, in_offs + add_offs, SEEK_SET);
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
			else if (block_type == 0x80000004) {
				fseeko(File, in_offs + add_offs, SEEK_SET);
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
			else if (block_type == 0x00000001) {
				fseeko(File, in_offs + add_offs, SEEK_SET);
				to_read = in_size;
				while (to_read > 0) {
					if (to_read > CHUNKSIZE)
						chunk = CHUNKSIZE;
					else
						chunk = to_read;
					to_write = fread(tmp, 1, chunk, File);
					fwrite(tmp, 1, chunk, Output);
					//copy
						to_read -= chunk;
				}
			} 
			else if (block_type == 0x00000000 || block_type == 0x00000002) {
				memset(tmp, 0, CHUNKSIZE);
				to_write = out_size;
				while (to_write > 0) {
					if (to_write > CHUNKSIZE)
						chunk = CHUNKSIZE;
					else
						chunk = to_write;
					if (fwrite(tmp, 1, chunk, Output) != chunk) {return 1; }
					total_written += chunk;
					to_write -= chunk;
				}
			}
			else if (block_type == 0xffffffff) {
				if (in_offs == 0 && partnum > i+1) {
					if (convert_char8((unsigned char *)parts[i+1].Data + 24) != 0)
						in_offs_add = result.DataForkOffset;
				} else
					in_offs_add = result.DataForkOffset;

			}
			offset += 0x28;
		}
	}
    
	if (tmp != NULL)
		free(tmp);
	if (otmp != NULL)
		free(otmp);
	if (dtmp != NULL)
		free(dtmp);
	for (int i = 0; i < partnum; i++) {
		if (parts[i].Data != NULL)
			free(parts[i].Data);
	}
	if (parts != NULL)
		free(parts);
	if (plist != NULL)
		free(plist);
	if (blkx != NULL)
		free(blkx);
	if (File != NULL)
		fclose(File);
	if (Output != NULL)
		fclose(Output);	
	return 0; 
}