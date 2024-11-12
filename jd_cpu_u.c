#include "jdos.h"
#ifdef JD_CPU_U_ENABLE

//100ms DWT最大计时次数
jd_uint32_t jd_cpu_u_100_base = 0;
jd_int32_t jd_cpu_u = 0;


//cpu利用率初始化  全速运行100ms  得到最大运行时间
void jd_cpu_u_init(void)
{
	jd_asm_dwt_init();
	jd_asm_dwt_time_start();
	HAL_Delay(100);
	jd_asm_dwt_time_stop();
	jd_cpu_u_100_base = jd_cpu_u_time_get();
	jd_cpu_u_time_set0();
}

//cpu计时逻辑
void jd_cpu_u_time(void)
{
	if(jd_task_runing==jd_task_frist)
	{
			jd_asm_dwt_time_start();
	}
	else
	{
		jd_asm_dwt_time_stop();
	}
}

//获取空闲任务运行时间
jd_uint32_t jd_cpu_u_time_get(void)
{
	return jd_asm_dwt_time_get();
}

//计时清0
void jd_cpu_u_time_set0(void)
{
	jd_asm_dwt_time_set0();
}
#endif