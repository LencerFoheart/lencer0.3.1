/*
 * Small/tools/mkfs.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ������ļ�ϵͳ���ɳ��򡣴˴������ļ���Ϊ���̾�����ģ����̡�
 * ���ļ�ϵͳ������UNIX system V���ļ�ϵͳ���ں�Ĭ�������ڵ�Ŵ�0��ʼ��
 *
 * 0.3.1�������ļ�ϵͳ��������һЩ���������Ա���һЩ���ݣ��ں��е�
 * ���Գ���Ὣ��Щ����д���ļ�ϵͳ�������Ϊ�ļ���Ҳ�������ļ�ϵͳ��
 * �����ļ���Ȼ�󷽱�ʹ�á����ں�֮�����ļ�ϵͳ��д���ļ����̫���� :-((
 *
 * NOTE! �ڸ���ÿ���ļ�������ʱ��Ϊ�˲������������������ݸ�ʽ�Ľṹ��
 * ��Խ�������̿飬���ǲ��ÿ���롣�µĸ����ļ����Ǵ��µĿ鿪ʼ������
 * ��ʼ���������������ݸ�ʽ�Ľṹ�壬������ļ�����Ȼ�����ļ����ݡ�
 *
 * ���ˣ����Ǵ������ļ�ϵͳ�Լ���߸��ӵ����ݶ�������������������Ҳ��512*n.
 */

#define __GCC_LIBRARY__

#include <stdio.h>					/* ��׼ͷ�ļ������Խ� */
#include <string.h>					/* ��׼ͷ�ļ������Խ� */
#include <dirent.h>					/* ��׼ͷ�ļ������Խ� */
#include "../include/file_system.h"	/* �����ļ�ϵͳ�Ĵ������� */
#include "../include/kernel.h"/* �����ļ�ϵͳ֮��ĸ������ݵĸ�ʽ˵�� */

/* ���ڹ����ļ�ϵͳ�Ĵ��̵Ĵ�С��MB��NOTE! �˴���������ʵ�ĳߴ硣 */
#define DISK_SIZE		(10)

/*
 * NOTE! �������� DISK_SIZE ��һ��ʹ���ɵ�Ӳ�̾�������ʵ��Ӳ����ƥ�䣬
 * �ŵ����˴��󡣶���Ӳ�̣���ͷ����ÿ�ŵ����������ŵ�����Ӧ��������
 *
 * ��PC��׼����һ������Ȥ������Ϊ�˼��ݴ�����ϵ�BIOS����������
 * ������ͨ��ִ��һ���ڲ�ת�룬ת��Ϊ�߼���ÿ�ŵ�63����������ˣ�
 * �������ǽ��ŵ�����������Ϊ63���Ա����VMware��(ע: Bochs������
 * Ϊ63Ҳ�ǿ��Ե�)��
 */
/* ����Ĭ�ϵĴ�ͷ�� */
#define HEADS			(2)
/* ����Ĭ�ϵ�ÿ�ŵ������������޸��� */
#define SPT				(63)
/* ���̴ŵ��� */
#define CYLINDERS		(DISK_SIZE*1024*1024/512/HEADS/SPT)

/*
 * �ļ�ϵͳ���ɳ���
 */
