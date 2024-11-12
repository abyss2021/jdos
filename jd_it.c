/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-18 16:12:28
 * @LastEditors: 江小鉴 abyss_er@163.com
 * @LastEditTime: 2024-11-12 14:03:14
 * @FilePath: \jdos\jd_it.c
 * @Description: jdos异常管理
 */

#include "jdos.h"

/**
 * @description: hal库已自动使能systick，以下为hal库systick异常回调函数
 * @return {*}
 */
void HAL_IncTick(void)
{
	jd_asm_cps_disable();
	uwTick += uwTickFreq; // 系统自带不可删除,否则hal_delay等hal库函数不可用

	jd_time++; // jd_lck++
	// 判断延时表头是否到达时间，若没有到达时间，则切换，若到达时间则将任务加入到就绪任务,再切换任务
	jd_task_t *jd_task;
	jd_task = (jd_task_t *)jd_task_list_delaying; // 获取任务数据

	while (jd_task->timeout == jd_time)
	{
		// 如果循环定时器任务，将下一次定时时间写入任务信息
		if (jd_task->timer_loop == JD_TIMER_LOOP)
		{
			jd_task->timeout = jd_time + jd_task->timer_loop_timeout;
		}

		jd_task_list_delaying = jd_node_delete(jd_task_list_delaying, jd_task_list_delaying); // 删除延时完成的节点
		
		jd_task->status = JD_TASK_READY; // 将任务更改为就绪状态
		// 加入就绪链表
		jd_task_list_readying = jd_node_in_rd(jd_task_list_readying, &jd_task->node);

		if (jd_task_list_delaying == JD_NULL)
			break;
		jd_task = (jd_task_t *)jd_task_list_delaying; // 获取任务数据
	}
	
	
	#ifdef JD_CPU_U_ENABLE
	//这里计算的是空闲任务的运行时间
			static jd_uint8_t jd_cpu_time_100ms = 0;
			if(++jd_cpu_time_100ms == 100)
			{
				jd_uint32_t jd_cpu_time = jd_cpu_u_time_get();  //15ns是64Mhz一条指令的时间，将时间转换成毫秒
				
				#ifdef JD_PRINTF_ENABLE
				jd_printf("jd_cpu_time:%f\r\n",(float)jd_cpu_time/jd_cpu_u_100_base);
				#endif
				
				jd_cpu_time_100ms = 0;
				jd_cpu_u_time_set0();
			}

	#endif

	jd_asm_cps_enable();

	jd_asm_pendsv_putup();
}

/**
 * @description: PendSV处理函数
 * @return {*}
 */
void jd_PendSV_Handler(void)
{
	jd_task_t *jd_task;
	
	jd_asm_cps_disable();

	// 获取数据域
	jd_task = (jd_task_t *)jd_task_list_readying; // 获取任务数据
	jd_task_runing->status = JD_TASK_READY;
	// 任务暂停或延时状态，或者当前任务优先级低，当前任务放弃CPU使用权
	jd_task->status = JD_TASK_RUNNING;					   // 即将运行的任务改为正在运行状态
	jd_task_stack_sp = &jd_task_runing->stack_sp;	   // 更新当前任务全局栈指针变量
	jd_task_runing = jd_task;						   // 更改当前为运行的任务
	jd_task_next_stack_sp = &jd_task_runing->stack_sp; // 更新下一个任务全局栈指针变量
	
	#ifdef JD_CPU_U_ENABLE
		jd_cpu_u_time();
	#endif
	
	jd_asm_cps_enable();

	jd_asm_pendsv_handler(); // 切换上下文
}

/**
 * @description: SVC处理函数
 * @return {*}
 */
void jd_SVC_Handler(void)
{
	#ifdef JD_CPU_U_ENABLE
		jd_cpu_u_time();
	#endif
	jd_asm_svc_handler();
}
