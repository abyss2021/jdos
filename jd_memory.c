#include "jdos.h"

/*内存使用状态*/
typedef enum jd_mem_used
{
    JD_MEM_USED = 1,
    JD_MEM_FREE = 2,
}jd_mem_used_t;

/*内存控制块*/
typedef struct jd_mem
{
    jd_node_list_t node;          // 链表节点
    jd_mem_used_t used;           // 当前内存是否被使用
    jd_uint32_t mem_size;         // 当前整体内存块大小
}jd_mem_t;


jd_mem_t *jd_mem_use = JD_NULL;
jd_uint8_t jd_mem_space[MEM_MAX_SIZE];



/*mem初始化*/
jd_uint32_t jd_mem_init()
{
    jd_mem_use = jd_mem_space; //传入内存块地址
    jd_mem_use->node.addr = jd_mem_use; //保存内存块地址

    jd_mem_use->node.next = JD_NULL;
    jd_mem_use->node.previous = JD_NULL;

    jd_mem_use->used = JD_MEM_FREE; //初始为空闲内存
    jd_mem_use->mem_size = MEM_MAX_SIZE; //初始内存块大小
}

/*分配内存空间*/
jd_uint32_t jd_malloc(jd_uint32_t mem_size)
{
    jd_mem_t *jd_mem_temp,*jd_mem_new_free;
    jd_mem_temp = jd_mem_use;
    while(1)
    {
        //找到足够的空闲空间
        if( jd_mem_temp->used == JD_MEM_FREE && (mem_size <= jd_mem_temp->mem_size + sizeof(jd_mem_t)))
        {
            jd_mem_new_free = jd_mem_temp + mem_size + sizeof(jd_mem_t); //将剩余的内存添加上内存块信息
            jd_mem_new_free->mem_size = jd_mem_temp->mem_size - mem_size - sizeof(jd_mem_t); //剩余内存大小
            jd_mem_new_free->node.addr = jd_mem_new_free; //保存当前内存的起点地址
            jd_mem_new_free->used = JD_MEM_FREE; //标记为空闲内存

            jd_mem_temp->used = JD_MEM_USED; //标记为使用状态
            jd_mem_temp->mem_size = mem_size + sizeof(jd_mem_t); //标记当前内存块总大小

            jd_node_insert(&jd_mem_temp->node,&jd_mem_new_free->node,&jd_mem_temp->node.next); //插入内存节点

            break;
        }
        //遍历完成，没有足够的空间进行分配，返回JD_NULL
        if(jd_mem_temp->node.next == JD_NULL)
        {
            return JD_NULL;
        }
        jd_mem_temp = jd_mem_temp->node.next->addr;
    }
}

/*释放内存空间*/
jd_uint32_t jd_free()
{

}