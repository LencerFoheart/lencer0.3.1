/*
 * Small/fs/link_mount.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * ���ļ��������ӻ�ȡ�����������ڵ��Լ���װ��ȡ����װ�ļ�ϵͳ������
 * �����ڵ�һ�����ڹ�����ȡ�����������ڵ�һ������ɾ���ļ�����ȡ��
 * ����
 *
 * [??] �����ں�Ĭ��ֻ��һ���ļ�ϵͳ���������Ҳ�ʵ��mount & unmount.
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "file_system.h"
#include "sched.h"
#include "kernel.h"
#include "types.h"
#include "string.h"
#include "sys_set.h"

/*
 * Check link-permit. At present, we have nothing but returning TRUE.
 * In the future, we will fill it.
 */
BOOL check_link_permit(struct m_inode * p)
{
	/* [??] nothing at present */
	return TRUE;
}

/*
 * Link file. We can link the normal-file and the directory-file.
 *
 * ��Щע����Ӣ�ĵģ�ԭ����һʱ�Ŀ��Ȱɣ��Ժ�Ӣ��ע�ͻ����� :-)
 *
 * [??]Ŀǰ�ú�����δ���ƣ���Ϊ��û������ȫ��������Ҫע�����¼������⣺
 *
 * ��������Ŀ¼�����ǲ���������֮���ļ�ϵͳ�����νṹ�ƻ�����˲�
 * �����²�Ŀ¼ָ�����ϲ���߸��ϲ��Ŀ¼��������Ŀ¼·����������
 * �ڸ�Ŀ¼��Ҳ���������ڵ�ǰ����Ŀ¼�����о���·�������·��֮��
 * ��͵����ڼ��Ŀ¼�����Ƿ�ṹ�ɻ�·ʱ�Ƚϸ��ӡ�Ŀǰ�뵽�Ľ����
 * ���ǣ����ǲ��������Ŀ¼�Լ���ǰĿ¼�������ڵ�ָ�룬���ұ�������
 * ��·�����Ա������ļ�ʱ�������ӵ�Դ·����Ŀ��·����ת���ɾ���·����
 * Ȼ�����ǰ׺�Ӵ�ƥ�䣬������һ��ȫ����������һ���У���ô���Ǿͼ�
 * �鵽������Υ�������Ӳ�����
 *
 * Ӧ������ͬһ��Ŀ¼�п����ж����ͬ���ֵ�Ŀ¼��ָ��ͬһ�������ڵ㡣
 */
BOOL sys_link(char * desname, char * srcname)
{
	unsigned short oldfs;
	struct m_inode * psrc = namei(srcname);
	if(!psrc)
		return FALSE;
	if(!check_link_permit(psrc)) {
		iput(psrc);
		return FALSE;
	}
	/*
	 * OK. First, we try to add the count of link. If
	 * we can't continue later on, we MUST recover it.
	 */
	psrc->nlinks++;
	write_inode(psrc, psrc->dev, psrc->nrinode, 0);
	/*
	 * unlock, in case the dead-lock.
	 */
	UNLOCK_INODE(psrc);
	
	/*
	 * We get the father-pathname-inode-nr & father-pathname
	 * & last-part-pathname of new link-pathname.
	 */
	char fdirpath[MAX_PATH];		/* father directory pathname */
	char lastpath[MAX_PATH_PART];	/* last part-pathname */
	struct m_inode * pfdes = fnamei(desname, fdirpath, lastpath);
	if(pfdes) {	/* father-pathname-inode have created */
		/*
		 * Check new file is created or not. Not only the part-pathname
		 * CAN'T be same, but also the nr-inode CAN'T be same.
		 */
		struct bmap_struct s_bmap = {0};
		struct buffer_head * bh = name_find_dir_elem(pfdes, lastpath, &s_bmap);
		if(bh) {	/* have the same part-pathname */
			brelse(bh);		/* name_find_dir_elem() not release */
			iput(pfdes);
			goto link_false_ret;
		}
		bh = nrinode_find_dir_elem(pfdes, psrc->nrinode, &s_bmap);
		if(bh) {	/* have the same nr-inode */
			brelse(bh);		/* name_find_dir_elem() not release */
			iput(pfdes);
			goto link_false_ret;
		}
	} else {	/* father-pathname-inode have NOT created */
		oldfs = set_fs_kernel();
		if(!(pfdes = _creat(fdirpath, FILE_DIR))) {
			set_fs(oldfs);
			goto link_false_ret;
		}
		set_fs(oldfs);
	}
	if(!put_to_dir(pfdes, psrc->nrinode, lastpath)) {
		iput(pfdes);
		goto link_false_ret;
	}
	/*
	 * If link file is directory-file, we need to add ".." to link file.
	 * So that we can find its father-directory.
	 */
	if(psrc->mode&FILE_DIR && !put_to_dir(psrc, pfdes->nrinode, "..")) {
		iput(pfdes);
		goto link_false_ret;
	}
	iput(pfdes);
	iput(psrc);
	return TRUE;

link_false_ret:
	psrc->nlinks--;
	write_inode(psrc, psrc->dev, psrc->nrinode, 0);
	iput(psrc);
	return FALSE;
}

