#include "adc.hpp"

int adc_decompress(int in_size, unsigned char *input, int avail_size, unsigned char *output, int *bytes_written)
{
	if (!in_size) {return 0;}
	short output_full = false;
	unsigned char *inp = input;
	unsigned char *outp = output;
	int chunk_type, chunk_size, offset, i;

	while (inp - input < in_size) {
		chunk_type = adc_chunk_type(*inp);
		switch (chunk_type) {
		case 1:
			chunk_size = adc_chunk_size(*inp);
			if (outp + chunk_size - output > avail_size) {
				output_full = true;
				break;
			}
			memcpy(outp, inp + 1, chunk_size);
			inp += chunk_size + 1;
			outp += chunk_size;
			break;

		case 2:
			chunk_size = adc_chunk_size(*inp);
			offset = adc_chunk_offset(inp);
			if (outp + chunk_size - output > avail_size) {
				output_full = true;
				break;
			}
			if (!offset) {
				memset(outp, *(outp - offset - 1), chunk_size);
				outp += chunk_size;
				inp += 2;
			} 
			else { 
				for (i = 0; i < chunk_size; i++) { 
					memcpy(outp, outp - offset - 1, 1); 
					outp++;}
				inp += 2;
			}
			break;

		case 3:
			chunk_size = adc_chunk_size(*inp);
			offset = adc_chunk_offset(inp);
			if (outp + chunk_size - output > avail_size) {
				output_full = 1;
				break;
			}
			if (!offset) {
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

char adc_chunk_type(char _byte)
{
	if (_byte & 0x80)
		return 1;
	if (_byte & 0x40)
		return 3;
	return 2;
}

int adc_chunk_size(char _byte)
{
	switch (adc_chunk_type(_byte)) {
		case 1:
			return (_byte & 0x7F) + 1;
		case 2:
			return ((_byte & 0x3F) >> 2) + 3;
		case 3:
			return (_byte & 0x3F) + 4;
	}
	return -1;
}

int adc_chunk_offset(unsigned char *chunk_start)
{
	unsigned char *c = chunk_start;
	switch (adc_chunk_type(*c)) {
		case 1:
			return 0;
		case 2:
			return ((((unsigned char)*c & 0x03)) << 8) + (unsigned char)*(c + 1);
		case 3:
			return (((unsigned char)*(c + 1)) << 8) + (unsigned char)*(c + 2);
	}
	return -1;
}
