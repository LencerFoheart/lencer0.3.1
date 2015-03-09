/*
 * Small/fs/buffer.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ��������̸��ٻ���������ش������
 *
 * ����������ʼ��ַ���ں˴�������ݵĽ�������NOTE! ����������ʼ��ַ
 * �ͽ�����ַ����mm�����á�
 *
 * ������ͨ���������б�͹�ϣ�����������б�����ͷָ��ָ���˫��
 * ѭ��������ɣ���ϣ������ͷָ�������Լ�ÿ��ͷָ��ָ���˫��ѭ����
 * ����ɡ�һ���������ڹ�ϣ���У�����ζ������Ч���ڿ��б��У�����ζ
 * �������С�һ����������ͬʱλ�ڿ��б�͹�ϣ����ʱ��ζ����������
 * ��Ч ...
 *
 * [??] ��û��ʵ���첽��дӲ�̣��첽��дʱgetblk�ｫ���в�ͬ��
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "file_system.h"
#include "harddisk.h"
#include "kernel.h"
#include "string.h"
#include "sched.h"
#include "types.h"
#include "sys_set.h"

/*
 * ��������ʼ�ͽ����������ַ��
 * NOTE! ����mm�г�ʼ�����ʲ����ڱ��ļ����ٴγ�ʼ��!!!
 */
unsigned long buffer_mem_start = 0, buffer_mem_end = 0;

/* ���������б�ͷ��ָ�� */
static struct buffer_head * free_head = NULL;
/* ��������ϣ��ͷ��ָ������ */
static struct buffer_head * hash_heads[NR_BUFF_HASH] = {0};

/* ָ��ȴ��������Ľ��� */
struct proc_struct * buff_wait = NULL;


/*
 * �ú���Ϊ��ϣ��ɢ�к����������豸�źͿ�ţ��������ڹ�ϣ��
 * �����е�������NOTE! �ú�������֪���ÿ��Ƿ������ɢ�б�
 */
int b_hash_fun(unsigned char dev, unsigned long nrblock)
{
	return (nrblock % NR_BUFF_HASH);
}

/*
 * �ú�������ϣ����ָ��������ڹ�ϣ���򷵻�ָ��û�������ָ�룬
 * ���򣬷���NULL�������豸�ź��߼�Ӳ�̿�Ŵ�0��ʼ������ʼ��������ͷ
 * ��ʱ������Ҳ��ʼ��Ϊ0������жϻ������Ƿ��ڹ�ϣ��Ҫ�ж�����Чλ��
 */
struct buffer_head * b_scan_hash(unsigned char dev, unsigned long nrblock)
{
	int index = b_hash_fun(dev, nrblock);
	struct buffer_head *p = hash_heads[index];

	while(p) {
		if((p->state&BUFF_VAL) && p->dev==dev && p->nrblock==nrblock)
			return p;
		p = p->p_hash_next;
		if(p == hash_heads[index])
			break;
	}
	return NULL;
}

/*
 * �ú����ӿ��б���ժȡһ�����л�������������Ϊ�գ�
 * ���ͷ��ժȡһ���������������Ļ�����ָ��ժȡ��
 */
struct buffer_head * b_get_from_free(struct buffer_head *p)
{
	if(p) {				/* �ӱ���ժ��ָ���Ŀ��л����� */
		if(p->p_free_next == p) {/* ����ֻ��һ�������� */
			free_head = NULL;
		} else {
			p->p_free_pre->p_free_next = p->p_free_next;
			p->p_free_next->p_free_pre = p->p_free_pre;
			if(p == free_head) {		/* pָ���ͷ */
				free_head = p->p_free_next;
			}
		}
	} else {
		if(NULL == free_head) {	/* ��� */
			p = NULL;
		} else if(free_head->p_free_next == free_head) {/* ����ֻ��һ�������� */
			p = free_head;
			free_head = NULL;
		} else {					/* �����ж�������� */
			p = free_head;
			p->p_free_pre->p_free_next = p->p_free_next;
			p->p_free_next->p_free_pre = p->p_free_pre;
			free_head = p->p_free_next;
		}
	}
	return p;
}

/*
 * �ú��������л�����������б��С����京����Ч���ݣ�����
 * ������б�β�������������б�ͷ�����Ա��´�ֱ��ʹ������
 */
void b_put_to_free(struct buffer_head *p, BOOL ishead)
{
	if(!p)
		panic("b_put_to_free: pointer is NULL.");

	if(NULL == free_head) {	/* ���б�� */
		free_head = p;
		p->p_free_next = p;
		p->p_free_pre = p;
	} else {
		if(ishead) {			/* ����ͷ�� */
			p->p_free_next = free_head;
			p->p_free_pre = free_head->p_free_pre;
			free_head->p_free_pre->p_free_next = p;
			free_head->p_free_pre = p;
			free_head = p;
		} else {				/* ����ĩβ */
			p->p_free_pre = free_head->p_free_pre;
			free_head->p_free_pre->p_free_next = p;
			p->p_free_next = free_head;
			free_head->p_free_pre = p;
		}
	}
}

