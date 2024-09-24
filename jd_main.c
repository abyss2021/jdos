#include "jdos.h"

jd_task_t *test_task1, *test_task2, *test_task3;
// 测试任务
void task1()
{
	// printf("1 hello\r\n");
		int a=1;
	int b =2;
	int c;
	// while (1)
	{
		jd_delay(100);
		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
	};
}
void task2()
{
	// printf("2 hello\r\n");
	int a=1;
	int b =2;
	int c;
	while (1)
	{

		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
		jd_delay(500);
		
		
		// test_task1 = jd_task_create(task1, 512, 3);
		// if (test_task1 != JD_NULL)
		// 	jd_task_run(test_task1);
	};
}
void task3()
{
	// printf("3 hello\r\n");
		int a=1;
	int b =2;
	int c;
	while (1)
	{
		jd_delay(100);
		//HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
	};
}

//jd_uint8_t mem_set[4096];

/*系统main,系统第一个任务，不可使用jd_task_delete删除，可添加其他任务初始化代码*/
__weak void jd_main(void)
{
	// printf("jd hello\r\n");
	test_task1 = jd_task_create(task1, 512, 3);
	if (test_task1 != JD_NULL)
		jd_task_run(test_task1);

	test_task2 = jd_task_create(task2, 512, 1);
	if (test_task1 != JD_NULL)
		jd_task_run(test_task2);

	test_task3 = jd_task_create(task3, 512, 2);
	if (test_task1 != JD_NULL)
		jd_task_run(test_task3);
	while (1)
	{
		//HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);

		// 注意此处调用延时切换任务，如果所有任务都不为就绪状态，程序将死循环，直到有就绪任务才会切换
		// 应该在此处休眠或者其他不重要的工作
		HAL_Delay(100);
	};
}
