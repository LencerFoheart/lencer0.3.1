/*
 * Small/lib/string.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ��������ں�̬���û�̬���ַ����Լ��ڴ�ļ򵥴���ע�����¼��㣺
 *
 *	1. �Գ��濪ͷ���������̣�Դ��ַ��Ŀ�ĵ�ַҪ����ͬ�Ķ��У�
 *	2. ��"fs_"��ͷ���������̣����ݵ�ַ��fs���У�
 *	3. ��"fs_ds_"��ͷ�����ģ�����ǰ�ߵ�ַ��fs���У����ߵ�ַ��ds���У�
 *	4. ��"ds_fs_"��ͷ�����ģ�����ǰ�ߵ�ַ��ds���У����ߵ�ַ��fs���У�
 *	5. ��"fs2ds_"��ͷ�����ģ�Դ��ַ��fs���У�Ŀ���ַ��ds���У�
 *	6. ��"ds2fs_"��ͷ�����ģ�Դ��ַ��ds���У�Ŀ���ַ��fs���У�
 *	......
 *
 * ʵ���ϣ����������Ŀ����û�̬���ں�̬ʹ�ã�����������ֻ�����ں�̬��
 */

/********************************************************************/

/*
 * ���ַ������ȡ�
 */
int strlen(char *str)
{
	int count;

	__asm__ __volatile__(
	"1:"
		"movb	%%ds:(%%esi),%%al\n\t"	/* ds */
		"incl	%%esi\n\t"
		"incl	%%ecx\n\t"
		"cmpb	%%al,%%bl\n\t"
		"jne	1b\n\t"
		"decl	%%ecx\n\t"
		:"=c"(count) :"0"(0),"b"(0),"S"(str) :
	);

	return count;
}
int fs_strlen(char *str)
{
	int count;

	__asm__ __volatile__(
	"1:"
		"movb	%%fs:(%%esi),%%al\n\t"	/* fs */
		"incl	%%esi\n\t"
		"incl	%%ecx\n\t"
		"cmpb	%%al,%%bl\n\t"
		"jne	1b\n\t"
		"decl	%%ecx\n\t"
		:"=c"(count) :"0"(0),"b"(0),"S"(str) :
	);

	return count;
}

/*
 * �����ַ���������Ŀ���ַ���ָ�롣
 */
char * strcpy(char *des, char *src)
{
	__asm__ __volatile__(
	"1:"
		"movb	%%ds:(%%esi),%%al\n\t"	/* ds */
		"cmpb	%%al,%%bl\n\t"
		"je		2f\n\t"
		"movb	%%al,%%ds:(%%edi)\n\t"	/* ds */
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"jmp	1b\n\t"
	"2:"
		"movb	%%bl,%%ds:(%%edi)\n\t"	/* ds */
		::"S"(src),"D"(des),"b"(0) :
	);
	
	return des;
}
char * fs2ds_strcpy(char *des, char *src)
{
	__asm__ __volatile__(
	"1:"
		"movb	%%fs:(%%esi),%%al\n\t"	/* fs */
		"cmpb	%%al,%%bl\n\t"
		"je		2f\n\t"
		"movb	%%al,%%ds:(%%edi)\n\t"	/* ds */
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"jmp	1b\n\t"
	"2:"
		"movb	%%bl,%%ds:(%%edi)\n\t"	/* ds */
		::"S"(src),"D"(des),"b"(0) :
	);
	
	return des;
}
char * ds2fs_strcpy(char *des, char *src)
{
	__asm__ __volatile__(
	"1:"
		"movb	%%ds:(%%esi),%%al\n\t"	/* ds */
		"cmpb	%%al,%%bl\n\t"
		"je		2f\n\t"
		"movb	%%al,%%fs:(%%edi)\n\t"	/* fs */
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"jmp	1b\n\t"
	"2:"
		"movb	%%bl,%%fs:(%%edi)\n\t"	/* fs */
		::"S"(src),"D"(des),"b"(0) :
	);
	
	return des;
}

/*
 * �ַ����Ƚϡ�
 */
