#include "bootpack.h"

//初始化GDT、IDT
void init_gdtidt(void)
{
	struct SEGMENT_DESCRIPTOR *gdt=(struct SEGMENT_DESCRIPTOR *)0x00270000;
	struct GATE_DESCRIPTOR *idt=(struct GATE_DESCRIPTOR *)0x0026f800;
	int i;
	
	/*GDT的初始化*/
	for(i=0;i<8192;i++){
		set_segmdesc(gdt+i,0,0,0);
	}
	set_segmdesc(gdt+1,0xffffffff,0x00000000,AR_DATA32_RW);
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
	load_gdtr(LIMIT_GDT, ADR_GDT);
	
	/*IDT的初始化*/
	for(i=0;i<256;i++){
		set_gatedesc(idt+i,0,0,0);
	}
	load_idtr(LIMIT_IDT, ADR_IDT);
	
	/*IDT的设定*/
	set_gatedesc(idt+0x21,(int)asm_inthandler21,2*8,AR_INTGATE32);
	set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt+0x2c,(int)asm_inthandler2c,2*8,AR_INTGATE32);
	
	return ;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd,unsigned int limit, int base, int ar)
{
	if(limit>0xffffff){
		ar|=0x8000;
		limit/=0x1000;
	}
	sd->limit_low=limit&0xffff;
	sd->base_low=base&0xffff;
	sd->base_mid=(base>>16)&0xff;
	sd->access_right=ar&0xff;
	sd->limit_high=((limit>>16)&0x0f)|((ar>>8)&0xf0);
	sd->base_high=(base>>24)&0xff;
	return ;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd,int offset,int selector,int ar)
{
	gd->offset_low=offset&0xffff;
	gd->selector=selector;
	gd->dw_count=(ar>>8)&0xff;
	gd->access_right=ar&0xff;
	gd->offset_high=(offset>>16)&0xffff;
	return;
}