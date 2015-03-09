/* * Small/tools/add_data/init.c *  * (C) 2014 Yafei Zheng <e9999e@163.com> */
#include "libs.h"
void init(void){	if(!fork()) {		__asm__("int $0x81");		for(;;) {		}	}	pause();	for(;;) {	}}
void _main(void)
{
	if(!fork())
		init();
	for(;;)
		pause();
}
