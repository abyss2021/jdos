#include "stdio.h"
#include "stdlib.h"


//系统默认堆栈大小
#define JDOS_MAIN_STACK_SIZE 512 
//系统时钟，单位ms
unsigned int jdos_lck = 0;

/*枚举函数返回状态*/
enum jdos_return_status{
	JDOS_NULL=0,    //NULL
    JDOS_OK,        //成功
	JDOS_ERR,		//失败
};

/*枚举任务状态*/
enum jdos_task_status{
	JDOS_TASK_READY=0, 	//任务就绪状态
	JDOS_TASK_RUNNING, 	//任务运行状态
    JDOS_TASK_DELAY, 	//任务延时状态
	JDOS_TASK_PAUSE, 	//任务暂停状态
};

/*定义任务链表*/
struct jdos_task{
	struct jdos_task *previous;						//指向链表上一个节点
int (*task_entry)();								//指向任务入口函数
	enum jdos_task_status jdos_task_statu;          //当前任务状态
	unsigned int stack_size;						//当前任务的堆栈大小
	int *stack_sp;									//当前任务的堆栈指针
	struct jdos_task *next;							//指向链表下一个节点
};
struct jdos_task *jdos_task_sp_frist= NULL;	//创建系统链表指针，用于保存链表第一个任务位置
struct jdos_task *jdos_task_sp = NULL;	    //创建一个任务链表指针

/*申请任务空间
* jdos_task_sp：链表指针
* stack_size：堆栈大小
* return：JDOS_OK或JDOS_ERR
*/
struct jdos_task * jdos_request_space(unsigned int stack_size)
{
    struct jdos_task *jdos_task;
	jdos_task = (struct jdos_task *)malloc(sizeof(struct jdos_task)); 	//分配空间
	if(jdos_task==NULL)return JDOS_NULL;								//判断分配空间是否成功
	
	jdos_task->stack_sp = (int *)malloc(stack_size);					//申请堆栈空间
	if(jdos_task==NULL)return JDOS_NULL;							    //判断分配空间是否成功
	return jdos_task;
}


/*创建任务
* task_entry:函数入口
* stack_size：任务栈大小
* return：返回当前任务节点指针
*/
struct jdos_task *jdos_create_task(void (*task_entry)(),unsigned int stack_size)
{
	struct jdos_task *jdos_new_task = NULL;	//创建一个任务链表指针
    jdos_new_task = jdos_request_space(JDOS_MAIN_STACK_SIZE);	
	if(jdos_new_task==JDOS_NULL)return JDOS_NULL;	    //申请空间
	jdos_new_task->previous = jdos_task_sp;																//当前节点的previous指向上一个节点
	jdos_new_task->next = jdos_task_sp_frist;															//当前节点的next指向第一个节点
	jdos_task_sp->next = jdos_new_task;																	//上一节点的next指向当前节点
	jdos_task_sp_frist->previous = jdos_new_task;													    //第一个节点的previous指向当前节点
	jdos_task_sp = jdos_new_task;																		//链表指针移动到当前节点
	
	jdos_task_sp->task_entry = task_entry;																//任务入口
	jdos_task_sp->jdos_task_statu = JDOS_TASK_PAUSE;                                                    //创建任务，状态为暂停状态，等待启动
	jdos_task_sp->stack_size = stack_size;																//记录当前任务堆栈大小
	
	return jdos_task_sp;																				//返回当前任务节点
}

/*任务开执行
*	jdos_task:任务节点指针
* return：返回JDOS_OK
*/
int jdos_run_task(struct jdos_task *jdos_task)
{
	jdos_task->jdos_task_statu = JDOS_TASK_READY;														//将任务更改为就绪状态
	return JDOS_OK;
}

/*删除任务
* jdos_task:任务节点指针
* return：返回JDOS_OK或JDOS_ERR
*/
int jdos_delete_task(struct jdos_task *jdos_task)
{
	struct jdos_task *jdos_task_previous = jdos_task->previous;
	struct jdos_task *jdos_task_next = jdos_task->next;
	
	if(jdos_task==jdos_task_sp_frist)return JDOS_ERR;	    //判断是否为系统第一个节点，系统第一个节点不可删除
	
	jdos_task_previous->next = jdos_task_next;				//上一个节点的next指向下一个节点
	jdos_task_next->previous = jdos_task_previous;			//下一个节点的previous指向上一个节点
	free(jdos_task->stack_sp);								//释放当前节点的堆栈内存
	free(jdos_task);		                                //释放当前节点内存
    jdos_task_sp = 	jdos_task_next;															
	return JDOS_OK;
}

int main();
int jdos_main();
/*jdos初始化*/
int jdos_init()
{				
    jdos_task_sp_frist = jdos_request_space(JDOS_MAIN_STACK_SIZE);	
	if(jdos_task_sp_frist==JDOS_NULL)return JDOS_ERR;	                //申请空间
    jdos_task_sp_frist->previous = jdos_task_sp_frist;					//节点与自身相连
	jdos_task_sp_frist->next = jdos_task_sp_frist;						//节点与自身相连
	
	jdos_task_sp_frist->task_entry = jdos_main;							//任务入口

	jdos_task_sp_frist->jdos_task_statu = JDOS_TASK_READY;				//就绪状态，直接运行								
	jdos_task_sp_frist->stack_size = JDOS_MAIN_STACK_SIZE;				//堆栈大小
	jdos_task_sp = jdos_task_sp_frist;									//更新节点指针
	return JDOS_OK;
}


//测试任务
 void task1()
 {
    printf("1 hello\r\n");
    while(1)
    {
    };
 }
 void task2()
 {
    printf("2 hello\r\n");
    while(1)
    {
    };
 }
 void task3()
 {
    printf("3 hello\r\n");
    while(1)
    {
    };
 }


 /*main函数*/
int main(void)
{
    jdos_init();
    jdos_task_sp->task_entry();
	while(1)
	{
	};
}

/*系统main*/
int jdos_main()
{
    printf("jdos hello\r\n");
    struct jdos_task *test_task1 = jdos_create_task(task1,512);
    struct jdos_task *test_task2 = jdos_create_task(task2,512);
    struct jdos_task *test_task3 = jdos_create_task(task3,512);

    jdos_delete_task(test_task1);
    jdos_delete_task(test_task2);
    jdos_delete_task(test_task3);
    while(1)
	{
	};
}
