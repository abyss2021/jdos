#include "jdos.h"

struct jd_node_list *jd_task_list_readying;	 // 创建就绪任务链表
struct jd_node_list *jd_task_list_delaying;	 // 创建延时任务链表
struct jd_task *jd_task_runing;				 // 创建当前任务指针
unsigned long *jd_task_stack_sp = NULL;		 // 创建当前任务堆栈指针的地址
unsigned long *jd_task_next_stack_sp = NULL; // 创建下一个任务堆栈指针的地址

struct jd_task *jd_task_frist = NULL; // 创建一个系统空闲任务

/*新节点插入链表中
 * node_previous:想要插入的链表节点处的上一个节点
 * node:想要插入的节点,为JD_NULL表示连接前后两个节点
 * node_next：想要插入的链表节点处的下一个节点
 */
int jd_node_insert(struct jd_node_list *node_previous, struct jd_node_list *node, struct jd_node_list *node_next)
{
	// 传入节点无效，无法插入
	if (node_previous == JD_NULL && node_next == JD_NULL)
	{
		return JD_ERR;
	}
	// 连接前后两个节点
	else if (node == JD_NULL)
	{
		// 前一个节点为空
		if (node_previous == JD_NULL)
		{
			node_next->previous = JD_NULL;
		}
		// 后一个节点为空
		else if (node_next == JD_NULL)
		{
			node_previous->next = JD_NULL;
		}
		// 连接两个节点
		else
		{
			node_next->previous = node_next;
			node_previous->next = node_next;
		}
	}
	// 当前插入节点为表头
	else if (node_previous == JD_NULL)
	{
		node->next = node_next;		// 新节点指向下一个节点
		node->previous = JD_NULL;	// 新节点指向上一个JD_NULL
		node_next->previous = node; // 下一个节点指向新节点
	}
	// 当前插入节点为表尾
	else if (node_next == JD_NULL)
	{
		node_previous->next = node;		// 上一个节点指向新节点
		node->previous = node_previous; // 新节点指向上一个节点
		node->next = JD_NULL;			// 新节点指向JD_NULL
	}
	// 当前插入节点为表中
	else
	{
		node->next = node_next;			// 新节点指向下一个节点
		node->previous = node_previous; // 新节点指向上一个节点
		node_previous->next = node;		// 上一个节点指向新节点
		node_next->previous = node;		// 下一个节点指向新节点
	}
	return JD_OK;
}

/*jdos延时，让出CPU使用权
 * ms:延时时间，单位ms
 */
void jd_delay(unsigned long ms)
{
	if (ms == 0)
		return;
	jd_task_runing->status = JD_DELAY;
	jd_task_runing->timeout = jd_time + ms; // 将延时时间写入节点

	struct jd_task *jd_task_temp;
	struct jd_node_list *node_temp;

	// 删除就绪链表中的节点
	// 判断表头
	if (jd_task_runing->node == jd_task_list_readying)
	{
		jd_task_list_readying = jd_task_list_readying->next; // 表头移动
		jd_task_list_readying->previous = JD_NULL;
	}
	else
	{
		// 删除当前节点
		jd_node_insert(jd_task_runing->node->previous, JD_NULL, jd_task_runing->node->next);
	}

	node_temp = jd_task_list_delaying;
	// 延时链表没有正在延时的任务
	if (jd_task_list_delaying->next == JD_NULL && jd_task_list_delaying->previous == JD_NULL)
	{
		jd_task_list_delaying = jd_task_runing->node;
	}
	// 有延时任务
	else
	{
		// 遍历链表
		while (1)
		{
			jd_task_temp = container_of(&node_temp, struct jd_task, node);
			// 如果延时小或相同 则插入在当前节点前
			if (jd_task_runing->priority <= jd_task_temp->priority)
			{
				// 判断为表头
				if (node_temp->previous == JD_NULL)
				{
					jd_task_list_readying = jd_task_runing->node; // 切换表头
				}
				jd_node_insert(node_temp->previous, jd_task_temp->node, node_temp);
				break;
			}
			// 判断表尾
			if (node_temp->next == JD_NULL)
			{
				// 将任务插入到表尾
				jd_node_insert(node_temp, jd_task_temp->node, node_temp->next);
				break;
			}
			// 临时节点切换为下一个节点
			node_temp = node_temp->next;
		}
	}
	jd_task_switch(); // 切换线程，让出CPU，等延时后调度
}

