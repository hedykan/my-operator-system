// bootpack

#include "bootpack.h"
#include <stdio.h>

#define MEMMAN_FREES		4090	// ��Լ32KB
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

unsigned int memtest (unsigned int start, unsigned int end);	// �ڴ����
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
	
	struct MOUSE_DEC mdec;		// ����źŻ��弰����

	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	init_gdtidt ();
	init_pic ();
	io_sti ();

	fifo8_init (&keyfifo, 32, keybuf);		// ���̻�������ʼ��
	fifo8_init (&mousefifo, 128, mousebuf);	// ��껺������ʼ��
	io_out8 (PIC0_IMR, 0xf9);
	io_out8 (PIC1_IMR, 0xef);

	init_keyboard ();				// ��ʼ�����̿��Ƶ�·
	enable_mouse (&mdec);			// �����

	memtotal = memtest (0x00400000, 0xbfffffff);
	memman_init (memman);
	memman_free (memman, 0x00001000, 0x0009e000);
	memman_free (memman, 0x00400000, memtotal - 0x00400000);
		
	init_palette ();
	init_screen8 (binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;		    	// �ҵ������е�
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
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)	// ����keyfifo��״̬��ȷ���Ƿ�����
		{
		  	io_stihlt ();
		}
		else
		{
		  	if (fifo8_status(&keyfifo) != 0)		// ���̻��������Ϊ��0
			{
			  	i = fifo8_get (&keyfifo);
				io_sti ();
				sprintf (s, "%02X", i);
				boxfill8 (binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15,31);
				putfonts8_asc (binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			}
			else if (fifo8_status(&mousefifo) != 0)	// ��껺�������Ϊ��0
			{
			  	i = fifo8_get (&mousefifo);
				io_sti ();

				if (mouse_decode (&mdec, i) != 0)
				{
				  	sprintf (s, "[lcr, %4d, %4d]", mdec.x, mdec.y);	// �����medc����ָ�룬���Բ�����->
					if ((mdec.btn & 0x01) !=0)	// �������������º�����ַ����滻
					  	s[1] = 'L';
					if ((mdec.btn & 0x02) != 0)
					  	s[3] = 'R';
					if ((mdec.btn & 0x04) != 0)
					  	s[2] = 'C';
					boxfill8 (binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 17 * 8 - 1, 31);
					//Ҫע��ˢ�µ������Ƕ���
					putfonts8_asc (binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

					boxfill8 (binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
					// ˢ��ԭ���������
					mx += mdec.x;
					my += mdec.y;
					// ��������ƶ���Χ
					if (mx < 0)
						mx = 0;
					if (my < 0)
					  	my = 0;
					if (mx > binfo->scrnx - 16)
					  	mx = binfo->scrnx - 16;
					if (my > binfo->scrny - 16)
					  	my = binfo->scrny - 16;
					sprintf (s, "(%3d, %3d)", mx, my);
					// ���»������
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

	// ���cpu��386����486ģʽ
	eflg = io_load_eflags ();	// ȡ��eflg��ֵ
	eflg |= EFLAGS_AC_BIT;	// ��18λAC-bit = 1 486��18λ��ACλ��386û�У���ͨ������ж��Ƿ�Ϊ486
	io_store_eflags (eflg);
	eflg = io_load_eflags ();
	if ((eflg & EFLAGS_AC_BIT) != 0)	// 386û��ACλ�����Լ�ʹ����λҲ����0��������0
	{
	  	flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT;	// ȡ�������룬��ACλΪ0
	io_store_eflags (eflg);

	if (flg486 != 0)
	{
	  	cr0 = load_cr0 ();
		cr0 |= CR0_CACHE_DISABLE;	//�ص�����
		store_cr0 (cr0);
	}

	i = memtest_sub (start, end);		// �����С

	if (flg486 != 0)
	{
	  	cr0 = load_cr0 ();
		cr0 &= ~CR0_CACHE_DISABLE;	// �򿪻���
		store_cr0 (cr0);
	}

	return i;
}

void memman_init (struct MEMMAN *man)
{
  	man->frees = 0;		// ������Ϣ����
	man->maxfrees = 0;		// ���ڹ۲����״����frees�����ֵ
	man->lostsize = 0;		// �ͷ�ʧ�ܵ��ڴ�Ĵ�С�ܺ�
	man->losts = 0;		// �ͷ�ʧ������
	return;
}

unsigned int memman_total (struct MEMMAN *man)		// ��������ڴ��С�ĺϼ�
{
  	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++)
	{
	  	t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc (struct MEMMAN *man, unsigned int size)	// �ڴ����
{
  	unsigned int i, a;
	for(i = 0; i < man->frees; i++)
	{
	  	if (man ->free[i].size >= size)		// �ҵ��㹻����ڴ�
		{
		  	a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0)	// ���free[i]�����0���ͼ���һ��������Ϣ
			{
			  	man->frees--;
				for (; i < man->frees; i++)
				{
				  	man->free[i] = man->free[i + 1];	// ����ṹ��
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
	// Ϊ�˱��ڹ����ڴ棬��free[]����addr��˳������
	// ���ԣ��Ⱦ���Ӧ�÷�������
	for (i = 0; i < man->frees; i++)
	{
	  	if (man->free[i].addr > addr)		// �ҵ�����free��λ�ã�free[i - 1].addr<addr<free[i].addr
		  	break;
	}
	if (i > 0)		// ǰ���п����ڴ棬��Ϊǰ���free[i]���ڣ���������Ҳ�����м�Ͽ�
	{
	  	if (man->free[i - 1].addr + man->free[i - 1].size == addr)		// ����ǰ������ڴ������һ��
		{
		  	man->free[i -1].size += size;
			if (i < man ->frees)		// ����Ҳ��
			{
			  	if (addr + size == man->free[i].addr)		// �����������ڴ������һ��
				{
				  	man->free[i -1].size += man->free[i].size;		// ˳���ǰ��ϲ��������һ�κϲ�����
					man->frees--;		// ����һ�����ù���
					for (; i < man->frees; i++)
					{
					  	man->free[i] = man->free[i + 1];		// ��ǰ��һλ
					}
				}
			}
			return 0;
		}
	}
	if (i < man->frees)		// ������ǰ��Ŀ��ÿռ������һ��
	{
	  	if (addr + size == man->free[i].addr)		// ������������һ��
		{
		  	man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;
		}
	}
	if (man->frees < MEMMAN_FREES)		// �޷���ǰ�������һ��
	{
	  	for (j = man->frees; j > i; j--)
		{
		  man->free[j] = man->free[j - 1];		// ��Ϊ�������iǰ�棬��j�Ƶ�iǰ��һλ�ڳ�j��һλ���γ��¿����
		}
		man->frees++;	// ���������
		if (man->maxfrees < man->frees)
		{
		  	man->maxfrees = man->frees;	// �������ֵ
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;
	}
	// �޷�����
	man->losts++;
	man->lostsize += size;
	return -1;
}
