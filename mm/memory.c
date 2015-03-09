/*
 * Small/mm/memory.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ����ڶ��ڴ���й���֧�ֵ������ڴ��С�μ�ͷ�ļ���
 * ���ļ�Ҳ�Ը��ٻ������Ŀ�ʼ�ͽ�����ַ�����˳�ʼ����
 *
 * �ڴ����������̹���ҳĿ¼������ҳ����ͨҳ�棬������Ƕ�������
 * ҳ�����ü�����
 *
 * ����0�Ĵ�������ں˴�����(main()��)����͵��������û�̬ҳ���ӳ��
 * �����ں˴��룬����ʱ�������̣������¶��ں˴���Ҳ����д������ͬʱҲ
 * ����Ϊ�û�̬ҳ��ӳ����ҳ�����Ӷ����¹����ڴ�ʱҳ��������������
 * ������ȷ�������Ǻֲܿ��ġ����ǵ������Ǹտ�ʼִ�н���0������execve()��
 * �滻���û��ռ䣬�Ӵ˽���0�Ĵ�������ں˴��뻮���˽��ޣ��û�̬ҳ��Ҳ
 * ����ӳ�䱾���������Ϳ��Է��Ĵ��������ˡ����н���Ҳ��������дʱ����
 * ���ơ�
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "memory.h"
#include "string.h"
#include "kernel.h"
#include "sched.h"
#include "sys_nr.h"
#include "types.h"

/* ��������������Ԥ������� */
extern unsigned long _etext;
extern unsigned long _end;

/* ��������ʼ��ַ */
extern unsigned long buffer_mem_start;
/* ������ĩ�˵�ַ */
extern unsigned long buffer_mem_end;

/* UNION(��ʼ�����̽ṹ, �����ں�̬��ջ) */
extern union proc_union init_proc;

/* �ڴ�߶˵�ַ */
unsigned long high_mem = 0;
/*
 * �ڴ�ҳ��ʹ�������ӳ���ڴ淶Χ��0MB - 16MB��
 * ��ֵ��ʾ��ҳ���ô�����0��ʾ��ҳ���С�
 */
unsigned char mem_map[MEM_MAP_SIZE] = {0};

/*
 * �ú�����������ڴ�ʹ�ñ�ȡ��һ���ڴ����ҳ�棬�����ڴ�ʹ�ñ�
 * ��Ӧ���1������ҳ����ʼ�����ַ����ΪNULL�����ʾ�޿���ҳ�档
 */
unsigned long get_free_page(void)
{
	unsigned long i;
	for(i=M2MAP(high_mem)-1; i>=M2MAP(buffer_mem_end); i--) {	
		if(0 == mem_map[i]) {
			++mem_map[i];
			break;
		}
	}
	return i>=M2MAP(buffer_mem_end) ? MAP2M(i) : NULL;
}

/*
 * �ú�����ȡָ��ҳ�沢����0�����ػ�ȡ��ҳ��������ַ��
 */
unsigned long get_clean_page(void)
{
	unsigned long addr = get_free_page();
	if(addr)
		zeromem((char *)addr, PAGE_SIZE);
	return addr;
}

/*
 * �ú�����ȡһ������ҳ�沢�ҷ���ҳ�����ػ�ȡ��ҳ��������ַ��
 */
unsigned long get_mapped_free_page(unsigned long vaddr, unsigned long flag)
{
	unsigned long addr = get_free_page();
	if(addr && !put_page(addr, vaddr, flag))
		return NULL;
	else
		return addr;
}

/*
 * �ú�����ȡһ����0�Ŀ���ҳ�沢�ҷ���ҳ�����ػ�ȡ��ҳ��������ַ��
 */
unsigned long get_mapped_clean_page(unsigned long vaddr, unsigned long flag)
{
	unsigned long addr = get_clean_page();
	if(addr && !put_page(addr, vaddr, flag))
		return NULL;
	else
		return addr;
}

/*
 * �ú�������һ��ҳ����ΪҳĿ¼������������ҳĿ¼��
 */
BOOL copy_page_dir(long * cr3)
{
	long tmp;
	if(!cr3)
		panic("copy_page_dir: cr3-pointer is NULL!");
	if(NULL == (tmp=get_clean_page())) {
		k_printf("copy_page_dir: have no free-page!");
		return FALSE;
	}
	memcpy((char *)tmp, (char *)current->tss.cr3, PAGE_SIZE);
	*cr3 = tmp;
	return TRUE;
}

