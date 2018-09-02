// bootpack

#include "bootpack.h"
#include <stdio.h>

#define MEMMAN_FREES		4090	// 大约32KB
#define MEMMAN_ADDR		0x003c0000

struct FREEINFO
{
  	unsigned int addr, size;
};

struct MEMMAN
{
  	int frees, maxfrees, lostsize, losts;
  	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest (unsigned int start, unsigned int end);	// 内存测试
void memman_init (struct MEMMAN *man);
unsigned int memman_total (struct MEMMAN *man);
unsigned int memman_alloc (struct MEMMAN *man, unsigned int size);
int memman_free (struct MEMMAN *man, unsigned int addr, unsigned int size);

void HariMain (void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;

	unsigned int memtotal;
	
	struct MOUSE_DEC mdec;		// 鼠标信号缓冲及区分

	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	init_gdtidt ();
	init_pic ();
	io_sti ();

	fifo8_init (&keyfifo, 32, keybuf);		// 键盘缓冲区初始化
	fifo8_init (&mousefifo, 128, mousebuf);	// 鼠标缓冲区初始化
	io_out8 (PIC0_IMR, 0xf9);
	io_out8 (PIC1_IMR, 0xef);

	init_keyboard ();				// 初始化键盘控制电路
	enable_mouse (&mdec);			// 打开鼠标

	memtotal = memtest (0x00400000, 0xbfffffff);
	memman_init (memman);
	memman_free (memman, 0x00001000, 0x0009e000);
	memman_free (memman, 0x00400000, memtotal - 0x00400000);
		
	init_palette ();
	init_screen8 (binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;		    	// 找到画面中点
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8 (mcursor, COL8_008484);
	putblock8_8 (binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf (s, "(%d, %d)", mx, my);
	putfonts8_asc (binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	sprintf (s, "memory %dMB  free : %dKB", memtotal / (1024 * 1024), memman_total (memman) / 1024);
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

void memman_init (struct MEMMAN *man)
{
  	man->frees = 0;		// 可用信息总数
	man->maxfrees = 0;		// 用于观察可用状况：frees的最大值
	man->lostsize = 0;		// 释放失败的内存的大小总和
	man->losts = 0;		// 释放失败总数
	return;
}

unsigned int memman_total (struct MEMMAN *man)		// 报告空余内存大小的合计
{
  	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++)
	{
	  	t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc (struct MEMMAN *man, unsigned int size)	// 内存分配
{
  	unsigned int i, a;
	for(i = 0; i < man->frees; i++)
	{
	  	if (man ->free[i].size >= size)		// 找到足够大的内存
		{
		  	a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0)	// 如果free[i]变成了0，就减掉一条可用信息
			{
			  	man->frees--;
				for (; i < man->frees; i++)
				{
				  	man->free[i] = man->free[i + 1];	// 带入结构体
				}
			}
			return a;
		}
	}
	return 0;
}

int memman_free (struct MEMMAN *man, unsigned int addr, unsigned int size)
{
  	int i, j;
	// 为了便于归纳内存，将free[]按照addr的顺序排列
	// 所以，先决定应该放在哪里
	for (i = 0; i < man->frees; i++)
	{
	  	if (man->free[i].addr > addr)		// 找到程序free的位置，free[i - 1].addr<addr<free[i].addr
		  	break;
	}
	if (i > 0)		// 前面有可用内存，因为前面的free[i]存在，可能连续也可能中间断开
	{
	  	if (man->free[i - 1].addr + man->free[i - 1].size == addr)		// 可与前面可用内存归纳在一起
		{
		  	man->free[i -1].size += size;
			if (i < man ->frees)		// 后面也有
			{
			  	if (addr + size == man->free[i].addr)		// 可与后面可用内存归纳在一起
				{
				  	man->free[i -1].size += man->free[i].size;		// 顺便把前面合并的与后面一段合并起来
					man->frees--;		// 减少一个可用工况
					for (; i < man->frees; i++)
					{
					  	man->free[i] = man->free[i + 1];		// 往前移一位
					}
				}
			}
			return 0;
		}
	}
	if (i < man->frees)		// 不能与前面的可用空间归纳在一起
	{
	  	if (addr + size == man->free[i].addr)		// 可与后面归纳在一起
		{
		  	man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;
		}
	}
	if (man->frees < MEMMAN_FREES)		// 无法与前后归纳在一起
	{
	  	for (j = man->frees; j > i; j--)
		{
		  man->free[j] = man->free[j - 1];		// 因为程序段在i前面，把j移到i前面一位腾出j这一位，形成新空余段
		}
		man->frees++;	// 空余段增加
		if (man->maxfrees < man->frees)
		{
		  	man->maxfrees = man->frees;	// 更新最大值
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;
	}
	// 无法后移
	man->losts++;
	man->lostsize += size;
	return -1;
}
