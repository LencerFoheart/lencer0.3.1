/*
 * Small/include/file_system.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _FILE_SYSTEM_H_
#define _FILE_SYSTEM_H_

#include "types.h"

/********************************************************************/

#define BUFFER_SIZE		(512)

/* buffer state */
#define BUFF_LOCK		(1 << 0)	/* 上锁 */
#define BUFF_VAL		(1 << 1)	/* 有效 */
#define BUFF_DELAY_W	(1 << 2)	/* 延迟写 */

/* inode state */
#define NODE_LOCK		(1 << 0)	/* 上锁 */
#define NODE_I_ALT		(1 << 1)	/* 索引节点已修改 */
#define NODE_D_ALT		(1 << 2)	/* 文件数据已修改 */
#define NODE_MOUNT		(1 << 3)	/* 该文件是否安装 */
#define NODE_VAL		(1 << 4)	/* 有效 */

/* super-block state */
#define SUP_LOCK		(1 << 0)	/* 超级块锁 */
#define SUP_ALT			(1 << 1)	/* 超级块已修改 */

/* [inode] file type & file permit */
#define FILE_DIR		(1 << 0)	/* 目录 */
#define FILE_FILE		(1 << 1)	/* 文件 */
#define FILE_RONLY		(1 << 2)	/* 只读 */
#define FILE_RW			(1 << 3)	/* 读写 */
#define FILE_EXEC		(1 << 4)	/* 可执行 */

/* [open] file open type */
#define O_RONLY			(1)	/* 只读 */
#define O_WONLY			(2)	/* 只写，相当于修改，不是追加，也不是全清 */
#define O_RW			(3)	/* 读写，写时相当于修改，不是追加，也不是全清 */
#define O_CW			(4)	/* 清除之后写 */
#define O_WA			(5)	/* 以追加写的方式打开 */

/* lseek type */
#ifndef __GCC_LIBRARY__
#define SEEK_SET		(0)
#define SEEK_CUR		(1)
#define SEEK_END		(2)
#endif

/*
 * NOTE! 以下路径分量最大长度和完整路径长度的设置，
 * 以使一个磁盘块(512bytes)能存储整数个目录项为宜!
 */
#define MAX_PATH			(280)	/* 最大路径名长度 */
#define MAX_PATH_PART		(28)	/* 最大路径分量长度 */
#define PROC_MAX_FILES		(50)	/* 一个进程打开文件最大数 */
#define SYS_MAX_FILES		(256)	/* 系统打开文件最大数 */

#define NR_BUFF_HASH		(200)	/* 内存中缓冲区哈希表数组项数 */
#define NR_INODE_HASH		(15)	/* 内存中inode哈希表数组项数 */
#define NR_INODE			(50)	/* 内存中inode数组容量 */

#define NR_D_INODE			(300)	/* 磁盘中索引节点个数，最多存储的文件数。 */
#define NR_D_FBT			(90)	/* 磁盘中空闲块表项数 */
#define NR_D_FIT			(10)	/* 磁盘中空闲索引节点表项数 */

#define END_D_FB			(0)		/* 磁盘中空闲块结束标志 */


/********************************************************************/

/* 计算索引节点对应磁盘块。2为索引节点开始逻辑磁盘块，索引节点号从0开始。 */
#define INODE_TO_BLOCK(nrinode) (2 + ((nrinode) / (512 / sizeof(struct d_inode))))

/* 计算索引节点在磁盘块中的偏移量，索引节点号从0开始。 */
#define INODE_START_IN_BLOCK(nrinode) (sizeof(struct d_inode) * ((nrinode) % (512 / sizeof(struct d_inode))))

/* 该结构体记录bmap函数中文件逻辑字节偏移量到文件磁盘块的映射结果 */
struct bmap_struct {
	unsigned char step;				/* 间接级 */
	unsigned long mid_jmp_nrblock;	/* 三次间址中间一个间址块的块号 */
	unsigned long mid_b_index;		/* 三次间址中间一个间址块在上一级间址块中的索引值 */
	unsigned long end_jmp_nrblock;	/* 二次间址或三次间址的最后一个间址块的块号 */
	unsigned long end_b_index;		/* 二次间址或三次间址的最后一个间址块在上一级间址块中的索引值 */
	unsigned long nrblock;			/* 最终的数据块号 */
	unsigned long b_index;			/* 最终的数据块在前一级间接块的索引 */
	unsigned long offset;			/* 块中字节偏移量 */
	unsigned long bytes;			/* 块中可使用字节数，从块开始处到块结束或文件结束的字节数 */
};

/*
 * 目录文件的目录表项结构。本版内核默认是一块存储整数个目录项。
 * 我们判断目录项为空的依据是其文件名字段为空，而不能依据其索引
 * 节点号是否为0判断，因为根索引节点的索引节点号就为0。所以，在
 * 写入目录项和清空目录项时，均要查看或清空其文件名字段。
 */
