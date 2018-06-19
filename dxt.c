#include <stdint.h>
#include "dxt.h"


#define EXP5TO8R(col)	((((col) >> 8) & 0xf8) | (((col) >> 13) & 0x7))
#define EXP6TO8G(col)	((((col) >> 3) & 0xfc) | (((col) >>  9) & 0x3))
#define EXP5TO8B(col)	((((col) << 3) & 0xf8) | (((col) >>  2) & 0x7))


static void
decode_color_block (void *rgb, int width, int x, int y, void *dxt)
{
	uint32_t *out = rgb;
	uint16_t pcol[2];
	uint8_t col[4][4];
	int i, j;
	
	* (uint32_t *) pcol = * (uint32_t *) dxt;
	col[0][0] = EXP5TO8R (pcol[0]);
	col[0][1] = EXP6TO8G (pcol[0]);
	col[0][2] = EXP5TO8B (pcol[0]);
	col[0][3] = 255;
	col[1][0] = EXP5TO8R (pcol[1]);
	col[1][1] = EXP6TO8G (pcol[1]);
	col[1][2] = EXP5TO8B (pcol[1]);
	col[1][3] = 255;
	
	if (pcol[0] > pcol[1]) {
		col[2][0] = (col[0][0] * 2 + col[1][0]) / 3;
		col[2][1] = (col[0][1] * 2 + col[1][1]) / 3;
		col[2][2] = (col[0][2] * 2 + col[1][2]) / 3;
		col[2][3] = 255;
		col[3][0] = (col[0][0] + col[1][0] * 2) / 3;
		col[3][1] = (col[0][1] + col[1][1] * 2) / 3;
		col[3][2] = (col[0][2] + col[1][2] * 2) / 3;
		col[3][3] = 255;
	} else {
		col[2][0] = (col[0][0] + col[1][0]) / 2;
		col[2][1] = (col[0][1] + col[1][1]) / 2;
		col[2][2] = (col[0][2] + col[1][2]) / 2;
		col[2][3] = 255;
		col[3][0] = 0;
		col[3][1] = 0;
		col[3][2] = 0;
		col[3][3] = 0;
	}
	
	uint32_t idx = * (uint32_t *) (dxt + 4);
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			out[(y+i) * width + (x+j)] = * (uint32_t *) col[idx & 3];
			idx >>= 2;
		}
	}
}


void
dxt1_to_rgba (void *rgb, int width, int height, void *dxt)
{
	int x, y;
	
	for (y = 0; y < height; y += 4) {
		for (x = 0; x < width; x += 4) {
			decode_color_block (rgb, width, x, y, dxt);
			dxt += 8;
		}
	}
}


void
dxt5_to_rgba (void *rgb, int width, int height, void *dxt)
{
	uint8_t *out = rgb;
	int x, y, i, j;
	
	for (y = 0; y < height; y += 4) {
		for (x = 0; x < width; x += 4) {
			decode_color_block (rgb, width, x, y, dxt + 8);
			
			uint16_t a[8];
			a[0] = * (uint8_t *) (dxt + 0);
			a[1] = * (uint8_t *) (dxt + 1);
			
			if (a[0] > a[1]) {
				for (i = 1; i < 7; i++)
					a[i + 1] = (a[0] * (7 - i) + a[1] * i) / 7;
			} else {
				for (i = 1; i < 5; i++)
					a[i + 1] = (a[0] * (5 - i) + a[1] * i) / 5;
				a[6] = 0;
				a[7] = 255;
			}
			
			uint64_t idx = * (uint64_t *) (dxt + 2);
			for (i = 0; i < 4; i++) {
				for (j = 0; j < 4; j++) {
					out[((y+i) * width + (x+j)) * 4 + 3] = (uint8_t) a[idx & 7];
					idx >>= 3;
				}
			}
			
			dxt += 16;
		}
	}
}
