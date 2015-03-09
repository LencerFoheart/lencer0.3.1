/*
 * Small/kernel/console.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * ���ļ���������̨�������������̨д��������fs���С����⣬ͨ��
 * �Ĵ�������"��Ļ��ʼλ��"��"���λ��"ʱ�����ס���Ƕ���������Դ�
 * ��ʼ���ģ�����������ڵ�ǰ����̨��˵�ġ�
 *
 * [??] NOTE! �����ж������̨�����console_write()������Ӧָ��ʹ��
 * �ĸ�����̨����Ŀǰ�������ơ�
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "types.h"
#include "sys_set.h"
#include "console.h"
#include "string.h"

CONSOLE	console;

/*
 * ���ù��λ�á����λ�����������Դ�ģ���������������Ļ�ġ�
 */
void set_cur_pos(CONSOLE * cons, unsigned long curpos)
{
	/* �ǲ�����ʾ�����򷵻� */
	if(cur_oo_zone(cons, curpos))
		return;
	cons->cur_pos = curpos;
	curpos /= 2;	/* curpos�ǹ����ڴ��ַ������2����������� */
	out_b(CRT_ADDR_REG, CRT_CUR_HIGH_REG);
	out_b(CRT_DATA_REG, (curpos >> 8) & 0xff);
	out_b(CRT_ADDR_REG, CRT_CUR_LOW_REG);
	out_b(CRT_DATA_REG, curpos & 0xff);
}

/*
 * ���ݵ�ǰ���λ������ƶ���ꡣoffset�����ɸ����㡣
 */
void move_cursor(CONSOLE * cons, long offset)
{
	return set_cur_pos(cons, cons->cur_pos+offset);
}

/*
 * ������Ļ��ʾ��ʼ��ַ��������Դ濪ʼ����
 */
void set_scr_org(CONSOLE * cons, unsigned long org)
{
	org = ALIGN_LINE(org);	/* �ж��� */
	if(scr_oo_zone(cons, org))
		return;
	cons->scr_org = org;
	org /= 2;	/* org����ʾ���ڴ濪ʼ��ַ������2����������� */
	out_b(CRT_ADDR_REG, CRT_START_HIGH_REG);
	out_b(CRT_DATA_REG, (org >> 8) & 0xff);
	out_b(CRT_ADDR_REG, CRT_START_LOW_REG);
	out_b(CRT_DATA_REG, org & 0xff);
}

/*
 * ���ݵ�ǰ��Ļ��ʾ��ʼ��������ƫ�Ƶ�����offset�����ɸ����㡣
 */
void move_scr_org(CONSOLE * cons, long offset)
{
	return set_scr_org(cons, cons->scr_org+offset);
}

/*
 * ����̨д�ַ�������ͬʱ�ƶ���ꡣ
 */
void write_char(CONSOLE * cons, char c)
{
	set_gs_word(cons->cur_pos, (SHOW_MODE<<8)|c);
	cons->cur_pos += 2;
	/*
	 * ���λ�ó�����Ļ��Χ������Ļ�¹������ڿ��ܴ�
	 * cons->org ���¿�ʼ�����������ꡣ
	 */
	if(cur_oo_zone(cons, cons->cur_pos)) {
		screen_down(cons);
		set_cur_pos(cons, cons->scr_org+SCR_BYTES-LINE_BYTES);
	} else {
		set_cur_pos(cons, cons->cur_pos);
	}
}

/*
 * ���һ���ַ���ֻ���һ���е��ַ���
 */
void clean_char(CONSOLE * cons)
{
	if(is_line_align(cons->cur_pos))/* ����һ�е��ʼ */
		return;
	move_cursor(cons, -2);
	write_char(cons, CLEAR_CHAR);
	move_cursor(cons, -2);
}

/*
 * ���һ���ַ���start - Ҫ����������ַ��Ŀ�ʼ��ַ.
 */
void clean_line(unsigned long start)
{
	start = ALIGN_LINE(start);	/* �ж��� */
	for(int i=0; i<CON_COLS; i++) {
		set_gs_word(start+i*2, (SHOW_MODE<<8)|CLEAR_CHAR);
	}
}

/*
 * ��Ļ����һ�С�Ҳ���ַ��Ϲ��������ﲢû�жԹ��������á�
 */
void screen_down(CONSOLE * cons)
{
	/* �������һ�лᳬ������̨�ڴ����ƣ���ص�����̨��ʼ�� */
	if(scr_oo_zone(cons, cons->scr_org+LINE_BYTES)) {
		gs2gs_memcpy((char *)cons->org, (char *)cons->scr_org+LINE_BYTES,
			SCR_BYTES-LINE_BYTES);
		cons->scr_org = cons->org;
	} else {
		cons->scr_org += LINE_BYTES;
	}
	set_scr_org(cons, cons->scr_org);
	/* �����Ļ����֮������һ�е��ַ� */
	clean_line(cons->scr_org+SCR_BYTES-LINE_BYTES);
}

/*
 * ����̨д���������뻺����fs��ָ���ڡ�
 */
void console_write(char *buff, long count)
{
	if(count <= 0)
		return;

	int index = 0;	/* д�������� */
	int i = 0;		/* ��ʱ����ѭ������ */
	char fs_c;
	while(count--) {
		fs_c = get_fs_byte(&buff[index]);
		switch(fs_c) {
		case '\b':	/* �˸� */
			clean_char(&console);
			break;
		case '\t':	/* ˮƽ�Ʊ�4�ַ����� */
			i = 4 - ((console.cur_pos/2) % 4);/* NOTE: cur_pos/2 */
			for(; i>0; i--) {
				write_char(&console, CLEAR_CHAR);
			}
			break;
		case '\n':	/* ���� */
			/* ��괦�����һ�У�����Ļ�¹� */
			if(console.cur_pos-console.scr_org >= SCR_BYTES-LINE_BYTES) {
				screen_down(&console);
				set_cur_pos(&console, ALIGN_LINE(console.scr_org+SCR_BYTES-LINE_BYTES));
			} else {
				set_cur_pos(&console, ALIGN_LINE(console.cur_pos+LINE_BYTES));
			}
			break;
		case '\r':	/* �س� */
			set_cur_pos(&console, ALIGN_LINE(console.cur_pos));
			break;
		default:	/* д�ַ� */
			write_char(&console, fs_c);
			break;
		}

		index++;
	}
}

/*
 * �ó������ڳ�ʼ��һ������̨��
 */
void _console_init(CONSOLE * cons, unsigned long org,
			unsigned long size, unsigned long curpos)
{
	cons->org		= org;
	cons->size		= size;
	cons->scr_org	= org;
	cons->cur_pos	= curpos;
	set_scr_org(cons, cons->scr_org);
	set_cur_pos(cons, cons->cur_pos);
}

void console_init(void)
{
	/* ȡ�ù��λ�ã����λ����boot�б�������0x90000! ע�Ᵽ��ͬ��! */
	unsigned long pos = (unsigned long)*((unsigned short*)0x90000);
	
	/* 32KB��ע�⣺head.s���Դ��������������Ϊ�޳�32KB */
	_console_init(&console, 0, 32*1024, pos*2);	/* NOTE: pos*2 */
}
