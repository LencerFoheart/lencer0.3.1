/*
 * Small/fs/exec.c
 * 
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ����� ELF ��ִ���ļ��ļ���ִ�С�
 *
 * Ŀǰֻ����ɾ�̬���ص� ELF ��ִ���ļ�����̬�����һ�û�о��� :-)
 * ��gcc�б�������ELF��ʽ��ִ���ļ�ʱ������ -static ѡ����磺
 * gcc -static -o test test.c��
 *
 * ���Ҳ��ò��п����� ELF ��׼��ν�ǲ�һ�����ͨ�á���Щ ELF 
 * ���������ǰ�������� (-:-)
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
 * ���ִ��Ȩ�ޡ��ں˼�����ַǷ���������ļ�ΪĿ¼���ͣ��ǲ�����
 * ��ִ�еġ����ǶԿ�ִ�����Բ���ǿ��Ҫ����Ϊ�û���һ���ѿ�ִ��
 * �ļ�������Ͽ�ִ�����ԣ����û�������ѿ�ִ���ļ����ΪĿ¼���ԣ�
 * �������˻ᣬ���Ҳ��ܡ�
 */
BOOL check_exec_file_permit(struct m_inode * p)
{
	return !(p->mode&FILE_DIR);
}

/*
 * ��� ELF ͷ��� e_ident �����ֶΣ���ȷ����Ϸ��ԡ�
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
 * ���Ǵ򿪿�ִ���ļ������رգ��Ա��Ժ�ʹ�á����ǵ�������ֹʱ�������˹ص�����
 *
 * [??] ��sys_execve()ִ�д���Ӧ��Ӧ�÷���FALSE����Ϊ�û�̬�ռ�����ѱ��ͷţ�
 * ���Ժú�˼���£�Ȼ����ע��ʹ��execve()��sys_execve()�����ĵط���OH,Ӧ�ü�һ��
 * �����ص�! ��ǰ���Ƿ���FALSE��������ǹ���exit��
 *
 * [??] current->execָ���ҳ����������������������ڴ�ҳ�棬�����Ƿ���Ҫ
 * ӳ�䵽ҳ��
 * [??] return֮ǰ���������Ƿ�ָ���ע���ļ����ܹرա�
 * [??] lseek����ELF�ǿգ������䳤���㹻ELF Header��Program Header�ֿ���lseek�������ⲻ��ͬ
 * [??] ����Ƿ񳬹��������segments
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

	/* �򿪿�ִ���ļ������Ȩ�� */
	if(-1 == (desc=sys_open(file, O_RONLY)))
		return FALSE;
	pmi = current->file[desc]->inode;
	if(!check_exec_file_permit(pmi)) {
		k_printf("do_execve: have no permit to execute specified file.");
		return FALSE;
	}

	/* �����ڴ���������ִ���ļ������Ϣ */
	if(0 == (addr=get_free_page())) {
		k_printf("do_execve: have no free-page!");
		return FALSE;
	}

	current->exec = (struct exec_info *)addr;
	current->exec->filename = file;	/* ָ���û��ռ� */
	current->exec->desc = desc;
	/* ��ȡELFͷ������һ����� */
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

	/* Ϊ�滻�û�̬�ռ���׼�� */
	if(!pre_replace_user_zone()) {
		k_printf("do_execve: prepare for replace user zone failed.");
		return FALSE;
	}
	
	/* ���ζ�ȡELF����ͷ�������� */
	d_printf("ehdr.e_phnum = %d.\n", ehdr.e_phnum);
	current->code_start = current->data_start = V_USER_ZONE_END;
	current->code_end = current->data_end = V_USER_ZONE_START;	
	for(i=0; i<ehdr.e_phnum; i++) {
		/* ÿ�ζ�ȡ֮ǰ��lseekһ�£���Ϊ���������Ŷ��ļ��ġ� */
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
		
		/* ӳ��������ڴ�ռ䲢����֮ */
		if(PT_LOAD == phdr.p_type) {
			d_printf("====PT_LOAD====\n");
			if(phdr.p_filesz > phdr.p_memsz) {
				k_printf("do_execve: p_filesz > p_memsz.");
				return FALSE;
			}
			/* ����������Լ����ݶ�ջ����ֹ��ַ */
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
			/* ����ӳ����ڴ�ռ� */
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
			 * ���ؿ�ִ�����ݵ��û���ַ�ռ䡣NOTE! �˴�fs����������Ӧָ���û��Σ�
			 * �����ж���ڴ�ʹfsָ���û��Σ������ڴ˻���������һ�£��Ա㴦����
			 * ��̬�ĵ��Գ�����ʹfsָ���ں˶�֮�����sys_exec()ʱ��������ִ���
			 *
			 * ͬʱ��д����Ӧ�ñ���ʱȡ�����Ա㽫������ص��û��ռ�ʱ�������
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
	
	/* û�п�ִ�д��� */
	if(V_USER_ZONE_START == current->code_end) {
		k_printf("do_execve: not executable ELF file!");
		return FALSE;
	}
	
	/* �����������ݺͶ�ջ����ֹ��ַ */
	if(V_USER_ZONE_START == current->data_end)
		current->data_start = current->data_end = current->code_end;
	bottomstack = ALIGN_PAGE(current->data_end+PAGE_SIZE-1)
		+ PAGE_SIZE*MAX_USER_STACK_PAGES;
	current->data_end = bottomstack;

	/*
	 * �����û�ջ���û�ջ�����ݽ�������һ����ҳ�濪ʼ������֮����
	 * ����������ݽ���ҳ���棬����Ϊ����ܽ�ʡһҳ�ڴ�(����ҳ��)��
	 * ͬʱ�û�ջ���Բ�ֹһ��ҳ�棬�������ں�Ĭ��Ϊ1��ҳ�档
	 */
	if(0 == (addr=get_mapped_free_page(bottomstack-PAGE_SIZE, P_UWP))) {
		k_printf("do_execve: have no free-page!");
		return FALSE;
	}
	/* �޸��û�ջesp. NOTE: ջ��... - ss,esp,eflags,cs,eip... */
	set_ss_long(current->tss.esp0-8, bottomstack);
	d_printf("user STACK addr: %x.\n", addr);

	/* �����ص�ַ��Ϊ�³������. NOTE: ջ��... - ss,esp,eflags,cs,eip... */
	set_ss_long(current->tss.esp0-20, current->exec->ventry);
	d_printf("NEW ret-EIP is %x.\n", current->exec->ventry);
	
	d_printf("do_execve finished.\n");
	return TRUE;
}
