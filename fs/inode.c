/*
 * Small/fs/inode.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 此文件为内存inode处理文件。内存inode的处理方式与buffer的相似，
 * 都采用了空闲表和哈希表。故可用相似的空闲表和哈希表操作，此文件
 * 与buffer.c中的对空闲表和哈希表的处理程序雷同!其实我是从buffer.c
 * 直接复制过来的，这部分的注释就参见buffer吧 :-)
 *
 * [??] 写索引节点，write_inode的延迟写与状态标记延迟写(不调用
 * write_inode)，在使用时是否沉余，待查看!!!
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "file_system.h"
#include "types.h"
#include "string.h"
#include "sched.h"
#include "kernel.h"
#include "sys_set.h"

/* inode空闲表 */
static struct m_inode inode_table[NR_INODE] = {{0,},};
/* inode空闲表头部指针 */
static struct m_inode * free_head = NULL;
/* inode哈希表头部指针数组 */
static struct m_inode * hash_heads[NR_INODE_HASH] = {0};

/* 指向等待inode的进程 */
struct proc_struct * inode_wait = NULL;


int i_hash_fun(unsigned char dev, unsigned long nrinode)
{
	return (nrinode % NR_INODE_HASH);
}

struct m_inode * i_scan_hash(unsigned char dev, unsigned long nrinode)
{
	int index = i_hash_fun(dev, nrinode);
	struct m_inode *p = hash_heads[index];

	while(p) {
		if((p->state&NODE_VAL) && p->dev==dev && p->nrinode==nrinode)
			return p;
		p = p->p_hash_next;
		if(p == hash_heads[index])
			break;
	}
	return NULL;
}

struct m_inode * i_get_from_free(struct m_inode *p)
{
	if(p) {					/* 从表中摘下指定的空闲inode */
		if(p->p_free_next == p) {	/* 表中只有一个inode */
			free_head = NULL;
		} else {
			p->p_free_pre->p_free_next = p->p_free_next;
			p->p_free_next->p_free_pre = p->p_free_pre;
			if(p == free_head) {		/* p指向表头 */
				free_head = p->p_free_next;
			}
		}
	} else {
		if(NULL == free_head) {	/* 表空 */
			p = NULL;
		} else if(free_head->p_free_next == free_head) {/* 表中只有一个inode */
			p = free_head;
			free_head = NULL;
		} else {					/* 表中有多个inode */
			p = free_head;
			p->p_free_pre->p_free_next = p->p_free_next;
			p->p_free_next->p_free_pre = p->p_free_pre;
			free_head = p->p_free_next;
		}
	}
	return p;
}

void i_put_to_free(struct m_inode *p, BOOL ishead)
{
	if(!p)
		panic("i_put_to_free: pointer is NULL.");

	if(NULL == free_head) {	/* 空闲表空 */
		free_head = p;
		p->p_free_next = p;
		p->p_free_pre = p;
	} else {
		if(ishead) {		/* 放在头部 */
			p->p_free_next = free_head;
			p->p_free_pre = free_head->p_free_pre;
			free_head->p_free_pre->p_free_next = p;
			free_head->p_free_pre = p;
			free_head = p;
		} else {				/* 放在末尾 */
			p->p_free_pre = free_head->p_free_pre;
			free_head->p_free_pre->p_free_next = p;
			p->p_free_next = free_head;
			free_head->p_free_pre = p;
		}
	}
}

void i_remove_from_hash(struct m_inode *p)
{
	if(!p)
		panic("i_remove_from_hash: pointer is NULL.");
	if(!i_scan_hash(p->dev, p->nrinode))
		panic("i_remove_from_hash: not in hash-table.");

	int index = i_hash_fun(p->dev, p->nrinode);

	if(p->p_hash_next == p) {	/* 表中只有一个inode */
		hash_heads[index] = NULL;
	} else {
		p->p_hash_pre->p_hash_next = p->p_hash_next;
		p->p_hash_next->p_hash_pre = p->p_hash_pre;
		if(p == hash_heads[index]) {	/* p指向表头 */
			hash_heads[index] = p->p_hash_next;
		}
	}
}