/*申请任务空间
 * jd_task_sp：节点指针
 * stack_size：堆栈大小
 * return：JD_OK或JD_ERR
 */
struct jd_task *jd_request_space(unsigned int stack_size)
{
	struct jd_task *jd_task;
	jd_task = (struct jd_task *)malloc(sizeof(struct jd_task)); // 分配空间
	if (jd_task == NULL)
		return JD_NULL; // 判断分配空间是否成功

	jd_task->stack_sp = (unsigned long)malloc(stack_size); // 申请堆栈空间
	if (jd_task == NULL)
		return JD_NULL;								// 判断分配空间是否成功
	jd_task->stack_origin_addr = jd_task->stack_sp; // 记录栈顶指针

	jd_task->node = (struct jd_node_list *)malloc(sizeof(struct jd_node_list)); // 申请节点空间
	jd_task->node->next = JD_NULL;												// 初始化节点指针
	jd_task->node->previous = JD_NULL;											// 初始化节点指针

	return jd_task;
}

/*创建任务
 * task_entry:函数入口
 * stack_size：任务栈大小
 * return：返回当前任务节点指针
 */
struct jd_task *jd_task_create(void (*task_entry)(), unsigned int stack_size, char priority)
{
	struct jd_task *jd_new_task = NULL; // 创建一个任务节点指针
	jd_new_task = jd_request_space(JD_DEFAULT_STACK_SIZE);
	if (jd_new_task == JD_NULL)
		return JD_NULL; // 申请空间

	jd_new_task->timeout = 0;			  // 没有延时时间
	jd_new_task->entry = task_entry;	  // 任务入口
	jd_new_task->status = JD_PAUSE;		  // 创建任务，状态为暂停状态，等待启动
	jd_new_task->stack_size = stack_size; // 记录当前任务堆栈大小

	jd_new_task->stack_sp = jd_new_task->stack_origin_addr + JD_DEFAULT_STACK_SIZE - sizeof(struct all_register) - 4; // 腾出寄存器的空间
	struct all_register *stack_register = (struct all_register *)jd_new_task->stack_sp;								  // 将指针转换成寄存器指针

	// 将任务运行数据搬移到内存中
	stack_register->r0 = 0;
	stack_register->r1 = 0;
	stack_register->r2 = 0;
	stack_register->r3 = 0;
	stack_register->r12 = 0;
	stack_register->lr = (unsigned long)jd_new_task->entry;
	stack_register->pc = (unsigned long)jd_new_task->entry;
	stack_register->xpsr = 0x01000000L; // 由于Armv7-M只支持执行Thumb指令，因此必须始终将其值保持为1，否则任务切换会异常

	jd_new_task->priority = priority; // 设置优先级

	return jd_new_task; // 返回当前任务节点
}

/*删除任务 暂时未实现
 * jd_task:任务节点指针
 * return：返回JD_OK或JD_ERR
 */
int jd_task_delete(struct jd_task *jd_task)
{
	if (jd_task == JD_NULL)
		return JD_ERR;

	if (jd_task == jd_task_frist)
		return JD_ERR; // 判断是否为系统第一个任务，系统第一个任务不可删除

	jd_task_pause(jd_task);

	free((unsigned long *)jd_task->stack_sp); // 释放任务堆栈内存
	free(jd_task->node);					  // 释放节点内存
	free(jd_task);							  // 释放任务内存
	return JD_OK;
}

/*将任务加入就绪链表
 * jd_task:任务节点指针
 * return：返回JD_OK或JD_ERR
 */
