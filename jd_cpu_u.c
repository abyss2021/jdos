/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-11-12 18:00:28
 * @LastEditors: 江小鉴 abyss_er@163.com
 * @LastEditTime: 2024-11-13 10:52:08
 * @FilePath: \jdos\jd_cpu_u.c
 * @Description: CPU利用率监测
 */
#include "jdos.h"
#ifdef JD_CPU_U_ENABLE

// 标志位
#define U_FLAG 1

jd_int32_t jd_cpu_u = 0;
jd_uint8_t jd_cpu_u_flag = 0;

extern void jd_asm_dwt_init(void);
extern void jd_asm_dwt_start(void);
extern void jd_asm_dwt_stop(void);
extern jd_uint32_t jd_asm_dwt_get(void);
extern void jd_asm_dwt_set0(void);

/**
 * @description: cpu利用率初始化  得到最大运行时间
 * @return {*}
 */
void jd_cpu_u_init(void)
{
	jd_asm_dwt_init();
	jd_asm_dwt_set0();
	jd_asm_dwt_start();
}

/**
 * @description: cpu计时逻辑
 * @return {*}
 */
void jd_cpu_u_start_stop(void)
{
	if (jd_cpu_u_flag == U_FLAG)
	{
		if (jd_task_runing == jd_task_frist)
		{
			jd_asm_dwt_start();
		}
		else
		{
			jd_asm_dwt_stop();
		}
	}
}

/**
 * @description: 返回CPU利用率
 * @return {*}
 */
jd_uint32_t jd_cpu_u_get(void)
{
	return jd_cpu_u;
}

/**
 * @description: 外部1ms周期性调用
 * @return {*}
 */
void jd_cpu_u_ctr(void)
{
	static jd_uint8_t jd_cpu_time_100ctr = 0;
	static jd_uint32_t jd_cpu_100max = 0;

	if (jd_time == 100&&jd_cpu_u_flag!=U_FLAG)
	{
		jd_cpu_u_flag = U_FLAG;
		jd_cpu_100max = jd_asm_dwt_get();
		jd_cpu_u = jd_cpu_100max;
	}
	if (jd_cpu_u_flag == U_FLAG)
	{
		if (++jd_cpu_time_100ctr == 100)
		{
			jd_cpu_u = 100 - (float)jd_asm_dwt_get() / jd_cpu_100max * 100;

#ifdef JD_PRINTF_ENABLE
			//jd_printf("jd_cpu_u:%d%%\r\n", jd_cpu_u);
#endif

			jd_cpu_time_100ctr = 0;
			jd_asm_dwt_stop();
			jd_asm_dwt_set0();
		}
	}
}
#endif
