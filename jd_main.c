/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-09 10:14:21
 * @LastEditors: 江小鉴 abyss_er@163.com
 * @LastEditTime: 2024-11-13 22:18:46
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
#ifdef JD_PRINTF_ENABLE
		// jd_printf("task1\r\n");
#endif
		// jd_delay(100);
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
	};
}
void task2()
{

	while (1)
	{
		// jd_printf("task2\r\n");
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
		HAL_Delay(20);
		jd_delay(80);

	};
}
void task3()
{
	while (1)
	{
#ifdef JD_PRINTF_ENABLE
		// jd_printf("task3\r\n");
#endif
		jd_mem_used_get();
		jd_uint32_t *test_sp1 = jd_malloc(1024*20);
		jd_mem_used_get();
		jd_uint32_t *test_sp2 = jd_malloc(1024*10);
		jd_mem_used_get();
		jd_delay(320);
		jd_free(test_sp1);
		jd_mem_used_get();
		jd_free(test_sp2);
		jd_mem_used_get();
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
	};
}

/**
 * @description: 系统main,系统第一个任务，不可使用jd_task_delete删除，可添加其他任务初始化代码
 * @return {*}
 */
__weak void jd_main(void)
{
	test_task1 = jd_task_create(task1, 512, 3);
#ifdef JD_TIMER_ENABLE
	if (test_task1 != JD_NULL)
		jd_timer_start(test_task1, 380, JD_TIMER_LOOP);
#endif

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
		jd_asm_power_sleep();
	};
}
