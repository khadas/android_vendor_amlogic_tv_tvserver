#ifndef __SSM_DATA_STRUCT_H__
#define __SSM_DATA_STRUCT_H__

#include <common.h>

struct SSMHeader_section1_t
{
	unsigned int magic;
	unsigned int version;//count by ID and its size
	unsigned int count;//count by column of excel
	unsigned int rsv[6];//reserve
};

struct SSMHeader_section2_t
{
    unsigned int addr;//Id's addr
	unsigned int size;//this item size
	unsigned int valid;
	unsigned char rsv[6];
};

extern struct SSMHeader_section1_t gSSMHeader_section1;
extern struct SSMHeader_section2_t gSSMHeader_section2[];

#endif