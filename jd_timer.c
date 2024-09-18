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
	jd_task_runing->status = JD_DELAY;
	jd_task_runing->timeout = jd_time + ms; // 将延时时间写入节点
	// 删除就绪链表中的节点
	jd_task_list_readying = jd_node_delete(jd_task_list_readying, jd_task_runing->node);

	// 将节点加入延时链表
	jd_task_list_delaying = jd_node_in_rd(jd_task_list_delaying, jd_task_runing->node);

	// 切换线程，让出CPU，等延时后调度，用svc指令
	
	
	jd_task_t *jd_task;
	jd_node_list_t *node_temp;
	node_temp = jd_task_list_readying;
	
	//就绪链表只有一个任务
	if(node_temp->addr == JD_NULL)
		return;
	
	// 获取数据域
	jd_task = node_temp->addr; // 获取任务数据

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
	
	
	jd_asm_svc_call();
}



/*timer创建*/
jd_task_t *jd_timer_create(jd_task_t *task)
{
    
}


/*timer删除*/
jd_int32_t jd_timer_delete(jd_task_t *task)
{

}