/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "crc.h"

static uint16_t calc_crc16(const uint8_t *data, size_t data_len)
{
	crc_t crc=crc_init();

	crc=crc_update(crc,data,data_len);
	crc=crc_finalize(crc);
	return (uint16_t)crc;
}

int get_frame(FILE *fp, uint8_t *frame)
{
	static uint8_t current_page;
	int i;
	int c;
	int bytes_read;
	unsigned pos;
	uint16_t crc;

	frame[0]=current_page++;
	pos=1;
	bytes_read=0;

	for (i=0;i<ASURO_PAGE_SIZE;i++)
	{
		c=fgetc(fp);
		if (c!=EOF)
		{
			frame[pos++]=(uint8_t)c;
			bytes_read++;
		}
		else
		{
			break;
		}
	}
	for ( ;i<ASURO_PAGE_SIZE;i++)
	{
		frame[pos++]=0xff;
	}
	crc=calc_crc16(&frame[0],ASURO_FRAME_SIZE-sizeof(crc));
	/* Send LSB first, little-endian */
	frame[pos++]=(uint8_t)crc;
	frame[pos++]=(uint8_t)(crc>>8);

	return bytes_read;
}

static char to_hex(int n)
{
	int i;

	n&=0xf;
	if (n<10)
	{
		i='0'+n;
	}
	else
	{
		i='a'+(n-10);
	}
	return i;
}

void print_frame(const uint8_t *frame)
{
	int i;

	for (i=0;i<ASURO_FRAME_SIZE;i++)
	{
		if (i==1 || i==ASURO_PAGE_SIZE+1)
		{
			putchar(' ');
		}
		printf("%c%c",to_hex(frame[i]>>4),to_hex(frame[i]));
	}
	putchar('\n');
}

