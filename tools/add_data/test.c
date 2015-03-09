/*
 * Small/tools/add_data/test.c
 *
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 *
 * �˳���ʹ���ں�ϵͳ��������ӡһ���ַ�����ʹ�ÿ⺯�������ڲ����ںˡ�
 *
 * ��������Ϊ��_main�������ǡ�main������Ϊmainֻ������C������һ����Ҫ��
 * ��Ŷ��ѣ����Ǹ����������ġ�ʵ����һ�������C���򾭹��������Ӻ�
 * main�����ǳ����������ڣ���GCC���������Ϊ��_start����_start����
 * һЩ��ʼ��������Ȼ�����main����ִ�У���mainһ�����أ�_start�ͻ�
 * ��main�ķ���ֵ��Ϊ�˳������exit���������յ��½��̵Ľ���ִ�С�
 * �������ǲ�������GCC����ʼ����������Ϊ���ǵ��ں˻���֧������GCC��
 * ��Щ��ʼ�����̡�����һ���main��������_main����֮������_main�ǳ�
 * ���������ڣ�����û�����������������������Լ�����exit����return��
 *
 * Ϊ�˲�����GCC�ĳ�ʼ�����̣���_main�������������ڣ����õ�GCC��LD��
 * �������£�
 * 		-- ����:				gcc -c test.c
 *		-- ����:				ld -e _main test.o -o _test
 * 		-- ��С��ִ���ļ��ߴ�:	strip -s -o test _test
 * 		-- ELF�ļ���Ϣ��ȡ:		readelf -a test > readelf_test.txt
 *		-- ɾ���м��ļ�:		rm _test test.o
 *
 * ������������GCC�ĳ�ʼ�����̣����Ŀǰ��ʹ�����ǡ�
 *
 * ��̬���������:
 * 		-- ��������:			gcc -static -o _s_test test.c
 * 		-- ��С��ִ���ļ��ߴ�:	strip -s -o s_test _s_test
 * 		-- ELF�ļ���Ϣ��ȡ:		readelf -a s_test > s_readelf_test.txt
 *		-- ɾ���м��ļ�:		rm _s_test
 *
 * ��̬���������:
 * 		-- ��������:			gcc -o _d_test test.c
 * 		-- ��С��ִ���ļ��ߴ�:	strip -s -o d_test _d_test
 * 		-- ELF�ļ���Ϣ��ȡ:		readelf -a d_test > d_readelf_test.txt
 *		-- ɾ���м��ļ�:		rm _d_test
 */

#include "libs.h"
#include "stdio.h"
#include "string.h"

void _main(void)
{
	char s[] = "\n\tHello world!\n";

	printf(s, strlen(s));

	for(;;)
		;
}
