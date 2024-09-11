#include "stdio.h"
#include <stdlib.h>
#include "stm32f1xx_hal.h"

/*宏定义函数返回状态*/
#define JD_NULL 0
#define JD_OK 1
#define JD_ERR 2
/*系统默认堆栈大小*/
#define JD_DEFAULT_STACK_SIZE 512 
/*系统时钟，单位ms*/
unsigned long jd_lck = 0;
/*第一次进入线程*/
extern void jd_asm_task_first_switch(unsigned long*,void*);
/*切换任务节点，悬挂PendSV异常，PendSV中进行上下文切换*/
extern void jd_asm_pendsv_putup(void);
/*PendSV切换上下文*/
extern void jd_asm_pendsv_handler(void);
/*systick初始化*/
extern void jd_asm_systick_init(void);
/*除能 NMI 和硬 fault 之外的所有异常*/
extern void jd_asm_cps_disable(void);
/*使能中断*/
extern void jd_asm_cps_enable(void);


/*jdos main*/
void jd_main(void);


/*枚举任务状态*/
enum jd_task_status{
	JD_TASK_READY=0, 	//任务就绪状态
	JD_TASK_RUNNING, 	//任务运行状态
    JD_TASK_DELAY, 	    //任务延时状态
	JD_TASK_PAUSE, 	    //任务暂停状态
};

/*定义所有寄存器，根据入栈规则有先后顺序*/
struct all_register
{
    //手动入栈
    unsigned long r4;
    unsigned long r5;
    unsigned long r6;
    unsigned long r7;
    unsigned long r8;
    unsigned long r9;
    unsigned long r10;
    unsigned long r11;

    //自动入栈
    unsigned long r0;
    unsigned long r1;
    unsigned long r2;
    unsigned long r3;
    unsigned long r12;
    unsigned long lr;
    unsigned long pc;
    unsigned long xpsr;
};


/*定义任务链表*/
struct jd_task{
	struct jd_task *previous;						//指向上一个节点
  	void (*task_entry)();							//指向任务入口函数
	enum jd_task_status jd_task_status;             //当前任务状态
	unsigned long stack_size;						//堆栈大小
	unsigned long stack_sp;							//堆栈指针
  	unsigned long stack_origin_addr;               	//堆栈起始地址
	struct jd_task *next;							//指向下一个节点
};
struct jd_task *jd_task_sp_frist= NULL;			//创建系统链表指针，用于保存链表第一个任务位置
struct jd_task *jd_task_sp = NULL;				//创建一个任务链表指针
struct all_register *stack_register =  NULL;	//创建一个所有寄存器指针
unsigned long *jd_task_stack_sp = NULL;			//创建当前任务堆栈指针的地址
unsigned long *jd_task_next_stack_sp = NULL;	//创建下一个任务堆栈指针的地址

/*申请任务空间
* jd_task_sp：链表指针
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
struct jd_task *jd_create_task(void (*task_entry)(),unsigned int stack_size)
{
	struct jd_task *jd_new_task = NULL;	//创建一个任务链表指针
  	jd_new_task = jd_request_space(JD_DEFAULT_STACK_SIZE);	
	if(jd_new_task==JD_NULL)return JD_NULL;	    //申请空间

	if(jd_task_sp!=NULL)
	{
		jd_new_task->previous = jd_task_sp; //新节点指向当前节点
		jd_new_task->next = jd_task_sp->next;//新节点指向下一个节点
		jd_task_sp->next->previous = jd_new_task; //下一个节点指向当前节点
		jd_task_sp->next = jd_new_task; //当前节点指向新节点

	}
	
	jd_new_task->task_entry = task_entry; //任务入口
	jd_new_task->jd_task_status = JD_TASK_PAUSE; //创建任务，状态为暂停状态，等待启动
	jd_new_task->stack_size = stack_size; //记录当前任务堆栈大小
	
	jd_new_task->stack_sp = jd_new_task->stack_origin_addr+JD_DEFAULT_STACK_SIZE-sizeof(struct all_register)-4; //腾出寄存器的空间
	struct all_register *stack_register = (struct all_register *)jd_new_task->stack_sp; //将指针转换成寄存器指针

	//将任务运行数据搬移到内存中
	stack_register->r0 = 0;
	stack_register->r1 = 0;
	stack_register->r2 = 0;
	stack_register->r3 = 0;
	stack_register->r12 = 0;
	stack_register->lr = (unsigned long)jd_new_task->task_entry;
	stack_register->pc = (unsigned long)jd_new_task->task_entry;
	stack_register->xpsr = 0x01000000L; //由于Armv7-M只支持执行Thumb指令，因此必须始终将其值保持为1
	
	return jd_new_task; //返回当前任务节点
}

/*任务开执行
*	jd_task:任务节点指针
* return：返回JD_OK
*/
int jd_run_task(struct jd_task *jd_task)
{
	jd_task->jd_task_status = JD_TASK_READY; //将任务更改为就绪状态
	return JD_OK;
}

