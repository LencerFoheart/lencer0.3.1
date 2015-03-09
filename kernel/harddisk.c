/*
 * Small/kernel/harddisk.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/* * 此文件包含硬盘读写请求处理，我们没使用DMA，而是直接操作硬盘控制器。
 *
 * 它目前还不完善，但是够用就行，先将就着吧 :-)
 */


#ifndef __DEBUG__
#define __DEBUG__
#endif


#include "harddisk.h"
#include "sched.h"
#include "sys_set.h"
#include "sys_nr.h"
#include "kernel.h"
#include "types.h"
#include "string.h"

extern int hd_int(void);


void (*do_hd)(void) = NULL;				/* 硬盘中断函数指针 */

struct hd_struct hd = {0};				/* 硬盘参数表结构 */
struct hd_request request[NR_HD_REQUEST] = {{0,},};	/* 硬盘请求项队列 */
int rear = -1, head = -1, req_count = 0;/* 硬盘请求项队列尾,头,个数 */

/*
 * 请求硬盘驱动器，即向其发送命令。NOTE! 扇区号:从1开始!!!!!! 
 * 驱动器号:0或1, 磁头号:从0开始, 柱面号:从0开始.
 */
void request_hd_drv(struct hd_request *q)
{
	if(BUSY_STAT & in_b(HD_STATE)) {
		int tmp = 10000;
		while(tmp-- && (BUSY_STAT & in_b(HD_STATE)))
			;
		if(BUSY_STAT & in_b(HD_STATE))
			panic("request_hd_drv: the controller is always busy.");
	}
	if(!(READY_STAT & in_b(HD_STATE))) {
		int tmp = 10000;
		while(tmp-- && !(READY_STAT & in_b(HD_STATE)))
			;
		if(!(READY_STAT & in_b(HD_STATE)))
			panic("request_hd_drv: the drive isn't ready.");
	}
	do_hd = (*q).int_addr;
	out_b(HD_CTRL, hd.ctrl);
	out_b(HD_PRECOMP, (hd.pre_w_cyl>>2) & 0xff);	/* 需除以4!!!! */
	out_b(HD_SECTORS, (*q).sectors & 0xff);
	out_b(HD_NRSECTOR, (*q).nrsector & 0xff);
	out_b(HD_LCYL, (*q).nrcyl & 0xff);
	out_b(HD_HCYL, ((*q).nrcyl >> 8) & 0x3);
	out_b(HD_DRV_HEAD, ((0x5<<5) | (((*q).nrdrv&0x1)<<4) | ((*q).nrhead&0xf)) & 0xff);
	out_b(HD_COMMAND, (*q).cmd & 0xff);
}

/* * 读硬盘中断函数。 */
void hd_read_int(void)
{
	if(ERR_STAT & in_b(HD_STATE)) {
		k_printf("hd_read_int: read harddisk ERROR, error code: %x.", in_b(HD_ERROR));
		return;
	}
	__asm__(
		"cld; rep; insw"
		::"c"(256),"D"(request[head].buff),"d"(HD_DATA) :
	);
	if(--(request[head].sectors)) {
		do_hd = hd_read_int;
		/* 还有扇区未读，缓冲区指针后移 */
		request[head].buff += 512;
		return;
	}
	--req_count;
	wake_up(&request[head].wait);
	do_hd_request();
}

/*
 * 写硬盘中断函数。
 */
void hd_write_int(void)
{
	if(ERR_STAT & in_b(HD_STATE)) {
		k_printf("hd_write_int: write harddisk ERROR, error code: %x.", in_b(HD_ERROR));
		return;
	}
	__asm__(
		"cld; rep; outsw"
		::"c"(256),"S"(request[head].buff),"d"(HD_DATA) :
	);
	if(--(request[head].sectors)) {
		do_hd = hd_write_int;
		request[head].buff += 512;
		return;
	}
	--req_count;
	wake_up(&request[head].wait);
	do_hd_request();
}

void hd_default_int(void)
{
	panic("hd_default_int: bad harddisk interrupt.");
}

/*
 * 插入硬盘请求项。I：buff - 缓冲区指针; cmd - 命令;
 * dev - 设备号; nr_v_sector - 逻辑扇区号; sectors - 扇区数。
 */
