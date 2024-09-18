#include "jdos.h"

/*hal库已自动使能systick，以下为hal库systick异常回调函数*/
void HAL_IncTick(void)
{
	uwTick += uwTickFreq; // 系统自带不可删除,否则hal_delay等hal库函数不可用

	jd_time++; // jd_lck++
	// 判断延时表头是否到达时间，若没有到达时间，则切换，若到达时间则将任务加入到就绪任务,在切换任务
	jd_task_t *jd_task;
	jd_task = jd_task_list_delaying->addr; // 获取任务数据
	while (jd_task->timeout == jd_time)
	{
		jd_task = jd_task_list_delaying->addr;												  // 获取任务数据
		jd_task_list_delaying = jd_node_delete(jd_task_list_delaying, jd_task_list_delaying); // 删除延时完成的节点
		jd_task_run(jd_task);																  // 加入就绪链表
		
		if(jd_task_list_delaying == JD_NULL)
			break;
	}
	jd_asm_pendsv_putup();
}


/*PendSV处理函数*/
 void jd_PendSV_Handler(void)
 {
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

	jd_asm_pendsv_handler(); //切换上下文
}
 


