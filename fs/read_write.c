/*
 * Small/fs/read_write.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * 该文件包含创建、打开、读写、关闭等与文件相关的操作。
 *
 * NOTE! 文件系统的系统调用一般情况下，出错返回-1。
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "file_system.h"
#include "sched.h"
#include "kernel.h"
#include "string.h"
#include "types.h"
#include "sys_set.h"

/* 系统文件表 */
struct file_table_struct file_table[SYS_MAX_FILES] = {{0,},};

/*
 * 检查文件打开类型是否合法。此处只过滤非法的，其余皆为合法。
 * NOTE! 内核检查这种非法情况，当文件为目录类型，是不允许被
 * 进程以可写的方式打开的!!!
 */
BOOL check_open_file_permit(struct m_inode * p, unsigned short mode)
{
	BOOL isok = TRUE;
	switch(mode) {
	case O_WONLY:
	case O_RW:
	case O_CW:
	case O_WA:
		if(!(FILE_RW&p->mode)
		|| (p->mode&FILE_DIR)) {
			isok = FALSE;
		}
		break;
	default:
		/* nothing */
		break;
	}
	return isok;
}

/*
 * 分配系统文件表项。若分配失败，返回-1，否则返回索引值。
 */
long find_sys_file_table(void)
{
	int i = 0;
	for(; i<SYS_MAX_FILES && file_table[i].count; i++) {
		/* nothing, it's right */
	}
	return (i==SYS_MAX_FILES) ? -1 : i;
}

/*
 * 分配用户文件描述符表项。若分配失败，返回-1，否则返回用户文件描述符。
 */
long find_user_file_table(void)
{
	int j = 0;
	for(; j<PROC_MAX_FILES && current->file[j]; j++) {
		/* nothing, it's right */
	}
	return (j==PROC_MAX_FILES) ? -1 : j;
}

/*
 * 分配系统文件表项和用户文件描述符表项，并进行设置。
 * 返回用户文件描述符，若分配失败，返回-1。
 */
long find_file_table(struct m_inode * p, unsigned short mode)
{
	int i = find_sys_file_table();
	if(-1 == i)
		return -1;
	/*
	 * 我们将查找进程文件描述符表的操作，紧随其后，以防万一...
	 */
	int j = find_user_file_table();
	if(-1 == j)
		return -1;
	/* 设置文件表项 */
	file_table[i].inode = p;
	file_table[i].mode = mode;
	file_table[i].count = 1;
	file_table[i].offset = 0;
	/* 设置进程文件描述符表 */
	current->file[j] = &file_table[i];

	return j;
}

/*
 * Open file which have been created.
 * I: pathname - 文件名; mode - 打开类型
 * O: 文件描述符，错误输出-1
 */
long sys_open(char * pathname, unsigned short mode)
{
	char * name = pathname;
	struct m_inode * p = namei(name);
	if(!p) {
		k_printf("open: have no specified file.");
		return -1;
	}
	if(!check_open_file_permit(p, mode)) {
		k_printf("open: have no permit to open specified file.");
		goto open_false_ret;
	}
	int j = find_file_table(p, mode);
	if(-1 == j) {
		k_printf("open: sys-file-table or proc-file-desc-table is full.");
		goto open_false_ret;
	}
	/* 打开类型为清文件，则释放所有磁盘块 */
	if(O_CW == mode) {
		free_all(p);
		p->size = 0;
		p->state |= NODE_I_ALT;
		p->state |= NODE_D_ALT;
		/* 设置文件修改时间等 */
		/* [??] nothing at present */
	}
	/* 打开类型为追加写，修改偏移量 */
	if(O_WA == mode) {
		current->file[j]->offset = p->size;
	}
	/*
	 * 解锁索引节点，在namei中加锁。NOTE! 此处确实只是解锁，并不是iput,
	 * 因为open之后，文件还要被使用，故需保持该索引节点在内存，即不能
	 * 减小其引用计数值。实际上在close中iput，正是此做法让内存索引节点
	 * 的引用计数值发挥了作用!
	 */
	UNLOCK_INODE(p);

	return j;

open_false_ret:
	iput(p);
	return -1;
}

/*
 * 当创建文件时，若文件已存在，创建时将清除原来的文件内容，因此检查
 * 创建文件的权限是否合法。NOTE! 内核检查这种非法情况，当文件为目录
 * 类型，是不允许被进程清空的!!!
 */
BOOL check_creat_file_permit(struct m_inode * p, unsigned short mode)
{
	return ((p->mode&FILE_RONLY)||(p->mode&FILE_DIR)) ? FALSE : TRUE;
}