int jd_task_run(struct jd_task *jd_task)
{
	if (jd_task == JD_NULL)
		return JD_ERR;
	jd_task->status = JD_READY; // 将任务更改为就绪状态

	// 根据优先级插入节点位置
	struct jd_task *jd_task_temp;
	struct jd_node_list *node_temp;
	node_temp = jd_task_list_readying;
	// 遍历就绪链表
	while (1)
	{
		// 获取节点数据域
		jd_task_temp = container_of(&node_temp, struct jd_task, node);
		// 如果优先级高或相同
		if (jd_task->priority >= jd_task_temp->priority)
		{
			// 判断不为表头
			if (node_temp->previous == JD_NULL)
			{
				jd_task_list_readying = jd_task->node; // 切换表头
			}
			jd_node_insert(node_temp->previous, jd_task->node, node_temp);
			break;
		}
		// 如果已经到达表尾
		if (node_temp->next == JD_NULL)
		{
			jd_node_insert(node_temp, jd_task->node, JD_NULL);
			break;
		}
		// 临时节点切换为下一个节点
		node_temp = node_temp->next;
	}
	// 插入节点
	return JD_OK;
}

/*任务暂停
 *	jd_task:任务节点指针
 * return：返回JD_OK或JD_ERR
 */
int jd_task_pause(struct jd_task *jd_task)
{
	if (jd_task == JD_NULL)
		return JD_ERR;

	// 不能更改系统空闲任务状态，始终在就绪链表
	if (jd_task == jd_task_frist)
		return JD_ERR;

	// 本来就为暂停状态
	if (jd_task->status == JD_PAUSE)
		return JD_OK;

	// 在延时列表或就绪列表时，直接删除节点
	jd_node_insert(jd_task->node->previous, JD_NULL, jd_task->node->next);

	// 在就绪链表表头
	if (jd_task_list_readying == jd_task->node)
	{
		// 移动表头，同时将表头中指向的上一个节点信息删除
		jd_task_list_readying = jd_task->node->next;
		jd_task_list_readying->previous = JD_NULL;
	}
	// 在延时链表表头
	if (jd_task_list_delaying == jd_task->node)
	{
		// 移动表头，同时将表头中指向的上一个节点信息删除
		jd_task_list_delaying = jd_task->node->next;
		jd_task_list_delaying->previous = JD_NULL;
	}
	jd_task->status = JD_PAUSE; // 将任务更改为暂停状态状态
	return JD_OK;
}

/*任务切换*/
void jd_task_switch(void)
{
	struct jd_task *jd_task;
	struct jd_node_list *node_temp;
	node_temp = jd_task_list_readying;

	// 获取数据域
	jd_task = container_of(&node_temp, struct jd_task, node);

	// 没有外部条件改变当前任务状态,
	if (jd_task_runing->status == JD_RUNNING)
	{
		// 就绪任务表头仍然是当前任务
		if (jd_task_runing == jd_task)
		{
			// 高优先级任务正在执行，低优先级任务不能打断，
			// RUNGING说明任务没有放弃CPU使用权
			return;
		}
		// 表头已经移动，当前任务被高优先级任务打断
		else
		{
			// 将当前任务状态改为就绪状态，切换到下一个任务
			jd_task_runing->status = JD_READY;
		}
	}
	// 任务暂停或延时状态，或者当前任务优先级低，当前任务放弃CPU使用权
	jd_task->status = JD_RUNNING;					   // 即将运行的任务改为正在运行状态
	jd_task_stack_sp = &jd_task_runing->stack_sp;	   // 更新当前任务全局栈指针变量
	jd_task_runing = jd_task;						   // 更改当前为运行的任务
	jd_task_next_stack_sp = &jd_task_runing->stack_sp; // 更新下一个任务全局栈指针变量

	jd_asm_pendsv_putup(); // 挂起PendSV异常
}

