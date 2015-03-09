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

#define HD_DATA		(0x1f0)		/* ���ݼĴ�����Read/Write */

#define HD_ERROR	(0x1f1)		/* ����״̬�Ĵ�����R */
#define HD_PRECOMP	(0x1f1)		/* дǰԤ�����Ĵ�����W */

#define HD_SECTORS	(0x1f2)		/* �������Ĵ�����R/W */

#define HD_NRSECTOR	(0x1f3)		/* �����żĴ�����R/W */

#define HD_LCYL		(0x1f4)		/* ����ŵ��ֽڼĴ�����R/W */

#define HD_HCYL		(0x1f5)		/* ����Ÿ��ֽڼĴ�����R/W */

#define HD_DRV_HEAD	(0x1f6)		/* ����������ͷ�żĴ���(101dhhhh,d=��������,h=��ͷ��)��R/W */

#define HD_STATE	(0x1f7)		/* ��״̬�Ĵ�����R */
#define HD_COMMAND	(0x1f7)		/* ����Ĵ�����W */

#define HD_CTRL		(0x3f6)		/* Ӳ�̿��ƼĴ�����Write ONLY */

/* �Ĵ��������������� */

/*
Ӳ�̿��ƼĴ���(0x3f6)��������λ����:
0: none(0)
1: ����(0)���ر�IRQ��
2: ����λ
3: ����ͷ������8������1
4: none(0)
5: ����������+1���������̵Ļ���ͼ������1
6: ��ֹECC����
7: ��ֹ��������
 */

/* ��״̬�Ĵ���(0x1f7)��������λ����: */
#define ERR_STAT	(1 << 0)		/* 0: ����ִ�д��� */
#define INDEX_STAT	(1 << 1)		/* 1: �յ���������������ת����������־ʱ�ø�λ */
#define ECC_STAT	(1 << 2)		/* 2: ECCУ���������һ���ɻָ������ݴ��������
									�õ��������ͻ��ø�λ��������������ж�һ���������������� */
#define DRQ_STAT	(1 << 3)		/* 3: ����������񡣱�ʾ��������׼�������ݴ��䡣 */
#define SEEK_STAT	(1 << 4)		/* 4: ������Ѱ����������ʾѰ������� */
#define WRERR_STAT	(1 << 5)		/* 5: ���������ϣ�д���� */
#define READY_STAT	(1 << 6)		/* 6: ���������� */
#define BUSY_STAT	(1 << 7)		/* 7: �������Ŀ�����æ */

/*
 * ����Ĵ���(0x1f7)�����б�NOTE! ������Ĭ�ϵ�������к������0x0��
 * �ǹ̶���������ǿɱ�������пɱ�Ķ�����λ�μ�������������˵����
 * ���У�RΪ�������ʣ�R=0����������Ϊ35us��R=1��Ϊ0.5ms���Դ���������
 * L������ģʽ��L=0����ʾ��д����Ϊ512�ֽڣ�L=1����ʾ��дΪ512+4�ֽڵ�ECC�롣
 * T��ʾ�ظ�ģʽ��T=0����ʾ�����ظ���T=1����ʾ��ֹ�ظ���
 */
#define WIN_RESTORE		(0x10)			/* ����������У��. 0,1,2,3 - R */
#define WIN_READ		(0x20)			/* ������. 0 - T, 1 - L */
#define WIN_WRITE		(0x30)			/* д����. 0 - T, 1 - L */
#define WIN_VERIFY		(0x40)			/* �������. 0 - T */
#define WIN_FORMAT		(0x50 + 0x0)	/* ��ʽ���ŵ� */
#define WIN_INIT		(0x60 + 0x0)	/* ��������ʼ�� */
#define WIN_SEEK		(0x70)			/* Ѱ��. 0,1,2,3 - R */
#define WIN_DIAGNOSE	(0x90 + 0x0)	/* ��������� */
#define WIN_SPECIFY		(0x90 + 0x1)	/* �������������� */

/********************************************************************/

/* Ӳ�̲�����ṹ */
struct hd_struct {
	unsigned short cyls;			/* ������ */
	unsigned char heads;			/* ��ͷ�� */
	unsigned short w_small_elec_cyl;/* ��ʼ��Сд����������(��PC XTʹ�ã�����Ϊ0) */
	unsigned short pre_w_cyl;		/* ��ʼдǰԤ��������ţ�ʹ��ʱ�����4!!!! */
	unsigned char max_ECC;			/* ���ECC⧷�����(��XTʹ�ã�����Ϊ0) */
	unsigned char ctrl;				/* �����ֽڡ������λ����ͬӲ�̿��ƼĴ����� */
	unsigned char n_time_limit;		/* ��׼��ʱֵ(��XTʹ�ã�����Ϊ0) */
	unsigned char f_time_limit;		/* ��ʽ����ʱֵ(��XTʹ�ã�����Ϊ0) */
	unsigned char c_time_limit;		/* �����������ʱֵ(��XTʹ�ã�����Ϊ0) */
	unsigned short head_on_cyl;		/* ��ͷ��½(ֹͣ)����� */
	unsigned char sectors;			/* ÿ�ŵ������� */
	unsigned char none;				/* ���� */
};

#define NR_HD_REQUEST	(30)				/* Ӳ����������� */

/*
 * Ӳ��������ṹ��NOTE! ������:��1��ʼ!!!!!! 
 * ��������:0��1, ��ͷ��:��0��ʼ, �����:��0��ʼ.
 */
struct hd_request {
	unsigned int sectors;
	unsigned int nrsector;
	unsigned char nrdrv;
	unsigned int nrcyl;
	unsigned char nrhead;
	unsigned int cmd;
	void (*int_addr)(void);			/* Ӳ���жϺ��� */
	unsigned char * buff;
	struct proc_struct * wait;		/* �ȴ���Ӳ����������Ľ��� */
};

/********************************************************************/

extern void insert_hd_request(unsigned char * buff, unsigned int cmd,
					unsigned char dev, unsigned int nr_v_sector,
					unsigned int sectors);
extern void do_hd_request(void);
extern void hd_init(void);

/********************************************************************/

#endif /* _HARD_DISK_H_ */