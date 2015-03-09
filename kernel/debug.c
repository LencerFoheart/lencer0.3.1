/*
 * Small/kernel/debug.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * ���ļ������������(Bochs,VMware��)�е����ںˡ�������Щ�����õ�
 * ���ں˵�һЩ��ϵͳ���õĹ����ຯ�������������Ҫ�������ں�̬��
 * ����һ���������ڵ����ں˺���ʱ����Ҫ�ر�С��fs�Ĵ�����ָ���ˣ�
 * ���������Ǽ��û� :-)
 */


#ifndef __DEBUG__
#define __DEBUG__
#endif


#include "kernel.h"
#include "file_system.h"
#include "sched.h"
#include "stdio.h"
#include "string.h"
#include "sys_set.h"
#include "exec.h"

extern int debug_call(void);
extern struct m_super_block super;

/*
 * ���̸�Ŀ¼�͹���Ŀ¼��ʼ����
 */
void proc_dir_init(unsigned char dev, unsigned int nrinode)
{
	current->root_dir = current->current_dir = iget(dev, nrinode);
	UNLOCK_INODE(current->root_dir);	/* ���� */
}

/*
 * ��ӡĿ¼���ݡ�NOTE! Ŀ¼��ֻ���ġ�
 */
void print_dir(char * dir)
{
	unsigned long size = 0;
	FILEDESC desc;
	struct file_dir_struct fdirs;
	unsigned short oldfs;
	
	oldfs = set_fs_kernel();
	if(-1 == (desc=sys_open(dir, O_RONLY))) {
		set_fs(oldfs);
		k_printf("print_dir: Open file failed!");
		return;
	}
	set_fs(oldfs);
	d_printf("Open file return.\n");
	size = sys_lseek(desc, 0, SEEK_END);
	sys_lseek(desc, 0, SEEK_SET);
	_k_printf("The dir-content:");
	for(int i=0; i<size; i+=sizeof(struct file_dir_struct)) {
		/* һ�����̿�洢������Ŀ¼�� */
		if((512-(i%512)) < sizeof(struct file_dir_struct)) {
			i += 512-1;
			i -= i%512;
			if(-1 == sys_lseek(desc, i, SEEK_SET)) {
				k_printf("print_dir: Lseek file failed!");
				return;
			}
			d_printf("Lseek file return.\n");
		}
		oldfs = set_fs_kernel();
		if(-1 == sys_read(desc, &fdirs, sizeof(struct file_dir_struct))) {
			set_fs(oldfs);
			k_printf("print_dir: Read file failed!");
			return;
		}
		set_fs(oldfs);
		d_printf("Read file return.\n");	
		_k_printf(" '%s', ", fdirs.name);
	}
	_k_printf("\n\n");
	if(-1 == sys_close(desc)) {
		k_printf("print_dir: Close file failed!");
		return;
	}
	d_printf("Close file return.\n");
}

/*
 * �����ļ����ó���Ӳ�����ļ�ϵͳ֮��ĸ����ļ����ݶ�ȡ����������
 * �ļ���ʽд���ļ�ϵͳ�����ڸ����ļ����ݲμ�mkfs��NOTE! ����һ����
 * Ҫ�ĺ���������Щ���Ժ�����ռ����Ҫ��λ��ǧ���ɾ�����������޸ģ�
 * ������mkfsͬ����
 */
void produce_files(unsigned char dev)
{
	struct addition * psadd;/* �������ݵĸ�ʽ˵�����ݽṹָ�� */
	unsigned long nrblock;	/* Ҫ��ȡ��Ӳ�̿�ţ�Ӳ�̿��0��� */
	struct buffer_head * bh;
	FILEDESC desc;
	char filename[256] = "/";
	char buff[BUFFER_SIZE] = {0};
	int i, onewc, leftc;/* onewc: һ��д���ֽ�����leftc: ʣ���ֽ��� */
	unsigned short oldfs;
	
	nrblock = super.size;
	psadd = (struct addition *)buff;
	/* ��ͣ��ȡ�������ݿ飬�����ļ� */
	while(1) {
		bh = bread(dev, nrblock++);
		memcpy(buff, bh->p_data, BUFFER_SIZE);
		brelse(bh);
		
		/* �����ļ���Ӧ�ĵ�һ��Ӳ�̿� */
		if(!psadd->fileflag)
			break;
		i = sizeof(struct addition);
		/* filename[0] -- '/' */
		memcpy(&filename[1], &buff[i], psadd->namelen);
		filename[psadd->namelen+1] = 0;
		oldfs = set_fs_kernel();
		desc = sys_creat(filename, FILE_FILE | FILE_RW);
		set_fs(oldfs);
		i += psadd->namelen;
		leftc = psadd->datalen;
		onewc = BUFFER_SIZE-i;
		onewc = onewc>leftc ? leftc : onewc;
		leftc -= onewc;
		oldfs = set_fs_kernel();
		sys_write(desc, &buff[i], onewc);
		set_fs(oldfs);

		/* �����ļ����ݶ�Ӧ�ĺ���Ӳ�̿� */
		while(leftc) {
			onewc = BUFFER_SIZE;
			onewc = onewc>leftc ? leftc : onewc;
			leftc -= onewc;
			bh = bread(dev, nrblock++);
			oldfs = set_fs_kernel();
			sys_write(desc, bh->p_data, onewc);
			set_fs(oldfs);
			brelse(bh);
		}
		
		sys_close(desc);
	}
}

/* * �ļ�ϵͳ�е��ļ���ʼ�����ú�����main�б����á� */
void files_init(void)
{
	proc_dir_init(0, 0);
	produce_files(0);
}