/*
 * Check unlink-permit. ��Ҫ���Ŀ¼�ļ���������δʵ�ּ���û��Ƿ���
 * �����û�����Ϊ���ǶԲ���Ŀ¼��Ȩ�޲�ͬ��
 */
BOOL check_unlink_permit(struct m_inode * p)
{
	/* [??] �����û�δ��� */
	if(p->mode & FILE_DIR)
		if(!is_dir_empty(p))
			return FALSE;
	return TRUE;
}

/*
 * Unlink file.
 *
 * [??]Ŀǰ�ú�����δ���ơ�����unlink����Ҫ���ǵ�����ܶ࣬��Ҫ�����
 * ��ͨ�ļ���Ŀ¼�ļ��������ں��ڴ���ǰ���̵ĸ��ڵ�ĸ��ڵ�ʱ��Ĭ��
 * �����ϴ��������ǲ��Եģ�ֻΪ������򵥻� �ѩn��! ��ע���������⣺
 *
 * ���ȣ��ǿ�Ŀ¼�ļ�(Ŀ¼���г���".", ".."֮�⣬��������Ŀ¼��)�Ľڵ�
 * �ǲ�����ɾ���ģ��Ա���Ū���ļ�ϵͳ��
 *
 * ��Σ���������һ�������һ����Ŀ¼�ļ�����������1����unlinkʱ������
 * �����ڵ���ÿ�Ŀ¼�ļ��ڵ��ѹ���������ɾ���ÿ�Ŀ¼�ļ��ڵ㣬���ÿ�Ŀ
 * ¼��������2��(������2��)Ŀ¼��(".", "..")������".."Ŀ¼��ָ��ĸ���
 * �������ڵ���Ѿ�������Ҫ���ˣ��ڴ˵������ǽ����޸�Ϊ�������Ӹ�������
 * ��ĸ��ڵ㣬��ǰ����������linkʱ���ѽ�ÿ�����ӵĸ��ڵ�д�����Ŀ¼��
 * ���У�������ֻȡ��һ��".."��ΪĬ�ϵĸ��ڵ㡣
 *
 * �ٴΣ����ͷ�Ŀ¼����ʱ����������ļ������name�ֶ�(��Ϊ�ļ�����Ϊ��
 * �ı�־)��ͬʱ���ں˲����ñ����Ƿ�λ��Ŀ¼��󣬵�ȻҲ�Ͳ�ȥ�޸�Ŀ
 * ¼�ļ�size�ֶΣ�������������Ŀ¼�ļ�size�ֶ�ֻ������ֻ�е���ֻ��2
 * ��Ŀ¼��(".", "..")ʱ���ſ��Խ���ɾ����
 *
 * ���ٴΣ�������ִ��nameiʱ�����ܻ�˯�ߣ��ڴ��ڼ䣬unlink���ܻ�����
 * Ӧ�Ľڵ㣬ִ��namei�Ľ�������ʱ�����ܻ��ȡ�Ѿ������ڵ������ڵ㣬��
 * ��namei�������������Ƿ�Ϊ0����Ϊ0���򱨸���󡣵���˯���ڼ䣬����
 * �����ڸ��ļ�ϵͳ�������ط�����һ����Ŀ¼����ʹ����ͬ�������ڵ�ţ���
 * ʱ�ý�������ʱ�ͱ���ƭ�ˣ������ᵼ�½���ȡ���ļ����ƻ��ļ��ı����ԣ�
 * �����־���������ʵ���к��ٷ�����
 *
 * �����ٴΣ���һ���ļ����ڴ�״̬ʱ��unlink����ִ�гɹ�������ʱ�ļ���
 * ���Ա��Ѿ��򿪸��ļ��Ľ���ʹ�ã����������Ѿ��޷��ҵ����ļ��ˡ�����
 * ���ļ��Ľ��̹رո��ļ�ʱ��iput������������Ƿ�Ϊ0����Ϊ0�����ͷŸ���
 * ���Ĵ��̿�������ڵ㡣����unlink��ǰ����Ŀ¼�����Ҳ��һ���ģ�unlink
 * ֮�����ǲ����޸ĵ�ǰ����Ŀ¼ָ�룬���Ǽ���ʹ�ã�����������Ϊ0ʱ��
 * �ͻᱻ�����
 *
 * �������ٴΣ�unlink������·��������Ϊ���ַ���(������NULL)����ʱ��ָ��
 * ǰ����Ŀ¼��
 *
 * ���������ٴΣ�ϵͳ��Ŀ¼���ܱ�unlink��������δ֪��
 *
 * �����������ٴΣ�fnamei����֮��Ҫ���fdirpath��lastpath�Ƿ�Ϊ�գ���
 * fdirpathΪ�գ�����fnamei���ص��ǵ�ǰ����Ŀ¼��
 */
