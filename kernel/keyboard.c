/*
 * Small/kernel/keyboard.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 此文件包含键盘缓冲区的相关处理程序。
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/
 
#include "types.h"
#include "keyboard.h"
#include "console.h"
#include "string.h"
#include "sys_set.h"
#include "sys_nr.h"

#include "keymap.h"

extern int keyboard_int(void);


KB_QUEUE kb_queue;		/* 键盘缓冲区（队列） */
unsigned char mode = 0;	/* shift, ctrl, alt 标志 */
unsigned char lock = 0;	/* num_lock, caps_lock, scroll_lock 标志 */
unsigned char e0 = 0;	/* e0 标志 */

/*
 * 键盘处理程序。将产生键盘中断时的键盘扫描码转换之后，
 * 放入内核维护的键盘缓冲区，被keyboard_int调用。
 */
void keyboard_hander(void)
{
	unsigned char scan_code = in_b(0x60);
	unsigned short oldfs;
	
	/*  若缓冲区满，则丢弃扫描码，但我们必须从8042读入 */
	if(kb_queue.count < KEY_BUFF_SIZE) {
		/* 转换成相应按键 */
		if(TRUE == cook_scan_code(scan_code, &kb_queue.q_buff[kb_queue.q_tail])) {
			((++kb_queue.q_tail)>=KEY_BUFF_SIZE) ? kb_queue.q_tail=0 : 0;
			(kb_queue.count)++;			

			oldfs = set_fs_kernel();
			console_write(&kb_queue.q_buff[kb_queue.q_head], 1);/* 注意是取地址 */
			set_fs(oldfs);

			((++kb_queue.q_head)>=KEY_BUFF_SIZE) ? kb_queue.q_head=0 : 0;
			(kb_queue.count)--;
		}
	}
}

/*
 * 处理键盘扫描码，将其转换为对应的键。
 * 包括 e0，shift，!shift & !e0 这3种情况。
 */
unsigned int scan_to_key(unsigned char scan_code)
{
	/*
	 * 下述错误已完美解决! 错误原因：ELF文件中data数据段
	 * 页对齐所致，参考 tools/build.c
	 */
	/*
	 * 注意，这里的include。之所以这样做，是因为如果把全局变量放
	 * 在函数体外，则会初始化失败，导致错误! 但目前尚不清楚原因。
	 */
	 /* #include "keymap.h" */

	unsigned int key = 0;

	if(scan_code & 0x80)/* Break or Make ? */
		key |= FLAG_BREAK;

	if(e0)/* e0置位 */
		key |= key_map[2 + KEY_MAP_COLS * (scan_code & 0x7f)];/* 是0x7f,~(0x80)会出错 */
	else if((SHIFT_L_DOWN & mode) || (SHIFT_R_DOWN & mode))/* shift按下 */
		key |= key_map[1 + KEY_MAP_COLS * (scan_code & 0x7f)];
	else/* !shift and !e0 */
		key |= key_map[0 + KEY_MAP_COLS * (scan_code & 0x7f)];

	return key;
}

/*
 * F: 将对应的扫描码处理成对应的键，并设置mode，lock，e0，最后将
 *    相应的键转换为相应的字符或功能字符。
 * 注：由于shift+相应键，有另一种字符相对应。故不应将其分开处理。
 *     此处和Linux-0.11的做法相似，即在产生键盘中断时，由功能键
 *	   控制字符键来进行处理。
 * I: scan_code - 扫描码; cooked - 转换之后的字符指针
 * O: 若是完整的字符，则返回 TRUE
 */
BOOL cook_scan_code(unsigned char scan_code, char *cooked)
{
	unsigned int key = 0;	/* 按键 */
	*cooked = 0;			/* cooked == 0,返回FALSE */

	if(0xe0 == scan_code) {
		e0 = 1;
	} else {
		key = scan_to_key(scan_code);
		e0 = 0;			/* 注意，复位 e0 须在 scan_to_key 被调用之后 */
	
		if(FLAG_BREAK & key) {/* BREAK，只处理shift，ctrl， alt */
			key &= 0x7fff;
			switch(key) {
			case SHIFT_L:
				mode &= (0xff ^ SHIFT_L_DOWN); break;
			case SHIFT_R:
				mode &= (0xff ^ SHIFT_R_DOWN); break;
			case CTRL_L:
				mode &= (0xff ^ CTRL_L_DOWN); break;
			case CTRL_R:
				mode &= (0xff ^ CTRL_R_DOWN); break;
			case ALT_L:
				mode &= (0xff ^ ALT_L_DOWN); break;
			case ALT_R:
				mode &= (0xff ^ ALT_R_DOWN); break;
			default: break;
			}
		} else {	/* MAKE */
			key &= 0x7fff;
			if(key >= FLAG_CONTR_KEY) {/* 控制键 */
				switch(key) {
				case SHIFT_L:
					mode |= SHIFT_L_DOWN; break;
				case SHIFT_R:
					mode |= SHIFT_R_DOWN; break;
				case CTRL_L:
					mode |= CTRL_L_DOWN; break;
				case CTRL_R:
					mode |= CTRL_R_DOWN; break;
				case ALT_L:
					mode |= ALT_L_DOWN; break;
				case ALT_R:
					mode |= ALT_R_DOWN; break;
				case NUM_LOCK:
					(lock & NUM_LOCK_DOWN)
					? (lock &= (0xff ^ NUM_LOCK_DOWN))
					: (lock |= NUM_LOCK_DOWN); break;
				case CAPS_LOCK:
					(lock & CAPS_LOCK_DOWN)
					? (lock &= (0xff ^ CAPS_LOCK_DOWN))
					: (lock |= CAPS_LOCK_DOWN); break;
				case SCROLL_LOCK:
					(lock & SCROLL_LOCK_DOWN)
					? (lock &= (0xff ^ SCROLL_LOCK_DOWN))
					: (lock |= SCROLL_LOCK_DOWN); break;
				case BACKSPACE:				/* 退格 */
					*cooked = 8; break;
				case TAB:					/* 水平制表 */
					*cooked = 9; break;
				case ENTER:					/* 换行 */
					*cooked = 10; break;
				case DELETE:				/* 删除 */
					*cooked = 127; break;
				default: break;
				}
			} else {	/* 小于 FLAG_CONTR_KEY（256） 的普通键 */
				*cooked = key;
				/* 大写锁定，并且是小写字符 */
				if((CAPS_LOCK_DOWN & lock) 
				&& (('a'<=*cooked) && (*cooked<='z'))) {
					*cooked -= 32;
				}
			}
		}
	}

	return ((0==*cooked) ? FALSE : TRUE);
}

void keyboard_init(void)
{
	set_idt(INT_R0, keyboard_int, NR_KEYBOARD_INT);
}