int mkfs(void)
{
	static char canrun = 1;				/* Ϊ1ʱ���ú�������ִ�� */
	FILE * fp = NULL;
	unsigned long i=0;					/* ѭ�����Ʊ��� */
	unsigned long pfreeblock = 0;		/* ��������һ���账��Ŀ��п�Ŀ�� */
	char buff[512] = {0};
	struct d_super_block sup_b = {0};	/* ������ */
	struct d_inode root = {0};			/* �������ڵ� */
	struct file_dir_struct dir = {0};	/* �������ڵ��Ŀ¼���� */

	if(!canrun)
		return 0;
	canrun = 0;

	if(!(fp = fopen("rootImage","wb"))) {
		printf("create rootImage error!!!\n");
		return -1;
	}

	/* ������������0 */
	memset(buff, 0, 512);
	for(i=0; i<HEADS*SPT*CYLINDERS; i++) {
		/* ò��fwriteһ��д��������̫�󣬻����!!! ��ԭ����δ֪�� :-( */
		fwrite(buff, 512, 1, fp);
	}

	/*
	 * ���ó�����ṹ��ע�⣺��0�������ڵ��Ǹ������ڵ㣬
	 * ��һ�����п��Ǹ������ڵ�����ݿ顣
	 */
	sup_b.size = HEADS * SPT * CYLINDERS;
	sup_b.nfreeblocks = sup_b.size - 2 - ((NR_D_INODE+512/sizeof(struct d_inode)-1) / (512/sizeof(struct d_inode))) - 1;
	sup_b.nextfb = 0;
	sup_b.ninodes = NR_D_INODE;
	sup_b.nfreeinodes = NR_D_INODE-1;
	sup_b.nextfi = 0;
	pfreeblock = sup_b.size - sup_b.nfreeblocks;/* ��ʼ��pfreeblock */
	/* ���ó������е� ���п� �� */
	for(i=0; i<NR_D_FBT && pfreeblock<sup_b.size; i++, pfreeblock++)
		sup_b.fb_table[i] = pfreeblock;
	/* ���ó������е� ���������ڵ� �� */
	for(i=0; i<NR_D_FIT && i<NR_D_INODE; i++)
		sup_b.fi_table[i] = i+1;
	sup_b.store_nexti = i+1;
	fseek(fp, 512L, SEEK_SET);	/* ������������ */
	fwrite(&sup_b, sizeof(struct d_super_block), 1, fp);/* д������ */

	/* ����ʣ����п� */
	while(1) {
		/* ���ϴε����һ�����д��̿���Ϊ���ӿ飬�洢���п����� */
		fseek(fp, 512 * sup_b.fb_table[NR_D_FBT-1], SEEK_SET);
		/* �������� ���п� �� */
		for(i=0; i<NR_D_FBT && pfreeblock<sup_b.size; i++, pfreeblock++)
			sup_b.fb_table[i] = pfreeblock;
		if(i < NR_D_FBT) {			/* ���п鴦����� */
			sup_b.fb_table[i] = END_D_FB;/* END_D_FB ���п������־ */
			fwrite(&sup_b.fb_table, sizeof(unsigned long)*NR_D_FBT, 1, fp);
			break;
		}
		fwrite(&sup_b.fb_table, sizeof(unsigned long)*NR_D_FBT, 1, fp);
	}

	/* ���������ڵ�� */
	/* do nothing but setup the root-inode, because that the all is 0 */
	/* NOTE! �������ڵ��޸��ڵ㣬����Ŀ¼��������'..' */
	root.mode |= FILE_DIR;
	root.mode |= FILE_RONLY;
	root.nlinks = 1;
	root.size = sizeof(struct file_dir_struct);
	root.zone[0] = sup_b.size - sup_b.nfreeblocks - 1;
	fseek(fp, 512 * 2, SEEK_SET);
	fwrite(&root, sizeof(struct d_inode), 1, fp);
	dir.nrinode = 0;
	strcpy(dir.name, ".");
	fseek(fp, 512 * root.zone[0], SEEK_SET);
	fwrite(&dir, sizeof(struct file_dir_struct), 1, fp);

	fclose(fp);
	return 1;
}

/*
 * �ļ�ϵͳ֮�󸽼����ݿ����
 *
 * �ðɣ�����Ϊ�б�׼C�������Կ�ƽ̨����ȡĿ¼�����������Ǵ��ˡ�
 * _findfirst ... ������linux��ʹ�ã������ջ���˵linux�¿���ʹ��
 * readdir����Internet��search search������readdirȷʵ������ :-)
 */
