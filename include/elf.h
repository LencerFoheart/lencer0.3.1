/*
 * Small/include/elf.h
 * 
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ����� ELF �ļ���ʽ������ݽṹ�Լ��궨�塣
 */

#ifndef _ELF_H_
#define _ELF_H_

/********************************************************************/
/* ELF �ļ���ʽ������ݽṹ */


/* ELF data types */

typedef unsigned short 	Elf32_Half;
typedef unsigned long 	Elf32_Addr;
typedef unsigned long	Elf32_Off;
typedef signed long		Elf32_SWord;
typedef unsigned long	Elf32_Word;


/* ELF Header */

/* ELF Header structure */
#define EI_NIDENT	(16)
typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* Ŀ���ļ���ʶ */
	Elf32_Half	e_type;		/* Ŀ���ļ����� */
	Elf32_Half	e_machine;	/* �ļ���Ŀ����ϵ�ṹ���� */
	Elf32_Word	e_version;	/* Ŀ���ļ��汾 */
	Elf32_Addr	e_entry;	/* ������ڵ������ַ */
	Elf32_Off	e_phoff;	/* ����ͷ������ƫ���������ֽڼ��㣩 */
	Elf32_Off	e_shoff;	/* ����ͷ������ƫ���������ֽڼ��㣩 */
	Elf32_Word  e_flags;	/* �������ļ���صģ��ض��ڴ������ı�־ */
	Elf32_Half  e_ehsize;	/* ELF ͷ���Ĵ�С�����ֽڼ��㣩 */
	Elf32_Half  e_phentsize;/* ����ͷ�����ı����С�����ֽڼ��㣩 */
	Elf32_Half  e_phnum;	/* ����ͷ�����ı�����Ŀ */
	Elf32_Half  e_shentsize;/* ����ͷ�����ı����С�����ֽڼ��㣩 */
	Elf32_Half  e_shnum;	/* ����ͷ�����ı�����Ŀ */
	Elf32_Half  e_shstrndx;	/* ����ͷ�����������������ַ�������صı�������� */
} Elf32_Ehdr;

/* e_ident[EI_NIDENT] ���ֶ��±꼰���Ӧ��ֵ */
#define EI_MAG0		(0)		/* ħ�� -- ��1�ֽ� -- 0x7f */
#define ELFMAG0		(0x7f)
#define EI_MAG1		(1)		/* ħ�� -- ��2�ֽ� -- 'E' */
#define ELFMAG1		('E')
#define EI_MAG2		(2)		/* ħ�� -- ��3�ֽ� -- 'L' */
#define ELFMAG2		('L')
#define EI_MAG3		(3)		/* ħ�� -- ��4�ֽ� -- 'F' */
#define ELFMAG3		('F')
#define EI_CLASS	(4)		/* ��ʶ�ļ����ͣ�32λĿ�ꡢ64λĿ�� */
#define ELFCLASSNONE	(0)
#define ELFCLASS32	(1)
#define ELFCLASS64	(2)
#define EI_DATA		(5)		/* ���������ڴ����ݵı��뷽ʽ��С�ˡ���� */
#define ELFDATANONE	(0)
#define ELFDATA2LSB	(1)
#define ELFDATA2MSB	(2)
#define EI_VERSION	(6)		/* ELF ͷ���İ汾���룬Ŀǰ��ֵ������ EV_CURRENT */
#define EI_PAD		(7)		/* ��� e_ident ��δʹ���ֽڵĿ�ʼ����ʼ��Ϊ0 */
#define ELFPAD		(0)

/* e_type */
#define ET_NONE		(0)		/* δ֪Ŀ���ļ���ʽ */
#define ET_REL		(1)		/* ���ض�λ�ļ� */
#define ET_EXEC		(2)		/* ��ִ���ļ� */
#define ET_DYN		(3)		/* ����Ŀ���ļ� */
#define ET_CORE		(4)		/* Core�ļ���ת����ʽ�� */
#define ET_LOPROC	(0xff00)/* ET_LOPROC -- ET_HIPROC ��ʶ����������ļ� */
#define ET_HIPROC	(0xffff)

/* e_machine */
#define EM_NONE		(0)		/* δָ�� */
#define EM_M32		(1)		/* AT&T WE 32100 */
#define EM_SPARC	(2)		/* SPARC */
#define EM_386		(3)		/* Intel 80386 */
#define EM_68K		(4)		/* Motorola 68000 */
#define EM_88K		(5)		/* Motorola 88000 */
#define EM_860		(7)		/* Intel 80860 */
#define EM_MIPS		(8)		/* MIPS RS3000 */

/* e_version */
#define EV_NONE		(0)		/* �Ƿ��汾 */
#define EV_CURRENT	(1)		/* ��ǰ�汾 */


/* Program Header */

