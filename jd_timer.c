/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-18 09:10:51
 * @LastEditors: 江小鉴 abyss_er@163.com
 * @LastEditTime: 2024-11-14 10:48:38
 * @FilePath: \jdos\jd_timer.c
 * @Description:用于定时管理
 */
#include "jdos.h"

extern void jd_asm_svc_task_switch(void); // 任务上下文切换

/*系统时钟，单位ms*/
jd_time_t jd_time = 0;

/**
 * @description: jdos延时，让出CPU使用权
 * @param {jd_uint32_t} ms  延时时间，单位ms
 * @return {*}
 */
void jd_delay(jd_uint32_t ms)
{
	if (ms == 0)
		return;

	jd_asm_cps_disable();
	jd_task_runing->status = JD_TASK_DELAY;
	jd_task_runing->timeout = jd_time + ms; // 将延时时间写入节点
	// 删除就绪链表中的节点
	jd_task_list_readying = jd_node_delete(jd_task_list_readying, &jd_task_runing->node);

	// 将节点加入延时链表
	jd_task_list_delaying = jd_node_in_rd(jd_task_list_delaying, &jd_task_runing->node);

	// 切换线程，让出CPU，等延时后调度，用svc指令
	jd_task_t *jd_task;

	// 获取数据域
	jd_task = (jd_task_t *)jd_task_list_readying; // 获取任务数据

	// 任务暂停或延时状态，或者当前任务优先级低，当前任务放弃CPU使用权
	jd_task->status = JD_TASK_RUNNING;				   // 即将运行的任务改为正在运行状态
	jd_task_stack_sp = &jd_task_runing->stack_sp;	   // 更新当前任务全局栈指针变量
	jd_task_runing = jd_task;						   // 更改当前为运行的任务
	jd_task_next_stack_sp = &jd_task_runing->stack_sp; // 更新下一个任务全局栈指针变量

	jd_asm_svc_task_switch();
}
#ifdef JD_TIMER_ENABLE
/**
 * @description: 定时任务创建
 * @param {jd_task_t} *jd_task 创建的普通任务
 * @param {jd_uint32_t} ms 定时时间
 * @param {jd_timer_status_t} timer_status 是否为循环任务
 * @return {*}
 */
jd_int32_t jd_timer_start(jd_task_t *jd_task, jd_uint32_t ms, jd_timer_status_t timer_status)
{
	if (jd_task == JD_NULL)
		return JD_ERR;

	// 定时任务，不是循环
	if (timer_status == JD_TIMER_NOLOOP)
	{
		jd_task->timer_loop = JD_TIMER_NOLOOP;
	}
	// 循环定时任务
	else if (timer_status == JD_TIMER_LOOP)
	{
		jd_task->timer_loop = JD_TIMER_LOOP;
	}
	else
	{
		return JD_ERR;
	}
	jd_asm_cps_disable();
	// 定时时间
	jd_task->timer_loop_timeout = ms;
	// 将延时时间写入节点
	jd_task->timeout = jd_time + jd_task->timer_loop_timeout;

	// 定义寄存器
	all_register_t *stack_register = (struct all_register *)jd_task->stack_sp;

	// 定时任务执行完成，执行销毁程序
	stack_register->lr = (jd_uint32_t)jd_task_exit;

	// 判断是否在就绪链表中
	if (jd_task->status == JD_TASK_RUNNING || jd_task->status == JD_TASK_READY)
	{
		// 删除就绪链表中的节点
		jd_task_list_readying = jd_node_delete(jd_task_list_readying, &jd_task->node);
	}

	// 将节点加入延时链表
	jd_task_list_delaying = jd_node_in_rd(jd_task_list_delaying, &jd_task->node);

	jd_asm_cps_enable();

	return JD_OK;
}

/*timer删除
 * jd_task:创建的普通任务
 */
jd_int32_t jd_timer_stop(jd_task_t *jd_task)
{
	if (jd_task == JD_NULL)
		return JD_ERR;

	// 暂停任务，将任务从链表中删除
	jd_task_pause(jd_task);

	// 关闭定时器任务
	jd_task->timer_loop = JD_TIMER_NOTIMER;
	return JD_OK;
}
#endif
