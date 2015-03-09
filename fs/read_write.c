/*
 * Small/fs/read_write.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * ���ļ������������򿪡���д���رյ����ļ���صĲ�����
 *
 * NOTE! �ļ�ϵͳ��ϵͳ����һ������£�������-1��
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

/* ϵͳ�ļ��� */
struct file_table_struct file_table[SYS_MAX_FILES] = {{0,},};

/*
 * ����ļ��������Ƿ�Ϸ����˴�ֻ���˷Ƿ��ģ������Ϊ�Ϸ���
 * NOTE! �ں˼�����ַǷ���������ļ�ΪĿ¼���ͣ��ǲ�����
 * �����Կ�д�ķ�ʽ�򿪵�!!!
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
 * ����ϵͳ�ļ����������ʧ�ܣ�����-1�����򷵻�����ֵ��
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
 * �����û��ļ����������������ʧ�ܣ�����-1�����򷵻��û��ļ���������
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
 * ����ϵͳ�ļ�������û��ļ�������������������á�
 * �����û��ļ���������������ʧ�ܣ�����-1��
 */
long find_file_table(struct m_inode * p, unsigned short mode)
{
	int i = find_sys_file_table();
	if(-1 == i)
		return -1;
	/*
	 * ���ǽ����ҽ����ļ���������Ĳ�������������Է���һ...
	 */
	int j = find_user_file_table();
	if(-1 == j)
		return -1;
	/* �����ļ����� */
	file_table[i].inode = p;
	file_table[i].mode = mode;
	file_table[i].count = 1;
	file_table[i].offset = 0;
	/* ���ý����ļ��������� */
	current->file[j] = &file_table[i];

	return j;
}

/*
 * Open file which have been created.
 * I: pathname - �ļ���; mode - ������
 * O: �ļ����������������-1
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
	/* ������Ϊ���ļ������ͷ����д��̿� */
	if(O_CW == mode) {
		free_all(p);
		p->size = 0;
		p->state |= NODE_I_ALT;
		p->state |= NODE_D_ALT;
		/* �����ļ��޸�ʱ��� */
		/* [??] nothing at present */
	}
	/* ������Ϊ׷��д���޸�ƫ���� */
	if(O_WA == mode) {
		current->file[j]->offset = p->size;
	}
	/*
	 * ���������ڵ㣬��namei�м�����NOTE! �˴�ȷʵֻ�ǽ�����������iput,
	 * ��Ϊopen֮���ļ���Ҫ��ʹ�ã����豣�ָ������ڵ����ڴ棬������
	 * ��С�����ü���ֵ��ʵ������close��iput�����Ǵ��������ڴ������ڵ�
	 * �����ü���ֵ����������!
	 */
	UNLOCK_INODE(p);

	return j;

open_false_ret:
	iput(p);
	return -1;
}

/*
 * �������ļ�ʱ�����ļ��Ѵ��ڣ�����ʱ�����ԭ�����ļ����ݣ���˼��
 * �����ļ���Ȩ���Ƿ�Ϸ���NOTE! �ں˼�����ַǷ���������ļ�ΪĿ¼
 * ���ͣ��ǲ�����������յ�!!!
 */
BOOL check_creat_file_permit(struct m_inode * p, unsigned short mode)
{
	return ((p->mode&FILE_RONLY)||(p->mode&FILE_DIR)) ? FALSE : TRUE;
}

/*
 * �������ڵ�ź�·����������Ŀ¼�ļ��С��ú������Ȳ���Ŀ¼�ļ��Ŀ��
 * ���ҵ�������д����������Ŀ¼�ļ�ĩβд�롣�����ж�Ŀ¼��Ϊ
 * �յ����������ļ����ֶ�Ϊ�գ������������������ڵ���Ƿ�Ϊ0�жϣ���
 * Ϊ�������ڵ�������ڵ�ž�Ϊ0.
 *
 * NOTE! �ú���Ĭ�������·���������ں˵�ַ�ռ䣬���μ���һ�㡣
 */
