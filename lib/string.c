/*
 * Small/lib/string.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 该文件可用于内核态和用户态的字符串以及内存的简单处理。注意以下几点：
 *
 *	1. 以常规开头命名的例程，源地址和目的地址要在相同的段中；
 *	2. 以"fs_"开头命名的例程，数据地址在fs段中；
 *	3. 以"fs_ds_"开头命名的，参数前者地址在fs段中，后者地址在ds段中；
 *	4. 以"ds_fs_"开头命名的，参数前者地址在ds段中，后者地址在fs段中；
 *	5. 以"fs2ds_"开头命名的，源地址在fs段中，目标地址在ds段中；
 *	6. 以"ds2fs_"开头命名的，源地址在ds段中，目标地址在fs段中；
 *	......
 *
 * 实际上，常规命名的可在用户态和内核态使用，其他命名的只用在内核态。
 */

/********************************************************************/

/*
 * 求字符串长度。
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
 * 复制字符串，返回目的字符串指针。
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
 * 字符串比较。
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
 * 内存复制，返回目的内存指针。
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
 * 内存填充，返回目的内存指针。
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
 * 初始化内存为0，返回目的内存指针。
 */
void * zeromem(void *des, int count)
{
	return memset(des, 0, count);
}
void * fs_zeromem(void *des, int count)
{
	return fs_memset(des, 0, count);
}
