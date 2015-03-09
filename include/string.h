/*
 * Small/include/string.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _STRING_H_
#define _STRING_H_
/********************************************************************/
/* 从fs段取得指定地址的值 */
#define get_fs_byte(addr) ({ \
	unsigned char var; \
	__asm__("movb %%fs:(%%esi),%%al" :"=a"(var) :"S"(addr):); \
	var; \
})
#define get_fs_word(addr) ({ \
	unsigned short var; \
	__asm__("movw %%fs:(%%esi),%%ax" :"=a"(var) :"S"(addr):); \
	var; \
})
#define get_fs_long(addr) ({ \
	unsigned long var; \
	__asm__("movl %%fs:(%%esi),%%eax" :"=a"(var) :"S"(addr):); \
	var; \
})

/* 将指定的值送外fs段指定地址 */
#define set_fs_byte(addr,a) ({ \
	__asm__("movb %%al,%%fs:(%%edi)" ::"a"(a),"D"(addr) :); \
})
#define set_fs_word(addr,a) ({ \
	__asm__("movw %%ax,%%fs:(%%edi)" ::"a"(a),"D"(addr) :); \
})
#define set_fs_long(addr,a) ({ \
	__asm__("movl %%eax,%%fs:(%%edi)" ::"a"(a),"D"(addr) :); \
})

#define set_gs_word(addr,a) ({ \
	__asm__("movw %%ax,%%gs:(%%edi)" ::"a"(a),"D"(addr) :); \
})

#define set_ss_long(addr,a) ({ \
	__asm__("movl %%eax,%%ss:(%%edi)" ::"a"(a),"D"(addr) :); \
})
/********************************************************************/
extern int strlen(char *str);
extern int fs_strlen(char *str);

extern char * strcpy(char *des, char *src);
extern char * fs2ds_strcpy(char *des, char *src);
extern char * ds2fs_strcpy(char *des, char *src);

extern int strcmp(char *str1, char *str2);
extern int fs_ds_strcmp(char *str1, char *str2);
extern int ds_fs_strcmp(char *str1, char *str2);


extern void * memcpy(void *des, void *src, int count);
extern void * fs2ds_memcpy(void *des, void *src, int count);
extern void * ds2fs_memcpy(void *des, void *src, int count);
extern void * gs2gs_memcpy(void *des, void *src, int count);

extern void * memset(void *des, unsigned char c, int count);
extern void * fs_memset(void *des, unsigned char c, int count);

extern void * zeromem(void *des, int count);
extern void * fs_zeromem(void *des, int count);
/********************************************************************/
#endif /* _STRING_H_ */
