/*
 * Small/fs/inode.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ�Ϊ�ڴ�inode�����ļ����ڴ�inode�Ĵ���ʽ��buffer�����ƣ�
 * �������˿��б�͹�ϣ���ʿ������ƵĿ��б�͹�ϣ����������ļ�
 * ��buffer.c�еĶԿ��б�͹�ϣ��Ĵ��������ͬ!��ʵ���Ǵ�buffer.c
 * ֱ�Ӹ��ƹ����ģ��ⲿ�ֵ�ע�;Ͳμ�buffer�� :-)
 *
 * [??] д�����ڵ㣬write_inode���ӳ�д��״̬����ӳ�д(������
 * write_inode)����ʹ��ʱ�Ƿ���࣬���鿴!!!
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

/* inode���б� */
static struct m_inode inode_table[NR_INODE] = {{0,},};
/* inode���б�ͷ��ָ�� */
static struct m_inode * free_head = NULL;
/* inode��ϣ��ͷ��ָ������ */
static struct m_inode * hash_heads[NR_INODE_HASH] = {0};

/* ָ��ȴ�inode�Ľ��� */
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
	if(p) {					/* �ӱ���ժ��ָ���Ŀ���inode */
		if(p->p_free_next == p) {	/* ����ֻ��һ��inode */
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
		} else if(free_head->p_free_next == free_head) {/* ����ֻ��һ��inode */
			p = free_head;
			free_head = NULL;
		} else {					/* �����ж��inode */
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

	if(NULL == free_head) {	/* ���б�� */
		free_head = p;
		p->p_free_next = p;
		p->p_free_pre = p;
	} else {
		if(ishead) {		/* ����ͷ�� */
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

void i_remove_from_hash(struct m_inode *p)
{
	if(!p)
		panic("i_remove_from_hash: pointer is NULL.");
	if(!i_scan_hash(p->dev, p->nrinode))
		panic("i_remove_from_hash: not in hash-table.");

	int index = i_hash_fun(p->dev, p->nrinode);

	if(p->p_hash_next == p) {	/* ����ֻ��һ��inode */
		hash_heads[index] = NULL;
	} else {
		p->p_hash_pre->p_hash_next = p->p_hash_next;
		p->p_hash_next->p_hash_pre = p->p_hash_pre;
		if(p == hash_heads[index]) {	/* pָ���ͷ */
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

/* * �ú�����ָ��Ӳ�������ڵ�����ڴ������ڵ㡣 */
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
 * �ú�����ָ���ڴ������ڵ�д��Ӳ�������ڵ㡣isnowָʾ�Ƿ�����д��
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
		/* ȥ���ӳ�д��־����ʹû�д˱�־ */
		bh->state &= (~BUFF_DELAY_W) & 0xff;
		bwrite(bh);
	}
}

/* * �ú��������豸�ź������ڵ�Ż�ȡָ�������ڵ㡣�����غ���
 * ��Ч���ݵ��ڴ������ڵ�ָ�롣ע�⣺�ڴ������ڵ������ü����� */
struct m_inode * iget(unsigned char dev, unsigned int nrinode)
{
	struct m_inode * p = NULL;

	while(1) {
		if((p=i_scan_hash(dev, nrinode))) {/* ��ɢ�б� */
			/* ��ɢ�б������� */
			if(p->state & NODE_LOCK) {
				sleep_on(&inode_wait, INTERRUPTIBLE);
				continue;		/* �ص�whileѭ�� */
			}
			LOCK_INODE(p);
			/* ��ɢ�б����ڿ��б� */
			if(!(p->count++)) {
				i_get_from_free(p);
			}
			return p;
		} else {	/* ����ɢ�б� */
			/* ����ɢ�б��ҿ��б�� */
			if(NULL == free_head) {
				sleep_on(&inode_wait, INTERRUPTIBLE);
				continue;		/* �ص�whileѭ�� */
			}
			p = i_get_from_free(NULL);
			/* ����ɢ�б��ڿ��б��ҵ�һ��inode */
			LOCK_INODE(p);
			if(p->state & NODE_VAL) {
				i_remove_from_hash(p);
				p->state &= (~NODE_VAL) & 0xff;
			}
			i_put_to_hash(p, dev, nrinode);
			/* ��ȡ���������ڵ� */
			read_inode(p, dev, nrinode);
			/* �����ڴ������ڵ� */
			p->state |= NODE_VAL;
			p->count = 1;
			return p;
		}
	}
}

/* * �ú�����ָ���ڴ������ڵ��ͷţ�������������������Ӧ�Ĳ����� */
void iput(struct m_inode * p)
{
	if(!p)
		panic("iput: the m-inode-pointer can't be NULL.");

	if(!(p->state&NODE_LOCK))
		LOCK_INODE(p);
	if(0 == (--p->count)) {
		if(p->state & NODE_VAL) {
			/* ������Ϊ0���ͷ��ļ����̿�������ڵ� */
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
 * �ú������ļ��߼��ֽ�ƫ����ӳ�䵽�ļ����̿顣���߼��ֽ�
 * �����ļ���С������FALSE������ӳ������¼�ڽṹ���С�
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
	
	unsigned char step;		/* ��Ӽ� */
	unsigned long v_nrblock = v_offset / 512;
	if(v_nrblock > (9+512/4+512/4*512/4)) {
		step = 3;
		s_bmap->nrblock = p->zone[12];
		v_nrblock -= 10 + 512/4 + 512/4*512/4;
	} else if(v_nrblock > (9+512/4)) {
		step = 2;
		s_bmap->nrblock = p->zone[11];
		v_nrblock -= 10 + 512/4;
	} else if(v_nrblock > 9) {	/* 10��ֱ�ӿ� */
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
	s_bmap->bytes = 512-((512-(p->size%512))%512);	/* Ĭ��size>0 */
	s_bmap->mid_jmp_nrblock = 0;
	s_bmap->mid_b_index = 0;
	s_bmap->end_jmp_nrblock = 0;
	s_bmap->end_b_index = 0;
	
	unsigned long tmp, tmp_index;
	struct buffer_head * bh;
	while(step--) {
		/* �жϿ��ܵ��ļ��ն� */
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
 * name_find_dir_elem() ����·����������Ŀ¼��˺����������ֹ��ܣ�
 * �������·������Ϊ��(������NULL,���ǿ��ַ���)ʱ�����ҿ�Ŀ¼�
 * ��·��������Ϊ��ʱ�����Һ͸�·��������ȵ�Ŀ¼�ͬʱע�⣺
 * ���ҵ���Ӧ��ʱ�����ǲ��ͷŸû���飬�Ա�����߿���ʹ�øÿ顣
 *
 * NOTE! �ú���Ĭ�������·���������ں˵�ַ�ռ䣬���μ���һ�㡣
 *
 * I: dir - �ڴ������ڵ�ָ��; path_part - ·������; s_bmap - 
 * ���������Ϣ��bmap_structָ�롣
 * O: ������ָ�롣���ҵ���Ӧ��ʱ���û����������Ŷ�Ӧ���ݡ�
 */
struct buffer_head * name_find_dir_elem(struct m_inode * dir, 
				char * path_part, struct bmap_struct * s_bmap)
{
	unsigned long offset = 0;
	struct buffer_head * bh = NULL;

	/* ��ȡĿ¼�ļ������Ŀ¼��Ƚϣ����ҵ����򷵻ػ�����ָ�� */
	while(offset < dir->size) {
		if(bmap(dir, offset, s_bmap) && s_bmap->nrblock) {
			bh = bread(dir->dev, s_bmap->nrblock);
			/* �˴��ѿ��ǵ�һ�����̿�洢������Ŀ¼�� */
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
 * nrinode_find_dir_elem() ���������ڵ�Ų���Ŀ¼�
 * �����μ�name_find_dir_elem()
 */
struct buffer_head * nrinode_find_dir_elem(struct m_inode * dir, 
			long nrinode, struct bmap_struct * s_bmap)
{
	unsigned long offset = 0;
	struct buffer_head * bh = NULL;

	/* ��ȡĿ¼�ļ������Ŀ¼��Ƚϣ����ҵ����򷵻ػ�����ָ�� */
	while(offset < dir->size) {
		if(bmap(dir, offset, s_bmap) && s_bmap->nrblock) {
			bh = bread(dir->dev, s_bmap->nrblock);
			/* �˴��ѿ��ǵ�һ�����̿�洢������Ŀ¼�� */
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
 * is_dir_empty() �ж�Ŀ¼�ļ��Ƿ�Ϊ�ա���Ŀ¼�г��˿�Ŀ¼��֮�⣬ֻ��
 * 2��(������2��)Ŀ¼��(".","..")ʱ��Ŀ¼�ļ�Ϊ�ա�������ǣ�ϵͳ��Ŀ¼
 * û�и�Ŀ¼���ʵ�����ֻ��һ��Ŀ¼�.��ʱ����Ϊ�ա�
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
	
	/* ��ȡĿ¼�ļ������Ŀ¼��Ƚϣ����ܷ��ҵ����ǿա�Ŀ¼�� */
	while(offset < p->size) {
		if(bmap(p, offset, &s_bmap) && s_bmap.nrblock) {
			bh = bread(p->dev, s_bmap.nrblock);
			/* �˴��ѿ��ǵ�һ�����̿�洢������Ŀ¼�� */
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
 * �ú�����ָ��·����ת���������ڵ㡣�����Ǵ�fs�λ�ȡ·���ġ�
 * �ں�����������ַ���(������NULL)����ʱ���õ����̵�ǰ����Ŀ¼��
 * ������������ͬһ��Ŀ¼�У����ǲ�����������ͬ��Ŀ¼�����һ����
 * �ļ�������һ����Ŀ¼����Ҳ��������Ȼ��˵�������ǰ����ļ���׺�ġ�
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

	if('/' == get_fs_byte(pstr)) {	/* ��Ŀ¼ */
		dir = iget(current->root_dir->dev, current->root_dir->nrinode);
		++pstr;
	} else {	/* ��ǰĿ¼ */
		dir = iget(current->current_dir->dev, current->current_dir->nrinode);
	}

	/*
	 * ���Ǽ�������ڵ��������Ƿ�Ϊ0��Ϊ0���׳�������Ϣ��������һ�㣬
	 * �μ�unlink()���˴�Ӧ�ü���£���Ϊ���ܲ���ִ��while�ڲ��Ĵ��롣
	 */
	if(0 == dir->nlinks) {
		k_printf("namei: inode-link-count can't be 0. [1]");
		goto namei_false_ret;
	}
	
	while(get_fs_byte(pstr)) {
		/* ����Ŀ¼ */
		if(!(dir->mode&FILE_DIR)) {
			goto namei_false_ret;
		}
		/* ��ȡһ��·������ */
		i = 0;
		while('/'!=get_fs_byte(pstr) && get_fs_byte(pstr))
			path_part[i++] = get_fs_byte(pstr++);
		path_part[i] = 0;
		if(get_fs_byte(pstr))
			++pstr;
		/* ��Ŀ¼�޸�Ŀ¼ */
		if(dir==current->root_dir && 0==strcmp(path_part,"..")) {
			goto namei_false_ret;
		}
		bh = name_find_dir_elem(dir, path_part, &s_bmap);
		if(bh) {	/* ����ҵ� */
			iput(dir);
			dir = iget(bh->dev,
				((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->nrinode);
			brelse(bh);			/* name_find_dir_elem() û���ͷ� */
			if(0 == dir->nlinks) {
				k_printf("namei: inode-link-count can't be 0. [2]");
				goto namei_false_ret;
			}
		} else {	/* ����Ŀ¼���޴˷��� */
			goto namei_false_ret;
		}
	}

	return dir;

namei_false_ret:
	iput(dir);
	return NULL;
}

/*
 * fnamei() ��ȡ��·������Ӧ�������ڵ㣬ͬʱ��ȡ��·���������һ��
 * ·��������NOTE! �����ǻ�ȡ·������Ӧ�ڵ�ĸ��ڵ㣬��Ϊ���һ��
 * ·������������"."/".."��
 *
 * I: name - ������ļ���·����;
 *	  fdirpath - �������մ洢��·����;
 *    lastpath - �������մ洢���һ��·������.
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

	if(0 == strcmp("/", lastpath)) {/* ��Ŀ¼û�и��ڵ� */
		return NULL;
	} else {
		unsigned short oldfs = set_fs_kernel();/* fdirpath���ں˿ռ� */
		struct m_inode * p = namei(fdirpath);
		set_fs(oldfs);
		return p;
	}
}

void inode_init(void)
{
	int inodes = NR_INODE;					/* inode�� */
	struct m_inode * p = &inode_table[0];	/* ָ��ָ��ǰҪ����inode */
	struct m_inode * plast = NULL;			/* ǰһ��inodeͷ��ָ�� */

	free_head = p;
	inode_wait = NULL;

	/* ����˫��ѭ������inode���� */
	while(inodes--) {
		p->p_free_next = p + 1;
		p->p_free_pre = plast;
		/* ������Ա�����ʼ����bss������ */
		plast = p;
		p = p->p_free_next;
	}
	plast->p_free_next = free_head;
	free_head->p_free_pre = plast;

	/* ��ʼ����ϣ���� */
	/* nothing.��bss��,�ѱ����� */
}
