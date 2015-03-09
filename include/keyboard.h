/*
 * Small/include/keyboard.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "types.h"

/********************************************************************/
/* 定义用于键盘缓冲区循环队列 */

#define KEY_BUFF_SIZE (32)				/* 键盘缓冲区大小 */

typedef struct keyboard_buff_queue {
	int q_head;
	int q_tail;
	int count;
	char q_buff[KEY_BUFF_SIZE];
}KB_QUEUE;

/********************************************************************/
/* 以下宏定义用于按键标志 */

#define FLAG_BREAK		(1<<31)			/* 键松开标志 */

/* mode */
#define SHIFT_L_DOWN	(1<<0)
#define SHIFT_R_DOWN	(1<<1)
#define CTRL_L_DOWN		(1<<2)
#define CTRL_R_DOWN		(1<<3)
#define ALT_L_DOWN		(1<<4)
#define ALT_R_DOWN		(1<<5)

/* lock */
#define NUM_LOCK_DOWN	(1<<0)
#define CAPS_LOCK_DOWN	(1<<1)
#define SCROLL_LOCK_DOWN	(1<<2)

/********************************************************************/
/* 以下宏定义用于 keymap */

#define NR_SCAN_CODES	(0x80)			/* 键盘扫描码个数 */
#define KEY_MAP_COLS	(3)				/* key_map 列数 */

#define FLAG_CONTR_KEY	(0x100)			/* 控制键开始值 */

/* Special keys */
#define ESC			(0x01 + FLAG_CONTR_KEY)	/* Esc		*/
#define TAB			(0x02 + FLAG_CONTR_KEY)	/* Tab		*/
#define ENTER		(0x03 + FLAG_CONTR_KEY)	/* Enter	*/
#define BACKSPACE	(0x04 + FLAG_CONTR_KEY)	/* BackSpace	*/

#define GUI_L		(0x05 + FLAG_CONTR_KEY)	/* L GUI	*/
#define GUI_R		(0x06 + FLAG_CONTR_KEY)	/* R GUI	*/
#define APPS		(0x07 + FLAG_CONTR_KEY)	/* APPS	*/

/* Shift, Ctrl, Alt */
#define SHIFT_L		(0x08 + FLAG_CONTR_KEY)	/* L Shift	*/
#define SHIFT_R		(0x09 + FLAG_CONTR_KEY)	/* R Shift	*/
#define CTRL_L		(0x0A + FLAG_CONTR_KEY)	/* L Ctrl	*/
#define CTRL_R		(0x0B + FLAG_CONTR_KEY)	/* R Ctrl	*/
#define ALT_L		(0x0C + FLAG_CONTR_KEY)	/* L Alt	*/
#define ALT_R		(0x0D + FLAG_CONTR_KEY)	/* R Alt	*/

/* Lock keys */
#define CAPS_LOCK	(0x0E + FLAG_CONTR_KEY)	/* Caps Lock	*/
#define	NUM_LOCK	(0x0F + FLAG_CONTR_KEY)	/* Number Lock	*/
#define SCROLL_LOCK	(0x10 + FLAG_CONTR_KEY)	/* Scroll Lock	*/

/* Function keys */
#define F1		(0x11 + FLAG_CONTR_KEY)	/* F1		*/
#define F2		(0x12 + FLAG_CONTR_KEY)	/* F2		*/
#define F3		(0x13 + FLAG_CONTR_KEY)	/* F3		*/
#define F4		(0x14 + FLAG_CONTR_KEY)	/* F4		*/
#define F5		(0x15 + FLAG_CONTR_KEY)	/* F5		*/
#define F6		(0x16 + FLAG_CONTR_KEY)	/* F6		*/
#define F7		(0x17 + FLAG_CONTR_KEY)	/* F7		*/
#define F8		(0x18 + FLAG_CONTR_KEY)	/* F8		*/
#define F9		(0x19 + FLAG_CONTR_KEY)	/* F9		*/
#define F10		(0x1A + FLAG_CONTR_KEY)	/* F10		*/
#define F11		(0x1B + FLAG_CONTR_KEY)	/* F11		*/
#define F12		(0x1C + FLAG_CONTR_KEY)	/* F12		*/

