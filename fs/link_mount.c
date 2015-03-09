/*
 * Small/fs/link_mount.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * 该文件用于链接或取消链接索引节点以及安装或取消安装文件系统。链接
 * 索引节点一般用于共享，而取消链接索引节点一般用于删除文件或者取消
 * 共享。
 *
 * [??] 本版内核默认只有一个文件系统，所以暂且不实现mount & unmount.
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
 * 有些注释是英文的，原谅我一时的狂热吧，以后英文注释会少了 :-)
 *
 * [??]目前该函数还未完善，因为还没考虑周全。我们需要注意以下几个问题：
 *
 * 对于链接目录，我们不允许链接之后将文件系统的树形结构破坏，因此不
 * 允许将下层目录指向其上层或者更上层的目录。但由于目录路径可能依据
 * 于根目录，也可能依据于当前工作目录，即有绝对路径和相对路径之别，
 * 这就导致在检查目录链接是否会构成回路时比较复杂。目前想到的解决办
 * 法是：我们不仅保存根目录以及当前目录的索引节点指针，而且保存它们
 * 的路径，以便链接文件时，把链接的源路径和目的路径都转换成绝对路径，
 * 然后进行前缀子串匹配，若其中一个全部包含在另一个中，那么我们就检
 * 查到了这种违法的链接操作。
 *
 * 应该允许同一个目录中可以有多个不同名字的目录项指向同一个索引节点。
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
 * Check unlink-permit. 主要针对目录文件，本程序还未实现检查用户是否是
 * 超级用户，因为它们对操作目录的权限不同。
 */
BOOL check_unlink_permit(struct m_inode * p)
{
	/* [??] 超级用户未检查 */
	if(p->mode & FILE_DIR)
		if(!is_dir_empty(p))
			return FALSE;
	return TRUE;
}

/*
 * Unlink file.
 *
 * [??]目前该函数还未完善。对于unlink，需要考虑的情况很多，主要是针对
 * 普通文件和目录文件。本版内核在处理当前进程的根节点的父节点时，默认
 * 不向上处理，但这是不对的，只为了问题简单化 ⊙﹏⊙! 请注意以下问题：
 *
 * 首先，非空目录文件(目录项中除了".", ".."之外，还有其他目录项)的节点
 * 是不允许被删除的，以避免弄乱文件系统。
 *
 * 其次，考虑这样一种情况：一个空目录文件连接数超过1，在unlink时，我们
 * 将父节点与该空目录文件节点脱钩，而不是删除该空目录文件节点，但该空目
 * 录中至少有2种(而不是2个)目录项(".", "..")，其中".."目录项指向的父节
 * 点索引节点号已经不符合要求了，在此的做法是将其修改为其他连接该索引节
 * 点的父节点，但前提是我们在link时，已将每次连接的父节点写入该子目录文
 * 件中，但我们只取第一个".."作为默认的父节点。
 *
 * 再次，在释放目录表项时，我们清空文件表项的name字段(其为文件表项为空
 * 的标志)。同时，内核不检查该表项是否位于目录最后，当然也就不去修改目
 * 录文件size字段，此做法将导致目录文件size字段只会增大，只有当其只有2
 * 种目录项(".", "..")时，才可以将其删除。
 *
 * 再再次，进程在执行namei时，可能会睡眠，在此期间，unlink可能会拆除相
 * 应的节点，执行namei的进程醒来时，可能会存取已经不存在的索引节点，因
 * 此namei中需检查连接数是否为0，若为0，则报告错误。但在睡眠期间，其他
 * 进程在该文件系统的其他地方创建一个新目录，并使用相同的索引节点号，这
 * 时该进程醒来时就被欺骗了，进而会导致进程取错文件，破坏文件的保密性，
 * 但这种竞争条件在实际中很少发生。
 *
 * 再再再次，当一个文件处于打开状态时，unlink可以执行成功，但此时文件仍
 * 可以被已经打开该文件的进程使用，其他进程已经无法找到该文件了。当打开
 * 该文件的进程关闭该文件时，iput检测其引用数是否为0，若为0，则释放该文
 * 件的磁盘块和索引节点。对于unlink当前工作目录，情况也是一样的，unlink
 * 之后，我们并不修改当前工作目录指针，而是继续使用，当引用数减为0时，
 * 就会被清除。
 *
 * 再再再再次，unlink的输入路径名允许为空字符串(并不是NULL)，此时是指当
 * 前工作目录。
 *
 * 再再再再再次，系统根目录不能被unlink，否则结果未知。
 *
 * 再再再再再再次，fnamei调用之后，要检查fdirpath，lastpath是否为空，若
 * fdirpath为空，则是fnamei返回的是当前工作目录。
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
	/* 查找并获取子路径分量对应的索引节点号 */
	struct m_inode * pc = NULL;
	struct bmap_struct s_bmap = {0};
	struct buffer_head * bh = name_find_dir_elem(pf, lastpath, &s_bmap);
	if(!bh) {
		iput(pf);
		return FALSE;
	}
	pc = iget(bh->dev, ((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->nrinode);
	brelse(bh);		/* name_find_dir_elem() not release */

	/* 若最后一个分量为"."/"..", 需调整父目录和当前文件 */
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
		 * 由于查找目录项的顺序是从前向后的，因此我们此处找到的即是最前面的"..", 
		 * 它应位于第二个目录项位置。同时它也应该存在，否则就是出错了 ⊙﹏⊙!
		 */
		bh = name_find_dir_elem(pc, "..", &s_bmap);
		if(!bh)
			panic("unlink: can't find '..' from directory. [1]");
		pf = iget(bh->dev, ((struct file_dir_struct *)&bh->p_data[s_bmap.offset])->nrinode);
		brelse(bh);		/* name_find_dir_elem() not release */
	}

	/* 检查 unlink 权限 */
	if(!check_unlink_permit(pc)) {
		k_printf("unlink: have no permit.");
		iput(pf);
		iput(pc);
		return FALSE;
	}

	/* 对于连接数多于1的目录文件，我们修改其".."目录项指向的索引节点号 */
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

	/* 清除父目录中对应的子文件的目录项，并使子文件的连接数减1 */
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