/*
 * �ӹ�ϣ���Ƴ������ǲ��ܽ��豸�ŵ���ΪNULL����Ϊ�豸��
 * �ȿ���Ϊ0��ʵ����ֻҪ����Чλ������ɣ��μ�scan_hash().
 */
void b_remove_from_hash(struct buffer_head *p)
{
	if(!p)
		panic("b_remove_from_hash: pointer is NULL.");
	if(!b_scan_hash(p->dev, p->nrblock))
		panic("b_remove_from_hash: not in hash-table.");

	int index = b_hash_fun(p->dev, p->nrblock);

	if(p->p_hash_next == p) {	/* ����ֻ��һ�������� */
		hash_heads[index] = NULL;
	} else {
		p->p_hash_pre->p_hash_next = p->p_hash_next;
		p->p_hash_next->p_hash_pre = p->p_hash_pre;
		if(p == hash_heads[index]) {		/* pָ���ͷ */
			hash_heads[index] = p->p_hash_next;
		}
	}
}

/* * �ú�����ָ�����������뵽��ϣ���У�ͬʱ�����û�������
 * �豸���Լ����̿�š� */
void b_put_to_hash(struct buffer_head *p, 
		unsigned char newdev, unsigned long newnrblock)
{
	if(!p)
		panic("b_put_to_hash: pointer is NULL.");

	int index = b_hash_fun(newdev, newnrblock);

	p->dev = newdev;
	p->nrblock = newnrblock;
	if(NULL == hash_heads[index]) {
		hash_heads[index] = p;
		p->p_hash_next = p;
		p->p_hash_pre = p;
	} else {
		p->p_hash_next = hash_heads[index];
		p->p_hash_pre = hash_heads[index]->p_hash_pre;
		hash_heads[index]->p_hash_pre->p_hash_next = p;
		hash_heads[index]->p_hash_pre = p;
		hash_heads[index] = p;
	}
}

/* * �ú��������豸�ź�Ӳ�̿�Ż�ȡָ���������������غ�����Ч���ݵĻ���
 * ��ָ�롣ע�⣺�������ͷ�֮��ͽ�����б�����������Ч��Ҳ��ͬʱ��
 * ���ڹ�ϣ���С�
 */
struct buffer_head * getblk(unsigned char dev, unsigned long nrblock)
{
	struct buffer_head * p = NULL;

	while(1) {
		if((p = b_scan_hash(dev, nrblock))) {/* ��ɢ�б� */
			/* ��ɢ�б������� */
			if(p->state & BUFF_LOCK) {
				sleep_on(&buff_wait, INTERRUPTIBLE);
				continue;		/* �ص�whileѭ�� */
			}
			/* ��ɢ�б��ҿ��� */
			LOCK_BUFF(p);
			b_get_from_free(p);
			return p;
		} else {/* ����ɢ�б� */
			/* ����ɢ�б��ҿ��б�� */
			if(NULL == free_head) {
				sleep_on(&buff_wait, INTERRUPTIBLE);
				continue;		/* �ص�whileѭ�� */
			}
			/* ����ɢ�б��ڿ��б��ҵ�һ������������������ӳ�д */
			p = b_get_from_free(NULL);
			if(p->state & BUFF_DELAY_W) {
				/*
				 * ����Ҫ���ǻ����Ƿ��������Ϊ���б��еĻ������ǲ�����
				 * ���ڼ���״̬�ġ�
				 *
				 * �˴�Ӧ�����첽д�����첽д��ûʵ���أ�����ͬ��д�ɡ�
				 * ͬ��д�Ļ������ǽ���д�����֮�󣬾�����ֱ��ȥ���ˣ�
				 * ֻ��Ч�ʲ����á��첽д�Ļ�������Ͳ�һ���ˣ�������Ҫ
				 * �����ص�whileѭ����������ֱ��ʹ�øû��壬ͬʱ�û���
				 * �����Ǵӿ��б���ժ�����ˣ���˻��迼��Ҫ��Ҫ�����Ż�
				 * ���б���...��������ܸ��� :-)
				 */
				bwrite(p);
			}
			/* ����ɢ�б��ڿ��б��ҵ�һ�������� */
			LOCK_BUFF(p);
			if(p->state & BUFF_VAL) {
				b_remove_from_hash(p);
				p->state &= (~BUFF_VAL) & 0xff;
			}
			b_put_to_hash(p, dev, nrblock);
			return p;
		}
	}
}

