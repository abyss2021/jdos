/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-18 16:11:38
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-09-28 10:58:10
 * @FilePath: \jdos\jd_task.c
 * @Description: 任务管理
 */

#include "jdos.h"

jd_node_list_t *jd_task_list_readying = NULL; // 创建就绪任务链表
jd_node_list_t *jd_task_list_delaying = NULL; // 创建延时任务链表
jd_task_t *jd_task_runing = NULL;			  // 创建当前任务指针
void *jd_task_stack_sp = NULL;				  // 创建当前任务堆栈指针的地址
void *jd_task_next_stack_sp = NULL;			  // 创建下一个任务堆栈指针的地址
jd_task_t *jd_task_frist = NULL;			  // 创建一个系统空闲任务
jd_uint32_t jd_task_entry;					  // 任务入口
jd_uint32_t jd_task_exit_entry;				  // 任务exit入口

/**
 * @description: 新节点插入链表中
 * @param {jd_node_list_t} *node_previous 想要插入的链表节点处的上一个节点
 * @param {jd_node_list_t} *node 想要插入的节点,为JD_NULL表示连接前后两个节点
 * @param {jd_node_list_t} *node_next 想要插入的链表节点处的下一个节点
 * @return {*}
 */
jd_int32_t jd_node_insert(jd_node_list_t *node_previous, jd_node_list_t *node, jd_node_list_t *node_next)
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

/**
 * @description: 删除节点
 * @param {jd_node_list_t} *list 需要进行删除的链表
 * @param {jd_node_list_t} *node 需要删除的节点
 * @return {*}
 */
jd_node_list_t *jd_node_delete(jd_node_list_t *list, jd_node_list_t *node)
{
	if (list == JD_NULL || node == JD_NULL)
		return JD_NULL;

	// 判断节点是否在表头
	if (node == list)
	{
		// 移动表头
		list = list->next;
		// 如果移动后表头不为空
		if (list != JD_NULL)
			list->previous = JD_NULL;
	}
	// 判断是否在表尾
	else if (node->next == JD_NULL)
	{
		// 删除最后一个节点
		node->previous->next = JD_NULL;
	}
	// 在表中
	else
	{
		jd_node_insert(node->previous, JD_NULL, node->next);
	}
	// 清空节点信息
	node->previous = JD_NULL;
	node->next = JD_NULL;
	return list;
}

/**
 * @description: 比较函数，用于jd_node_in_rd中使用
 * @param {jd_task_t} *task1 用于比较的任务1
 * @param {jd_task_t} *task2 用于比较的任务2
 * @return {*}
 */
jd_int64_t compare_priority(jd_task_t *task1, jd_task_t *task2)
{
	return task1->priority - task2->priority;
}
jd_int64_t compare_timeout(jd_task_t *task1, jd_task_t *task2)
{
	return task1->timeout - task2->timeout;
}

/**
 * @description: 将节点插入就绪或者延时链表
 * @param {jd_node_list_t} *list 要插入的链表
 * @param {jd_node_list_t} *node 要插入的节点
 * @return {*}
 */
jd_node_list_t *jd_node_in_rd(jd_node_list_t *list, jd_node_list_t *node)
{
	jd_task_t *jd_task_temp, *jd_task_in_temp;
	jd_node_list_t *node_temp;

	// 链表没有任务
	if (list == JD_NULL)
	{
		list = node;
		list->next = JD_NULL;
		list->previous = JD_NULL;
	}
	// 链表中有任务
	else
	{
		// 比较函数选择
		jd_int64_t (*compare)(jd_task_t *task1, jd_task_t *task2);
		if (list == jd_task_list_readying)
		{
			compare = compare_priority;
		}
		else
		{
			compare = compare_timeout;
		}

		// 临时节点，用于遍历链表
		node_temp = list;
		// 插入节点的任务数据
		jd_task_in_temp = (jd_task_t *)node;
		// 遍历链表
		while (1)
		{
			jd_task_temp = (jd_task_t *)node_temp; // 获取任务数据

			// 如果数字越小，优先级越高，或者延时时间越短
			if (compare(jd_task_in_temp, jd_task_temp) <= 0)
			{
				// 判断为表头
				if (node_temp->previous == JD_NULL)
				{
					list = node; // 切换表头
					jd_node_insert(JD_NULL, list, node_temp);
				}
				else
				{
					jd_node_insert(node_temp->previous, node, node_temp);
				}
				break;
			}
			// 判断表尾
			if (node_temp->next == JD_NULL)
			{
				// 将任务插入到表尾
				jd_node_insert(node_temp, node, JD_NULL);
				break;
			}
			// 临时节点切换为下一个节点
			node_temp = node_temp->next;
		}
	}
	return list;
}