struct file_dir_struct {
	unsigned long nrinode;
	char name[MAX_PATH_PART];	/* 为空，则表示该项空闲 */
};

/* 系统文件表表项结构 */
struct file_table_struct {
	struct m_inode * inode;	/* 索引节点指针 */
	unsigned short mode;	/* 读写权限等 */
	unsigned char count;	/* 引用数，是该表项是否空闲的标志 */
	unsigned long offset;	/* 偏移量 */
};

/* 文件信息结构，用于进程获取文件信息。与磁盘索引节点结构相似 */
struct file_info {
	unsigned short mode;			/* 文件类型、存取权限 */
	unsigned long create_t;			/* 创建时间 */
	unsigned long use_t;			/* 最后一次使用时间 */
	unsigned long alter_t;			/* 最后一次修改时间 */
	unsigned long i_alter_t;		/* 索引节点最后一次修改时间 */
	unsigned char nlinks;			/* 连接数 */
	unsigned long size;				/* 文件大小，字节 */
};

/* pipeline READ/WRITE describe-sign */
struct pipe_rw_desc {
	long rdesc;
	long wdesc;
};

/********************************************************************/

/* 缓冲头部 */
struct buffer_head {
	unsigned char dev;				/* 设备号 */
	unsigned long nrblock;			/* 块号 */
	unsigned char state;			/* 状态 */
	unsigned char * p_data;			/* 指向数据区域 */
	struct buffer_head *p_hash_next;/* 散列队列上的下一个缓冲区指针 */
	struct buffer_head *p_hash_pre;	/* 散列队列上的上一个缓冲区指针 */
	struct buffer_head *p_free_next;/* 空闲表上的下一个缓冲区指针 */
	struct buffer_head *p_free_pre;	/* 空闲表上的上一个缓冲区指针 */
};

/********************************************************************/

/* 磁盘索引节点，每个磁盘块只存储整数个索引节点 */
struct d_inode {
	unsigned short mode;			/* 文件类型、存取权限 */
	unsigned long create_t;			/* 创建时间 */
	unsigned long use_t;			/* 最后一次使用时间 */
	unsigned long alter_t;			/* 最后一次修改时间 */
	unsigned long i_alter_t;		/* 索引节点最后一次修改时间 */
	unsigned char nlinks;			/* 连接数 */
	unsigned long size;				/* 文件大小，字节 */
	unsigned long zone[13];			/* 0-9直接，10一次间址，11二次，12三次 */
};

/* 内存索引节点 */
struct m_inode {
	unsigned short mode;			/* 文件类型、存取权限 */
	unsigned long create_t;			/* 创建时间 */
	unsigned long use_t;			/* 最后一次使用时间 */
	unsigned long alter_t;			/* 最后一次修改时间 */
	unsigned long i_alter_t;		/* 索引节点最后一次修改时间 */
	unsigned char nlinks;			/* 连接数 */
	unsigned long size;				/* 文件大小，字节 */
	unsigned long zone[13];			/* 0-9直接，10一次间址，11二次，12三次 */
	/* the above same to disk */
	unsigned char count;			/* 引用数 */
	unsigned char state;			/* 内存索引节点状态 */
	unsigned char dev;				/* 设备号 */
	unsigned long nrinode;			/* 索引节点号 */
	struct m_inode *p_hash_next;	/* 散列队列上的下一个缓冲区指针 */
	struct m_inode *p_hash_pre;		/* 散列队列上的上一个缓冲区指针 */
	struct m_inode *p_free_next;	/* 空闲表上的下一个缓冲区指针 */
	struct m_inode *p_free_pre;		/* 空闲表上的上一个缓冲区指针 */
};

/********************************************************************/

/* 磁盘超级块 */
struct d_super_block {
	unsigned long size;				/* 文件系统尺寸，磁盘块数 */
	unsigned long nfreeblocks;		/* 文件系统中空闲块个数 */
	unsigned long fb_table[NR_D_FBT];	/* 空闲块表，空闲块索引表 */
	unsigned char nextfb;			/* 空闲块表中下一个空闲块 */
	unsigned long ninodes;			/* 索引节点表大小，索引节点个数 */
	unsigned long nfreeinodes;		/* 空闲索引节点个数 */
	unsigned long fi_table[NR_D_FIT];	/* 空闲索引节点表，空闲索引节点索引表 */
	unsigned char nextfi;			/* 空闲索引节点表中下一个空闲索引节点 */
	unsigned long store_nexti;		/* 铭记的索引节点表中下一个开始搜索的索引节点号 */
};

