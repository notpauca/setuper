#include "base64.hpp"

void cleanup_base64(char *inp, const unsigned int size) {
	char *tinp1 = inp, *tinp2 = inp;
	for (unsigned int i = 0; i < size; i++) {
		if (((unsigned)*tinp2-'A' <= 25) || ((unsigned)*tinp2-'a' <= 25) || ((unsigned)*tinp2-'0' <= 9) || *tinp2 == '+' || *tinp2 == '/' || *tinp2 == '=') {
			*tinp1++ = *tinp2++;
		} 
		else {*tinp1 = *tinp2++;}
	}
	*(tinp1) = 0;
}

unsigned char decode_base64_char(const char c) {
	if (((unsigned)c - 'A') <= 25) {return c - 'A';}
	if (((unsigned)c - 'a') <= 25) {return c - 'a' + 26;} 
	if (((unsigned)c - '0') <= 9) {return c - '0' + 52;}
	if (c == '+') {return 62;} 
	if (c == '=') {return 0;} 
	return 63;
}

void decode_base64(const char *inp, unsigned int isize, char *out, unsigned int *osize) {
	char *tinp = (char *)inp;
	char *tout;

	*osize = isize / 4 * 3;
	if (inp != out) {tout = (char *)malloc(*osize);} 
	else {tout = tinp;}
	for (unsigned int i = 0; i < (isize >> 2); i++) {
		*tout = decode_base64_char(*tinp++) << 2;
		*tout++ |= decode_base64_char(*tinp) >> 4;
		*tout = decode_base64_char(*tinp++) << 4;
		*tout++ |= decode_base64_char(*tinp) >> 2;
		*tout = decode_base64_char(*tinp++) << 6;
		*tout++ |= decode_base64_char(*tinp++);
	}
	if (*(tinp - 1) == '=') {(*osize)--;}
	if (*(tinp - 2) == '=') {(*osize)--;}
}