/**
 * @description:任务退出函数,用户任务处理完后自动处理,系统自动调用
 * @return {*}
 */
void jd_task_exit()
{
	jd_task_t *jd_task = jd_task_runing;

	jd_task_entry = (jd_uint32_t)jd_task->entry;	// 传递程序入口值
	jd_task_exit_entry = (jd_uint32_t)jd_task_exit; // 传递退出时程序销毁入口

	if (jd_task->auto_delate == JD_TASK_NODELATE)
	{
		// 暂停任务
		jd_task_pause(jd_task);
	}
	else
	{
		// 删除任务
		jd_task_delete(jd_task);
	}

	jd_task->stack_sp = (jd_uint32_t)(jd_task->stack_origin_addr) + jd_task->stack_size - sizeof(struct all_register) - 4; // 腾出寄存器的空间
	all_register_t *stack_register = (struct all_register *)jd_task->stack_sp;											   // 将指针转换成寄存器指针

	// 设置必要数据
	stack_register->lr = (jd_uint32_t)jd_task_exit;
	stack_register->pc = (jd_uint32_t)jd_task->entry;
	stack_register->xpsr = 0x01000000L;

	jd_task->status = JD_DELAY;

	if (jd_task->timer_loop == JD_TIMER_LOOP)
		// 将节点加入延时链表
		jd_task_list_delaying = jd_node_in_rd(jd_task_list_delaying, &jd_task_runing->node);

	jd_task_stack_sp = &jd_task->stack_sp;
	// 获取数据域
	jd_task = (jd_task_t *)jd_task_list_readying; // 获取任务数据
	// 任务暂停或延时状态，或者当前任务优先级低，当前任务放弃CPU使用权
	jd_task->status = JD_RUNNING;					   // 即将运行的任务改为正在运行状态
	jd_task_runing = jd_task;						   // 更改当前为运行的任务
	jd_task_next_stack_sp = &jd_task_runing->stack_sp; // 更新下一个任务全局栈指针变量

	// 这里不是悬挂PendSV异常，所以直接跳转会出发异常，寄存器数据不会自动出栈，应该使用SVC呼叫异常

	jd_asm_svc_task_exit();
}

/**
 * @description: 创建任务
 * @param {jd_uint32_t} stack_size 任务栈大小
 * @param {jd_int8_t} priority 任务优先级-128-127，数字越小，优先级越高
 * @return {*}
 */
jd_task_t *jd_task_create(void (*task_entry)(), jd_uint32_t stack_size, jd_int8_t priority)
{
	jd_task_t *jd_new_task = NULL; // 创建一个任务节点指针
#ifdef JD_MEMORY_ENABLE
	jd_new_task = (jd_task_t *)jd_malloc(stack_size); // 分配空间
#else
	jd_new_task = (jd_task_t *)malloc(stack_size); // 分配空间
#endif
	if (jd_new_task == NULL)
		return JD_NULL; // 判断分配空间是否成功

	jd_new_task->node.next = JD_NULL;						   // 初始化节点指针
	jd_new_task->node.previous = JD_NULL;					   // 初始化节点指针
	jd_new_task->stack_origin_addr = (jd_uint32_t)jd_new_task; // 记录栈底指针

	jd_new_task->timeout = 0;			  // 没有延时时间
	jd_new_task->entry = task_entry;	  // 任务入口
	jd_new_task->status = JD_PAUSE;		  // 创建任务，状态为暂停状态，等待启动
	jd_new_task->stack_size = stack_size; // 记录当前任务堆栈大小

	jd_new_task->stack_sp = (jd_uint32_t)(jd_new_task->stack_origin_addr) + jd_new_task->stack_size - sizeof(struct all_register) - 4; // 腾出寄存器的空间
	all_register_t *stack_register = (struct all_register *)jd_new_task->stack_sp;													   // 将指针转换成寄存器指针

	// 将任务运行数据搬移到内存中
	stack_register->lr = (jd_uint32_t)jd_task_exit;
	stack_register->pc = (jd_uint32_t)jd_new_task->entry;
	stack_register->xpsr = 0x01000000L; // 由于Armv7-M只支持执行Thumb指令，因此必须始终将其值保持为1，否则任务切换会异常

	jd_new_task->priority = priority; // 设置优先级
	// jd_new_task->node->addr = jd_new_task; // 记录节点内存地址，方便通过节点找到任务数据域

	jd_new_task->timer_loop = JD_TIMER_NOTIMER;	 // 不是定时任务
	jd_new_task->auto_delate = JD_TASK_NODELATE; //	任务执行完成后不自动回收内存，任务不删除，下次可直接运行

	return jd_new_task; // 返回当前任务节点
}

/**
 * @description: 删除任务
 * @param {jd_task_t} *jd_task 任务节点指针
 * @return {*}
 */
