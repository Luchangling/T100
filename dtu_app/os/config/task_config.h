#ifndef __TASK_CONFIG_H__
#define __TASK_CONFIG_H__

/*ϵͳ����������Ϣ*/

/*Start Task*/
//�����������ȼ�
#define START_TASK_PRIO      			10 //��ʼ��������ȼ�����Ϊ���
//���������ջ��С
#define START_STK_SIZE  				1024/4


/*UART task*/
#define UART_TASK_PRIO       		 	6
//���������ջ��С
#define UART_STK_SIZE  				   64

/*MAIN Task*/
#define MAIN_TASK_PRIO       			4 
//���������ջ��С
#define MAIN_STK_SIZE  					128


/*NET Task*/
#define NET_TASK_PRIO       			3 
//���������ջ��С
#define NET_STK_SIZE  					1024

/*AT TASK*/
#define AT_TASK_PRIO       			    2 
//���������ջ��С
#define AT_STK_SIZE  					1024/4


#endif