/*
 * �ú�������һ��ҳ����ΪҳĿ¼���������ں�̬ҳĿ¼���
 */
BOOL copy_kernel_page_dir(long * cr3)
{
	long tmp;
	if(!cr3)
		panic("copy_kernel_page_dir: cr3-pointer is NULL!");
	if(NULL == (tmp=get_clean_page())) {
		k_printf("copy_kernel_page_dir: have no free-page!");
		return FALSE;
	}
	memcpy((char *)(tmp+PG_DIR_KERNEL_START),
		(char *)(current->tss.cr3+PG_DIR_KERNEL_START),
		PAGE_SIZE-PG_DIR_KERNEL_START);
	*cr3 = tmp;
	return TRUE;
}

/*
 * �ú�����ҳ�����ڴ�ҳ��ҽӡ�
 * I: page - ҳ����ʼ�����ַ;
 *	  vaddr - ҳ��ӳ����������Ե�ַ;
 * 	  flag - ҳ�����Ա�־.
 * O: �����󣬷���FALSE.
 */
BOOL put_page(unsigned long page, unsigned long vaddr, unsigned long flag)
{
	if(page<buffer_mem_end || page>(high_mem-PAGE_SIZE))
		panic("put_page: page address out of the limit.");
	if(vaddr > (V_USER_ZONE_END-PAGE_SIZE))
		panic("put_page: vaddr out of the limit.");
	if(page & (PAGE_SIZE-1))	/* �ڴ�ҳ����ʼ��ַ����PAGE_SIZE���� */
		panic("put_page: page address with wrong alignment.");
	if(vaddr & (PAGE_SIZE-1))	/* �������Ե�ַ����PAGE_SIZE���� */
		panic("put_page: vaddr with wrong alignment.");
	if(flag!=P_URP && flag!=P_UWP && flag!=P_SRP && flag!=P_SWP)
		panic("put_page: ERROR page-flag.");

	/* ��Ҫ���õ�ҳĿ¼����ĵ�ַ */
	unsigned long put_addr = ADDR_IN_1PT(current->tss.cr3, vaddr);
	/* ҳĿ¼������Pλ�Ƿ���λ */
	if(!(*(unsigned long *)put_addr & 1)) {
		unsigned long tmp;
		if(NULL == (tmp = get_free_page()))
			return FALSE;
		*(unsigned long *)put_addr = tmp + P_UWP; /* �û�̬����д */
	}
	/* �õ�����ҳ������ҳ����׵�ַ */
	put_addr = ALIGN_PAGE(*(unsigned long *)put_addr);
	/* �õ�����ҳ����ĵ�ַ */
	put_addr = ADDR_IN_2PT(put_addr, vaddr);
	*(unsigned long *)put_addr = page + flag;

	return TRUE;
}

/*
 * �ú�����ָ���û�̬���Ե�ַ�ռ��Ӧ��ҳ���Լ�
 * ����ҳ��ʹ�ô�����1��������ҳ������Ϊֻ����
 * I: from - ��ʼ���Ե�ַ; size - �ڴ�ռ��С.
 */
void set_shared_pages(unsigned long from, unsigned long size)
{
	if(from & (PAGE_SIZE-1))
		panic("set_shared_pages: start address with wrong alignment.");
	size = ALIGN_PAGE(size+PAGE_SIZE-1);
	if(from + size > V_USER_ZONE_END)
		panic("set_shared_pages: trying to set kernel address space.");

	/* �������ҳ����ָ���ҳ�� */
	unsigned long to, addr, tmp;
	for(to=from+size; from<to; from+=PAGE_SIZE) {
		addr = ADDR_IN_1PT(current->tss.cr3, from);
		if(*(unsigned long *)addr & 1) {
			addr = ALIGN_PAGE(*(unsigned long *)addr);
			addr = ADDR_IN_2PT(addr, from);
			if(*(unsigned long *)addr & 1) {
				tmp = ALIGN_PAGE(*(unsigned long *)addr);
				++mem_map[M2MAP(tmp)];
				*(unsigned long *)addr = tmp + P_URP;
			}
		}
	}
	
	/* �������ҳ��ҳ��, from 4MB����ȡ��, to 4MB����ȡ�� */
	from = (to-size) & 0xffc00000;
	to = (to+PAGE_SIZE/4*PAGE_SIZE-1) & 0xffc00000;
	for(; from<to; from+=PAGE_SIZE/4*PAGE_SIZE) {
		addr = ADDR_IN_1PT(current->tss.cr3, from);
		if(*(unsigned long *)addr & 1) {
			++mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
		}
	}
}

