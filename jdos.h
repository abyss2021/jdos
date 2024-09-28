/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-11 11:09:06
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-09-28 11:00:30
 * @FilePath: \jdos\jdos.h
 * @Description: jdos 头文件
 */

#ifndef __JDOS_H
#define __JDOS_H

#include "stm32f1xx_hal.h"

/******************选择开启的功能************************/
#define JD_PRINTF_ENABLE // 开启打印功能
#define JD_MEMORY_ENABLE // 开启内存管理功能，关闭后请添加标准库
#define JD_TIMER_ENABLE  // 开启定时任务管理功能

/******************宏定义************************/
/*开辟内存大小*/
#define MEM_MAX_SIZE (1024 * 8)

/*宏定义函数返回状态*/
#define JD_NULL 0
#define JD_OK 1
#define JD_ERR 2

/*系统默认堆栈大小*/
#define JD_DEFAULT_STACK_SIZE 512

// jdos变量重新定义
typedef unsigned char jd_uint8_t;
typedef char jd_int8_t;
typedef unsigned short jd_uint16_t;
typedef signed short jd_int16_t;
typedef unsigned int jd_uint32_t;
typedef signed int jd_int32_t;
typedef unsigned long jd_uint64_t;
typedef signed long jd_int64_t;
typedef jd_uint32_t jd_time_t;

/******************结构体定义************************/
/*枚举任务状态*/
typedef enum jd_task_status
{
    JD_READY = 0, // 任务就绪状态
    JD_RUNNING,   // 任务运行状态
    JD_DELAY,     // 任务延时状态
    JD_PAUSE,     // 任务暂停状态
} jd_task_status_t;

/*定时任务使用状态*/
typedef enum jd_timer_status
{
    JD_TIMER_NOTIMER = 0, // 不是定时器任务
    JD_TIMER_NOLOOP,      // 是定时任务，但不是循环定时任务
    JD_TIMER_LOOP,        // 是循环定时任务
} jd_timer_status_t;

/*定时任务使用状态*/
typedef enum jd_task_auto_delate
{
    JD_TASK_NODELATE = 0, // 不销毁任务，不回收内存，下次可不用从新create
    JD_TASK_DELATE,       // 系统销毁任务，回收内存
} jd_task_auto_delate_t;

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

// 定义链表节点
typedef struct jd_node_list
{
    struct jd_node_list *previous; // 上一个节点
    struct jd_node_list *next;     // 下一个节点
} jd_node_list_t;

/*定义任务控制块*/
typedef struct jd_task
{
    jd_node_list_t node;               // 链表节点
    void (*entry)();                   // 指向任务入口函数
    jd_task_status_t status;           // 当前任务状态
    jd_uint32_t stack_size;            // 堆栈大小
    jd_uint32_t stack_sp;              // 堆栈指针
    jd_uint32_t stack_origin_addr;     // 堆栈起始地址
    jd_uint32_t timeout;               // 延时溢出时间，单位ms，为0则没有延时
    jd_int8_t priority;                // 优先级-128 - 127,越低优先级越高,一般从0开始用
    jd_timer_status_t timer_loop;      // 是否为定时任务，如果是定时任务是否为循环模式
    jd_uint32_t timer_loop_timeout;    // 任务处于循环状态，定时时间
    jd_task_auto_delate_t auto_delate; // 任务执行完成后是否需要系统销毁任务
} jd_task_t;

/*内存使用状态*/
typedef enum jd_mem_used
{
    JD_MEM_USED = 1,
    JD_MEM_FREE,
} jd_mem_used_t;

#pragma pack(4) // 4字节对齐
/*内存控制块*/
typedef struct jd_mem
{
    jd_node_list_t node;  // 链表节点
    jd_mem_used_t used;   // 当前内存是否被使用
    jd_uint32_t mem_size; // 当前整体内存块大小
} jd_mem_t;
#pragma pack() // 取消结构体对齐

/******************全局变量************************/
extern jd_node_list_t *jd_task_list_readying; // 创建就绪任务链表
extern jd_node_list_t *jd_task_list_delaying; // 创建延时任务链表
extern jd_task_t *jd_task_runing;             // 创建当前任务指针
extern void *jd_task_stack_sp;                // 创建当前任务堆栈指针的地址
extern void *jd_task_next_stack_sp;           // 创建下一个任务堆栈指针的地址
extern jd_task_t *jd_task_frist;              // 创建一个系统空闲任务
extern jd_uint32_t jd_task_entry;             // 任务入口
extern jd_uint32_t jd_task_exit_entry;        // 任务exit入口
extern jd_time_t jd_time;                     // 系统时钟，单位ms

/******************汇编函数************************/
extern void jd_asm_task_first_switch(jd_uint32_t *, void *); // 第一次进入任务
extern void jd_asm_pendsv_putup(void);                       // 切换任务节点，悬挂PendSV异常，PendSV中进行上下文切换
extern void jd_asm_pendsv_handler(void);                     // PendSV切换上下文
extern void jd_asm_systick_init(void);                       // systick初始化
extern void jd_asm_cps_disable(void);                        // 除能 NMI 和硬 fault 之外的所有异常
extern void jd_asm_cps_enable(void);                         // 使能中断
extern void jd_asm_svc_handler(void);                        // pendsv异常处理
extern void jd_asm_svc_task_switch(void);                    // 任务上下文切换
extern void jd_asm_svc_task_exit(void);                      // 任务推出

/******************jd_timer************************/
void jd_delay(jd_uint32_t ms);
#ifdef JD_TIMER_ENABLE                                                                         // jdos延时，让出CPU使用权
jd_int32_t jd_timer_start(jd_task_t *jd_task, jd_uint32_t ms, jd_timer_status_t timer_status); // 定时器任务创建
jd_int32_t jd_timer_stop(jd_task_t *task);                                                     // 定时器任务停止
#endif

/******************jd_task************************/
jd_task_t *jd_task_create(void (*task_entry)(), jd_uint32_t stack_size, jd_int8_t priority); // 创建任务
jd_int32_t jd_task_delete(jd_task_t *jd_task);                                               // 删除任务
jd_int32_t jd_task_auto_delate(jd_task_t *jd_task);                                          // 设置任务运行完成后自动回收内存，删除任务
jd_int32_t jd_task_run(jd_task_t *jd_task);                                                  // 将任务加入就绪链表
jd_int32_t jd_task_pause(jd_task_t *jd_task);                                                // 任务暂停
jd_int32_t jd_init(void);                                                                    // jd初始化
void jd_main(void);                                                                          // jd main

jd_int32_t jd_node_insert(jd_node_list_t *node_previous, jd_node_list_t *node, jd_node_list_t *node_next); // 节点连接函数
jd_node_list_t *jd_node_delete(jd_node_list_t *list, jd_node_list_t *node);                                // 删除节点
jd_int64_t compare_priority(jd_task_t *task1, jd_task_t *task2);                                           // 比较函数，用于jd_node_in_rd中使用
jd_node_list_t *jd_node_in_rd(jd_node_list_t *list, jd_node_list_t *node);                                 // 将节点插入就绪或者延时链表
void jd_task_exit(void);                                                                                   // 任务执行完成后由系统调用

/******************jd_memory************************/
#ifdef JD_MEMORY_ENABLE
jd_uint32_t jd_mem_init(void);         // mem初始化
void *jd_malloc(jd_uint32_t mem_size); // malloc
void jd_free(void *ptr);               // free
#endif

/******************jd_printf************************/
#ifdef JD_PRINTF_ENABLE
void jd_printf(const jd_int8_t *format, ...);
#endif

#endif
