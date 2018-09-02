#include "bootpack.h"

struct FIFO8 mousefifo;

void inthandler2c (int *esp)
{
  	unsigned char data;
	
	io_out8 (PIC1_OCW2, 0x64);
	io_out8 (PIC0_OCW2, 0x62);
	data = io_in8 (PORT_KEYDAT);
	fifo8_put (&mousefifo, data);

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
		  	mdec->phase = 1;
		return 0;
	}
	else if (mdec->phase == 1)	// 鼠标数据分割
	{
	  	if((dat & 0xc8) == 0x08)	// 确认第一个字节是鼠标按键，防止错位
		{
	    		mdec->buf[0] = dat;
			mdec->phase = 2;
		}
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

		mdec->btn = mdec->buf[0] & 0x07;		// 取鼠标按键的低三位
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0)		// 鼠标按键区高四位的低一位为一
		  	mdec->x |= 0xffffff00;
		if ((mdec->buf[0] & 0x20) != 0)		// 鼠标按键区高四位的低二位为一
		  	mdec->y |= 0xffffff00;
		mdec->y = -mdec->y;	// y是反过来的
		return 1;		// 接收完毕，允许输出
	}
	return -1;
}