/*
 * �ú�������ҳĿ¼��Ҳ���ǹ����˽���ȫ���ڴ档��Ȼ�ٴ����ô�
 * ���ֻ����û��Ҫ�ģ������Ӵ�����ڴ�ҳ������ô���ȴ�Ǳ�Ҫ�ġ�
 */
void share_page_dir(long * cr3)
{
	if(!cr3)
		panic("share_page_dir: cr3-pointer is NULL!");
	*cr3 = current->tss.cr3;
	++mem_map[M2MAP(*cr3)];
	set_shared_pages(current->code_start,
		current->data_end-current->code_start);
	invalidate_tlb();
}

/*
 * �ú�����ָ���û�̬���Ե�ַ�ռ��Ӧ��ҳ���Լ�����ҳ��ʹ�ô�����1��
 * ������һ��ҳ���������Ϊ1����������һ��ҳ���е�����λ����ΪNULL.
 * I: from - ��ʼ���Ե�ַ; size - �ڴ�ռ��С.
 */
void free_shared_pages(unsigned long from, unsigned long size)
{
	if(from & (PAGE_SIZE-1))
		panic("free_shared_pages: start address with wrong alignment.");
	size = ALIGN_PAGE(size+PAGE_SIZE-1);
	if(from+size > V_USER_ZONE_END)
		panic("free_shared_pages: trying to free kernel address space.");

	/* �������ҳ����ָ���ҳ�� */
	unsigned long to, addr, tmp;
	for(to=from+size; from<to; from+=PAGE_SIZE) {
		addr = ADDR_IN_1PT(current->tss.cr3, from);
		if(*(unsigned long *)addr & 1) {
			tmp = ALIGN_PAGE(*(unsigned long *)addr);
			addr = ADDR_IN_2PT(tmp, from);
			if(*(unsigned long *)addr & 1) {
				--mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
				if(1 == mem_map[M2MAP(tmp)])
					*(unsigned long *)addr = NULL;
			}
		}
	}

	/* �������ҳ��ҳ�� */
	unsigned long begin, end;
	from = to - size;
	for(; from<to; from+=PAGE_SIZE/4*PAGE_SIZE, from&=0xffc00000) {
		if(!(from&(PAGE_SIZE/4*PAGE_SIZE-1))
		&& from+PAGE_SIZE/4*PAGE_SIZE<=to) { /* 4MB����,�Ҳ�������Χ */
			begin = from;
		} else {
			if(!(from & (PAGE_SIZE/4*PAGE_SIZE-1))) {
				begin = from & 0xffc00000;
				end = from;
			} else {	/* from+PAGE_SIZE/4*PAGE_SIZE > to */
				begin = to;
				end = (to+PAGE_SIZE/4*PAGE_SIZE) & 0xffc00000;
			}
			/* �����������ҳ�����ȫ��Ϊ�գ����轫��ҳ����������1 */
			for(; begin<end; begin+=PAGE_SIZE) {
				addr = ADDR_IN_1PT(current->tss.cr3, begin);
				if(*(unsigned long *)addr & 1) {
					addr = ALIGN_PAGE(*(unsigned long *)addr);
					addr = ADDR_IN_2PT(addr, begin);
					if(*(unsigned long *)addr & 1)
						break;
				}
			}
			if(begin < end)
				continue;
			begin = from & 0xffc00000;
		}
		/* ����ҳ��ҳ����������1 */
		addr = ADDR_IN_1PT(current->tss.cr3, begin);
		if(*(unsigned long *)addr & 1) {
			--mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
			if(1 == mem_map[M2MAP(current->tss.cr3)])
				*(unsigned long *)addr = NULL;
		}
	}
}

