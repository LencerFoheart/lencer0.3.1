/*
 * Small/fs/exec.c
 * 
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */

/*
 * 此文件包含 ELF 可执行文件的加载执行。
 *
 * 目前只处理可静态加载的 ELF 可执行文件，动态加载我还没研究呢 :-)
 * 在gcc中编译生成ELF格式可执行文件时，加上 -static 选项，例如：
 * gcc -static -o test test.c。
 *
 * 令我不得不感慨的是 ELF 标准可谓是不一般的灵活、通用。那些 ELF 
 * 的设计者老前辈辛苦啦 (-:-)
 */

#ifndef __DEBUG__
#define __DEBUG__
#endif

#include "file_system.h"
#include "elf.h"
#include "exec.h"
#include "sched.h"
#include "types.h"
#include "kernel.h"
#include "memory.h"
#include "string.h"
#include "sys_set.h"

/*
 * 检查执行权限。内核检查这种非法情况，当文件为目录类型，是不允许
 * 被执行的。我们对可执行属性不做强制要求，因为用户不一定把可执行
 * 文件都标记上可执行属性，但用户绝不会把可执行文件标记为目录属性，
 * 或许有人会，但我不管。
 */
BOOL check_exec_file_permit(struct m_inode * p)
{
	return !(p->mode&FILE_DIR);
}

/*
 * 检查 ELF 头表的 e_ident 数组字段，以确认其合法性。
 */
BOOL check_elf_ident(Elf32_Ehdr * pehdr)
{
	unsigned char * id = pehdr->e_ident;

	if(id[EI_MAG0]!=ELFMAG0 || id[EI_MAG1]!=ELFMAG1
	|| id[EI_MAG2]!=ELFMAG2 || id[EI_MAG3]!=ELFMAG3)
		return FALSE;
	if(id[EI_CLASS]!=ELFCLASS32 && id[EI_CLASS]!=ELFCLASS64)
		return FALSE;
	if(id[EI_DATA]!=ELFDATA2LSB && id[EI_DATA]!=ELFDATA2MSB)
		return FALSE;
	if(id[EI_VERSION] != EV_CURRENT)
		return FALSE;
	if(id[EI_PAD] != ELFPAD)
		return FALSE;
	
	return TRUE;
}

/*
 * 我们打开可执行文件而不关闭，以便以后使用。但是当进程终止时，别忘了关掉它。
 *
 * [??] 若sys_execve()执行错误，应不应该返回FALSE，因为用户态空间可能已被释放，
 * 所以好好思考下，然后再注意使用execve()和sys_execve()函数的地方。OH,应该加一个
 * 不返回点! 点前边是返回FALSE，后边则是故障exit。
 *
 * [??] current->exec指向的页面与参数、环境变量共用内存页面，考虑是否需要
 * 映射到页表。
 * [??] return之前其他操作是否恢复，注：文件不能关闭。
 * [??] lseek与检查ELF非空，并且其长度足够ELF Header和Program Header分开，lseek出错与这不等同
 * [??] 检查是否超过最大允许segments
 */