/* Program Header structure */
typedef struct {
	Elf32_Word 	p_type;  	/* ��ν��ʹ�����Ԫ�ص���Ϣ */
	Elf32_Off   p_offset;  	/* ���ļ�ͷ���öε�һ���ֽڵ�ƫ�� */
	Elf32_Addr 	p_vaddr;  	/* �εĵ�һ���ֽڽ����ŵ��ڴ��е������ַ */
	Elf32_Addr 	p_paddr;  	/* �������������ַ��ص�ϵͳ�� */
	Elf32_Word 	p_filesz;  	/* �����ļ�ӳ������ռ���ֽ���������Ϊ0 */
	Elf32_Word 	p_memsz;  	/* �����ڴ�ӳ����ռ�õ��ֽ���������Ϊ0 */
	Elf32_Word 	p_flags;  	/* �����صı�־ */
	Elf32_Word 	p_align;  	/* �����ļ��к��ڴ�����ζ��� */
} Elf32_Phdr;

/* p_type */
#define PT_NULL		(0)	/* �����ṹ��������Ա����δ����� */
#define PT_LOAD		(1)	/* һ���ɼ��صĶ� */
#define PT_DYNAMIC	(2)	/* ��̬������Ϣ */
#define PT_INTERP	(3)	/* һ���ַ�����λ�úͳ��ȣ����ַ��������������������� */
#define PT_NOTE 	(4)	/* ������Ϣ��λ�úʹ�С */
#define PT_SHLIB	(5)	/* [����] */
#define PT_PHDR		(6)	/* ����ͷ��������Ĵ�С��λ�� */
#define PT_LOPROC	(0x70000000) /* [������������] */
#define PT_HIPROC 	(0x7fffffff) /* [������������] */

/* p_flags */
#define PF_X		(0x1)			/* [Execute] */
#define PF_W		(0x2) 			/* [Write] */
#define PF_R		(0x4)			/* [Read] */
#define PF_MASKOS	(0x0ff00000)	/* [���ж�����λδָ��������������ϵͳ] */
#define PF_MASKPROC	(0xf0000000)	/* [���ж�����λδָ�����������û�����] */


/* Section Header */

/* Section Header structure */
typedef struct {
	Elf32_Word	sh_name;  	/* �������ơ��ǽ���ͷ���ַ�������������� */
	Elf32_Word	sh_type;  	/* �������� */
	Elf32_Word	sh_flags;	/* ���ԡ����һ����־λ�����ã����λȡֵΪ1��δ����ĸ�λ������Ϊ0. */
	Elf32_Addr	sh_addr;  	/* �������������ڽ��̵��ڴ�ӳ���У��˳�Ա���������ĵ�һ���ֽ�Ӧ����λ�� */
	Elf32_Off	sh_offset; 	/* �����ĵ�һ���ֽ����ļ�ͷ֮���ƫ�� */
	Elf32_Word	sh_size;  	/* �����ĳ��ȣ��ֽ����� */
	Elf32_Word	sh_link;  	/* ����ͷ������������ */
	Elf32_Word	sh_info;  	/* ������Ϣ */
	Elf32_Word	sh_addralign;	/* ��ַ����Լ�� */
	Elf32_Word	sh_entsize;	/* ĳЩ�����а����̶���С����Ŀ����������������˳�Ա����ÿ������ĳ����ֽ��� */
} Elf32_Shdr; 

/* sh_type */
#define SHT_NULL		(0)	/* ��־����ͷ���Ƿǻ�ģ�û�ж�Ӧ�Ľ��� */
#define SHT_PROGBITS	(1)	/* �˽����������������Ϣ�����ʽ�ͺ��嶼�ɳ��������� */
#define SHT_SYMTAB		(2)	/* �˽�������һ�����ű� */
#define SHT_STRTAB		(3)	/* �˽��������ַ����� */
#define SHT_RELA		(4)	/* �˽��������ض�λ���� */
#define SHT_HASH		(5)	/* �˽����������Ź�ϣ�� */
#define SHT_DYNAMIC		(6)	/* �˽���������̬���ӵ���Ϣ */
#define SHT_NOTE		(7)	/* �˽���������ĳ�ַ�ʽ������ļ�����Ϣ */
#define SHT_NOBITS		(8)	/* �˽�����ռ���ļ��еĿռ䣬��������� SHT_PROGBITS ���� */
#define SHT_REL 		(9)	/* �˽��������ض�λ���� */
#define SHT_SHLIB 		(10)	/* [����] */
#define SHT_DYNSYM		(11)	/* ���涯̬���ӷ��ŵ�һ����С���� */
#define SHT_LOPROC		(0x70000000)	/* SHT_LOPROC - SHT_HIPROC, ������������ר������ */
#define SHT_HIPROC		(0x7fffffff)
#define SHT_LOUSER		(0x80000000)	/* ������Ӧ�ó���������½� */
#define SHT_HIUSER		(0x8fffffff)	/* ������Ӧ�ó���������Ͻ� */

/* sh_flags */
#define SHF_WRITE		(0x1)	/* ��д������ */
#define SHF_ALLOC		(0x2)	/* ռ���ڴ�ռ� */
#define SHF_EXECINSTR	(0x4)	/* ��ִ�еĻ���ָ�� */
#define SHF_MASKPROC	(0xf0000000)	/* ���а����ڴ������е���λ�����ڴ�����ר�õ����� */

/********************************************************************/

#endif	/* _ELF_H_ */