int strcmp(char *str1, char *str2)
{
	int ret;
	
	__asm__ __volatile__(
	"1:"
		"movb	%%ds:(%%esi),%%al\n\t"	/* ds */
		"movb	%%ds:(%%edi),%%bl\n\t"	/* ds */
		"cmpb	%%al,%%bl\n\t"
		"ja		3f\n\t"
		"jb		4f\n\t"
		"testb	%%al,%%al\n\t"
		"je		2f\n\t"
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"jmp	1b\n\t"
	"2:"
		"movl	$0,%%ecx\n\t"
		"jmp	5f\n\t"
	"3:"
		"movl	$1,%%ecx\n\t"
		"jmp	5f\n\t"
	"4:"
		"movl	$-1,%%ecx\n\t"
	"5:"
		:"=c"(ret) :"S"(str1),"D"(str2) :
	);
	
	return ret;
}
int fs_ds_strcmp(char *str1, char *str2)
{
	int ret;
	
	__asm__ __volatile__(
	"1:"
		"movb	%%fs:(%%esi),%%al\n\t"	/* fs */
		"movb	%%ds:(%%edi),%%bl\n\t"	/* ds */
		"cmpb	%%al,%%bl\n\t"
		"ja		3f\n\t"
		"jb		4f\n\t"
		"testb	%%al,%%al\n\t"
		"je		2f\n\t"
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"jmp	1b\n\t"
	"2:"
		"movl	$0,%%ecx\n\t"
		"jmp	5f\n\t"
	"3:"
		"movl	$1,%%ecx\n\t"
		"jmp	5f\n\t"
	"4:"
		"movl	$-1,%%ecx\n\t"
	"5:"
		:"=c"(ret) :"S"(str1),"D"(str2) :
	);
	
	return ret;
}
int ds_fs_strcmp(char *str1, char *str2)
{
	int ret;
	
	__asm__ __volatile__(
	"1:"
		"movb	%%ds:(%%esi),%%al\n\t"	/* ds */
		"movb	%%fs:(%%edi),%%bl\n\t"	/* fs */
		"cmpb	%%al,%%bl\n\t"
		"ja		3f\n\t"
		"jb		4f\n\t"
		"testb	%%al,%%al\n\t"
		"je		2f\n\t"
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"jmp	1b\n\t"
	"2:"
		"movl	$0,%%ecx\n\t"
		"jmp	5f\n\t"
	"3:"
		"movl	$1,%%ecx\n\t"
		"jmp	5f\n\t"
	"4:"
		"movl	$-1,%%ecx\n\t"
	"5:"
		:"=c"(ret) :"S"(str1),"D"(str2) :
	);
	
	return ret;
}

/********************************************************************/

/*
 * �ڴ渴�ƣ�����Ŀ���ڴ�ָ�롣
 */
void * memcpy(void *des, void *src, int count)
{
	__asm__ __volatile__(
	"1:"
		"testl	%%ecx,%%ecx\n\t"
		"je		2f\n\t"
		"movb	%%ds:(%%esi),%%al\n\t"	/* ds */
		"movb	%%al,%%ds:(%%edi)\n\t"	/* ds */
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"decl	%%ecx\n\t"
		"jmp	1b\n\t"
	"2:"
		::"c"(count),"S"(src),"D"(des) :
	);

	return des;
}
void * fs2ds_memcpy(void *des, void *src, int count)
{
	__asm__ __volatile__(
	"1:"
		"testl	%%ecx,%%ecx\n\t"
		"je		2f\n\t"
		"movb	%%fs:(%%esi),%%al\n\t"	/* fs */
		"movb	%%al,%%ds:(%%edi)\n\t"	/* ds */
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"decl	%%ecx\n\t"
		"jmp	1b\n\t"
	"2:"
		::"c"(count),"S"(src),"D"(des) :
	);
	
	return des;
}
void * ds2fs_memcpy(void *des, void *src, int count)
{
	__asm__ __volatile__(
	"1:"
		"testl	%%ecx,%%ecx\n\t"
		"je		2f\n\t"
		"movb	%%ds:(%%esi),%%al\n\t"	/* ds */
		"movb	%%al,%%fs:(%%edi)\n\t"	/* fs */
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"decl	%%ecx\n\t"
		"jmp	1b\n\t"
	"2:"
		::"c"(count),"S"(src),"D"(des) :
	);

	return des;
}
void * gs2gs_memcpy(void *des, void *src, int count)
{
	__asm__ __volatile__(
	"1:"
		"testl	%%ecx,%%ecx\n\t"
		"je		2f\n\t"
		"movb	%%gs:(%%esi),%%al\n\t"	/* gs */
		"movb	%%al,%%gs:(%%edi)\n\t"	/* gs */
		"incl	%%esi\n\t"
		"incl	%%edi\n\t"
		"decl	%%ecx\n\t"
		"jmp	1b\n\t"
	"2:"
		::"c"(count),"S"(src),"D"(des) :
	);

	return des;
}

/*
 * �ڴ���䣬����Ŀ���ڴ�ָ�롣
 */
void * memset(void *des, unsigned char c, int count)
{
	__asm__ __volatile__(
	"1:"
		"testl	%%ecx,%%ecx\n\t"
		"je		2f\n\t"
		"decl	%%edi\n\t"
		"movb	%%al,%%ds:(%%edi)\n\t"	/* ds */
		"decl	%%ecx\n\t"
		"jmp	1b\n\t"
	"2:"
		::"c"(count),"a"(c),"D"((unsigned long)des+count) :
	);

	return des;
}
void * fs_memset(void *des, unsigned char c, int count)
{
	__asm__ __volatile__(
	"1:"
		"testl	%%ecx,%%ecx\n\t"
		"je		2f\n\t"
		"decl	%%edi\n\t"
		"movb	%%al,%%fs:(%%edi)\n\t"	/* fs */
		"decl	%%ecx\n\t"
		"jmp	1b\n\t"
	"2:"
		::"c"(count),"a"(c),"D"((unsigned long)des+count) :
	);
	
	return des;
}

/*
 * ��ʼ���ڴ�Ϊ0������Ŀ���ڴ�ָ�롣
 */
void * zeromem(void *des, int count)
{
	return memset(des, 0, count);
}
void * fs_zeromem(void *des, int count)
{
	return fs_memset(des, 0, count);
}