/*内核第一次运行空闲任务*/
void jd_task_first_switch(void)
{
	struct jd_task *jd_task;
	jd_task = container_of(&jd_task_list_readying, struct jd_task, node);
	jd_asm_task_first_switch(&jd_task->stack_sp, jd_main);
}

/*PendSV处理上下文切换*/
/*void PendSV_Handler(void)
{
	jd_asm_pendsv_handler(); //切换上下文
}*/

/*hal库已自动使能systick，以下为hal库systick异常回调函数*/
void HAL_IncTick(void)
{
	uwTick += uwTickFreq; // 系统自带不可删除,否则hal_delay等hal库函数不可用

	jd_time++; // jd_lck++
	// 判断延时表头是否到达时间，若没有到达时间，则切换，若到达时间则将任务加入到就绪任务,在切换任务
	struct jd_task *jd_task;
	jd_task = container_of(&jd_task_list_delaying, struct jd_task, node);
	while (jd_task->timeout == jd_time)
	{
		jd_node_insert(jd_task->node->previous, JD_NULL, jd_task->node->next); // 删除当前节点
		jd_task_run(jd_task);												   // 加入就绪链表
		jd_task_list_delaying = jd_task_list_delaying->next;				   // 切换表头
		jd_task = container_of(&jd_task_list_delaying, struct jd_task, node);  // 获取数据域
	}
	jd_task_switch(); // jd_task_switch
}

/*jd初始化*/
int jd_init(void)
{
	// 为第一个节点创建空间
	jd_task_list_readying = malloc(sizeof(struct jd_node_list));
	jd_task_list_delaying = malloc(sizeof(struct jd_node_list));
	// 节点指向NULL
	jd_task_list_readying->next = JD_NULL;
	jd_task_list_readying->previous = JD_NULL;
	jd_task_list_delaying->next = JD_NULL;
	jd_task_list_delaying->previous = JD_NULL;

	jd_task_frist = jd_task_create(jd_main, JD_DEFAULT_STACK_SIZE, 127);
	while (jd_task_frist == NULL)
		; // 空闲任务不能创建则死循环

	jd_task_frist->status = JD_READY; // 任务就绪

	jd_task_frist->stack_sp = jd_task_frist->stack_origin_addr + JD_DEFAULT_STACK_SIZE - 4; // 栈顶

	jd_task_list_readying = jd_task_frist->node; // 将任务挂在就绪链表上
	jd_task_runing = jd_task_frist;				 // 保存当前任务为正在运行任务
	// jd_asm_systick_init(); //启动systick,hal库已自动使能systick

	jd_task_first_switch(); // 进入空闲任务
	return JD_OK;
}

// 测试任务
void task1()
{
	// printf("1 hello\r\n");
	while (1)
	{
		jd_delay(100);
		// HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
	};
}
void task2()
{
	// printf("2 hello\r\n");
	while (1)
	{
		jd_delay(150);
		// HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
	};
}
void task3()
{
	// printf("3 hello\r\n");
	while (1)
	{
		jd_delay(80);
		// HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_7);
	};
}

/*系统main,系统第一个任务，不可使用jd_task_delete删除，可添加其他任务初始化代码*/
__weak void jd_main(void)
{
	// printf("jd hello\r\n");
	struct jd_task *test_task1 = jd_task_create(task1, 512, 0);
	if (test_task1 != JD_NULL)
		jd_task_run(test_task1);

	struct jd_task *test_task2 = jd_task_create(task2, 512, 1);
	if (test_task1 != JD_NULL)
		jd_task_run(test_task2);

	struct jd_task *test_task3 = jd_task_create(task3, 512, 2);
	if (test_task1 != JD_NULL)
		jd_task_run(test_task3);
	while (1)
	{
		// jd_task_switch();
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);

		// 注意此处调用延时切换任务，如果所有任务都不为就绪状态，程序将在jd_task_switch函数中死循环，直到有就绪任务才会切换
		// 应该在此处休眠或者其他不重要的工作
		jd_delay(500);
	};
}