void insert_hd_request(unsigned char * buff, unsigned int cmd,
					unsigned char dev, unsigned int nr_v_sector,
					unsigned int sectors)
{
	d_printf("insert_hd_request: sectors=%u, nr_v_sector=%u, %s, total_sectors:%u.\n",
		sectors, nr_v_sector, (cmd==WIN_READ ? ("READ") : ("WRITE")), hd.cyls * hd.heads * hd.sectors);

	if(!buff || sectors<0 || sectors>255 || nr_v_sector<0
	|| (nr_v_sector+sectors)>=(hd.cyls * hd.heads * hd.sectors))
		panic("insert_hd_request: bad parameters.");
	if(NR_HD_REQUEST == req_count)	/* 队列满 */
		panic("insert_hd_request: the hd-request-queue is full.");
/*	cli(); */	/* 我们在此不需要cli、sti，因为内核态... */
	++req_count;
	++rear;
	rear %= NR_HD_REQUEST;
/*	sti(); */
	request[rear].wait = NULL;		/* NOTE! that's right */
	request[rear].buff = buff;
	switch(cmd) {
	case WIN_READ:
		request[rear].int_addr = hd_read_int; break;
	case WIN_WRITE:
		request[rear].int_addr = hd_write_int; break;
		/* 尚未完整 */
	default:
		panic("insert_hd_request: bad hd-cmd."); break;
	}
	request[rear].cmd = cmd;
	request[rear].sectors = sectors;
	request[rear].nrdrv = 0;	/* 默认为0 */
	request[rear].nrsector = (nr_v_sector % hd.sectors) + 1;/* 扇区号:从1开始!!!!!! */
	request[rear].nrcyl = (nr_v_sector / hd.sectors) / hd.heads;
	request[rear].nrhead = (nr_v_sector / hd.sectors) % hd.heads;

	/*
	 * 若插入之前，没有请求项，则调用do_hd_request，执行硬盘请求。
	 * 在执行硬盘命令之前关闭中断，执行硬盘命令之后将当前进程睡眠，
	 * 然后当当前进程被唤醒时开启中断。NOTE! 当前进程开关中断并不会
	 * 影响到其他进程，因为每个进程都有自己的tss，硬盘控制器中断请求
	 * 可以在当其他进程运行时响应。
	 *
	 * 不管有没有可运行进程，亦或是只有0进程，这里都是没有问题的，
	 * 因为0进程不会睡觉，因此在这些情况下，它总可以响应中断。
	 */
	if(1 == req_count) {
        cli();
        do_hd_request();
		sleep_on(&request[rear].wait, INTERRUPTIBLE);
		sti();
	} else {
        sleep_on(&request[rear].wait, INTERRUPTIBLE);
	}
}

/*
 * 执行硬盘请求。
 */
void do_hd_request(void)
{
	if(req_count) {
		++head;
		head %= NR_HD_REQUEST;
		request_hd_drv(&request[head]);
	}
}

void hd_init(void)
{
	do_hd = hd_default_int;

	/*
	 * NOTE! 由于字节对齐，以下语句是错误的!!! 应该一个一个赋值。正是由
	 * 于此处错误，导致硬盘读取错误!!! 因为会导致写前预补偿柱面号错误。
	 */
	/* memcpy(&hd, (char *)0x90004, sizeof(struct hd_struct)); */
	
	d_printf("sizeof(struct hd_struct) = %d\n", sizeof(struct hd_struct));
	hd.cyls				= *((unsigned short*)(0x90004 + 0));
	hd.heads			= *((unsigned char*)(0x90004 + 2));
	hd.w_small_elec_cyl = *((unsigned short*)(0x90004 + 3));
	hd.pre_w_cyl		= *((unsigned short*)(0x90004 + 5));
	hd.max_ECC			= *((unsigned char*)(0x90004 + 7));
	hd.ctrl				= *((unsigned char*)(0x90004 + 8));
	hd.n_time_limit		= *((unsigned char*)(0x90004 + 9));
	hd.f_time_limit		= *((unsigned char*)(0x90004 + 10));
	hd.c_time_limit		= *((unsigned char*)(0x90004 + 11));
	hd.head_on_cyl		= *((unsigned short*)(0x90004 + 12));
	hd.sectors			= *((unsigned char*)(0x90004 + 14));
	hd.none				= *((unsigned char*)(0x90004 + 15));
	d_printf(" hd.ctrl=%u\n hd.pre_w_cyl=%u\n hd.cyls=%u\n hd.heads=%u\n hd.sectors=%u\n",
		hd.ctrl, hd.pre_w_cyl, hd.cyls, hd.heads, hd.sectors);

	set_idt(INT_R0, hd_int, NR_HD_INT);
}