BOOL sys_execve(char * file)
{
	FILEDESC desc;
	struct m_inode * pmi;
	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr;
	unsigned long addr,vaddr;
	int i,count;
	unsigned long pflag;
	unsigned short oldfs;
	unsigned long bottomstack;

	/* 打开可执行文件，检查权限 */
	if(-1 == (desc=sys_open(file, O_RONLY)))
		return FALSE;
	pmi = current->file[desc]->inode;
	if(!check_exec_file_permit(pmi)) {
		k_printf("do_execve: have no permit to execute specified file.");
		return FALSE;
	}

	/* 申请内存用来保存执行文件相关信息 */
	if(0 == (addr=get_free_page())) {
		k_printf("do_execve: have no free-page!");
		return FALSE;
	}

	current->exec = (struct exec_info *)addr;
	current->exec->filename = file;	/* 指向用户空间 */
	current->exec->desc = desc;
	/* 读取ELF头部，进一步检查 */
	oldfs = set_fs_kernel();
	if(sizeof(Elf32_Ehdr) != sys_read(desc, &ehdr, sizeof(Elf32_Ehdr))) {
		set_fs(oldfs);
		k_printf("do_execve: nonstandard ELF file!");
		return FALSE;
	}
	set_fs(oldfs);
	
	if(!check_elf_ident(&ehdr)) {
		k_printf("do_execve: nonstandard ELF file!");
		return FALSE;
	}
	if(ET_EXEC != ehdr.e_type) {
		k_printf("do_execve: not 'ET_EXEC' ELF file!");
		return FALSE;
	}
	if(EM_386 != ehdr.e_machine) {
		k_printf("do_execve: not 'EM_386' ELF file!");
		return FALSE;
	}
	if(EV_CURRENT != ehdr.e_version) {
		k_printf("do_execve: not 'EV_CURRENT' ELF file!");
		return FALSE;
	}
	current->exec->ventry = ehdr.e_entry;
	
	if(sizeof(Elf32_Ehdr) != ehdr.e_ehsize) {
		k_printf("do_execve: sizeof(Elf32_Ehdr) != e_ehsize.");
		return FALSE;
	}
	
	if(ehdr.e_phoff<ehdr.e_ehsize || -1==sys_lseek(desc, ehdr.e_phoff, SEEK_SET)) {
		k_printf("do_execve: nonstandard ELF file!");
		return FALSE;
	}
	
	if(sizeof(Elf32_Phdr) != ehdr.e_phentsize) {
		k_printf("do_execve: sizeof(Elf32_Phdr) != e_phentsize.");
		return FALSE;
	}

	/* 为替换用户态空间做准备 */
	if(!pre_replace_user_zone()) {
		k_printf("do_execve: prepare for replace user zone failed.");
		return FALSE;
	}
	
	/* 依次读取ELF程序头部并处理 */
	d_printf("ehdr.e_phnum = %d.\n", ehdr.e_phnum);
	current->code_start = current->data_start = V_USER_ZONE_END;
	current->code_end = current->data_end = V_USER_ZONE_START;	
	for(i=0; i<ehdr.e_phnum; i++) {
		/* 每次读取之前都lseek一下，因为我们是跳着读文件的。 */
		if(-1 == sys_lseek(desc, ehdr.e_phoff+i*ehdr.e_phentsize, SEEK_SET)) {
			return FALSE;
		}
		oldfs = set_fs_kernel();
		if(sizeof(Elf32_Phdr) != sys_read(desc, &phdr, sizeof(Elf32_Phdr))) {
			set_fs(oldfs);
			k_printf("do_execve: nonstandard ELF file!");
			return FALSE;
		}
		set_fs(oldfs);
		d_printf("read program-header %dst return.\n", i+1);
		
		/* 映射段数据内存空间并加载之 */
		if(PT_LOAD == phdr.p_type) {
			d_printf("====PT_LOAD====\n");
			if(phdr.p_filesz > phdr.p_memsz) {
				k_printf("do_execve: p_filesz > p_memsz.");
				return FALSE;
			}
			/* 调整代码段以及数据堆栈段起止地址 */
			if(phdr.p_flags & PF_X) {
				if(phdr.p_vaddr < current->code_start)
					current->code_start = phdr.p_vaddr;
				if(phdr.p_vaddr+phdr.p_memsz > current->code_end)
					current->code_end = phdr.p_vaddr+phdr.p_memsz;
			} else {
				if(phdr.p_vaddr < current->data_start)
					current->data_start = phdr.p_vaddr;
				if(phdr.p_vaddr+phdr.p_memsz > current->data_end)
					current->data_end = phdr.p_vaddr+phdr.p_memsz;	
			}
			/* 分配映射的内存空间 */
			pflag = (phdr.p_flags&PF_W) ? P_UWP : P_URP;
			vaddr = phdr.p_vaddr-(phdr.p_vaddr%PAGE_SIZE);
			count = 0;
			while(count < phdr.p_memsz) {
				if(0 == (addr=get_mapped_clean_page(vaddr, pflag))) {
					k_printf("do_execve: have no free-page!");
					return FALSE;
				}
				vaddr += PAGE_SIZE;
				count = vaddr-phdr.p_vaddr;
			}
			

			if(-1 == sys_lseek(desc, phdr.p_offset, SEEK_SET)) {
				k_printf("do_execve: nonstandard ELF file!");
				return FALSE;
			}
			/*
			 * 加载可执行数据到用户地址空间。NOTE! 此处fs不管怎样都应指向用户段，
			 * 尽管中断入口处使fs指向用户段，我们在此还是再重置一下，以便处于内
			 * 核态的调试程序在使fs指向内核段之后调用sys_exec()时，不会出现错误。
			 *
			 * 同时，写保护应该被暂时取消，以便将代码加载到用户空间时不会出错。
			 */
			oldfs = set_fs_user();
			unset_wp_bit();
			if(phdr.p_filesz != sys_read(desc, (void *)phdr.p_vaddr, phdr.p_filesz)) {
				set_wp_bit();
				set_fs(oldfs);
				k_printf("do_execve: nonstandard ELF file!");
				return FALSE;
			}
			set_wp_bit();
			set_fs(oldfs);
		}
	}
	
	/* 没有可执行代码 */
	if(V_USER_ZONE_START == current->code_end) {
		k_printf("do_execve: not executable ELF file!");
		return FALSE;
	}
	
	/* 继续调整数据和堆栈段起止地址 */
	if(V_USER_ZONE_START == current->data_end)
		current->data_start = current->data_end = current->code_end;
	bottomstack = ALIGN_PAGE(current->data_end+PAGE_SIZE-1)
		+ PAGE_SIZE*MAX_USER_STACK_PAGES;
	current->data_end = bottomstack;

	/*
	 * 分配用户栈。用户栈从数据结束的下一虚拟页面开始，我们之所以
	 * 将其跟在数据结束页后面，是因为这可能节省一页内存(二级页表)。
	 * 同时用户栈可以不止一个页面，但本版内核默认为1个页面。
	 */
	if(0 == (addr=get_mapped_free_page(bottomstack-PAGE_SIZE, P_UWP))) {
		k_printf("do_execve: have no free-page!");
		return FALSE;
	}
	/* 修改用户栈esp. NOTE: 栈底... - ss,esp,eflags,cs,eip... */
	set_ss_long(current->tss.esp0-8, bottomstack);
	d_printf("user STACK addr: %x.\n", addr);

	/* 将返回地址改为新程序入口. NOTE: 栈底... - ss,esp,eflags,cs,eip... */
	set_ss_long(current->tss.esp0-20, current->exec->ventry);
	d_printf("NEW ret-EIP is %x.\n", current->exec->ventry);
	
	d_printf("do_execve finished.\n");
	return TRUE;
}
