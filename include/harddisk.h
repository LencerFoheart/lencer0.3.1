/*
 * Small/include/harddisk.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _HARD_DISK_H_
#define _HARD_DISK_H_

#include "sched.h"

/********************************************************************/

/* register for 'AT' harddisk */

#define HD_DATA		(0x1f0)		/* 数据寄存器，Read/Write */

#define HD_ERROR	(0x1f1)		/* 错误状态寄存器，R */
#define HD_PRECOMP	(0x1f1)		/* 写前预补偿寄存器，W */

#define HD_SECTORS	(0x1f2)		/* 扇区数寄存器，R/W */

#define HD_NRSECTOR	(0x1f3)		/* 扇区号寄存器，R/W */

#define HD_LCYL		(0x1f4)		/* 柱面号低字节寄存器，R/W */

#define HD_HCYL		(0x1f5)		/* 柱面号高字节寄存器，R/W */

#define HD_DRV_HEAD	(0x1f6)		/* 驱动器、磁头号寄存器(101dhhhh,d=驱动器号,h=磁头号)，R/W */

#define HD_STATE	(0x1f7)		/* 主状态寄存器，R */
#define HD_COMMAND	(0x1f7)		/* 命令寄存器，W */

#define HD_CTRL		(0x3f6)		/* 硬盘控制寄存器，Write ONLY */

/* 寄存器具体命令或操作 */

/*
硬盘控制寄存器(0x3f6)各二进制位含义:
0: none(0)
1: 保留(0)（关闭IRQ）
2: 允许复位
3: 若磁头数大于8，则置1
4: none(0)
5: 若在柱面数+1处有生产商的坏区图，则置1
6: 禁止ECC重试
7: 禁止访问重试
 */

/* 主状态寄存器(0x1f7)各二进制位含义: */
#define ERR_STAT	(1 << 0)		/* 0: 命令执行错误 */
#define INDEX_STAT	(1 << 1)		/* 1: 收到索引。当磁盘旋转遇到索引标志时置该位 */
#define ECC_STAT	(1 << 2)		/* 2: ECC校验错。当遇到一个可恢复的数据错误而且以
									得到纠正，就会置该位。这种情况不会中断一个多扇区读操作。 */
#define DRQ_STAT	(1 << 3)		/* 3: 数据请求服务。表示驱动器已准备好数据传输。 */
#define SEEK_STAT	(1 << 4)		/* 4: 驱动器寻道结束。表示寻道已完成 */
#define WRERR_STAT	(1 << 5)		/* 5: 驱动器故障（写出错） */
#define READY_STAT	(1 << 6)		/* 6: 驱动器就绪 */
#define BUSY_STAT	(1 << 7)		/* 7: 驱动器的控制器忙 */

/*
 * 命令寄存器(0x1f7)命令列表。NOTE! 以下是默认的命令，其中后面加上0x0的
 * 是固定命令，其他是可变命令，其中可变的二进制位参见各个命令后面的说明。
 * 其中，R为步进速率，R=0，步进速率为35us，R=1，为0.5ms，以此量递增。
 * L是数据模式，L=0，表示读写扇区为512字节，L=1，表示读写为512+4字节的ECC码。
 * T表示重复模式，T=0，表示允许重复，T=1，表示禁止重复。
 */
#define WIN_RESTORE		(0x10)			/* 驱动器重新校正. 0,1,2,3 - R */
#define WIN_READ		(0x20)			/* 读扇区. 0 - T, 1 - L */
#define WIN_WRITE		(0x30)			/* 写扇区. 0 - T, 1 - L */
#define WIN_VERIFY		(0x40)			/* 检查扇区. 0 - T */
#define WIN_FORMAT		(0x50 + 0x0)	/* 格式化磁道 */
#define WIN_INIT		(0x60 + 0x0)	/* 控制器初始化 */
#define WIN_SEEK		(0x70)			/* 寻道. 0,1,2,3 - R */
#define WIN_DIAGNOSE	(0x90 + 0x0)	/* 控制器诊断 */
#define WIN_SPECIFY		(0x90 + 0x1)	/* 建立驱动器参数 */

/********************************************************************/

/* 硬盘参数表结构 */
struct hd_struct {
	unsigned short cyls;			/* 柱面数 */
	unsigned char heads;			/* 磁头数 */
	unsigned short w_small_elec_cyl;/* 开始减小写电流的柱面(仅PC XT使用，其他为0) */
	unsigned short pre_w_cyl;		/* 开始写前预补偿柱面号，使用时需除以4!!!! */
	unsigned char max_ECC;			/* 最大ECC猝发长度(仅XT使用，其他为0) */
	unsigned char ctrl;				/* 控制字节。具体各位含义同硬盘控制寄存器。 */
	unsigned char n_time_limit;		/* 标准超时值(仅XT使用，其他为0) */
	unsigned char f_time_limit;		/* 格式化超时值(仅XT使用，其他为0) */
	unsigned char c_time_limit;		/* 检测驱动器超时值(仅XT使用，其他为0) */
	unsigned short head_on_cyl;		/* 磁头着陆(停止)柱面号 */
	unsigned char sectors;			/* 每磁道扇区数 */
	unsigned char none;				/* 保留 */
};

#define NR_HD_REQUEST	(30)				/* 硬盘请求项个数 */

/*
 * 硬盘请求项结构。NOTE! 扇区号:从1开始!!!!!! 
 * 驱动器号:0或1, 磁头号:从0开始, 柱面号:从0开始.
 */
struct hd_request {
	unsigned int sectors;
	unsigned int nrsector;
	unsigned char nrdrv;
	unsigned int nrcyl;
	unsigned char nrhead;
	unsigned int cmd;
	void (*int_addr)(void);			/* 硬盘中断函数 */
	unsigned char * buff;
	struct proc_struct * wait;		/* 等待此硬盘请求结束的进程 */
};

/********************************************************************/

extern void insert_hd_request(unsigned char * buff, unsigned int cmd,
					unsigned char dev, unsigned int nr_v_sector,
					unsigned int sectors);
extern void do_hd_request(void);
extern void hd_init(void);

/********************************************************************/

#endif /* _HARD_DISK_H_ */