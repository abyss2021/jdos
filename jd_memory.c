/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-23 09:36:38
 * @LastEditors: 江小鉴 abyss_er@163.com
 * @LastEditTime: 2024-11-14 17:39:36
 * @FilePath: \jdos\jd_memory.c
 * @Description: 用于内存管理
 */
#include "jdos.h"
#ifdef JD_MEMORY_ENABLE

#define JD_MEM_SIZE 0xC000
#define JD_CPU_START_MEM 0x20000000

extern jd_uint32_t jd_initial_sp_get(void);

jd_mem_t *jd_mem_use = JD_NULL;
jd_uint32_t jd_mem_space;


/**
 * @description: 内存初始化
 * @return {*}
 */
jd_uint32_t jd_mem_init()
{
	jd_mem_space = jd_initial_sp_get();
    jd_mem_use = (jd_mem_t *)jd_mem_space; // 传入内存块地址
    // jd_mem_use->node.addr = jd_mem_use;    // 保存内存块地址

    jd_mem_use->node.next = JD_NULL;
    jd_mem_use->node.previous = JD_NULL;

    jd_mem_use->used = JD_MEM_FREE;      // 初始为空闲内存
    jd_mem_use->mem_size = JD_MEM_SIZE-(jd_mem_space-JD_CPU_START_MEM); // 初始内存块大小
    return JD_OK;
}

/**
 * @description: 分配内存空间
 * @param {jd_uint32_t} mem_size 需要分配的空间
 * @return {*}
 */
void *jd_malloc(jd_uint32_t mem_size)
{
    jd_mem_t *jd_mem_temp, *jd_mem_new_free;

    if(mem_size==JD_NULL)
        return JD_NULL;
  
    jd_mem_temp = jd_mem_use;
    while (1)
    {
        // 找到足够的空闲空间
        if (jd_mem_temp->used == JD_MEM_FREE && (mem_size + sizeof(jd_mem_t) <= jd_mem_temp->mem_size))
        {
            jd_mem_temp->used = JD_MEM_USED; // 标记为使用状态
            // 防止内存管理中出现泄露,剩余内存足够分割至少sizeof(jd_mem_t)+1的空间
            if ((jd_mem_temp->mem_size - mem_size) > sizeof(jd_mem_t))
            {
                jd_mem_new_free = (jd_mem_t *)(((jd_uint8_t *)jd_mem_temp) + mem_size + sizeof(jd_mem_t)); // 将剩余的内存添加上内存块信息
                jd_mem_new_free->mem_size = jd_mem_temp->mem_size - mem_size - sizeof(jd_mem_t);           // 剩余内存大小
                jd_mem_new_free->used = JD_MEM_FREE;                                                       // 标记为空闲内存

                jd_mem_temp->mem_size = mem_size + sizeof(jd_mem_t); // 标记当前内存块总大小

                // 下一个控制块存在
                if (jd_mem_temp->node.next != JD_NULL)
                {
                    jd_mem_t *jd_mem_new_next;
                    jd_mem_new_next = (jd_mem_t *)jd_mem_temp->node.next;

                    jd_node_insert(&jd_mem_temp->node, &jd_mem_new_free->node, &jd_mem_new_next->node); // 插入内存节点
                }
                // 下一个不存在
                else
                {
                    jd_node_insert(&jd_mem_temp->node, &jd_mem_new_free->node, JD_NULL); // 插入内存节点
                }
            }
            break;
        }
        // 遍历完成，没有足够的空间进行分配，返回JD_NULL
        if (jd_mem_temp->node.next == JD_NULL)
        {
            jd_asm_cps_enable();
            return JD_NULL;
        }
        jd_mem_temp = (jd_mem_t *)jd_mem_temp->node.next;
    }
    return (void *)(((jd_uint8_t *)jd_mem_temp) + sizeof(jd_mem_t)); // 返回分配的地址
}

/**
 * @description: 释放内存空间
 * @param {void} *ptr 传入申请的空间的地址
 * @return {*}
 */
void jd_free(void *ptr)
{
    jd_mem_t *jd_mem_old, *jd_mem_previous, *jd_mem_next, *jd_mem_next_next;

    if(ptr==JD_NULL)
        return;
    jd_asm_cps_disable();    
    jd_mem_old = (jd_mem_t *)((jd_uint8_t *)ptr - sizeof(jd_mem_t)); // 获取控制块信息

    jd_mem_old->used = JD_MEM_FREE;
    // 下一个控制块存在
    if (jd_mem_old->node.next != JD_NULL)
    {
        jd_mem_next = (jd_mem_t *)jd_mem_old->node.next;
        // 判断下一个内存块是free
        if (jd_mem_next->used == JD_MEM_FREE)
        {
            // 合并内存块
            jd_mem_old->mem_size += jd_mem_next->mem_size;

            // 判断下一个控制块 的下一个存在
            if (jd_mem_next->node.next != JD_NULL)
            {
                jd_mem_next_next = (jd_mem_t *)jd_mem_next->node.next;
                jd_node_insert(&jd_mem_old->node, JD_NULL, &jd_mem_next_next->node);
            }
            // 不存在
            else
            {
                jd_mem_old->node.next = JD_NULL;
            }
        }
    }

    // 上一个控制块存在
    if (jd_mem_old->node.previous != JD_NULL)
    {
        // 获得上一个控制块的信息
        jd_mem_previous = (jd_mem_t *)jd_mem_old->node.previous;

        // 判断上一个内存块是free
        if (jd_mem_previous->used == JD_MEM_FREE)
        {
            jd_mem_previous->mem_size += jd_mem_old->mem_size; // 合并内存块
            // 判断下一块内存 存在
            if (jd_mem_old->node.next != JD_NULL)
            {
                jd_mem_next = (jd_mem_t *)jd_mem_old->node.next;
                jd_node_insert(&jd_mem_previous->node, JD_NULL, &jd_mem_next->node);
            }
            // 下一块内存不存在
            else
            {
                jd_mem_previous->node.next = JD_NULL;
            }
        }
    }
    ptr = JD_NULL;
    jd_asm_cps_enable();
}



/**
 * @description: 获取已经使用的空间大小
 * @return {*}
 */
jd_uint32_t jd_mem_used_get()
{
    jd_mem_t *jd_mem_temp;
    jd_uint32_t jd_mem_used;
    jd_asm_cps_disable();
    jd_mem_temp = jd_mem_use;
    jd_mem_used = jd_mem_space-JD_CPU_START_MEM;
    //遍历所有内存空间
    while(1)
    {

        if(jd_mem_temp->used==JD_MEM_USED)   
            jd_mem_used += jd_mem_temp->mem_size;
				if(jd_mem_temp->node.next==JD_NULL)
            break;
				else
						jd_mem_temp = (jd_mem_t *)jd_mem_temp->node.next;
    }
    #ifdef JD_PRINTF_ENABLE
    jd_printf("used mem:%dKB/%dKB\r\n",jd_mem_used/1000,JD_MEM_SIZE/1000);
    #endif

    jd_asm_cps_enable();
    return jd_mem_used;
}

#endif
