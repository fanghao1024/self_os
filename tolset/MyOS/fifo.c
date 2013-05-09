#include "bootpack.h"

#define FLAGS_OVERRUN 0x0001 //溢出标志

/*初始化FIFO缓冲区*/
void fifo8_init(struct FIFO8 *fifo,int size,unsigned char *buf)
{
	fifo->size=size;
	fifo->free=size;
	fifo->buf=buf;
	fifo->flags=0;
	fifo->p; //下一个数据读入位置
	fifo->q; //下一个数据写入位置
	return ;
}

/*向FIFO写入数据并缓存*/
int fifo8_put(struct FIFO8 *fifo,unsigned char data)
{
	if(fifo->free==0)//缓冲区已填满
	{
		fifo->flags|=FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p]=data;
	fifo->p++;
	if(fifo->p==fifo->size)
		fifo->p=0;
	fifo->free--;
	return 0;
}

/*从FIFO获得一个数据*/
int fifo8_get(struct FIFO8 *fifo)
{
	int data;
	if(fifo->free==fifo->size)
	return -1;
	data=fifo->buf[fifo->q];
	fifo->free++;
	fifo->q++;
	if(fifo->q==fifo->size)
		fifo->q=0;
	return data;
}

/*返回缓冲区中的数据量*/
int fifo8_status(struct FIFO8 *fifo)
{
	return fifo->size-fifo->free;
}