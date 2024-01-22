#include "convert.hpp"

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

uint32_t convert_char4(unsigned char *c)
{
	return (((uint32_t) c[0]) << 24) | (((uint32_t) c[1]) << 16) | (((uint32_t) c[2]) << 8) | ((uint32_t) c[3]);
}

uint64_t convert_char8(unsigned char *c)
{
	return ((uint64_t) convert_char4(c) << 32) | (convert_char4(c + 4));
}