int add_data(void)
{
	FILE * fpdes, * fpsrc;
	char path[256] = "./tools/add_data/";
	DIR * pdir;
	struct dirent * pdirent;
	char buff[512] = {0};
	char filepath[256] = {0};
	int count;
	struct addition sadd;	/* �������ݵĸ�ʽ˵�����ݽṹ */
	
	if(!(fpdes = fopen("rootImage","ab"))) {
		printf("open rootImage error!!!\n");
		return -1;
	}
	
	/* ��ȡĿ¼������Ŀ¼���ļ�������д��... */
	if(!(pdir = opendir(path))) {
		fclose(fpdes);
		return -1;
	}
	while((pdirent = readdir(pdir))) {
		if(0==strcmp(pdirent->d_name,".")
		|| 0==strcmp(pdirent->d_name,".."))
			continue;
		printf("FILE -- [%s]\n", pdirent->d_name);
		strcpy(filepath, path);
		strcat(filepath, pdirent->d_name);
		if(!(fpsrc = fopen(filepath, "rb"))) {
			fclose(fpdes);
			closedir(pdir);
			return -1;
		}
		
		/* д���ļ����� */
		sadd.fileflag = 1;
		sadd.namelen = strlen(pdirent->d_name);
		fseek(fpsrc, 0, SEEK_END);
		sadd.datalen = ftell(fpsrc);
		fseek(fpsrc, 0, SEEK_SET);
		fwrite(&sadd, sizeof(struct addition), 1, fpdes);
		fwrite(pdirent->d_name, sadd.namelen, 1, fpdes);
		while((count=fread(buff,1,sizeof(buff),fpsrc)) > 0)
			fwrite(buff, count, 1, fpdes);
			
		fclose(fpsrc);
		
		/* ����롣�ðɣ�fseek���ǲ��У�����ֻ��fwrite�ˡ� */
		memset(buff, 0, 512);
		fwrite(buff, (512-((sadd.namelen+sadd.datalen
			+sizeof(struct addition))%512)) % 512, 1, fpdes);		
	}
	closedir(pdir);
	
	/* д��һ���հ׿飬�����Ϳ���ָʾ�����ļ������� */
	memset(buff, 0, 512);
	fwrite(buff, sizeof(buff), 1, fpdes);
	
	/* ʹӲ�̴ŵ���Ϊ���� */
	count = ftell(fpdes) / 512;
	count = (HEADS*SPT-(count%(HEADS*SPT))) % (HEADS*SPT);
	while(count--)
		fwrite(buff, sizeof(buff), 1, fpdes);

	count = ftell(fpdes);
	fclose(fpdes);
	
	return (count/512 - HEADS*SPT*CYLINDERS);
}

int main(void)
{
	int addseccount = 0;	/* �ļ�ϵͳ�����ӵ������� */
	
	if(-1 == mkfs()) {
		printf("ERROR: make file-system error!\n");
		return -1;
	}
	if(-1 == (addseccount=add_data())) {
		printf("ERROR: add data to disk error!\n");
		return -1;
	}
	
	printf("===OK!\n");
	printf("Size of  FS  is %f MB, total %d sectors.\n",
		HEADS*SPT*CYLINDERS * 512.0/1024.0/1024.0,
		HEADS*SPT*CYLINDERS);
	printf("Size of DISK is %f MB, total %d sectors.\n",
		(HEADS*SPT*CYLINDERS + addseccount) * 512.0/1024.0/1024.0,
		(HEADS*SPT*CYLINDERS + addseccount));
	printf(" FS  -- cylinders: %d, heads: %d, spt: %d.\n",
		CYLINDERS, HEADS, SPT);
	printf("DISK -- cylinders: %d, heads: %d, spt: %d.\n",
		CYLINDERS + addseccount/HEADS/SPT, HEADS, SPT);
	printf("###Please check 'Bochs' or 'VMware' configuration.\n");
	printf("mkfs exit.\n\n");

	return 0;
}
