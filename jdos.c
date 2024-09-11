#include "jdos.h"

struct jd_task *jd_task_sp_frist= NULL;			//创建系统节点指针，用于保存节点第一个任务位置
struct jd_task *jd_task_sp = NULL;				//创建一个任务节点指针
struct all_register *stack_register =  NULL;	//创建一个所有寄存器指针
unsigned long *jd_task_stack_sp = NULL;			//创建当前任务堆栈指针的地址
unsigned long *jd_task_next_stack_sp = NULL;	//创建下一个任务堆栈指针的地址

/*jdos延时，让出CPU使用权
* ms:延时时间，单位ms
*/
void jd_delay(unsigned long ms)
{
	if(ms==0)return;
	jd_task_sp->status = JD_TASK_DELAY;
	jd_task_sp->timeout = jd_time+ms; //将延时时间写入节点
	jd_task_switch(); //切换线程，让出CPU，等延时后调度
}


/*申请任务空间
* jd_task_sp：节点指针
* stack_size：堆栈大小
* return：JD_OK或JD_ERR
*/
struct jd_task * jd_request_space(unsigned int stack_size)
{
	struct jd_task *jd_task;
	jd_task = (struct jd_task *)malloc(sizeof(struct jd_task)); //分配空间
	if(jd_task==NULL)return JD_NULL; //判断分配空间是否成功
	
	jd_task->stack_sp = (unsigned long)malloc(stack_size); //申请堆栈空间
	if(jd_task==NULL)return JD_NULL; //判断分配空间是否成功
	jd_task->stack_origin_addr = jd_task->stack_sp; //记录栈顶指针 
	return jd_task;
}


/*创建任务
* task_entry:函数入口
* stack_size：任务栈大小
* return：返回当前任务节点指针
*/
struct jd_task *jd_task_create(void (*task_entry)(),unsigned int stack_size)
{
	struct jd_task *jd_new_task = NULL;	//创建一个任务节点指针
  	jd_new_task = jd_request_space(JD_DEFAULT_STACK_SIZE);	
	if(jd_new_task==JD_NULL)return JD_NULL;	    //申请空间

	if(jd_task_sp!=NULL)
	{
		jd_new_task->previous = jd_task_sp; //新节点指向当前节点
		jd_new_task->next = jd_task_sp->next;//新节点指向下一个节点
		jd_task_sp->next->previous = jd_new_task; //下一个节点指向当前节点
		jd_task_sp->next = jd_new_task; //当前节点指向新节点

	}
	jd_new_task->timeout = 0; //没有延时时间
	jd_new_task->entry = task_entry; //任务入口
	jd_new_task->status = JD_TASK_PAUSE; //创建任务，状态为暂停状态，等待启动
	jd_new_task->stack_size = stack_size; //记录当前任务堆栈大小
	
	jd_new_task->stack_sp = jd_new_task->stack_origin_addr+JD_DEFAULT_STACK_SIZE-sizeof(struct all_register)-4; //腾出寄存器的空间
	struct all_register *stack_register = (struct all_register *)jd_new_task->stack_sp; //将指针转换成寄存器指针

	//将任务运行数据搬移到内存中
	stack_register->r0 = 0;
	stack_register->r1 = 0;
	stack_register->r2 = 0;
	stack_register->r3 = 0;
	stack_register->r12 = 0;
	stack_register->lr = (unsigned long)jd_new_task->entry;
	stack_register->pc = (unsigned long)jd_new_task->entry;
	stack_register->xpsr = 0x01000000L; //由于Armv7-M只支持执行Thumb指令，因此必须始终将其值保持为1
	
	return jd_new_task; //返回当前任务节点
}

/*删除任务
* jd_task:任务节点指针
* return：返回JD_OK或JD_ERR
*/
int jd_task_delete(struct jd_task *jd_task)
{
	if(jd_task==JD_NULL)return JD_ERR;
	struct jd_task *jd_task_previous = jd_task->previous;
	struct jd_task *jd_task_next = jd_task->next;
	
	if(jd_task==jd_task_sp_frist)return JD_ERR; //判断是否为系统第一个节点，系统第一个节点不可删除
	
	jd_task_previous->next = jd_task_next; //上一个节点的next指向下一个节点
	jd_task_next->previous = jd_task_previous; //下一个节点的previous指向上一个节点
	free((unsigned long *)jd_task->stack_sp); //释放当前节点的堆栈内存
	free(jd_task); //释放当前节点内存														
	return JD_OK;
}