/*删除任务
* jd_task:任务节点指针
* return：返回JD_OK或JD_ERR
*/
int jd_delete_task(struct jd_task *jd_task)
{
	struct jd_task *jd_task_previous = jd_task->previous;
	struct jd_task *jd_task_next = jd_task->next;
	
	if(jd_task==jd_task_sp_frist)return JD_ERR; //判断是否为系统第一个节点，系统第一个节点不可删除
	
	jd_task_previous->next = jd_task_next; //上一个节点的next指向下一个节点
	jd_task_next->previous = jd_task_previous; //下一个节点的previous指向上一个节点
	free((unsigned long *)jd_task->stack_sp); //释放当前节点的堆栈内存
	free(jd_task); //释放当前节点内存														
	return JD_OK;
}


/*当前任务切换为下一个任务*/
void jd_task_switch()
{
	jd_asm_cps_disable();
	
	jd_task_stack_sp = &jd_task_sp->stack_sp; //更新当前任务全局堆栈指针变量
	jd_task_sp = jd_task_sp->next; //移动节点
	jd_task_next_stack_sp = &jd_task_sp->stack_sp; //更新下一个任务全局堆栈指针变量
	jd_asm_pendsv_putup(); //挂起PendSV异常
	
	jd_asm_cps_enable();
}
/*内核第一次运行空闲任务*/
void jd_task_first_switch()
{
	jd_asm_task_first_switch(&jd_task_sp->stack_sp,jd_main);
}

/*PendSV处理上下文切换*/
void PendSV_Handler(void)
{
	jd_asm_pendsv_handler(); //切换上下文
}

/*hal库已自动使能systick，以下为hal库systick中断回调函数*/
void HAL_IncTick(void)
{
	uwTick += uwTickFreq;  //系统自带不可删除,否则hal_delay等hal库函数不可用
	
	jd_lck++; //jd_lck++
	jd_task_switch(); //jd_task_switch
}

/*jd初始化*/
int jd_init(void)
{				
	struct jd_task *jd_new_task = NULL;	//创建一个任务链表指针
	jd_new_task = jd_create_task(jd_main,JD_DEFAULT_STACK_SIZE);
	while(jd_new_task==NULL); //空闲任务不能创建则死循环
	
	jd_new_task->previous = jd_new_task; //第一个任务，指向自己
	jd_new_task->next = jd_new_task; //第一个任务，指向自己
	
	jd_new_task->jd_task_status = JD_TASK_READY; //任务就绪

	jd_new_task->stack_sp = jd_new_task->stack_origin_addr+JD_DEFAULT_STACK_SIZE-4; //栈顶
	
	jd_task_sp = jd_new_task; //指针移动到当前节点
	jd_task_sp_frist = jd_task_sp; //记录第一个节点
	
	//jd_asm_systick_init(); //启动systick,hal库已自动使能systick
	
	jd_task_first_switch(); //进入空闲线程
	return JD_OK;
}


//测试任务
 void task1()
 {
    //printf("1 hello\r\n");
    while(1)
    {
			HAL_Delay(80);
			HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
			HAL_Delay(20);
    };
 }
 void task2()
 {
    //printf("2 hello\r\n");
    while(1)
    {
			HAL_Delay(100);
			HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
			HAL_Delay(20);
    };
 }
 void task3()
 {
    //printf("3 hello\r\n");
    while(1)
    {
			HAL_Delay(50);
			HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
			HAL_Delay(50);
    };
 }


/*系统main*/
void jd_main(void)
{
    //printf("jd hello\r\n");
    struct jd_task *test_task1 = jd_create_task(task1,512);
    struct jd_task *test_task2 = jd_create_task(task2,512);
    struct jd_task *test_task3 = jd_create_task(task3,512);
    while(1)
	{
		//jd_task_switch();
		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
		HAL_Delay(500);
	};
}



