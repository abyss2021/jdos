#ifndef __JDOS_H
#define __JDOS_H

#include <stdio.h>
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
/*第一次进入任务*/
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


/*定义任务节点*/
struct jd_task{
    struct jd_task *previous;						//指向上一个节点
    void (*task_entry)();							//指向任务入口函数
    enum jd_task_status jd_task_status;             //当前任务状态
    unsigned long stack_size;						//堆栈大小
    unsigned long stack_sp;							//堆栈指针
    unsigned long stack_origin_addr;               	//堆栈起始地址
    struct jd_task *next;							//指向下一个节点
};

int jd_init(void); //系统初始化
struct jd_task *jd_create_task(void (*task_entry)(),unsigned int stack_size); //创建任务
int jd_run_task(struct jd_task *jd_task); //更改为就绪状态，等待调度
int jd_delete_task(struct jd_task *jd_task); //删除任务，释放内存
void jd_task_switch(void); //手动进行任务调度

#endif
