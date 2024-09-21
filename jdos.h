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

// jdos变量重新定义
typedef unsigned char jd_uint8_t;
typedef signed char jd_int8_t;
typedef unsigned short jd_uint16_t;
typedef signed short jd_int16_t;
typedef unsigned int jd_uint32_t;
typedef signed int jd_int32_t;
typedef unsigned long jd_uint64_t;
typedef signed long jd_int64_t;
typedef jd_uint32_t jd_time_t;


/*枚举任务状态*/
typedef enum jd_task_status
{
    JD_READY = 0, // 任务就绪状态
    JD_RUNNING,   // 任务运行状态
    JD_DELAY,     // 任务延时状态
    JD_PAUSE,     // 任务暂停状态
} jd_task_status_t;

/*定义所有寄存器，根据入栈规则有先后顺序*/
typedef struct all_register
{
    // 手动入栈
    jd_uint32_t r4;
    jd_uint32_t r5;
    jd_uint32_t r6;
    jd_uint32_t r7;
    jd_uint32_t r8;
    jd_uint32_t r9;
    jd_uint32_t r10;
    jd_uint32_t r11;

    // 自动入栈
    jd_uint32_t r0;
    jd_uint32_t r1;
    jd_uint32_t r2;
    jd_uint32_t r3;
    jd_uint32_t r12;
    jd_uint32_t lr;
    jd_uint32_t pc;
    jd_uint32_t xpsr;
} all_register_t;

// 定义链表
typedef struct jd_node_list
{
    struct jd_node_list *previous; // 上一个节点
    void *addr;                    // 当前节点地址
    struct jd_node_list *next;     // 下一个节点
} jd_node_list_t;

/*定义任务控制块*/
typedef struct jd_task
{
    jd_node_list_t *node;          // 链表节点
    void (*entry)();               // 指向任务入口函数
    jd_task_status_t status;       // 当前任务状态
    jd_uint32_t stack_size;        // 堆栈大小
    jd_uint32_t stack_sp;          // 堆栈指针
    jd_uint32_t stack_origin_addr; // 堆栈起始地址
    jd_uint32_t timeout;           // 延时溢出时间，单位ms，为0则没有延时
    char priority;                 // 优先级-128 - 127,越低优先级越高,一般从0开始用
} jd_task_t;

/*第一次进入任务*/
extern void jd_asm_task_first_switch(jd_uint32_t *, void *);
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

extern jd_node_list_t *jd_task_list_readying; // 创建就绪任务链表
extern jd_node_list_t *jd_task_list_delaying ; // 创建延时任务链表
extern jd_task_t *jd_task_runing;             // 创建当前任务指针
extern void *jd_task_stack_sp;                // 创建当前任务堆栈指针的地址
extern void *jd_task_next_stack_sp;           // 创建下一个任务堆栈指针的地址
extern jd_task_t *jd_task_frist;              // 创建一个系统空闲任务

/*系统时钟，单位ms*/
extern jd_time_t jd_time;

/*新节点插入链表中 */
jd_int32_t jd_node_insert(jd_node_list_t *node_previous, jd_node_list_t *node, jd_node_list_t *node_next);
/*删除节点*/
jd_node_list_t *jd_node_delete(jd_node_list_t *list, jd_node_list_t *node);
/*比较函数，用于jd_node_in_rd中使用*/
jd_int64_t compare_priority(jd_task_t *task1, jd_task_t *task2);
/*将节点插入就绪或者延时链表*/
jd_node_list_t *jd_node_in_rd(jd_node_list_t *list, jd_node_list_t *node);





/*jdos延时，让出CPU使用权 */
void jd_delay(jd_uint32_t ms);
/*申请任务空间 */
jd_task_t *jd_request_space(jd_uint32_t stack_size);
/*创建任务*/
jd_task_t *jd_task_create(void (*task_entry)(), jd_uint32_t stack_size, jd_int8_t priority);
/*删除任务 */
jd_int32_t jd_task_delete(jd_task_t *jd_task);
/*将任务加入就绪链表 */
jd_int32_t jd_task_run(jd_task_t *jd_task);
/*任务暂停*/
jd_int32_t jd_task_pause(jd_task_t *jd_task);
/*内核第一次运行空闲任务*/
void jd_task_first_switch(void);
/*jd初始化*/
jd_int32_t jd_init(void);
/*jd main*/
void jd_main(void);

#endif
