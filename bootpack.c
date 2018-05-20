#include <stdio.h>

/* 外部函数名 */
void io_hlt (void);
void io_cli (void);
void io_out8 (int port, int data);
int io_load_eflags (void);
void io_store_eflags (int eflags);

/*事先声明一下*/

void init_palette (void);
void set_palette (int start, int end, unsigned char *rgb);
void boxfill8 (unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);	//指向显存，xsize是每行共多少个像素便于换行，c是指定颜色，x0，x1分别为起始位置和终止位置，y0和y1同理
void init_screen (char *vram, int x, int y);
void putfont8 (char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc (char *vram, int xsize, int x, int y, char c, unsigned char *s);

#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

struct BOOTINFO
{
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
  	char *vram;
};
//结构体后面一定要加分号

void HariMain (void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	char s[40];
	char *test;
	
	init_palette ();
	init_screen (binfo->vram, binfo->scrnx, binfo->scrny);
	putfonts8_asc (binfo->vram, binfo->scrnx,  8,  8, COL8_FFFFFF, "ABC 123");
	putfonts8_asc (binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "Haribote OS.");
	putfonts8_asc (binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "Haribote OS.");
	putfonts8_asc (binfo->vram, binfo->scrnx, 31, 61, COL8_000000, "Hello,World!");
	putfonts8_asc (binfo->vram, binfo->scrnx, 30, 60, COL8_FFFFFF, "Hello,World!");
	sprintf (s, "scrnx = %d", binfo->scrnx);
	sprintf (test, "hello! %d", binfo->scrny);
	putfonts8_asc (binfo->vram, binfo->scrnx, 30, 90, COL8_FFFFFF, s);
	putfonts8_asc (binfo->vram, binfo->scrnx, 30, 120, COL8_FFFFFF, test);
	
	for (;;)
	{
		io_hlt ();
	}
 }

void init_palette (void)
{
	static unsigned char table_rgb[16 * 3] =
	{
		0x00, 0x00, 0x00, 	/* 0:黑 */
		0xff, 0x00, 0x00, 	/* 1:亮红 */
		0x00, 0xff, 0x00,	/* 2:亮绿 */
		0xff, 0xff, 0x00,	/* 3:亮黄 */
		0x00, 0x00, 0xff,	/* 4:亮蓝 */
		0xff, 0x00, 0xff,	/* 5:亮紫 */
		0x00, 0xff, 0xff,	/* 6:浅亮蓝 */
		0xff, 0xff, 0xff,		/* 7:白 */
		0xc6, 0xc6, 0xc6,	/* 8:亮灰 */
		0x84, 0x00, 0x00,	/* 9:暗红 */
		0x00, 0x84, 0x00,	/* 10:暗绿 */
		0x84, 0x84, 0x00,	/* 11:暗黄 */
		0x00, 0x00, 0x84,	/* 12:暗青 */
		0x84, 0x00, 0x84,	/* 13:暗紫 */
	       	0x00, 0x84, 0x84,	/* 14:浅暗蓝 */
		0x84, 0x84, 0x84	/* 15:暗灰 */
	};
	set_palette (0, 15, table_rgb);
	return;

  /* c语言中static char语句只能用于数据，相当于汇编中的db指令 */
}

void set_palette (int start, int end, unsigned char *rgb)
{
	int i, eflags;
	eflags = io_load_eflags ();	/* 记录中断许可标志的值 */
	io_cli ();
	io_out8 (0x03c8, start);
  	for (i = start; i <= end; i++)
	{
		io_out8 (0x03c9, rgb[0] / 4);
		io_out8 (0x03c9, rgb[1] / 4);
		io_out8 (0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags (eflags);	/* 复原中断许可标志 */
	return;
}

void boxfill8 (unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)	//每个像素点置入颜色
{
	int x, y;
	for (y = y0; y <= y1; y++)
	{
		for (x = x0; x <= x1; x++)
		{
			vram[y * xsize + x] = c;
		}
	}
	return;
}

void init_screen (char *vram, int x, int y)	//初始化显示界面，x和y分别是画面的长和宽
{
	boxfill8 (vram, x, COL8_008484,  0,     0,      x -  1, y - 29);
	boxfill8 (vram, x, COL8_C6C6C6,  0,     y - 28, x -  1, y - 28);
	boxfill8 (vram, x, COL8_FFFFFF,  0,     y - 27, x -  1, y - 27);
	boxfill8 (vram, x, COL8_C6C6C6,  0,     y - 26, x -  1, y -  1);

	boxfill8 (vram, x, COL8_FFFFFF,  3,     y - 24, 59,     y - 24);
	boxfill8 (vram, x, COL8_FFFFFF,  2,     y - 24,  2,     y -  4);
	boxfill8 (vram, x, COL8_848484,  3,     y -  4, 59,     y -  4);
	boxfill8 (vram, x, COL8_848484, 59,     y - 23, 59,     y -  5);
	boxfill8 (vram, x, COL8_000000,  2,     y -  3, 59,     y -  3);
	boxfill8 (vram, x, COL8_000000, 60,     y - 24, 60,     y -  3);

	boxfill8 (vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
	boxfill8 (vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
	boxfill8 (vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
	boxfill8 (vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);
	return;
}

void putfont8 (char *vram, int xsize, int x, int y, char c, char *font)
{
	int i;
	char *p, d;	// data
	for (i = 0; i < 16; i++)	//每个像素点确定有无
	{
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}

void putfonts8_asc (char *vram, int xsize, int x, int y, char c, unsigned char *s)	//重复调用putfont8
{
	extern char hankaku[4096];
	for (; *s != 0x00; s++)	//利用字符串结尾的0x00确定字符输出结束
	{
	  	putfont8 (vram, xsize, x, y, c, hankaku + *s * 16);	//乘16的原因是每个字符共使用16个8位数
		x += 8;	//每次x往后移8位，即每个字轮流输出
	}
	return;
}
