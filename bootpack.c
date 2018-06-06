// bootpack

#include "bootpack.h"
#include <stdio.h>

extern struct KEYBUF keybuf;			// 在int.c里定义的keybuf

void HariMain (void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256];
	int mx, my, i, j;

	init_gdtidt ();
	init_pic ();
	io_sti ();
	
	io_out8 (PIC0_IMR, 0xf9);
	io_out8 (PIC1_IMR, 0xef);
		
	init_palette ();
	init_screen8 (binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;					// 找到画面中点
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8 (mcursor, COL8_008484);
	putblock8_8 (binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf (s, "(%d, %d)", mx, my);
	putfonts8_asc (binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	for (;;)
	{
		io_cli ();
		if (keybuf.next == 0)
		{
		  	io_stihlt ();
		}
		else
		{
		  	i = keybuf.data[0];
			keybuf.next--;
			for (j = 0; j < keybuf.next; j++)
			{
			  	keybuf.data[j] = keybuf.data[j + 1];	// 把keybuf中的字符循环前移
			}
			io_sti ();
			sprintf (s, "%02X", i);
			boxfill8 (binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15,31);
			putfonts8_asc (binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
		}
	}
 }