jd_int32_t jd_task_delete(jd_task_t *jd_task)
{
	if (jd_task == JD_NULL)
		return JD_ERR;

	if (jd_task == jd_task_frist)
		return JD_ERR; // 判断是否为系统第一个任务，系统第一个任务不可删除

	jd_task_pause(jd_task); // 将任务修改为暂停状态，目的是从就绪或延时链表中删除节点

#ifdef JD_MEMORY_ENABLE
	jd_free((jd_uint32_t *)jd_task); // 释放任务栈内存
#else
	free((jd_uint32_t *)jd_task); // 释放任务栈内存
#endif

	return JD_OK;
}

/**
 * @description: 设置任务执行完成后自动回收内存，任务销毁
 * @param {jd_task_t} *jd_task 任务节点指针
 * @return {*}
 */
jd_int32_t jd_task_auto_delate(jd_task_t *jd_task)
{
	if (jd_task == JD_NULL)
		return JD_ERR;
	jd_task->auto_delate = JD_TASK_DELATE;
	return JD_OK;
}

/**
 * @description: 将任务加入就绪链表
 * @param {jd_task_t} *jd_task 任务节点指针
 * @return {*}
 */
jd_int32_t jd_task_run(jd_task_t *jd_task)
{
	if (jd_task == JD_NULL)
		return JD_ERR;
	jd_task->status = JD_READY; // 将任务更改为就绪状态

	// 加入就绪链表
	jd_task_list_readying = jd_node_in_rd(jd_task_list_readying, &jd_task->node);

	// 插入节点
	return JD_OK;
}

/**
 * @description: 任务暂停
 * @param {jd_task_t} *jd_task 任务节点指针
 * @return {*}
 */
jd_int32_t jd_task_pause(jd_task_t *jd_task)
{
	if (jd_task == JD_NULL)
		return JD_ERR;

	// 不能更改系统空闲任务状态，始终在就绪链表
	if (jd_task == jd_task_frist)
		return JD_ERR;

	// 本来就为暂停状态
	if (jd_task->status == JD_PAUSE)
		return JD_OK;

	// 在就绪链表表头
	if (jd_task_list_readying == &jd_task->node)
	{
		// 移动表头，同时将表头中指向的上一个节点信息删除
		jd_task_list_readying = jd_task->node.next;
		jd_task_list_readying->previous = JD_NULL;
	}
	// 在延时链表表头
	else if (jd_task_list_delaying == &jd_task->node)
	{
		// 移动表头，同时将表头中指向的上一个节点信息删除
		jd_task_list_delaying = jd_task->node.next;
		jd_task_list_delaying->previous = JD_NULL;
	}
	else
	{
		// 直接删除节点
		jd_node_insert(jd_task->node.previous, JD_NULL, jd_task->node.next);
	}
	jd_task->status = JD_PAUSE; // 将任务更改为暂停状态状态
	// 清除任务节点信息
	jd_task->node.next = JD_NULL;
	jd_task->node.previous = JD_NULL;
	return JD_OK;
}

/**
 * @description: jd初始化
 * @return {*}
 */
jd_int32_t jd_init(void)
{
#ifdef JD_PRINTF_ENABLE
	jd_printf("================\r\n");
#endif

#ifdef JD_MEMORY_ENABLE
	// 初始化内存
	jd_mem_init();
#endif

	// 初始化链表
	jd_task_list_readying = JD_NULL;
	jd_task_list_delaying = JD_NULL;

	// 设置优先级为最低
	jd_task_frist = jd_task_create(jd_main, JD_DEFAULT_STACK_SIZE, 127);
	while (jd_task_frist == JD_NULL)
		; // 空闲任务不能创建则死循环

	all_register_t *stack_register = (struct all_register *)jd_task_frist->stack_sp; // 将指针转换成寄存器指针

	// jd_main任务没有退出的程序，故返回地址指向自己
	stack_register->lr = (jd_uint32_t)jd_main;

	jd_task_frist->status = JD_READY; // 任务就绪

	// jd_task_frist->node->addr = jd_task_frist; // 记录节点内存地址，方便通过节点找到任务数据域

	jd_task_list_readying = &jd_task_frist->node; // 将任务挂在就绪链表上
	jd_task_runing = jd_task_frist;				  // 保存当前任务为正在运行任务

// jd_asm_systick_init(); //启动systick,hal库已自动使能systick
#ifdef JD_PRINTF_ENABLE
	jd_printf("jdos has completed initialization\r\n");
	jd_printf("Welcome!\r\n");
	jd_printf("================\r\n");
#endif
	// 进入空闲任务
	jd_asm_task_first_switch(&jd_task_frist->stack_sp, jd_main);
	return JD_OK;
}
