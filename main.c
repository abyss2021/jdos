#include "stm32f10x.h"

//枚举任务状态
enum jdos_task_status{
	JDOS_TASK_READY=0, //任务就绪状态
	JDOS_TASK_RUNNING, //任务运行状态
	JDOS_TASK_DELAY, //任务延时状态
};

//定义任务链表
struct jdos_task{
	struct jdos_task *previous;//指向链表上一个节点
	void (*task_entry);//指向任务入口函数
	enum jdos_task_status jdos_task_statu;//当前任务状态
	unsigned int jdos_task_pid;//当前任务的唯一编号
	struct jdos_task *next;//指向链表下一个节点
};

/*创建任务线程
* task_entry:函数入口
* stack_size：任务栈大小
*/
int jdos_create_task(void (*task_entry),unsigned int stack_size)
{
	
}

//任务开始运行
int jdos_run_task()
{
	
}

//删除任务线程
int jdos_delete_task(void (*task_entry))
{
	
}


//测试任务
 void task1()
 {
	 while(1)
	 {
	 };
 }
 void task2()
 {
	 while(1)
	 {
	 };
 }
 void task3()
 {
	 while(1)
	 {
	 };
 }

 //主函数
int main(void)
{
	while(1)
	{
	};
}