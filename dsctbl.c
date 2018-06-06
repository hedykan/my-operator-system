//GDT��IDT����

#include "bootpack.h"

void init_gdtidt (void)
{
  	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT; //ȫ�ֶκż�¼�����ݵ���ʼ��ַ
	struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *) ADR_IDT; //�жϼ�¼�����ʼ��ַ
	int i;

	//GDT��ʼ��
	for (i = 0; i <= LIMIT_GDT / 8; i++)
	{
	  	set_segmdesc (gdt + i, 0, 0, 0); //ȫ����ʼ��Ϊδʹ�õļ�¼��
	}
	set_segmdesc (gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW); //�����˸�ϵͳ��д��
	set_segmdesc (gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER); //�����˸�ϵͳִ�ж�
	load_gdtr (LIMIT_GDT, ADR_GDT);

	//IDT��ʼ��
	for (i = 0; i <= LIMIT_IDT / 8; i++);
	{
	  	set_gatedesc (idt + i, 0, 0, 0);
	}
	load_idtr (LIMIT_IDT, ADR_IDT);

	//IDT����
	set_gatedesc (idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
	set_gatedesc (idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc (idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);

	return;
}

void set_segmdesc (struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar) //��gdt�з�����
{
  	if (limit > 0xfffff)
	{
	  	ar |= 0x8000; //G_bit = 1,ҳģʽ��, ��limitһ������ʾ4KB�Ŀռ�, ��Ϊlimitֻ��20λ
		limit /= 0x1000;
	}
	sd->limit_low    = limit & 0xffff; //��limit�е�16λ����limit_low��
	sd->base_low     = base & 0xffff; //������Ļ�ַbase�еĵ�16λ���뵽base_low��
	sd->base_mid     = (base >> 16) & 0xff; //base��16λ�еĵ�8λ���뵽base_mid��
	sd->access_right = ar & 0xff; //��ϵͳģʽ��Ӧ�ó���ģʽд��access_right��
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0); //��ar��չ����Ȩ��limit��4λ�ϲ��õ���8λ����limit_high��
	sd->base_high    = (base >> 24) & 0xff; //��8λ���뵽base_high��
	
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