/* * �ú�����ָ���������ͷš� */
void brelse(struct buffer_head *p)
{
	if(!p)
		panic("brelse: the m-inode-pointer can't be NULL.");
	
	if(p->state & BUFF_VAL) {
		b_put_to_free(p, 0);
	} else {
		/*		 * ���ӹ�ϣ���Ƴ��������������Ƿ��ڹ�ϣ������Ч����ȷ������
		 * ��ϣ�������֮һ�����������ʱ������Ч���Ƴ�֮����ʹ֮��Ч��		 */
		p->state |= BUFF_VAL;
		b_remove_from_hash(p);
		p->state &= (~BUFF_VAL) & 0xff;
		b_put_to_free(p, 1);
	}
	UNLOCK_BUFF(p);
}

/* * �ú�����ȡָ�����̿����ݡ� */
struct buffer_head * bread(unsigned char dev, unsigned long nrblock)
{
	struct buffer_head *p = getblk(dev, nrblock);
	if(!(p->state&BUFF_VAL)) {	/* ��Ч�����������̶� */
		insert_hd_request(p->p_data, WIN_READ, dev, nrblock, 1);
		p->state |= BUFF_VAL;
	}
	return p;
}

/* * �ú���������д��ָ�����̿顣 */
void bwrite(struct buffer_head *p)
{
	if(!p)
		panic("bwrite: the m-inode-pointer can't be NULL.");
	
	if(!(p->state&BUFF_LOCK))
		LOCK_BUFF(p);
	/* ��������д */
	insert_hd_request(p->p_data, WIN_WRITE, p->dev, p->nrblock, 1);
	if(p->state & BUFF_DELAY_W) {
		p->state &= (~BUFF_DELAY_W) & 0xff;
		UNLOCK_BUFF(p);
	} else {
		brelse(p);
	}
}

/*
 * ��������ʼ�������ڴ��ʼ��֮�󱻵��á�
 * [??] buffer-struct�����ڴ����ֽڶ��봦��
 */
void buff_init(void)
{
	int buffers;				/* �������� */
	struct buffer_head * p;		/* ��ǰҪ�������� */
	struct buffer_head * plast;	/* ǰһ�������� */
	unsigned long tmp;

	/* ����1��Ϊ�˱��⣬��640kb���޷��洢�����Ļ���ͷ���򻺳��������� */
	buffers = (buffer_mem_end - buffer_mem_start - (1*1024*1024-640*1024))
			/ (BUFFER_SIZE + sizeof(struct buffer_head)) - 1;
	p = (struct buffer_head *)buffer_mem_start;
	plast = NULL;
	free_head = p;
	buff_wait = NULL;

	d_printf("buff_head+BUFFER_SIZE = %d\n buffers = %d\n",
		sizeof(struct buffer_head)+BUFFER_SIZE, buffers);

	/* ����˫��ѭ�����л��������� */
	while(buffers--) {
		/*
		 * ��������һ�ֿ����ԣ���pδ�ﵽ640KB����p->p_free_next 
		 * �ѳ����ˣ��������Ҫ̽�������������������Ӧ��
		 */
		tmp = (unsigned long)p + sizeof(struct buffer_head) + BUFFER_SIZE;
		/* ��ǰ����������[640KB,1MB]���� */
		if(tmp>640*1024 && tmp<1*1024*1024) {
			p = (struct buffer_head *)(1*1024*1024);
			/*
			 * Well! Here, we need to change the pointer of
			 * last-buffer. I debug it for a long time :-)
			 */
			NULL==plast ? 0 : (plast->p_free_next=p);
			NULL==plast ? (free_head=p) : 0;
		}

		d_printf("[1] p = %d, p->p_free_next = %d\n",
			(unsigned long)p, (unsigned long)p->p_free_next);

		zeromem(p, sizeof(struct buffer_head));
		p->p_free_next = (struct buffer_head *)((unsigned long)p 
			+ sizeof(struct buffer_head) + BUFFER_SIZE);
		p->p_data = (unsigned char *)((unsigned long)p 
			+ sizeof(struct buffer_head));
		p->p_free_pre = plast;
		
		plast = p;
		p = p->p_free_next;

		d_printf("[2] p = %d, p->p_free_next = %d\n", 
			(unsigned long)p, (unsigned long)p->p_free_next);

		d_printf("next_free_buff_head_addr = %d\n", p->p_free_next);
		d_delay(1000);
	}
	plast->p_free_next = free_head;
	free_head->p_free_pre = plast;

	/* ��ʼ����ϣ���� */
	/* nothing.��bss��,�ѱ����� */
}