/*任务就绪
*	jd_task:任务节点指针
* return：返回JD_OK或JD_ERR
*/
int jd_task_run(struct jd_task *jd_task)
{
	if(jd_task==JD_NULL)return JD_ERR;
	jd_task->status = JD_TASK_READY; //将任务更改为就绪状态
	return JD_OK;
}


/*任务暂停
*	jd_task:任务节点指针
* return：返回JD_OK或JD_ERR
*/
int jd_task_pause(struct jd_task *jd_task)
{
	if(jd_task==JD_NULL)return JD_ERR;
	jd_task->status = JD_TASK_PAUSE; //将任务更改为就绪状态
	return JD_OK;
}


/*当前任务切换为下一个任务*/
void jd_task_switch(void)
{
	jd_asm_cps_disable();

	static struct jd_task *jd_task_temp;
	jd_task_temp = jd_task_sp;
	//遍历任务，选择就绪的任务
	while (1)
	{
		jd_task_temp = jd_task_temp->next;
		if(jd_task_temp->status==JD_TASK_READY)break;
	}

	jd_task_stack_sp = &jd_task_sp->stack_sp; //更新当前任务全局堆栈指针变量
	jd_task_sp = jd_task_temp; //移动节点
	jd_task_next_stack_sp = &jd_task_sp->stack_sp; //更新下一个任务全局堆栈指针变量
	jd_asm_pendsv_putup(); //挂起PendSV异常
	
	jd_asm_cps_enable();
}
/*内核第一次运行空闲任务*/
void jd_task_first_switch(void)
{
	jd_asm_task_first_switch(&jd_task_sp->stack_sp,jd_main);
}

/*PendSV处理上下文切换*/
/*void PendSV_Handler(void)
{
	jd_asm_pendsv_handler(); //切换上下文
}*/

/*hal库已自动使能systick，以下为hal库systick异常回调函数*/
void HAL_IncTick(void)
{
	uwTick += uwTickFreq;  //系统自带不可删除,否则hal_delay等hal库函数不可用
	
	jd_time++; //jd_lck++
	//扫描所有任务，将延时完成的任务更改为就绪状态，当前任务改为就绪状态，下次直接执行
	jd_task_sp->status = JD_TASK_READY;
	static struct jd_task *jd_task_temp;
	jd_task_temp = jd_task_sp->next;
	while(jd_task_sp!=jd_task_temp)
	{
		if(jd_task_temp->timeout==jd_time)
			jd_task_temp->status = JD_TASK_READY;
		jd_task_temp = jd_task_temp->next;
	}

	jd_task_switch(); //jd_task_switch
}

/*jd初始化*/
int jd_init(void)
{				
	struct jd_task *jd_new_task = NULL;	//创建一个任务节点指针
	jd_new_task = jd_task_create(jd_main,JD_DEFAULT_STACK_SIZE);
	while(jd_new_task==NULL); //空闲任务不能创建则死循环
	
	jd_new_task->previous = jd_new_task; //第一个任务，指向自己
	jd_new_task->next = jd_new_task; //第一个任务，指向自己
	
	jd_new_task->status = JD_TASK_READY; //任务就绪

	jd_new_task->stack_sp = jd_new_task->stack_origin_addr+JD_DEFAULT_STACK_SIZE-4; //栈顶
	
	jd_task_sp = jd_new_task; //指针移动到当前节点
	jd_task_sp_frist = jd_task_sp; //记录第一个节点
	
	//jd_asm_systick_init(); //启动systick,hal库已自动使能systick
	
	jd_task_first_switch(); //进入空闲任务
	return JD_OK;
}


//测试任务
 void task1()
 {
    //printf("1 hello\r\n");
    while(1)
    {
			jd_delay(100);
			HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
    };
 }
 void task2()
 {
    //printf("2 hello\r\n");
    while(1)
    {
			jd_delay(150);
			HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
    };
 }
 void task3()
 {
    //printf("3 hello\r\n");
    while(1)
    {
			jd_delay(380);
			HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
    };
 }


/*系统main,系统第一个任务，不可删除，可添加其他任务初始化代码*/
__weak void jd_main(void)
{
    //printf("jd hello\r\n");
    struct jd_task *test_task1 = jd_task_create(task1,512);
	if(test_task1!=JD_NULL)jd_task_run(test_task1);

    struct jd_task *test_task2 = jd_task_create(task2,512);
	if(test_task1!=JD_NULL)jd_task_run(test_task2);

    struct jd_task *test_task3 = jd_task_create(task3,512);
	if(test_task1!=JD_NULL)jd_task_run(test_task3);
    while(1)
	{
		//jd_task_switch();
		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
		HAL_Delay(500);
	};
}



