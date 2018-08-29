// bootpack

#include "bootpack.h"
#include <stdio.h>

unsigned int memtest (unsigned int start, unsigned int end);	// 内存测试

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
	enable_mouse (&mdec);			// 打开鼠标
		
	init_palette ();
	init_screen8 (binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;		    	// 找到画面中点
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8 (mcursor, COL8_008484);
	putblock8_8 (binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf (s, "(%d, %d)", mx, my);
	putfonts8_asc (binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	i = memtest (0x00400000, 0xbfffffff) / (1024 * 1024);
	sprintf (s, "memory %dMB", i);
	putfonts8_asc (binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

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
				  	sprintf (s, "[lcr, %4d, %4d]", mdec.x, mdec.y);	// 这里的medc不是指针，所以不能用->
					if ((mdec.btn & 0x01) !=0)	// 三个按键被按下后进行字符的替换
					  	s[1] = 'L';
					if ((mdec.btn & 0x02) != 0)
					  	s[3] = 'R';
					if ((mdec.btn & 0x04) != 0)
					  	s[2] = 'C';
					boxfill8 (binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 17 * 8 - 1, 31);
					//要注意刷新的区域是多少
					putfonts8_asc (binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

					boxfill8 (binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
					// 刷新原有鼠标区域
					mx += mdec.x;
					my += mdec.y;
					// 限制鼠标移动范围
					if (mx < 0)
						mx = 0;
					if (my < 0)
					  	my = 0;
					if (mx > binfo->scrnx - 16)
					  	mx = binfo->scrnx - 16;
					if (my > binfo->scrny - 16)
					  	my = binfo->scrny - 16;
					sprintf (s, "(%3d, %3d)", mx, my);
					// 重新绘制鼠标
					boxfill8 (binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					putfonts8_asc (binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
					putblock8_8 (binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
				}
			}
		}
	}
 }

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE		0x60000000

unsigned int memtest (unsigned int start, unsigned int end)
{
  	char flg486 = 0;
	unsigned int eflg, cr0, i;

	// 检测cpu是386还是486模式
	eflg = io_load_eflags ();	// 取得eflg的值
	eflg |= EFLAGS_AC_BIT;	// 置18位AC-bit = 1 486第18位是AC位，386没有，可通过这个判断是否为486
	io_store_eflags (eflg);
	eflg = io_load_eflags ();
	if ((eflg & EFLAGS_AC_BIT) != 0)	// 386没有AC位，所以即使置了位也还是0，与后等于0
	{
	  	flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT;	// 取反后相与，置AC位为0
	io_store_eflags (eflg);

	if (flg486 != 0)
	{
	  	cr0 = load_cr0 ();
		cr0 |= CR0_CACHE_DISABLE;	//关掉缓存
		store_cr0 (cr0);
	}

	i = memtest_sub (start, end);		// 算出大小

	if (flg486 != 0)
	{
	  	cr0 = load_cr0 ();
		cr0 &= ~CR0_CACHE_DISABLE;	// 打开缓存
		store_cr0 (cr0);
	}

	return i;
}