void i_put_to_hash(struct m_inode *p,
			unsigned char newdev, unsigned long newnrinode)
{
	if(!p)
		panic("i_put_to_hash: pointer is NULL.");

	int index = i_hash_fun(newdev, newnrinode);

	p->dev = newdev;
	p->nrinode = newnrinode;
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

/* * 该函数将指定硬盘索引节点读入内存索引节点。 */
void read_inode(struct m_inode * p, unsigned char dev,
			unsigned int nrinode)
{
	if(!p)
		panic("read_inode: the m-inode-pointer can't be NULL.");

	d_printf("read_inode begin.\n");

	struct buffer_head * bh = bread(dev, INODE_TO_BLOCK(nrinode));
	memcpy(p, &bh->p_data[INODE_START_IN_BLOCK(nrinode)],
		sizeof(struct d_inode));
	brelse(bh);
}

/*
 * 该函数将指定内存索引节点写入硬盘索引节点。isnow指示是否立即写。
 */
void write_inode(struct m_inode * p, unsigned char dev,
				unsigned int nrinode, char isnow)
{
	if(!p)
		panic("write_inode: the m-inode-pointer can't be NULL.");

	struct buffer_head * bh = bread(dev, INODE_TO_BLOCK(p->nrinode));
	memcpy(&bh->p_data[INODE_START_IN_BLOCK(nrinode)], p,
		sizeof(struct d_inode));
	if(!isnow) {
		bh->state |= BUFF_DELAY_W;
		brelse(bh);
	} else {
		/* 去除延迟写标志，即使没有此标志 */
		bh->state &= (~BUFF_DELAY_W) & 0xff;
		bwrite(bh);
	}
}

/* * 该函数根据设备号和索引节点号获取指定索引节点。它返回含有
 * 有效数据的内存索引节点指针。注意：内存索引节点有引用计数。 */
struct m_inode * iget(unsigned char dev, unsigned int nrinode)
{
	struct m_inode * p = NULL;

	while(1) {
		if((p=i_scan_hash(dev, nrinode))) {/* 在散列表 */
			/* 在散列表，但上锁 */
			if(p->state & NODE_LOCK) {
				sleep_on(&inode_wait, INTERRUPTIBLE);
				continue;		/* 回到while循环 */
			}
			LOCK_INODE(p);
			/* 在散列表，且在空闲表 */
			if(!(p->count++)) {
				i_get_from_free(p);
			}
			return p;
		} else {	/* 不在散列表 */
			/* 不在散列表，且空闲表空 */
			if(NULL == free_head) {
				sleep_on(&inode_wait, INTERRUPTIBLE);
				continue;		/* 回到while循环 */
			}
			p = i_get_from_free(NULL);
			/* 不在散列表，在空闲表找到一个inode */
			LOCK_INODE(p);
			if(p->state & NODE_VAL) {
				i_remove_from_hash(p);
				p->state &= (~NODE_VAL) & 0xff;
			}
			i_put_to_hash(p, dev, nrinode);
			/* 读取磁盘索引节点 */
			read_inode(p, dev, nrinode);
			/* 调整内存索引节点 */
			p->state |= NODE_VAL;
			p->count = 1;
			return p;
		}
	}
}

/* * 该函数将指定内存索引节点释放，并根据其引用数做相应的操作。 */
void iput(struct m_inode * p)
{
	if(!p)
		panic("iput: the m-inode-pointer can't be NULL.");

	if(!(p->state&NODE_LOCK))
		LOCK_INODE(p);
	if(0 == (--p->count)) {
		if(p->state & NODE_VAL) {
			/* 连接数为0，释放文件磁盘块和索引节点 */
			if(0 == p->nlinks) {
				free_all(p);
				ifree(p->dev, p->nrinode);
			}
			if(p->state & NODE_I_ALT)
				write_inode(p, p->dev, p->nrinode, 0);
			i_put_to_free(p, 0);
		} else {
			p->state |= NODE_VAL;
			i_remove_from_hash(p);
			p->state &= (~NODE_VAL) & 0xff;
			i_put_to_free(p, 1);
		}
	}
	UNLOCK_INODE(p);
}

/*
 * 该函数将文件逻辑字节偏移量映射到文件磁盘块。若逻辑字节
 * 超过文件大小，返回FALSE；否则，映射结果记录在结构体中。
 */
BOOL bmap(struct m_inode * p, unsigned long v_offset,
		struct bmap_struct * s_bmap)
{
	if(!p || !s_bmap)
		panic("bmap: parameter is NULL.");
	if(v_offset >= p->size) {
		k_printf("bmap: the byte-offset out of limit.");
		return FALSE;
	}
	
	unsigned char step;		/* 间接级 */
	unsigned long v_nrblock = v_offset / 512;
	if(v_nrblock > (9+512/4+512/4*512/4)) {
		step = 3;
		s_bmap->nrblock = p->zone[12];
		v_nrblock -= 10 + 512/4 + 512/4*512/4;
	} else if(v_nrblock > (9+512/4)) {
		step = 2;
		s_bmap->nrblock = p->zone[11];
		v_nrblock -= 10 + 512/4;
	} else if(v_nrblock > 9) {	/* 10个直接块 */
		step = 1;
		s_bmap->nrblock = p->zone[10];
		v_nrblock -= 10;
	} else {
		step = 0;
		s_bmap->nrblock = p->zone[v_nrblock % 10];
		s_bmap->b_index = v_nrblock % 10;
	}
	s_bmap->step = step;
	s_bmap->offset = v_offset % 512;
	s_bmap->bytes = 512-((512-(p->size%512))%512);	/* 默认size>0 */
	s_bmap->mid_jmp_nrblock = 0;
	s_bmap->mid_b_index = 0;
	s_bmap->end_jmp_nrblock = 0;
	s_bmap->end_b_index = 0;
	
	unsigned long tmp, tmp_index;
	struct buffer_head * bh;
	while(step--) {
		/* 判断可能的文件空洞 */
		if(!s_bmap->nrblock)
			return TRUE;
		tmp = 1;
		for(int i=0; i<step; i++)
			tmp *= 512/4;
		tmp_index = v_nrblock / tmp;
		v_nrblock = v_nrblock % tmp;
		bh = bread(p->dev, s_bmap->nrblock);
		s_bmap->nrblock = *(unsigned long *)&bh->p_data[4*tmp_index];
		brelse(bh);
		if(2 == step) {
			s_bmap->mid_jmp_nrblock = s_bmap->nrblock;
			s_bmap->mid_b_index = tmp_index;			
		} else if(1 == step) {
			s_bmap->end_jmp_nrblock = s_bmap->nrblock;
			s_bmap->end_b_index = tmp_index;
		} else {
			s_bmap->b_index = tmp_index;
		}
	}

	return TRUE;
}

/*
 * name_find_dir_elem() 依据路径分量查找目录项。此函数包含两种功能：
 * 当输入的路径分量为空(并不是NULL,而是空字符串)时，查找空目录项；
 * 当路径分量不为空时，查找和该路径分量相等的目录项。同时注意：
 * 当找到对应项时，我们不释放该缓冲块，以便调用者可以使用该块。
 *
 * NOTE! 该函数默认输入的路径分量在内核地址空间，请牢记这一点。
 *
 * I: dir - 内存索引节点指针; path_part - 路径分量; s_bmap - 
 * 保存相关信息的bmap_struct指针。
 * O: 缓冲区指针。当找到对应项时，该缓冲区保存着对应数据。
 */
struct buffer_head * name_find_dir_elem(struct m_inode * dir, 
				char * path_part, struct bmap_struct * s_bmap)
{
	unsigned long offset = 0;
	struct buffer_head * bh = NULL;

	/* 读取目录文件，逐个目录项比较，若找到，则返回缓冲区指针 */
	while(offset < dir->size) {
		if(bmap(dir, offset, s_bmap) && s_bmap->nrblock) {
			bh = bread(dir->dev, s_bmap->nrblock);
			/* 此处已考虑到一个磁盘块存储整数个目录项 */
			while(s_bmap->offset+sizeof(struct file_dir_struct) <= s_bmap->bytes) {
				if(0 == strcmp(path_part,
					((struct file_dir_struct *)&bh->p_data[s_bmap->offset])->name))
					return bh;
				s_bmap->offset += sizeof(struct file_dir_struct);
			}
			brelse(bh);
		}
		offset = (offset/512+1)*512;
	}

	return NULL;
}

/*
 * nrinode_find_dir_elem() 依据索引节点号查找目录项。
 * 其他参见name_find_dir_elem()
 */
struct buffer_head * nrinode_find_dir_elem(struct m_inode * dir, 
			long nrinode, struct bmap_struct * s_bmap)
{
	unsigned long offset = 0;
	struct buffer_head * bh = NULL;

	/* 读取目录文件，逐个目录项比较，若找到，则返回缓冲区指针 */
	while(offset < dir->size) {
		if(bmap(dir, offset, s_bmap) && s_bmap->nrblock) {
			bh = bread(dir->dev, s_bmap->nrblock);
			/* 此处已考虑到一个磁盘块存储整数个目录项 */
			while(s_bmap->offset+sizeof(struct file_dir_struct) <= s_bmap->bytes) {
				if(nrinode ==
					((struct file_dir_struct *)&bh->p_data[s_bmap->offset])->nrinode)
					return bh;
				s_bmap->offset += sizeof(struct file_dir_struct);
			}
			brelse(bh);
		}
		offset = (offset/512+1)*512;
	}

	return NULL;
}

/*
 * is_dir_empty() 判断目录文件是否为空。当目录中除了空目录项之外，只有
 * 2种(而不是2个)目录项(".","..")时，目录文件为空。例外的是，系统根目录
 * 没有父目录，故当其内只有一个目录项“.”时，才为空。
 */
BOOL is_dir_empty(struct m_inode * p)
{
	unsigned long offset;
	struct buffer_head * bh;
	struct bmap_struct s_bmap;
	
	if(p!=current->root_dir && p->size>2*sizeof(struct file_dir_struct))
		offset = 2*sizeof(struct file_dir_struct);
	else if(p==current->root_dir && p->size>sizeof(struct file_dir_struct))
		offset = sizeof(struct file_dir_struct);
	else
		offset = p->size;
	
	/* 读取目录文件，逐个目录项比较，看能否找到“非空”目录项 */
	while(offset < p->size) {
		if(bmap(p, offset, &s_bmap) && s_bmap.nrblock) {
			bh = bread(p->dev, s_bmap.nrblock);
			/* 此处已考虑到一个磁盘块存储整数个目录项 */
			while(s_bmap.offset+sizeof(struct file_dir_struct) <= s_bmap.bytes) {
				if(0!=strcmp("", ((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->name)
				&& 0!=strcmp(".", ((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->name)
				&& 0!=strcmp("..", ((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->name)) {
					brelse(bh);
					return FALSE;
				}
				s_bmap.offset += sizeof(struct file_dir_struct);
			}
			brelse(bh);
		}
		offset = (offset/512+1)*512;
	}

	return TRUE;
}

/*
 * 该函数将指定路径名转换到索引节点。我们是从fs段获取路径的。
 * 内核允许输入空字符串(并不是NULL)，此时将得到进程当前工作目录。
 * 不管怎样，在同一个目录中，我们不允许有两个同名目录项，即便一个是
 * 文件，而另一个是目录，这也不允许，当然所说的名字是包含文件后缀的。
 */
struct m_inode * namei(char * name)
{
	int i;
	char * pstr = name;
	char path_part[MAX_PATH_PART+1];
	struct m_inode * dir;
	struct bmap_struct s_bmap;
	struct buffer_head * bh;

	if(!pstr) {
		k_printf("namei: name can't be NULL.");
		return NULL;
	}

	if('/' == get_fs_byte(pstr)) {	/* 根目录 */
		dir = iget(current->root_dir->dev, current->root_dir->nrinode);
		++pstr;
	} else {	/* 当前目录 */
		dir = iget(current->current_dir->dev, current->current_dir->nrinode);
	}

	/*
	 * 我们检查索引节点连接数是否为0，为0则抛出错误信息，关于这一点，
	 * 参见unlink()。此处应该检查下，因为可能不会执行while内部的代码。
	 */
	if(0 == dir->nlinks) {
		k_printf("namei: inode-link-count can't be 0. [1]");
		goto namei_false_ret;
	}
	
	while(get_fs_byte(pstr)) {
		/* 不是目录 */
		if(!(dir->mode&FILE_DIR)) {
			goto namei_false_ret;
		}
		/* 获取一个路径分量 */
		i = 0;
		while('/'!=get_fs_byte(pstr) && get_fs_byte(pstr))
			path_part[i++] = get_fs_byte(pstr++);
		path_part[i] = 0;
		if(get_fs_byte(pstr))
			++pstr;
		/* 根目录无父目录 */
		if(dir==current->root_dir && 0==strcmp(path_part,"..")) {
			goto namei_false_ret;
		}
		bh = name_find_dir_elem(dir, path_part, &s_bmap);
		if(bh) {	/* 如果找到 */
			iput(dir);
			dir = iget(bh->dev,
				((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->nrinode);
			brelse(bh);			/* name_find_dir_elem() 没有释放 */
			if(0 == dir->nlinks) {
				k_printf("namei: inode-link-count can't be 0. [2]");
				goto namei_false_ret;
			}
		} else {	/* 工作目录中无此分量 */
			goto namei_false_ret;
		}
	}

	return dir;

namei_false_ret:
	iput(dir);
	return NULL;
}

/*
 * fnamei() 获取父路径名对应的索引节点，同时获取父路径名和最后一个
 * 路径分量。NOTE! 并不是获取路径名对应节点的父节点，因为最后一个
 * 路径分量可能是"."/".."。
 *
 * I: name - 输入的文件总路径名;
 *	  fdirpath - 用来接收存储父路径名;
 *    lastpath - 用来接收存储最后一个路径分量.
 */
struct m_inode * fnamei(char * name, char * fdirpath, char * lastpath)
{
	char c;
	char * pstr = name;
	int i = 0, j = 0, k = 0;

	if(!pstr || !fdirpath || !lastpath) {
		k_printf("fnamei: parameter is NULL.");
		return NULL;
	}

	/* We get the father-pathname & last part-pathname of pathname. */
	c = get_fs_byte(pstr);
	while(c) {
		fdirpath[i++] = c;
		lastpath[k++] = c;
		pstr++;
		if('/'==c && get_fs_byte(pstr)) {
			j = i;
			k = 0;
		}
		c = get_fs_byte(pstr);
	}
	fdirpath[j] = 0;
	lastpath[k] = 0;

	if(0 == strcmp("/", lastpath)) {/* 根目录没有父节点 */
		return NULL;
	} else {
		unsigned short oldfs = set_fs_kernel();/* fdirpath在内核空间 */
		struct m_inode * p = namei(fdirpath);
		set_fs(oldfs);
		return p;
	}
}

void inode_init(void)
{
	int inodes = NR_INODE;					/* inode数 */
	struct m_inode * p = &inode_table[0];	/* 指向指向当前要处理inode */
	struct m_inode * plast = NULL;			/* 前一个inode头部指针 */

	free_head = p;
	inode_wait = NULL;

	/* 建立双向循环空闲inode链表 */
	while(inodes--) {
		p->p_free_next = p + 1;
		p->p_free_pre = plast;
		/* 其他成员无需初始化，bss已清零 */
		plast = p;
		p = p->p_free_next;
	}
	plast->p_free_next = free_head;
	free_head->p_free_pre = plast;

	/* 初始化哈希链表 */
	/* nothing.在bss段,已被清零 */
}
