/*
 * Small/include/elf.h
 * 
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */

/*
 * 此文件包含 ELF 文件格式相关数据结构以及宏定义。
 */

#ifndef _ELF_H_
#define _ELF_H_

/********************************************************************/
/* ELF 文件格式相关数据结构 */


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
	unsigned char	e_ident[EI_NIDENT];	/* 目标文件标识 */
	Elf32_Half	e_type;		/* 目标文件类型 */
	Elf32_Half	e_machine;	/* 文件的目标体系结构类型 */
	Elf32_Word	e_version;	/* 目标文件版本 */
	Elf32_Addr	e_entry;	/* 程序入口的虚拟地址 */
	Elf32_Off	e_phoff;	/* 程序头部表格的偏移量（按字节计算） */
	Elf32_Off	e_shoff;	/* 节区头部表格的偏移量（按字节计算） */
	Elf32_Word  e_flags;	/* 保存与文件相关的，特定于处理器的标志 */
	Elf32_Half  e_ehsize;	/* ELF 头部的大小（以字节计算） */
	Elf32_Half  e_phentsize;/* 程序头部表格的表项大小（按字节计算） */
	Elf32_Half  e_phnum;	/* 程序头部表格的表项数目 */
	Elf32_Half  e_shentsize;/* 节区头部表格的表项大小（按字节计算） */
	Elf32_Half  e_shnum;	/* 节区头部表格的表项数目 */
	Elf32_Half  e_shstrndx;	/* 节区头部表格中与节区名称字符串表相关的表项的索引 */
} Elf32_Ehdr;

/* e_ident[EI_NIDENT] 各字段下标及其对应的值 */
#define EI_MAG0		(0)		/* 魔数 -- 第1字节 -- 0x7f */
#define ELFMAG0		(0x7f)
#define EI_MAG1		(1)		/* 魔数 -- 第2字节 -- 'E' */
#define ELFMAG1		('E')
#define EI_MAG2		(2)		/* 魔数 -- 第3字节 -- 'L' */
#define ELFMAG2		('L')
#define EI_MAG3		(3)		/* 魔数 -- 第4字节 -- 'F' */
#define ELFMAG3		('F')
#define EI_CLASS	(4)		/* 标识文件类型，32位目标、64位目标 */
#define ELFCLASSNONE	(0)
#define ELFCLASS32	(1)
#define ELFCLASS64	(2)
#define EI_DATA		(5)		/* 处理器对内存数据的编码方式，小端、大端 */
#define ELFDATANONE	(0)
#define ELFDATA2LSB	(1)
#define ELFDATA2MSB	(2)
#define EI_VERSION	(6)		/* ELF 头部的版本号码，目前此值必须是 EV_CURRENT */
#define EI_PAD		(7)		/* 标记 e_ident 中未使用字节的开始。初始化为0 */
#define ELFPAD		(0)

/* e_type */
#define ET_NONE		(0)		/* 未知目标文件格式 */
#define ET_REL		(1)		/* 可重定位文件 */
#define ET_EXEC		(2)		/* 可执行文件 */
#define ET_DYN		(3)		/* 共享目标文件 */
#define ET_CORE		(4)		/* Core文件（转储格式） */
#define ET_LOPROC	(0xff00)/* ET_LOPROC -- ET_HIPROC 标识处理器相关文件 */
#define ET_HIPROC	(0xffff)

/* e_machine */
#define EM_NONE		(0)		/* 未指定 */
#define EM_M32		(1)		/* AT&T WE 32100 */
#define EM_SPARC	(2)		/* SPARC */
#define EM_386		(3)		/* Intel 80386 */
#define EM_68K		(4)		/* Motorola 68000 */
#define EM_88K		(5)		/* Motorola 88000 */
#define EM_860		(7)		/* Intel 80860 */
#define EM_MIPS		(8)		/* MIPS RS3000 */

/* e_version */
#define EV_NONE		(0)		/* 非法版本 */
#define EV_CURRENT	(1)		/* 当前版本 */


/* Program Header */

/* Program Header structure */
typedef struct {
	Elf32_Word 	p_type;  	/* 如何解释此数组元素的信息 */
	Elf32_Off   p_offset;  	/* 从文件头到该段第一个字节的偏移 */
	Elf32_Addr 	p_vaddr;  	/* 段的第一个字节将被放到内存中的虚拟地址 */
	Elf32_Addr 	p_paddr;  	/* 仅用于与物理地址相关的系统中 */
	Elf32_Word 	p_filesz;  	/* 段在文件映像中所占的字节数。可以为0 */
	Elf32_Word 	p_memsz;  	/* 段在内存映像中占用的字节数。可以为0 */
	Elf32_Word 	p_flags;  	/* 与段相关的标志 */
	Elf32_Word 	p_align;  	/* 段在文件中和内存中如何对齐 */
} Elf32_Phdr;

