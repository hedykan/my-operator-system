//GDT和IDT设置

#include "bootpack.h"

void init_gdtidt (void)
{
  	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT; //全局段号记录表数据的起始地址
	struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *) ADR_IDT; //中断记录表的起始地址
	int i;

	//GDT初始化
	for (i = 0; i <= LIMIT_GDT / 8; i++)
	{
	  	set_segmdesc (gdt + i, 0, 0, 0); //全部初始化为未使用的记录表
	}
	set_segmdesc (gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW); //设置了个系统读写段
	set_segmdesc (gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER); //设置了个系统执行段
	load_gdtr (LIMIT_GDT, ADR_GDT);

	//IDT初始化
	for (i = 0; i <= LIMIT_IDT / 8; i++);
	{
	  	set_gatedesc (idt + i, 0, 0, 0);
	}
	load_idtr (LIMIT_IDT, ADR_IDT);

	//IDT设置
	set_gatedesc (idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
	set_gatedesc (idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc (idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);

	return;
}

void set_segmdesc (struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar) //往gdt中放数据
{
  	if (limit > 0xfffff)
	{
	  	ar |= 0x8000; //G_bit = 1,页模式打开, 则limit一个数表示4KB的空间, 因为limit只有20位
		limit /= 0x1000;
	}
	sd->limit_low    = limit & 0xffff; //把limit中低16位放入limit_low中
	sd->base_low     = base & 0xffff; //把输入的基址base中的低16位输入到base_low中
	sd->base_mid     = (base >> 16) & 0xff; //base高16位中的低8位输入到base_mid中
	sd->access_right = ar & 0xff; //把系统模式或应用程序模式写入access_right中
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0); //把ar扩展访问权和limit高4位合并得到共8位放入limit_high中
	sd->base_high    = (base >> 24) & 0xff; //高8位输入到base_high中
	
	return;
}

void set_gatedesc (struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	
	return;
}