BOOL put_to_dir(struct m_inode * dir, unsigned long nrinode, char * path_part)
{
	unsigned long offset;
	struct bmap_struct s_bmap;
	struct buffer_head * bh;

	/* ����Ŀ¼���� */
	bh = name_find_dir_elem(dir, "", &s_bmap);

	/* ���û�ҵ����� */
	if(!bh) {
		offset = (512-(dir->size%512)) < sizeof(struct file_dir_struct) ?
			((dir->size/512+1)*512) : dir->size;
		if(!check_alloc_data_block(dir, offset))/* ���������ݿ�ʧ�� */
			return FALSE;
		dir->size = offset + sizeof(struct file_dir_struct);
		bmap(dir, offset, &s_bmap);
		/* ��������ڵ����޸� */
		dir->state |= NODE_I_ALT;
		dir->state |= NODE_D_ALT;
		bh = bread(dir->dev, s_bmap.nrblock);
	}

	/* д��Ŀ¼�� */
	((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->nrinode = nrinode;
	strcpy(((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->name, path_part);

	/* Ҫ���û����д����� */
	bh->state |= BUFF_DELAY_W;
	brelse(bh);				/* name_find_dir_elem() Ҳû���ͷ� */

	return TRUE;
}

/*
 * ���ļ�������ʱ��creat�����øú���������һ���ļ���
 *
 * �ú���ÿ��ѭ�����õ���namei�����Ч�ʲ��ߣ�ʵ��������û��Ҫ�ģ�
 * ��Ŀǰ���������ܹ������� :-)
 *
 * ����·�����кܶ�Ŀ¼�����ڵ����������Ҫһ��һ��ȥ����Ŀ¼����
 * ʱ����������ĳ��Ŀ¼���ļ�ʧ�ܵ���������ǲ�����ǰ������Ŀ¼ɾ
 * ������ֻ�ѵ�ǰ���������Ŀ¼���ļ����ѷ������Դ�ջء�
 *
 * NOTE: Ŀ¼�ǿ��Ա������ġ�
 */
struct m_inode * _creat(char * name, unsigned short mode)
{
	struct m_inode * dir;					/* ��ǰĿ¼���ļ� */
	struct m_inode * last_dir;				/* ��һ��Ŀ¼ */
	char path[MAX_PATH+1];					/* ��ǰȫ·�� */
	char path_part[MAX_PATH_PART+1];		/* ��ǰ·������ */
	unsigned short type;					/* �ļ����� */
	unsigned short oldfs;
	char * pstr = name;
	int i, j;

	i = 0;
	dir = last_dir = NULL;
	if('/' == get_fs_byte(pstr)) {	/* ��Ŀ¼ */
		dir = iget(current->root_dir->dev, current->root_dir->nrinode);
		path[i++] = get_fs_byte(pstr++);
	} else {	/* ��ǰĿ¼ */
		dir = iget(current->current_dir->dev, current->current_dir->nrinode);
	}
	while(get_fs_byte(pstr)) {
		j = 0;
		type = 0;
		/* ��ȡһ��·������ */
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
		 * ������һ��Ŀ¼��ͬʱ���������ڵ㣬�Ա�namei��֮���ټ�����
		 */
		last_dir = dir;
		UNLOCK_INODE(last_dir);
		/* ��ǰĿ¼���ļ����� */
		oldfs = set_fs_kernel();
		if((dir = namei(path))) {
			set_fs(oldfs);
			iput(last_dir);
			continue;
		}
		set_fs(oldfs);
		LOCK_INODE(last_dir);
		/* ��ǰĿ¼���ļ������� */
		if(!(dir = ialloc(0))) {
			k_printf("_creat: ialloc return NULL.");
			goto _creat_false_ret_1;
		}
		/* ����Ŀ¼���ͣ����������1,2��Ŀ¼��Ϊ '.', '..' */
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
		/* ������һ��Ŀ¼�� */
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
	/* �˴�������Ҫiput�ģ��������� */
_creat_false_ret_1:
	if(last_dir)
		iput(last_dir);
	if(dir)
		iput(dir);
	return NULL;
}

/*
 * Create file. �ú������ȷ����ļ���ȣ����ļ�������ˣ��Ͳ��ü�������
 * �����ˡ�ע������ļ����ʱ���������������ڵ�ָ�룬��Ϊ����֪������
 * ���٣����ļ�������֮����������������Ϊʲô���ȴ������ļ�֮���ٷ�
 * ���ļ�����أ�ԭ����������
 * 1. ���ļ���������ˣ���������������������Ч�ʣ�
 * 2. ���ȵ����Ǵ������ļ�֮�󣬲ŷ����ļ�������ˣ���ʱ����Ҫ���˵ģ�
 *	  Ҳ���轫�������ļ��ͷţ���ʱ������͸�����...
 *
 * I: pathname - �ļ���; mode - ������
 * O: �ļ����������������-1
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
	/* ����ļ��Ƿ���ڣ��ֱ�������ͬ���� */
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
	/* �����ļ���������ڵ�ָ�� */
	current->file[j]->inode = p;
	/* �����ڴ������ڵ����� */
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
 * Well, we begin to read file. ����֪�����ļ����пն���������������
 * ƫ���ļ�ָ�룬�Ա��д��ͬ�����ڶ�ȡ�ļ�ʱ���������ն��������û�
 * ���������0.
 * I: desc - �ļ�������; buff - �û������ַ; count - Ҫ�����ֽ���
 * O: �������û������ֽ���
 */
long sys_read(long desc, void * buff, unsigned long count)
{
	struct buffer_head * bh;
	unsigned long readcount = 0;	/* �Ѷ�ȡ���ֽ��� */
	unsigned long oneread = 0;		/* һ��Ҫ��ȡ���ֽ��� */
	struct bmap_struct s_bmap;
	unsigned char * rb = (unsigned char *)buff;
	struct file_table_struct * p_file = current->file[desc];
	
	if(!p_file) {
		k_printf("read: trying to read unknow-file.");
		return -1;
	}
	/* ������Ȩ�� */
	/* [??] nothing at present */
	struct m_inode * p = p_file->inode;
	if(!p)
		panic("read: BAD BAD, file-desc is OK, but m-inode-pointer is NULL!");
	LOCK_INODE(p);
	/* ��ʼ��ȡ */
	while(readcount<count && p_file->offset<p->size) {
		if(!bmap(p, p_file->offset, &s_bmap))
			panic("read: bmap return FALSE.");
		/* ����һ�ζ�ȡ���ֽ��� */
		oneread = 512;
		oneread = (oneread<(count-readcount)) ? oneread : (count-readcount);
		oneread = (oneread<(p->size-p_file->offset)) ? oneread : (p->size-p_file->offset);
		oneread = (oneread<(512-s_bmap.offset)) ? oneread : (512-s_bmap.offset);
		/* ��ȡһ�� */
		if(NULL == s_bmap.nrblock) {	/* �����ļ��ն� */
			for(int i=0; i<oneread; i++) {
				set_fs_byte(&rb[readcount++], 0);
			}
		} else {	/* �ǿն� */
			bh = bread(p->dev, s_bmap.nrblock);
			ds2fs_memcpy(&rb[readcount], &bh->p_data[s_bmap.offset], oneread);
			brelse(bh);
			readcount += oneread;
		}
		p_file->offset += oneread;
	}
	if(readcount) {
		/* �޸ķ���ʱ��� */
		/* [??] nothing at present */
	}
	UNLOCK_INODE(p);

	return readcount;
}

/*
 * Write file. �����Լ�����ֵͬread. Well, write file and read file is
 * simple. ���ǣ�������һЩ�Ż�������д�������ݣ��򲻱ض�ȡ�ÿ飬ֻ��
 * д�뼴�ɣ�����ֻд��ÿ��һ���֣��������ÿ顣
 * NOTE! Ŀ¼�ǲ���д��!
 */
long sys_write(long desc, void * buff, unsigned long count)
{
	struct buffer_head * bh;
	unsigned long writecount = 0;	/* ��д���ֽ��� */
	unsigned long onewrite = 0;		/* һ��Ҫд���ֽ��� */
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
	/* ������Ȩ�� */
	if(O_RONLY==p_file->mode || p->mode&FILE_DIR) {
		k_printf("write: have no permit to write the file.");
		return -1;
	}
	if(!p)
		panic("write: BAD BAD, file-desc is OK, but m-inode-pointer is NULL!");
	LOCK_INODE(p);
	/* ��ʼд */
	while(writecount < count) {
		/* ������ݿ���ӿ��Ƿ���ڣ�������������� */
		if(!check_alloc_data_block(p, p_file->offset)) {
			k_printf("write: check-alloc-disk-block ERROR!");
			return -1;
		}

		bmap(p, p_file->offset, &s_bmap);
		/* ����һ��д���ֽ��� */
		onewrite = 512;
		onewrite = (onewrite<(count-writecount)) ? onewrite : (count-writecount);
		onewrite = (onewrite<(512-s_bmap.offset)) ? onewrite : (512-s_bmap.offset);

		/* дһ�� */
		if(512 == onewrite) {
			bh = getblk(p->dev, s_bmap.nrblock);
			bh->state |= BUFF_VAL;	/* getblk���Ὣ��Чλ��λ */
		} else {
			bh = bread(p->dev, s_bmap.nrblock);
		}
		fs2ds_memcpy(&bh->p_data[s_bmap.offset], &wb[writecount], onewrite);
		bh->state |= BUFF_DELAY_W;	/* �ӳ�д */
		brelse(bh);

		writecount += onewrite;
		p_file->offset += onewrite;
	}
	/* �ļ�������޸�size�ֶ� */
	p->size = (p->size>p_file->offset) ? p->size : p_file->offset;
	if(writecount) {
		p->state |= NODE_I_ALT;
		p->state |= NODE_D_ALT;
		/* �޸��ļ�ʱ��� */
		/* [??] nothing at present */
	}
	UNLOCK_INODE(p);

	return writecount;
}

/*
 * �ú��������ļ�����/���λ�ã����ֽ�ƫ��ָ�롣����referenceΪ
 * �ֽ�ƫ��������λ�á�lseek�жϵ�ǰ�򿪵��ļ�״̬������ֻ����
 * ��ƫ���������Գ���size���������...
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
	 * ��ϵͳ�ļ���������ü���Ϊ0ʱ���ں˽����ͷţ����ں�
	 * ����տ��е�ϵͳ�ļ�����ں��ٴ�Ѱ�ҿ��б���ʱ��
	 * ���������ü���ֵ��ȷ���Ƿ���С�
	 */
	iput(p);

	return 1;
}

/*
 * dup�����ļ����������������κ��ļ����͡�
 * I:���ڸ��Ƶ�������, O:��������.
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
	/* nothing.��bss��,�ѱ����� */
}