/* 内存超级块 */
struct m_super_block {
	unsigned long size;				/* 文件系统尺寸，磁盘块数 */
	unsigned long nfreeblocks;		/* 文件系统中空闲块个数 */
	unsigned long fb_table[NR_D_FBT];	/* 空闲块表，空闲块索引表 */
	unsigned char nextfb;			/* 空闲块表中下一个空闲块 */
	unsigned long ninodes;			/* 索引节点表大小，索引节点个数 */
	unsigned long nfreeinodes;		/* 空闲索引节点个数 */
	unsigned long fi_table[NR_D_FIT];	/* 空闲索引节点表，空闲索引节点索引表 */
	unsigned char nextfi;			/* 空闲索引节点表中下一个空闲索引节点 */
	unsigned long store_nexti;		/* 铭记的索引节点表中下一个开始搜索的索引节点号 */
	/* the above same to disk */
	unsigned char state;			/* 状态 */
};

/********************************************************************/

extern struct proc_struct * inode_wait;
extern struct proc_struct * buff_wait;

#define LOCK_BUFF(p) \
do { \
	(p)->state |= BUFF_LOCK; \
} while(0)

#define UNLOCK_BUFF(p) \
do { \
	(p)->state &= (~BUFF_LOCK) & 0xff; \
	if(NULL != buff_wait) { \
		wake_up(&buff_wait); \
	} \
} while(0)


#define LOCK_INODE(p) \
do { \
	(p)->state |= NODE_LOCK; \
} while(0)

#define UNLOCK_INODE(p) \
do { \
	(p)->state &= (~NODE_LOCK) & 0xff; \
	if(NULL != inode_wait) { \
		wake_up(&inode_wait); \
	} \
} while(0)


#define LOCK_SUPER(p) \
do { \
	(p)->state |= SUP_LOCK; \
} while(0)

#define UNLOCK_SUPER(p) \
do { \
	(p)->state &= (~SUP_LOCK) & 0xff; \
	if(NULL != super_wait) { \
		wake_up(&super_wait); \
	} \
} while(0)

/* buffer */
extern struct buffer_head * getblk(unsigned char dev, unsigned long nrblock);
extern void brelse(struct buffer_head *p);
extern struct buffer_head * bread(unsigned char dev, unsigned long nrblock);
extern void bwrite(struct buffer_head *p);
extern void buff_init(void);

/* inode */
extern void read_inode(struct m_inode * p, unsigned char dev, unsigned int nrinode);
extern void write_inode(struct m_inode * p, unsigned char dev, unsigned int nrinode, char isnow);
extern struct m_inode * iget(unsigned char dev, unsigned int nrinode);
extern void iput(struct m_inode * p);
extern BOOL bmap(struct m_inode * p, unsigned long v_offset, struct bmap_struct * s_bmap);
extern struct buffer_head * name_find_dir_elem(struct m_inode * dir, 
				char * path_part, struct bmap_struct * s_bmap);
extern struct buffer_head * nrinode_find_dir_elem(struct m_inode * dir, 
			long nrinode, struct bmap_struct * s_bmap);
extern BOOL is_dir_empty(struct m_inode * p);
extern struct m_inode * namei(char * name);
extern struct m_inode * fnamei(char * name, char * fdirpath, char * newpartpath);
extern void inode_init(void);

/* alloc */
extern void read_super(unsigned char dev);
extern void write_super(unsigned char dev);
extern struct m_inode * ialloc(unsigned char dev);
extern void ifree(unsigned char dev, unsigned int nrinode);
extern struct buffer_head * alloc(unsigned char dev);
extern BOOL check_alloc_data_block(struct m_inode * p, unsigned long offset);
#ifndef __GCC_LIBRARY__
extern void free(unsigned char dev, unsigned long nrblock);
#endif
extern void free_all(struct m_inode * p);
extern void super_init(unsigned char dev);

/* read_write */
extern long find_file_table(struct m_inode * p, unsigned short mode);
extern long sys_open(char * pathname, unsigned short mode);
extern BOOL put_to_dir(struct m_inode * dir, unsigned long nrinode, char * path_part);
extern struct m_inode * _creat(char * name, unsigned short mode);
extern long sys_creat(char * pathname, unsigned short mode);
extern long sys_read(long desc, void * buff, unsigned long count);
extern long sys_write(long desc, void * buff, unsigned long count);
extern long sys_lseek(long desc, long offset, unsigned char reference);
extern long sys_close(long desc);
extern long sys_dup(long desc);
extern void file_table_init(void);

/* stat */
extern BOOL sys_fstat(long desc, struct file_info * f_info);
extern BOOL sys_stat(char * name, struct file_info * f_info);

/* sync */
extern void sys_sync(unsigned char dev);

/* pipe */
extern BOOL sys_pipe(struct pipe_rw_desc * p_rw);

/* link_mount */
extern BOOL sys_link(char * desname, char * srcname);
extern BOOL sys_unlink(char * name);

/* chdir */
extern BOOL sys_chdir(char * dirname);
extern BOOL sys_chroot(char * dirname);

/********************************************************************/

#endif /* _FILE_SYSTEM_H_ */