BOOL sys_unlink(char * name)
{
	if(!name) {
		k_printf("unlink: name can't be NULL.");
		return FALSE;
	}

	/*
	 * We get the father-pathname-inode-nr & father-pathname
	 * & last-part-pathname of new link-pathname.
	 */
	char fdirpath[MAX_PATH];		/* father directory pathname */
	char lastpath[MAX_PATH_PART];	/* last part-pathname */
	struct m_inode * pf = fnamei(name, fdirpath, lastpath);
	if(!pf)
		return FALSE;
	/* ���Ҳ���ȡ��·��������Ӧ�������ڵ�� */
	struct m_inode * pc = NULL;
	struct bmap_struct s_bmap = {0};
	struct buffer_head * bh = name_find_dir_elem(pf, lastpath, &s_bmap);
	if(!bh) {
		iput(pf);
		return FALSE;
	}
	pc = iget(bh->dev, ((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->nrinode);
	brelse(bh);		/* name_find_dir_elem() not release */

	/* �����һ������Ϊ"."/"..", �������Ŀ¼�͵�ǰ�ļ� */
	int repcount;						/* repeat count */
	repcount = (0==strcmp(".", lastpath) ? 1 : (0==strcmp("..", lastpath) ? 2 : 0));
	while(repcount--) {
		if(pf == current->root_dir) {
			k_printf("unlink: root_dir haven't father-dir.");
			iput(pf);
			iput(pc);
			return FALSE;
		}
		iput(pc);
		pc = pf;
		/*
		 * ���ڲ���Ŀ¼���˳���Ǵ�ǰ���ģ�������Ǵ˴��ҵ��ļ�����ǰ���"..", 
		 * ��Ӧλ�ڵڶ���Ŀ¼��λ�á�ͬʱ��ҲӦ�ô��ڣ�������ǳ����� �ѩn��!
		 */
		bh = name_find_dir_elem(pc, "..", &s_bmap);
		if(!bh)
			panic("unlink: can't find '..' from directory. [1]");
		pf = iget(bh->dev, ((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->nrinode);
		brelse(bh);		/* name_find_dir_elem() not release */
	}

	/* ��� unlink Ȩ�� */
	if(!check_unlink_permit(pc)) {
		k_printf("unlink: have no permit.");
		iput(pf);
		iput(pc);
		return FALSE;
	}

	/* ��������������1��Ŀ¼�ļ��������޸���".."Ŀ¼��ָ��������ڵ�� */
	if((pc->mode & FILE_DIR) && (pc->nlinks > 1)) {
		bh = name_find_dir_elem(pc, "..", &s_bmap);
		if(!bh)
			panic("unlink: can't find '..' from directory. [2]");
		strcpy(((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->name, "");
		long lastoffset = s_bmap.offset;
		struct buffer_head * tmpbh = name_find_dir_elem(pc, "..", &s_bmap);
		if(!tmpbh)
			panic("unlink: can't find '..' from directory. [3]");
		strcpy(((struct file_dir_struct *)&bh->p_data[lastoffset])->name, "..");
		((struct file_dir_struct *)&bh->p_data[lastoffset])->nrinode = 
			((struct file_dir_struct *)&tmpbh->p_data[s_bmap.offset])->nrinode;
		strcpy(((struct file_dir_struct *)&tmpbh->p_data[s_bmap.offset])->name, "");
		/* OK. We need to write them */
		bh->state |= BUFF_DELAY_W;
		tmpbh->state |= BUFF_DELAY_W;
		brelse(bh);
		brelse(tmpbh);
	}

	/* �����Ŀ¼�ж�Ӧ�����ļ���Ŀ¼���ʹ���ļ�����������1 */
	bh = nrinode_find_dir_elem(pf, pc->nrinode, &s_bmap);
	if(!bh)
		panic("unlink: can't find child-file-nrinode in father-directory.");
	strcpy(((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->name, "");
	bh->state |= BUFF_DELAY_W;
	brelse(bh);
	pf->state |= NODE_I_ALT;
	iput(pf);
	pc->nlinks--;
	pc->state |= NODE_I_ALT;
	iput(pc);

	return TRUE;
}
