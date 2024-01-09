#pragma once
#include <iostream> 

void decode_base64(const char *inp, unsigned int isize, char *out, unsigned int *osize);
unsigned char decode_base64_char(const char c);
void cleanup_base64(char *inp, const unsigned int size);