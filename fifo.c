// FIFO设定文件

#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

void fifo8_init (struct FIFO8 *fifo, int size, unsigned char *buf)
{
  	fifo->size = size;
	fifo->buf = buf;	// 为其指定一个地址空间
	fifo->free = size;
	fifo->flags = 0;
	fifo->p = 0;
	fifo->q = 0;

	return;
}

int fifo8_put (struct FIFO8 *fifo, unsigned char data)
{
  	if (fifo->free == 0)
	{
	  	fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p] = data;	// 置入缓冲区
	fifo->p++;
	if (fifo->p == fifo->size)
	{
	  	fifo->p = 0;	// 循环
	}
	fifo->free--;	// free从size开始减，每次中断减一次，直到溢出
	return 0;
}

int fifo8_get (struct FIFO8 *fifo)
{
  	int data;
	if (fifo->free == fifo->size)
	{
	  	return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	if (fifo->q == fifo->size)
	{
	  	fifo->q = 0;
	}
	fifo->free++;	// 释放空间
	return data;	// 返回数据
}

int fifo8_status (struct FIFO8 *fifo)
{
	return fifo->size - fifo->free;
}