/*
 * produce_files() ���Գ���
 */
void produce_files_test(void)
{
	char ascfile[30] = "/readelf_test.txt";/* �ı��ļ���--add_dataĿ¼ */
	char buff[512+1] = {0};			/* 512+1 */
	FILEDESC desc;
	int count = 0, size = 0;
	unsigned short oldfs;

	print_dir("/");

	/* ��ȡ���ɵ��ļ����ݲ���ӡ */
	oldfs = set_fs_kernel();
	if(-1 == (desc=sys_open(ascfile, O_RW))) {
		set_fs(oldfs);
		k_printf("produce_files_test: Open file failed!");
		return;
	}
	set_fs(oldfs);
	/* ���ֻ��ȡһ�����ݣ������� :-) */
	oldfs = set_fs_kernel();
	if(-1 == (count=sys_read(desc, buff, sizeof(buff)-1))) {
		set_fs(oldfs);
		k_printf("produce_files_test: Read file failed!");
		return;
	}
	set_fs(oldfs);
	buff[count] = 0;
	if(-1 == (size=sys_lseek(desc, 0, SEEK_END))) {
		k_printf("produce_files_test: Lseek file failed!");
		return;
	}
	if(-1 == sys_close(desc)) {
		k_printf("produce_files_test: Close file failed!");
		return;
	}

	_k_printf("File-size is: %d bytes.\n", size);
	_k_printf("This is the content of READ file: \n%s\n\n", buff);
}

void exec_test(void)
{
	char binfile[30] = "/test";			/* �������ļ���--add_dataĿ¼ */
	unsigned short oldfs;

	oldfs = set_fs_kernel();
	sys_execve(binfile);
	set_fs(oldfs);
}

void creat_lseek_read_write_open_close_file_test(void)
{
	int count = 0;
	char buff_in[100] = "Hello world!\nI'm writing a small OS.\nThanks.";
	char buff_out[100] = {0};
	char filename[50] = {0};
	unsigned short oldfs;

	/* �Ѵ������ں�Ĭ�ϵ������̴洢�ļ�������ֹͣ */
	while((++count) < NR_D_INODE) {

		_k_printf("This is the content will be written: \n%s\n\n", buff_in);

		
		sprintf(filename, "/hello--%d.txt", count);
		_k_printf("%s\n", filename);
		/* �����ļ���д�� */
		oldfs = set_fs_kernel();
		FILEDESC desc = sys_creat(filename, FILE_FILE | FILE_RW);
		set_fs(oldfs);
		if(-1 == desc) {
			k_printf("creat_lseek_...: Create file failed!");
			return;
		}
		d_printf("Create file return.\n");
		if(-1 == sys_lseek(desc, 995920, SEEK_SET)) {
			k_printf("creat_lseek_...: Lseek file failed!");
			return;
		}
		d_printf("Lseek file return.\n");
		oldfs = set_fs_kernel();
		if(-1 == sys_write(desc, buff_in, 1 + strlen(buff_in))) {
			set_fs(oldfs);
			k_printf("creat_lseek_...: Write file failed!");
			return;
		}
		set_fs(oldfs);
		d_printf("Write file return.\n");
		if(-1 == sys_close(desc)) {
			k_printf("creat_lseek_...: Close file failed!");
			return;
		}
		d_printf("Close file return.\n");


		/* ��ӡ��Ŀ¼���е��ļ� */
		print_dir("/");

		/* ��֮ǰ�������ļ�����ȡ */
		oldfs = set_fs_kernel();
		if(-1 == (desc = sys_open(filename, O_RW))) {
			set_fs(oldfs);
			k_printf("creat_lseek_...: Open file failed!");
			return;
		}
		set_fs(oldfs);
		d_printf("Open file return.\n");
		if(-1 == sys_lseek(desc, 995920, SEEK_SET)) {
			k_printf("creat_lseek_...: Lseek file failed!");
			return;
		}
		d_printf("Lseek file return.\n");
		oldfs = set_fs_kernel();
		if(-1 == sys_read(desc, buff_out, 1 + strlen(buff_in))) {
			set_fs(oldfs);
			k_printf("creat_lseek_...: Read file failed!");
			return;
		}
		set_fs(oldfs);
		d_printf("Read file return.\n");
		if(-1 == sys_close(desc)) {
			k_printf("creat_lseek_...: Close file failed!");
			return;
		}
		d_printf("Close file return.\n");

		
		_k_printf("This is the content of READ file: \n%s\n\n", buff_out);
	} /* while */
}

void debug_delay(unsigned long delay_count)
{
	for(int i=0; i<delay_count; i++)
		for(int j=0; j<DELAY_BASE; j++)
			;
}

void no_printf(char * fmt, ...)
{
	/* nothing, it's right */
}

/*
 * ����fs�Ĵ������ԡ�
 */
void set_fs_test(void)
{
	unsigned short oldfs;
	_k_printf("FS register before changed  - %x.\n", get_fs());
	oldfs = set_fs_kernel();
	_k_printf("FS register after changed   - %x.\n", get_fs());
	set_fs(oldfs);
	_k_printf("FS register after recovered - %x.\n", get_fs());
}

/*
 * �ú���Ϊ����������������ĸ��������̣���������������� :-)
 */
void debug(void)
{
//	set_fs_test();
//	d_delay(100000);

	creat_lseek_read_write_open_close_file_test();
	produce_files_test();
	exec_test();
}

void debug_init(void)
{
	set_idt(TRAP_R3, debug_call, 0x81);
}