/*
 * 将索引节点号和路径分量放入目录文件中。该函数首先查找目录文件的空项，
 * 若找到，则将其写入空项；否则，在目录文件末尾写入。我们判断目录项为
 * 空的依据是其文件名字段为空，而不能依据其索引节点号是否为0判断，因
 * 为根索引节点的索引节点号就为0.
 *
 * NOTE! 该函数默认输入的路径分量在内核地址空间，请牢记这一点。
 */
BOOL put_to_dir(struct m_inode * dir, unsigned long nrinode, char * path_part)
{
	unsigned long offset;
	struct bmap_struct s_bmap;
	struct buffer_head * bh;

	/* 先找目录空项 */
	bh = name_find_dir_elem(dir, "", &s_bmap);

	/* 如果没找到空项 */
	if(!bh) {
		offset = (512-(dir->size%512)) < sizeof(struct file_dir_struct) ?
			((dir->size/512+1)*512) : dir->size;
		if(!check_alloc_data_block(dir, offset))/* 若分配数据块失败 */
			return FALSE;
		dir->size = offset + sizeof(struct file_dir_struct);
		bmap(dir, offset, &s_bmap);
		/* 标记索引节点已修改 */
		dir->state |= NODE_I_ALT;
		dir->state |= NODE_D_ALT;
		bh = bread(dir->dev, s_bmap.nrblock);
	}

	/* 写入目录项 */
	((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->nrinode = nrinode;
	strcpy(((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->name, path_part);

	/* 要将该缓冲块写入磁盘 */
	bh->state |= BUFF_DELAY_W;
	brelse(bh);				/* name_find_dir_elem() 也没有释放 */

	return TRUE;
}

/*
 * 当文件不存在时，creat将调用该函数，创建一个文件。
 *
 * 该函数每次循环都用到了namei，因此效率不高，实际上这是没必要的，
 * 但目前先这样，能工作就行 :-)
 *
 * 对于路径上有很多目录不存在的情况，我们要一步一步去创建目录，此
 * 时若遇到创建某个目录或文件失败的情况，我们不把先前创建的目录删
 * 除，而只把当前创建错误的目录或文件的已分配的资源收回。
 *
 * NOTE: 目录是可以被创建的。
 */
struct m_inode * _creat(char * name, unsigned short mode)
{
	struct m_inode * dir;					/* 当前目录或文件 */
	struct m_inode * last_dir;				/* 上一级目录 */
	char path[MAX_PATH+1];					/* 当前全路径 */
	char path_part[MAX_PATH_PART+1];		/* 当前路径分量 */
	unsigned short type;					/* 文件类型 */
	unsigned short oldfs;
	char * pstr = name;
	int i, j;

	i = 0;
	dir = last_dir = NULL;
	if('/' == get_fs_byte(pstr)) {	/* 根目录 */
		dir = iget(current->root_dir->dev, current->root_dir->nrinode);
		path[i++] = get_fs_byte(pstr++);
	} else {	/* 当前目录 */
		dir = iget(current->current_dir->dev, current->current_dir->nrinode);
	}
	while(get_fs_byte(pstr)) {
		j = 0;
		type = 0;
		/* 获取一个路径分量 */
		while(get_fs_byte(pstr) && get_fs_byte(pstr)!='/') {
			path_part[j++] = path[i++] = get_fs_byte(pstr++);
		}
		if(get_fs_byte(pstr)) {
			type |= FILE_DIR;
			path[i++] = get_fs_byte(pstr++);
		}
		path[i] = 0;
		path_part[j] = 0;
		/*
		 * 保存上一级目录，同时解锁索引节点，以便namei。之后再加锁。
		 */
		last_dir = dir;
		UNLOCK_INODE(last_dir);
		/* 当前目录或文件存在 */
		oldfs = set_fs_kernel();
		if((dir = namei(path))) {
			set_fs(oldfs);
			iput(last_dir);
			continue;
		}
		set_fs(oldfs);
		LOCK_INODE(last_dir);
		/* 当前目录或文件不存在 */
		if(!(dir = ialloc(0))) {
			k_printf("_creat: ialloc return NULL.");
			goto _creat_false_ret_1;
		}
		/* 若是目录类型，则设置其第1,2个目录项为 '.', '..' */
		if(type & FILE_DIR) {
			if(!put_to_dir(dir, dir->nrinode, ".") 
			|| !put_to_dir(dir, last_dir->nrinode, "..")) {
				k_printf("_creat: put_to_dir return FALSE.");
				goto _creat_false_ret_2;
			}
			dir->mode |= FILE_DIR;
		} else {
			dir->mode |= FILE_FILE;
		}
		dir->mode |= mode;
		/* 放入上一级目录中 */
		if(!put_to_dir(last_dir, dir->nrinode, path_part)) {
			k_printf("_creat: put_to_dir return FALSE.");
			goto _creat_false_ret_2;
		}
		last_dir->state |= NODE_I_ALT;
		last_dir->state |= NODE_D_ALT;

		iput(last_dir);
	}

	return dir;

_creat_false_ret_2:
	ifree(dir->dev, dir->nrinode);
	/* 此处本来需要iput的，但下面有 */
_creat_false_ret_1:
	if(last_dir)
		iput(last_dir);
	if(dir)
		iput(dir);
	return NULL;
}

/*
 * Create file. 该函数首先分配文件表等，若文件表等满了，就不用继续下面
 * 操作了。注意分配文件表等时，不设置其索引节点指针，因为还不知道它是
 * 多少，等文件创建好之后我们再设置它。为什么不等创建好文件之后，再分
 * 配文件表等呢？原因有两个：
 * 1. 若文件表等若满了，则无需下面操作，提高了效率；
 * 2. 若等到我们创建好文件之后，才发现文件表等满了，此时是需要回退的，
 *	  也即需将创建的文件释放，这时的情况就复杂了...
 *
 * I: pathname - 文件名; mode - 打开类型
 * O: 文件描述符，错误输出-1
 */
long sys_creat(char * pathname, unsigned short mode)
{
	char * name = pathname;
	if(!name) {
		k_printf("creat: path-name is NULL.");
		return -1;
	}
	int j = find_file_table(NULL, mode);
	if(-1 == j) {
		k_printf("creat: sys-file-table or proc-file-desc-table is full.");
		return -1;
	}
	/* 检查文件是否存在，分别做出不同处理 */
	struct m_inode * p = namei(name);
	if(p) {
		if(!check_creat_file_permit(p, mode)) {
			k_printf("creat: have no permit to visit specified file.");
			goto creat_false_ret_2;
		}
		free_all(p);
		p->size = 0;
	} else {
		if(!(p = _creat(name, mode))) {
			k_printf("creat: call _creat and return NULL.");
			goto creat_false_ret_1;
		}
	}
	/* 设置文件表的索引节点指针 */
	current->file[j]->inode = p;
	/* 设置内存索引节点属性 */
	p->mode |= mode;
	p->state |= NODE_I_ALT;
	p->state |= NODE_D_ALT;
	/* [??] set time..., nothing at present */
	UNLOCK_INODE(p);

	return j;

creat_false_ret_2:
	iput(p);
creat_false_ret_1:
	current->file[j]->count = 0;
	current->file[j] = NULL;
	return -1;
}

/*
 * Well, we begin to read file. 我们知道，文件会有空洞，这样允许随意
 * 偏移文件指针，以便读写。同样，在读取文件时，若遇到空洞，则向用户
 * 缓冲区中填补0.
 * I: desc - 文件描述符; buff - 用户缓冲地址; count - 要读的字节数
 * O: 拷贝到用户区的字节数
 */
long sys_read(long desc, void * buff, unsigned long count)
{
	struct buffer_head * bh;
	unsigned long readcount = 0;	/* 已读取的字节数 */
	unsigned long oneread = 0;		/* 一次要读取的字节数 */
	struct bmap_struct s_bmap;
	unsigned char * rb = (unsigned char *)buff;
	struct file_table_struct * p_file = current->file[desc];
	
	if(!p_file) {
		k_printf("read: trying to read unknow-file.");
		return -1;
	}
	/* 检查许可权限 */
	/* [??] nothing at present */
	struct m_inode * p = p_file->inode;
	if(!p)
		panic("read: BAD BAD, file-desc is OK, but m-inode-pointer is NULL!");
	LOCK_INODE(p);
	/* 开始读取 */
	while(readcount<count && p_file->offset<p->size) {
		if(!bmap(p, p_file->offset, &s_bmap))
			panic("read: bmap return FALSE.");
		/* 计算一次读取的字节数 */
		oneread = 512;
		oneread = (oneread<(count-readcount)) ? oneread : (count-readcount);
		oneread = (oneread<(p->size-p_file->offset)) ? oneread : (p->size-p_file->offset);
		oneread = (oneread<(512-s_bmap.offset)) ? oneread : (512-s_bmap.offset);
		/* 读取一次 */
		if(NULL == s_bmap.nrblock) {	/* 处理文件空洞 */
			for(int i=0; i<oneread; i++) {
				set_fs_byte(&rb[readcount++], 0);
			}
		} else {	/* 非空洞 */
			bh = bread(p->dev, s_bmap.nrblock);
			ds2fs_memcpy(&rb[readcount], &bh->p_data[s_bmap.offset], oneread);
			brelse(bh);
			readcount += oneread;
		}
		p_file->offset += oneread;
	}
	if(readcount) {
		/* 修改访问时间等 */
		/* [??] nothing at present */
	}
	UNLOCK_INODE(p);

	return readcount;
}

/*
 * Write file. 参数以及返回值同read. Well, write file and read file is
 * simple. 但是，我们做一些优化，若是写整块数据，则不必读取该块，只需
 * 写入即可，若是只写入该块的一部分，则需读入该块。
 * NOTE! 目录是不可写的!
 */
long sys_write(long desc, void * buff, unsigned long count)
{
	struct buffer_head * bh;
	unsigned long writecount = 0;	/* 已写的字节数 */
	unsigned long onewrite = 0;		/* 一次要写的字节数 */
	struct bmap_struct s_bmap;
	unsigned char * wb = (unsigned char *)buff;
	
	if(!wb && count>0) {
		k_printf("write: write-count > 0. but, write-buffer-pointer is NULL.");
		return -1;
	}
	struct file_table_struct * p_file = current->file[desc];
	if(!p_file) {
		k_printf("write: trying to write unknow-file.");
		return -1;
	}
	struct m_inode * p = p_file->inode;
	/* 检查许可权限 */
	if(O_RONLY==p_file->mode || p->mode&FILE_DIR) {
		k_printf("write: have no permit to write the file.");
		return -1;
	}
	if(!p)
		panic("write: BAD BAD, file-desc is OK, but m-inode-pointer is NULL!");
	LOCK_INODE(p);
	/* 开始写 */
	while(writecount < count) {
		/* 检查数据块或间接块是否存在，若不存在则分配 */
		if(!check_alloc_data_block(p, p_file->offset)) {
			k_printf("write: check-alloc-disk-block ERROR!");
			return -1;
		}

		bmap(p, p_file->offset, &s_bmap);
		/* 计算一次写的字节数 */
		onewrite = 512;
		onewrite = (onewrite<(count-writecount)) ? onewrite : (count-writecount);
		onewrite = (onewrite<(512-s_bmap.offset)) ? onewrite : (512-s_bmap.offset);

		/* 写一次 */
		if(512 == onewrite) {
			bh = getblk(p->dev, s_bmap.nrblock);
			bh->state |= BUFF_VAL;	/* getblk不会将有效位置位 */
		} else {
			bh = bread(p->dev, s_bmap.nrblock);
		}
		fs2ds_memcpy(&bh->p_data[s_bmap.offset], &wb[writecount], onewrite);
		bh->state |= BUFF_DELAY_W;	/* 延迟写 */
		brelse(bh);

		writecount += onewrite;
		p_file->offset += onewrite;
	}
	/* 文件变大，则修改size字段 */
	p->size = (p->size>p_file->offset) ? p->size : p_file->offset;
	if(writecount) {
		p->state |= NODE_I_ALT;
		p->state |= NODE_D_ALT;
		/* 修改文件时间等 */
		/* [??] nothing at present */
	}
	UNLOCK_INODE(p);

	return writecount;
}

/*
 * 该函数调整文件输入/输出位置，即字节偏移指针。其中reference为
 * 字节偏移量依据位置。lseek判断当前打开的文件状态，若是只读，
 * 则偏移量不可以超出size，否则可以...
 */
long sys_lseek(long desc, long offset, unsigned char reference)
{
	struct file_table_struct * p_file = current->file[desc];
	if(!p_file) {
		k_printf("lseek: trying to handle unknow-file.");
		return -1;
	}
	struct m_inode * p = p_file->inode;
	if(!p)
		panic("lseek: BAD BAD, file-desc is OK, but m-inode-pointer is NULL!");
	if(reference!=SEEK_SET && reference!=SEEK_CUR && reference!=SEEK_END) {
		k_printf("lseek: BAD reference.");
		return -1;
	}

	offset += reference==SEEK_SET ? 0 :
		(reference==SEEK_CUR ? p_file->offset : p->size);
	offset = offset<0 ? 0 : offset;
	offset = (O_RONLY==p_file->mode && offset>p->size) ? p->size : offset;
	p_file->offset = offset;

	return p_file->offset;
}

long sys_close(long desc)
{
	struct file_table_struct * p_file = current->file[desc];
	if(!p_file) {
		k_printf("close: trying to close unknow-file.");
		return -1;
	}
	struct m_inode * p = p_file->inode;
	if(!p)
		panic("close: BAD BAD, file-desc is OK, but m-inode-pointer is NULL!");

	current->file[desc] = NULL;
	if(--p_file->count > 0)
		return 1;
	/*
	 * 当系统文件表项的引用计数为0时，内核将它释放，但内核
	 * 不清空空闲的系统文件表项，内核再次寻找空闲表项时，
	 * 依据其引用计数值来确定是否空闲。
	 */
	iput(p);

	return 1;
}

/*
 * dup复制文件描述符，适用于任何文件类型。
 * I:用于复制的描述符, O:新描述符.
 */
long sys_dup(long desc)
{
	int i = find_user_file_table();
	if(-1 == i)
		return -1;
	current->file[i] = current->file[desc];
	current->file[i]->count++;
	return i;
}

void file_table_init(void)
{
	/* nothing.在bss段,已被清零 */
}
