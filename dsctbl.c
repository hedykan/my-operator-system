//GDT��IDT����

#include "bootpack.h"

void init_gdtidt (void)
{
  	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) 0x00270000; //ȫ�ֶκż�¼�����ݵ���ʼ��ַ
	struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *) 0x0026f800; //�жϼ�¼�����ʼ��ַ
	int i;

	//GDT��ʼ��
	for (i = 0; i < 8192; i++)
	{
	  	set_segmdesc (gdt + i, 0, 0, 0); //ȫ����ʼ��Ϊδʹ�õļ�¼��
	}
	set_segmdesc (gdt + 1, 0xffffffff, 0x00000000, 0x4092); //�����˸�ϵͳ��д��
	set_segmdesc (gdt + 2, 0x0007ffff, 0x00280000, 0x409a); //�����˸�ϵͳִ�ж�
	load_gdtr (0xffff, 0x00270000);

	//IDT��ʼ��
	for (i = 0; i < 256; i++);
	{
	  	set_gatedesc (idt + i, 0, 0, 0);
	}
	load_idtr (0x7ff, 0x0026f800);

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