/* Control Pad */
#define PRINTSCREEN	(0x1D + FLAG_CONTR_KEY)	/* Print Screen	*/
#define PAUSEBREAK	(0x1E + FLAG_CONTR_KEY)	/* Pause/Break	*/
#define INSERT		(0x1F + FLAG_CONTR_KEY)	/* Insert	*/
#define DELETE		(0x20 + FLAG_CONTR_KEY)	/* Delete	*/
#define HOME		(0x21 + FLAG_CONTR_KEY)	/* Home		*/
#define END			(0x22 + FLAG_CONTR_KEY)	/* End		*/
#define PAGEUP		(0x23 + FLAG_CONTR_KEY)	/* Page Up	*/
#define PAGEDOWN	(0x24 + FLAG_CONTR_KEY)	/* Page Down	*/
#define UP			(0x25 + FLAG_CONTR_KEY)	/* Up		*/
#define DOWN		(0x26 + FLAG_CONTR_KEY)	/* Down		*/
#define LEFT		(0x27 + FLAG_CONTR_KEY)	/* Left		*/
#define RIGHT		(0x28 + FLAG_CONTR_KEY)	/* Right	*/

/* ACPI keys */
#define POWER		(0x29 + FLAG_CONTR_KEY)	/* Power	*/
#define SLEEP		(0x2A + FLAG_CONTR_KEY)	/* Sleep	*/
#define WAKE		(0x2B + FLAG_CONTR_KEY)	/* Wake Up	*/

/* Num Pad */
#define PAD_SLASH	(0x2C + FLAG_CONTR_KEY)	/* /		*/
#define PAD_STAR	(0x2D + FLAG_CONTR_KEY)	/* *		*/
#define PAD_MINUS	(0x2E + FLAG_CONTR_KEY)	/* -		*/
#define PAD_PLUS	(0x2F + FLAG_CONTR_KEY)	/* +		*/
#define PAD_ENTER	(0x30 + FLAG_CONTR_KEY)	/* Enter	*/
#define PAD_DOT		(0x31 + FLAG_CONTR_KEY)	/* .		*/
#define PAD_0		(0x32 + FLAG_CONTR_KEY)	/* 0		*/
#define PAD_1		(0x33 + FLAG_CONTR_KEY)	/* 1		*/
#define PAD_2		(0x34 + FLAG_CONTR_KEY)	/* 2		*/
#define PAD_3		(0x35 + FLAG_CONTR_KEY)	/* 3		*/
#define PAD_4		(0x36 + FLAG_CONTR_KEY)	/* 4		*/
#define PAD_5		(0x37 + FLAG_CONTR_KEY)	/* 5		*/
#define PAD_6		(0x38 + FLAG_CONTR_KEY)	/* 6		*/
#define PAD_7		(0x39 + FLAG_CONTR_KEY)	/* 7		*/
#define PAD_8		(0x3A + FLAG_CONTR_KEY)	/* 8		*/
#define PAD_9		(0x3B + FLAG_CONTR_KEY)	/* 9		*/
#define PAD_UP		PAD_8			/* Up		*/
#define PAD_DOWN	PAD_2			/* Down		*/
#define PAD_LEFT	PAD_4			/* Left		*/
#define PAD_RIGHT	PAD_6			/* Right	*/
#define PAD_HOME	PAD_7			/* Home		*/
#define PAD_END		PAD_1			/* End		*/
#define PAD_PAGEUP	PAD_9			/* Page Up	*/
#define PAD_PAGEDOWN	PAD_3			/* Page Down	*/
#define PAD_INS		PAD_0			/* Ins		*/
#define PAD_MID		PAD_5			/* Middle key	*/
#define PAD_DEL		PAD_DOT			/* Del		*/

/********************************************************************/
extern void keyboard_hander(void);
extern unsigned int scan_to_key(unsigned char scan_code);
extern BOOL cook_scan_code(unsigned char scan_code, char *cooked);
extern void keyboard_init(void);

/********************************************************************/
#endif	/* _KEYBOARD_H_ */
