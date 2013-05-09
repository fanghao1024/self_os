/* 告诉C编译器，有一个函数在别的文件里 */

#include<stdio.h>
#include "bootpack.h"

void HariMain(void)
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	char s[40],mcursor[256];
	int mx,my;
	
	init_gdtidt();
	init_palette();//设定调色板
	init_screen8(binfo->vram,binfo->scrnx,binfo->scrny);
	mx = (binfo->scrnx - 16) / 2; 
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor,COL8_008484);
	putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	for(;;){
	io_hlt(); /* 执行naskfunc.nas里的_io_hlt */
	}
}

