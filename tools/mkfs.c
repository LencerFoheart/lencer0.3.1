/*
 * Small/tools/mkfs.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 本文件包含文件系统生成程序。此处创建文件作为磁盘镜像来模拟磁盘。
 * 该文件系统类似于UNIX system V的文件系统。内核默认索引节点号从0开始。
 *
 * 0.3.1版又在文件系统后边添加了一些扇区，用以保存一些数据，内核中的
 * 调试程序会将这些数据写入文件系统，保存成为文件。也就是在文件系统中
 * 创建文件，然后方便使用。在内核之外往文件系统中写入文件真的太难了 :-((
 *
 * NOTE! 在附加每个文件的数据时，为了不至于让描述附加数据格式的结构体
 * 跨越两个磁盘块，我们采用块对齐。新的附加文件总是从新的块开始，并且
 * 开始处是描述附加数据格式的结构体，随后是文件名，然后是文件数据。
 *
 * 至此，我们创建的文件系统以及后边附加的数据都是扇区长的整数倍，也即512*n.
 */

#define __GCC_LIBRARY__

#include <stdio.h>					/* 标准头文件，非自建 */
#include <string.h>					/* 标准头文件，非自建 */
#include <dirent.h>					/* 标准头文件，非自建 */
#include "../include/file_system.h"	/* 包含文件系统的创建参数 */
#include "../include/kernel.h"/* 包含文件系统之后的附加数据的格式说明 */

/* 用于构建文件系统的磁盘的大小，MB。NOTE! 此处并不是真实的尺寸。 */
#define DISK_SIZE		(10)

/*
 * NOTE! 正是由于 DISK_SIZE 不一定使生成的硬盘镜像与真实的硬盘相匹配，
 * 才导致了错误。对于硬盘，磁头数、每磁道扇区数、磁道数均应是整数。
 *
 * 在PC标准中有一个很有趣的现象，为了兼容大多数老的BIOS和驱动器，
 * 驱动器通常执行一个内部转译，转译为逻辑上每磁道63个扇区。因此，
 * 这里我们将磁道扇区数设置为63，以便兼容VMware等(注: Bochs不设置
 * 为63也是可以的)。
 */
/* 磁盘默认的磁头数 */
#define HEADS			(2)
/* 磁盘默认的每磁道扇区数，别修改它 */
#define SPT				(63)
/* 磁盘磁道数 */
#define CYLINDERS		(DISK_SIZE*1024*1024/512/HEADS/SPT)

/*
 * 文件系统生成程序。
 */
int mkfs(void)
{
	static char canrun = 1;				/* 为1时，该函数可以执行 */
	FILE * fp = NULL;
	unsigned long i=0;					/* 循环控制变量 */
	unsigned long pfreeblock = 0;		/* 磁盘中下一个需处理的空闲块的块号 */
	char buff[512] = {0};
	struct d_super_block sup_b = {0};	/* 超级块 */
	struct d_inode root = {0};			/* 根索引节点 */
	struct file_dir_struct dir = {0};	/* 根索引节点的目录表项 */

	if(!canrun)
		return 0;
	canrun = 0;

	if(!(fp = fopen("rootImage","wb"))) {
		printf("create rootImage error!!!\n");
		return -1;
	}

	/* 将磁盘内容清0 */
	memset(buff, 0, 512);
	for(i=0; i<HEADS*SPT*CYLINDERS; i++) {
		/* 貌似fwrite一次写的数据量太大，会出错!!! 但原因尚未知否 :-( */
		fwrite(buff, 512, 1, fp);
	}

	/*
	 * 设置超级块结构。注意：第0号索引节点是根索引节点，
	 * 第一个空闲块是根索引节点的数据块。
	 */
	sup_b.size = HEADS * SPT * CYLINDERS;
	sup_b.nfreeblocks = sup_b.size - 2 - ((NR_D_INODE+512/sizeof(struct d_inode)-1) / (512/sizeof(struct d_inode))) - 1;
	sup_b.nextfb = 0;
	sup_b.ninodes = NR_D_INODE;
	sup_b.nfreeinodes = NR_D_INODE-1;
	sup_b.nextfi = 0;
	pfreeblock = sup_b.size - sup_b.nfreeblocks;/* 初始化pfreeblock */
	/* 设置超级块中的 空闲块 表 */
	for(i=0; i<NR_D_FBT && pfreeblock<sup_b.size; i++, pfreeblock++)
		sup_b.fb_table[i] = pfreeblock;
	/* 设置超级块中的 空闲索引节点 表 */
	for(i=0; i<NR_D_FIT && i<NR_D_INODE; i++)
		sup_b.fi_table[i] = i+1;
	sup_b.store_nexti = i+1;
	fseek(fp, 512L, SEEK_SET);	/* 跳过引导扇区 */
	fwrite(&sup_b, sizeof(struct d_super_block), 1, fp);/* 写超级块 */

	/* 处理剩余空闲块 */
	while(1) {
		/* 将上次的最后一个空闲磁盘块作为链接块，存储空闲块数组 */
		fseek(fp, 512 * sup_b.fb_table[NR_D_FBT-1], SEEK_SET);
		/* 继续设置 空闲块 表 */
		for(i=0; i<NR_D_FBT && pfreeblock<sup_b.size; i++, pfreeblock++)
			sup_b.fb_table[i] = pfreeblock;
		if(i < NR_D_FBT) {			/* 空闲块处理完毕 */
			sup_b.fb_table[i] = END_D_FB;/* END_D_FB 空闲块结束标志 */
			fwrite(&sup_b.fb_table, sizeof(unsigned long)*NR_D_FBT, 1, fp);
			break;
		}
		fwrite(&sup_b.fb_table, sizeof(unsigned long)*NR_D_FBT, 1, fp);
	}

	/* 处理索引节点表 */
	/* do nothing but setup the root-inode, because that the all is 0 */
	/* NOTE! 根索引节点无父节点，故其目录表项中无'..' */
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
 * 文件系统之后附加数据块程序。
 *
 * 好吧，我以为有标准C函数可以跨平台来读取目录，但无疑我是错了。
 * _findfirst ... 不能在linux中使用，很是恼火。听说linux下可以使用
 * readdir，在Internet上search search。不过readdir确实更好用 :-)
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
	struct addition sadd;	/* 附加数据的格式说明数据结构 */
	
	if(!(fpdes = fopen("rootImage","ab"))) {
		printf("open rootImage error!!!\n");
		return -1;
	}
	
	/* 读取目录，并将目录中文件的数据写入... */
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
		
		/* 写入文件数据 */
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
		
		/* 块对齐。好吧，fseek还是不行，看来只能fwrite了。 */
		memset(buff, 0, 512);
		fwrite(buff, (512-((sadd.namelen+sadd.datalen
			+sizeof(struct addition))%512)) % 512, 1, fpdes);		
	}
	closedir(pdir);
	
	/* 写入一个空白块，这样就可以指示附加文件结束了 */
	memset(buff, 0, 512);
	fwrite(buff, sizeof(buff), 1, fpdes);
	
	/* 使硬盘磁道数为整数 */
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
	int addseccount = 0;	/* 文件系统后边添加的扇区数 */
	
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
