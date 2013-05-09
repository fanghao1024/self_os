/* 告诉C编译器，有一个函数在别的文件里 */

#include<stdio.h>
#include "bootpack.h"

#define EFLAGS_AC_BIT 	0x00040000
#define CR0_CACHE_DISABLE	0x60000000

#define MEMMAN_ADDR 0x003c0000
#define MEMMAN_FREES	4090 //大约是32KB

unsigned int memtest(unsigned int start,unsigned int end);



struct FREEINFO{	//可用信息
	unsigned int addr,size;
	
};

struct	MEMMAN{
	int frees,maxfrees,lostsize,losts;
	struct FREEINFO free[MEMMAN_FREES];
};

void memman_init(struct MEMMAN *man){
	man->frees=0;	//可用信息数目
	man->maxfrees;	//用于观察可用状况：frees的最大值
	man->lostsize=0;	//释放失败的内存的大小总和
	man->losts=0;	//释放失败次数
	return ;
}

//返回空余内存总大小
unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i,t=0;
	for(i=0;i<man->frees;i++)
	{
		t+=man->free[i].size;
	}
	return t;
}

//分配内存
unsigned int memman_alloc(struct MEMMAN *man,unsigned int size)
{
	unsigned int i,a;
	for(i=0;i<man->frees;i++){
		if(man->free[i].size>size){
			//找到足够大的内存
			a=man->free[i].addr;
			man->free[i].addr+=size;
			man->free[i].size-=size;
			if(man->free[i].size==0){
				//如果free[i]变成了0，就减掉一条可用信息
				man->frees--;
				for(;i<man->frees;i++)
				{
					man->free[i]=man->free[i+1];
				}
			}
			return a;
		}
	}
	return 0;
}

//释放内存
int memman_free(struct MEMMAN *man,unsigned int addr,unsigned int size)
{
	int i,j;
	//为便于归纳内存，讲free[]按照addr的顺序排列
	//所以，先决定应该放在哪里
	for(i=0;i<man->frees;i++)
	{
		if(man->free[i].addr>addr)
			break;
	}
	//free[i-1].addr<addr<free[i].addr
	if(i>0){
	if(man->free[i-1].addr+man->free[i-1].size==addr){
		//可以与前面的可用内存归纳到一起
		man->free[i-1].size+=size;
		if(i<man->frees){
			if(addr+size==man->free[i].addr){
				man->free[i-1].size+=man->free[i].size;
				man->frees--;
				for(;i<man->frees;i++){
					man->free[i]=man->free[i+1];
					}
				}
			}
		return 0;
		}	
	}
	if(i<man->frees){
		if(addr+size==man->free[i].addr){
			man->free[i].addr=addr;
			man->free[i].size+=size;
			return 0;
		}
	}
	
	if(man->frees<MEMMAN_FREES){
		//free[i]之后，向后移动，腾出一点空间
		for(j=man->frees;j>i;j--){
			man->free[j]=man->free[j-1];
		}
		man->frees++;
		if(man->maxfrees<man->frees){
			man->maxfrees=man->frees;
		}
		man->free[i].addr=addr;
		man->free[i].size=size;
		return 0;
	}
	man->losts++;
	man->lostsize+=size;
	return -1;
}

void HariMain(void)
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	char s[40],mcursor[256],keybuf[32],mousebuf[128];
	int mx,my,i;
	struct MOUSE_DEC mdec;
	
	unsigned int memtotal;
	struct MEMMAN *memman=(struct MEMMAN *)MEMMAN_ADDR;

	init_gdtidt();
	init_pic();
	io_sti();
	fifo8_init(&keyfifo,32,keybuf);
	fifo8_init(&mousefifo,128,mousebuf);
	io_out8(PIC0_IMR,0xf9);
	io_out8(PIC1_IMR,0xef);
	
	init_keyboard();
	
	memtotal=memtest(0x00400000,0xbfffffff);
	memman_init(memman);
	memman_free(memman,0x00001000,0x0009e000);
	memman_free(memman,0x00400000,memtotal-0x00400000);
	
	init_palette();//设定调色板
	init_screen8(binfo->vram,binfo->scrnx,binfo->scrny);
	mx = (binfo->scrnx - 16) / 2; 
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor,COL8_008484);
	putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	enable_mouse(&mdec);
	
	sprintf(s, "memory %dMB   free : %dKB",
			memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

	for(;;){
	io_cli(); /* 屏蔽中断 */
	if(fifo8_status(&keyfifo)+fifo8_status(&mousefifo)==0){
			io_stihlt();
		}else{
			if(fifo8_status(&keyfifo)!=0){
				i=fifo8_get(&keyfifo);
				io_sti();
				sprintf(s,"%02X",i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			}else if(fifo8_status(&mousefifo)!=0){
				i = fifo8_get(&mousefifo);
				io_sti();
				if(mouse_decode(&mdec,i)!=0){
					sprintf(s, "[lcr %4d %4d]", mdec.x,mdec.y);
					if((mdec.btn&0x01)!=0){
						s[1]='L';
					}
					if((mdec.btn&0x02)!=0){
						s[3]='R';
					}	
					if((mdec.btn&0x04)!=0){
						s[2]='C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32+15*8-1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					/*鼠标指针的移动*/
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,mx,my,mx+15,my+15);//隐藏鼠标
					mx+=mdec.x;
					my+=mdec.y;
					if(mx<0){
						mx=0;
					}
					if(my<0){
						my=0;
					}
					if(mx>binfo->scrnx-16){
						mx=binfo->scrnx-16;
					}
					if(my>binfo->scrnx-16){
						my=binfo->scrnx-16;
					}
					sprintf(s,"(%3d,%3d)",mx,my);
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,0,0,79,15);//隐藏坐标
					putfonts8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);//显示坐标
					putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);//描画鼠标
				}
				
			}
		}	
	}
}


unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486=0;
	unsigned int eflg,cr0,i;
	//确认CPU是386还是486以上的
	eflg=io_load_eflags();
	eflg|=EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	eflg=io_load_eflags();
	if((eflg&EFLAGS_AC_BIT)!=0){//如果是386，即使设定AC=1，AC的值还会自动回到0
		flg486=1;
	}
	eflg&=~EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	
	if(flg486!=0){
		cr0=load_cr0();
		cr0|=CR0_CACHE_DISABLE;//禁止缓存
		store_cr0(cr0);
	}
	
	i=memtest_sub(start,end);
	
	if(flg486!=0){
		cr0=load_cr0();
		cr0&=~CR0_CACHE_DISABLE;//禁止缓存
		store_cr0(cr0);
	}
	
	return i;
}

