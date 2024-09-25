#include "jdos.h"


/*系统时钟，单位ms*/
jd_time_t jd_time = 0;

/*jdos延时，让出CPU使用权
 * ms:延时时间，单位ms
 */
void jd_delay(jd_uint32_t ms)
{
	if (ms == 0)
		return;
	
	jd_asm_cps_disable();
	jd_task_runing->status = JD_DELAY;
	jd_task_runing->timeout = jd_time + ms; // 将延时时间写入节点
	// 删除就绪链表中的节点
	jd_task_list_readying = jd_node_delete(jd_task_list_readying, jd_task_runing->node);

	// 将节点加入延时链表
	jd_task_list_delaying = jd_node_in_rd(jd_task_list_delaying, jd_task_runing->node);

	// 切换线程，让出CPU，等延时后调度，用svc指令
	jd_task_t *jd_task;
	
	// 获取数据域
	jd_task = jd_task_list_readying->addr; // 获取任务数据

	// 任务暂停或延时状态，或者当前任务优先级低，当前任务放弃CPU使用权
	jd_task->status = JD_RUNNING;					   // 即将运行的任务改为正在运行状态
	jd_task_stack_sp = &jd_task_runing->stack_sp;	   // 更新当前任务全局栈指针变量
	jd_task_runing = jd_task;						   // 更改当前为运行的任务
	jd_task_next_stack_sp = &jd_task_runing->stack_sp; // 更新下一个任务全局栈指针变量
	
	jd_asm_svc_task_switch();
}



/*timer创建*/
jd_int32_t jd_timer_create(jd_task_t *task)
{
    if(task == JD_NULL)
		return JD_ERR;

		
}


/*timer删除*/
jd_int32_t jd_timer_delete(jd_task_t *task)
{
	if(task == JD_NULL)
		return JD_ERR;
}