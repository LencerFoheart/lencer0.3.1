/*
 * Small/tools/complement_to_true_value.c
 *
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 *
 * ���ļ����ڽ�����ת��Ϊ��ֵ��������ԡ�
 */

#include <stdio.h>

int main(void)
{
	long val;

	while(1) {
		printf("Please input a complement number(input 0 to exit):\n\n");
		scanf("%x", &val);
		if(val == 0)
			break;
		else if(val > 0)
			printf("\nThe true value is: [10] %d, [16] %x.\n\n", val, val);
		else
			printf("\nThe true value is: [10] %d, [16] -%x.\n\n", val, 0-val);
	}

	return 0;
}
