#include "bootpack.h"

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

unsigned int memman_alloc_4k(struct MEMMAN *man,unsigned int size){
	unsigned int a;
	size=(size+0xfff)&0xfffff000;
	a=memman_alloc(man,size);
	return a;
}

int memman_free_4k(struct MEMMAN *man,unsigned int addr,unsigned int size){
	int i;
	size=(size+0xfff)&0xfffff000;
	i=memman_free(man,addr,size);
	return i;
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
