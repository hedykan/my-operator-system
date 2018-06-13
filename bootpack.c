// bootpack

#include "bootpack.h"
#include <stdio.h>

struct MOUSE_DEC
{
  	unsigned char buf[3], phase;
};

extern struct FIFO8 keyfifo, mousefifo;					// 在int.c里定义的keybuf
void enable_mouse (struct MOUSE_DEC *mdec);			// 打开鼠标接收
void init_keyboard (void);						// 初始化键盘电路
int mouse_decode (struct MOUSE_DEC *mdec, unsigned char dat);	// 鼠标输入分割

void HariMain (void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;		// 鼠标信号缓冲及区分

	init_gdtidt ();
	init_pic ();
	io_sti ();

	fifo8_init (&keyfifo, 32, keybuf);		// 键盘缓冲区初始化
	fifo8_init (&mousefifo, 128, mousebuf);	// 鼠标缓冲区初始化
	io_out8 (PIC0_IMR, 0xf9);
	io_out8 (PIC1_IMR, 0xef);

	init_keyboard ();				// 初始化键盘控制电路
		
	init_palette ();
	init_screen8 (binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;		    	// 找到画面中点
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8 (mcursor, COL8_008484);
	putblock8_8 (binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf (s, "(%d, %d)", mx, my);
	putfonts8_asc (binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	enable_mouse (&mdec);				// 打开鼠标

	for (;;)
	{
		io_cli ();
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)	// 返回keyfifo的状态以确定是否运行
		{
		  	io_stihlt ();
		}
		else
		{
		  	if (fifo8_status(&keyfifo) != 0)		// 键盘缓冲区检测为非0
			{
			  	i = fifo8_get (&keyfifo);
				io_sti ();
				sprintf (s, "%02X", i);
				boxfill8 (binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15,31);
				putfonts8_asc (binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			}
			else if (fifo8_status(&mousefifo) != 0)	// 鼠标缓冲区检测为非0
			{
			  	i = fifo8_get (&mousefifo);
				io_sti ();

				if (mouse_decode (&mdec, i) != 0)
				{
				  	sprintf (s, "%02X, %02X, %02X", mdec.buf[0], mdec.buf[1], mdec.buf[2]);	// 这里的medc不是指针，所以不能用->
					boxfill8 (binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 10 * 8 - 1, 31);	//要注意刷新的区域是多少
					putfonts8_asc (binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
				}
			}
		}
	}
 }

#define PORT_KEYDAT		0x0060
#define PORT_KEYSTA			0x0064
#define PORT_KEYCMD		0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE	0x60
#define KBC_MODE			0x47

void wait_KBC_sendready (void)		// 等待键盘完成
{
  	for (;;)
	{
	  	if ((io_in8 (PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
		{
		  	break;
		}
	}
	return;
}

void init_keyboard (void)
{
  	wait_KBC_sendready ();
	io_out8 (PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready ();
	io_out8 (PORT_KEYDAT, KBC_MODE);

	return;
}

#define KEYCMD_SENDTO_MOUSE	0xd4
#define MOUSECMD_ENABLE		0xf4

void enable_mouse (struct MOUSE_DEC *mdec)
{
  	wait_KBC_sendready ();
	io_out8 (PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready ();
	io_out8 (PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0;		// 开始接收数据

	return;
}

int mouse_decode (struct MOUSE_DEC *mdec, unsigned char dat)
{
  	if (mdec->phase == 0)
	{
	  	if (dat == 0xfa)       	// 鼠标接收开始
		{
		  	mdec->phase = 1;
		}
		return 0;
	}
	else if (mdec->phase == 1)	// 鼠标数据分割
	{
	    	mdec->buf[0] = dat;
		mdec->phase = 2;
		return 0;
	}
	else if (mdec->phase == 2)
	{
	    	mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	else if (mdec->phase == 3)
	{
	    	mdec->buf[2] = dat;
		mdec->phase = 1;	// 重复接收的循环
		return 1;		// 接收完毕，允许输出
	}
	return -1;
}