/*
 * �ú����ͷŽ����û�̬�ڴ�ռ䡣�����ͷŽ����ڴ�ռ䣬����ȷ���¼��㣺
 *
 * ���ڽ���0���ͷ��û�̬�ڴ�����ζ����տ�ʼִ��execve()����ʱֻ�轫��
 * �û�̬ҳĿ¼�����㼴�ɡ������ͷţ���Ϊ0�������ں˹���ҳ��!
 *
 * ���ڸ������̹���һ���ں�̬�Ķ���ҳ��ҳ�棬��˲����ͷ��ں˿ռ䡣
 *
 * ͬʱ�������ͷ�ҳĿ¼������ҳ�棬��Ϊ���̵�ǰ�������ں�̬����Ȼ����
 * ���ں˵�ַ�ռ䡣���ڸý��̹ҵ�֮�����丸�����ͷ�ҳĿ¼����ô0��
 * ���̵ĸ�������˭�أ��������ˣ�����ϵͳ�رգ�����0�Ž����ǲ��������� :-)
 *
 * ���ڿ��ܴ��ڶ��������ʹ�õ�ǰҳĿ¼��(ʵ�����ڴ����½���ʱ���ӽ���
 * Ĭ���븸���̾�����ͬ��CR3)������ͷŽ���ҳĿ¼��ʱ��ֻ�轫��������1��
 *
 * ���⣬������ҪʹTLB��Ч���Ա���TLB��һ����(�Һ�Intel CPU�Ḻ��ά��
 * Cache���ڴ��һ����)������ʱ(���統ǰ�����˳�ʱ)����������ʹTLB��Ч��
 * ��Ϊ��û�л�����ȥ���У��ܿ��丸���̾ͻὫ��ҳĿ¼���յ���
 *
 * I��from - ��ʼ���Ե�ַ, size - �ڴ�ռ��С, invtlb - �Ƿ�ʹTLB��Ч.
 */
void free_user_pages(unsigned long from, unsigned long size, BOOL invtlb)
{
	if(0 == get_cur_proc_nr())
		zeromem((char *)current->tss.cr3, 4*INIT_UPDE_COUNT);
	else
		free_shared_pages(from, size);
	if(invtlb)
		invalidate_tlb();
}

/* * �ú���Ϊ�滻�û�̬�ռ���׼���������ͷ��û�̬�ڴ棬Ȼ����ҳĿ¼
 * ʹ�ô��������ǹ���ģ������һ���µ�ҳĿ¼������ԭҳĿ¼��������1�� */
BOOL pre_replace_user_zone(void)
{
	free_user_pages(current->code_start,
		current->data_end-current->code_start, 1);
	if(1 < mem_map[M2MAP(current->tss.cr3)]) {
		unsigned long addr = current->tss.cr3;
		if(!copy_kernel_page_dir(&(current->tss.cr3))) {
			k_printf("free_user_zone: have no free-page!");
			return FALSE;
		}
		load_cr3(current->tss.cr3);
		--mem_map[M2MAP(addr)];
		d_printf("set cr3 finished, cr3 = %x.\n", current->tss.cr3);
	}
	return TRUE;
}

/* * ȱҳ����ע�⣺ȱҳ��ζ�Ž�����ִ���µĿ�ִ���ļ������������ڴ�ռ䡣
 * ���������ڴ�ռ䣬��ô������û���Լ�������ҳĿ¼��(��Ϊ��������ʱҳĿ
 * ¼��Ҳ�ǹ����)����ˣ����ǻ�Ҫ�����Ƿ�Ϊ����ĳЩ���̷���ҳĿ¼��
 * ����ҳ��ҳ��ͬ���迼�ǡ� */
BOOL do_no_page(unsigned long vaddr)
{
	panic("do_no_page: page fault.");
	return TRUE;
}

/*
 * �ú���ȡ��ҳ��д������ע�⣺д������ζ�Ž��̹������ڴ�ռ䣬������
 * ����û���Լ�������ҳĿ¼��(��Ϊ��������ʱҳĿ¼��Ҳ�ǹ����)����ˣ�
 * ���ǻ�Ҫ�����Ƿ�Ϊ����ĳЩ���̷���ҳĿ¼������ҳ��ҳ��ͬ���迼�ǡ�
 * I: vaddr - ����ҳ�����жϵ�ҳ��������ַ.
 */
