/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-09 10:14:21
 * @LastEditors: 江小鉴 abyss_er@163.com
 * @LastEditTime: 2024-09-27 10:23:34
 * @FilePath: \jdos\jd_main.c
 * @Description: jd main
 */

#include "jdos.h"

jd_task_t *test_task1, *test_task2, *test_task3;
// 测试任务
void task1()
{
	// while (1)
	{
		jd_printf("task1\r\n");
		// jd_delay(100);
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
	};
}
void task2()
{

	while (1)
	{
		jd_printf("task2\r\n");
		// HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
		jd_delay(500);
		/*
		 test_task1 = jd_task_create(task1, 512, 3);
		 if (test_task1 != JD_NULL)
			jd_task_run(test_task1);*/
	};
}
void task3()
{

	while (1)
	{
		jd_printf("task3\r\n");
		jd_delay(100);
		// HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
	};
}

/**
 * @description: 系统main,系统第一个任务，不可使用jd_task_delete删除，可添加其他任务初始化代码
 * @return {*}
 */
__weak void jd_main(void)
{
	test_task1 = jd_task_create(task1, 512, 3);
	if (test_task1 != JD_NULL)
		jd_timer_start(test_task1, 200, JD_TIMER_LOOP);

	test_task2 = jd_task_create(task2, 512, 1);
	if (test_task1 != JD_NULL)
		jd_task_run(test_task2);

	test_task3 = jd_task_create(task3, 512, 2);
	if (test_task1 != JD_NULL)
		jd_task_run(test_task3);
	while (1)
	{
		// 注意此处调用延时切换任务，如果所有任务都不为就绪状态，程序将死循环，直到有就绪任务才会切换
		// 应该在此处休眠或者其他不重要的工作
		HAL_Delay(100);
	};
}