/* p_type */
#define PT_NULL		(0)	/* 表明结构中其他成员都是未定义的 */
#define PT_LOAD		(1)	/* 一个可加载的段 */
#define PT_DYNAMIC	(2)	/* 动态链接信息 */
#define PT_INTERP	(3)	/* 一个字符串的位置和长度，该字符串将被当作解释器调用 */
#define PT_NOTE 	(4)	/* 附加信息的位置和大小 */
#define PT_SHLIB	(5)	/* [保留] */
#define PT_PHDR		(6)	/* 程序头部表自身的大小和位置 */
#define PT_LOPROC	(0x70000000) /* [保留给处理器] */
#define PT_HIPROC 	(0x7fffffff) /* [保留给处理器] */

/* p_flags */
#define PF_X		(0x1)			/* [Execute] */
#define PF_W		(0x2) 			/* [Write] */
#define PF_R		(0x4)			/* [Read] */
#define PF_MASKOS	(0x0ff00000)	/* [所有二进制位未指定，保留给操作系统] */
#define PF_MASKPROC	(0xf0000000)	/* [所有二进制位未指定，保留给用户程序] */


/* Section Header */

/* Section Header structure */
typedef struct {
	Elf32_Word	sh_name;  	/* 节区名称。是节区头部字符串表节区的索引 */
	Elf32_Word	sh_type;  	/* 节区类型 */
	Elf32_Word	sh_flags;	/* 属性。如果一个标志位被设置，则该位取值为1。未定义的各位都设置为0. */
	Elf32_Addr	sh_addr;  	/* 若节区将出现在进程的内存映像中，此成员给出节区的第一个字节应处的位置 */
	Elf32_Off	sh_offset; 	/* 节区的第一个字节与文件头之间的偏移 */
	Elf32_Word	sh_size;  	/* 节区的长度（字节数） */
	Elf32_Word	sh_link;  	/* 节区头部表索引链接 */
	Elf32_Word	sh_info;  	/* 附加信息 */
	Elf32_Word	sh_addralign;	/* 地址对齐约束 */
	Elf32_Word	sh_entsize;	/* 某些节区中包含固定大小的项目。对于这类节区，此成员给出每个表项的长度字节数 */
} Elf32_Shdr; 

/* sh_type */
#define SHT_NULL		(0)	/* 标志节区头部是非活动的，没有对应的节区 */
#define SHT_PROGBITS	(1)	/* 此节区包含程序定义的信息，其格式和含义都由程序来解释 */
#define SHT_SYMTAB		(2)	/* 此节区包含一个符号表 */
#define SHT_STRTAB		(3)	/* 此节区包含字符串表 */
#define SHT_RELA		(4)	/* 此节区包含重定位表项 */
#define SHT_HASH		(5)	/* 此节区包含符号哈希表 */
#define SHT_DYNAMIC		(6)	/* 此节区包含动态链接的信息 */
#define SHT_NOTE		(7)	/* 此节区包含以某种方式来标记文件的信息 */
#define SHT_NOBITS		(8)	/* 此节区不占用文件中的空间，其他方面和 SHT_PROGBITS 相似 */
#define SHT_REL 		(9)	/* 此节区包含重定位表项 */
#define SHT_SHLIB 		(10)	/* [保留] */
#define SHT_DYNSYM		(11)	/* 保存动态链接符号的一个最小集合 */
#define SHT_LOPROC		(0x70000000)	/* SHT_LOPROC - SHT_HIPROC, 保留给处理器专用语义 */
#define SHT_HIPROC		(0x7fffffff)
#define SHT_LOUSER		(0x80000000)	/* 保留给应用程序的索引下界 */
#define SHT_HIUSER		(0x8fffffff)	/* 保留给应用程序的索引上界 */

/* sh_flags */
#define SHF_WRITE		(0x1)	/* 可写的数据 */
#define SHF_ALLOC		(0x2)	/* 占用内存空间 */
#define SHF_EXECINSTR	(0x4)	/* 可执行的机器指令 */
#define SHF_MASKPROC	(0xf0000000)	/* 所有包含于此掩码中的四位都用于处理器专用的语义 */

/********************************************************************/

#endif	/* _ELF_H_ */
