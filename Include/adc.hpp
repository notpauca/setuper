#pragma once 
#include <iostream> 

int adc_decompress(int in_size, unsigned char *input, int avail_size, unsigned char *output, int *bytes_written);
char adc_chunk_type(char _byte);
int adc_chunk_size(char _byte);
int adc_chunk_offset(unsigned char *chunk_start);