BOOL un_page_wp(unsigned long vaddr)
{
	unsigned long addr, tmp;

	if(vaddr >= V_KERNEL_ZONE_START)	/* ���ڵ����ں˴��� */
		panic("un_page_wp: BAD, write to kernel address space.");
	if(vaddr < current->code_start || vaddr >= current->data_end
		|| (vaddr >= current->code_end && vaddr < current->data_start))
		panic("un_page_wp: out of code or data space limit.");

	/* ���̻�û���Լ�������ҳĿ¼������� */
	if(1 < mem_map[M2MAP(current->tss.cr3)]) {
		addr = current->tss.cr3;
		if(!copy_page_dir(&(current->tss.cr3))) {
			k_printf("un_page_wp: have no free-page!");
			return FALSE;
		}
		load_cr3(current->tss.cr3);
		--mem_map[M2MAP(addr)];
	}
	addr = ADDR_IN_1PT(current->tss.cr3, vaddr);

	/* ���̻�û���Լ������Ķ���ҳ��ҳ�棬����� */
	if(1 < mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))]) {
		/* get_free_page������clean����Ϊ�������Ḵ������ҳ�� */
		if(NULL == (tmp=get_free_page())) {
			k_printf("un_page_wp: have no free-page!");
			return FALSE;
		}
		memcpy((char *)tmp, (char *)ALIGN_PAGE(*(unsigned long *)addr),
			PAGE_SIZE);
		--mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
		*(unsigned long *)addr = tmp + P_UWP;
	}
	addr = ALIGN_PAGE(*(unsigned long *)addr);
	addr = ADDR_IN_2PT(addr, vaddr);

	/* ����ҳ�����ô���Ϊ1���������д������������ҳ�� */
	if(1 == mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))]) {
		*(unsigned long *)addr |= 0x00000002;	/* ����ҳ���д */
	} else {
		if(NULL == (tmp=get_free_page())) {
			k_printf("un_page_wp: have no enough memory!");
			return FALSE;
		}
		memcpy((char *)tmp, (char *)ALIGN_PAGE(*(unsigned long *)addr),
			PAGE_SIZE);
		--mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
		*(unsigned long *)addr = tmp + P_UWP;
	}

	/* ˢ��TLB */
	invalidate_tlb();

	return TRUE;
}

/*
 * �ú�������ҳ�����жϡ�����ҳд�����Լ�ȱҳ��
 * I: vaddr - ����ҳ�����жϵ�ҳ��������ַ; error - ������.
 *
 * �������λ�ĺ��壺
 * 	λ2(U/S) - 0 ��ʾ�ڳ����û�ģʽ��ִ�У�1 ��ʾ���û�ģʽ��ִ�У�
 * 	λ1(W/R) - 0 ��ʾ��������1 ��ʾд������
 * 	λ0(P) - 0 ��ʾҳ�����ڣ�1 ��ʾҳ��������
 */
void do_page_fault(unsigned long vaddr, unsigned long error)
{
	if(!(error & 1)) {	/* ҳ�治���� */
		if(!do_no_page(vaddr)) {
			/* [??] nothing at present */
		}
	} else if(error & 2) {	/* ҳ����ڵ�����д */
		if(!un_page_wp(vaddr)) {
			/* [??] nothing at present */
		}
	} else {
		panic("do_page_fault: unknown page fault.");
	}
}

void mem_init(void)
{
	/* ��ʼ������0�Ĵ���κ����ݶ���ֹ��ַ */
	init_proc.proc.code_start = 0;
	init_proc.proc.code_end = (unsigned long)&_etext;
	init_proc.proc.data_start = (unsigned long)&_etext;
	init_proc.proc.data_end = (unsigned long)&_end;

	/* �ڴ沼�� */	
	/* 0x90002��ַ����������չ�ڴ��С */
	high_mem = (*(unsigned short *)0x90002+1024)*1024;
	d_printf("memory-size: %d Bytes.\n", high_mem);
	high_mem = ALIGN_PAGE(high_mem);
	if(high_mem >= 16*1024*1024)
		high_mem = 16*1024*1024;
	if(high_mem < 10*1024*1024)
		panic("the size of memory can't below 10MB.");
	buffer_mem_end = 3*1024*1024;
	buffer_mem_start = (unsigned long)&_end;
	
	/* ��ʼ��mem_map */
	memset(mem_map, (unsigned char)USED, MEM_MAP_SIZE);
	/* NOTE! (&mem_map)+M2MAP(buffer_mem_end) �Ǵ���ģ�����C�﷨�� */
	zeromem(&mem_map[M2MAP(buffer_mem_end)],
		(high_mem-buffer_mem_end)/PAGE_SIZE);
	/*
	 * ��init����(����0)ҳĿ¼��ʹ�ô�������Ϊ1����Ϊ
	 * ҳд�������ڴ�������������ҳĿ¼�ȵ�ʹ�ô�����
	 */
	mem_map[M2MAP(init_proc.proc.tss.cr3)] = 1;
}
