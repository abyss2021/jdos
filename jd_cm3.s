; @Author: 江小鉴 abyss_er@163.com
; @Date: 2024-09-18 14:56:45
; @LastEditors: 江小鉴 abyss_er@163.com
; @LastEditTime: 2024-09-26 13:10:46
; @FilePath: \jdos\jd_cm3.s
; @Description: 汇编文件

JD_ICRS				EQU 0XE000ED04	;中断控制及状态寄存器
JD_PRI_14 			EQU 0XE000ED23	;PendSV的优先级设置寄存器
JD_SYSTICK_CTRL 	EQU 0xE000E010	;SysTick控制及状态寄存器
JD_POWER_SLEEP 		EQU 0xE000ED10	;睡眠控制寄存器
JD_DEMCR			EQU 0xE000EDFC  ;DEMCR的地址 用于使能DWT
JD_DWT_CYCCNT		EQU 0xE0001004	;DWT计数寄存器
JD_DWT_CTRL			EQU 0xE0001000	;DWT控制寄存器
	
	IMPORT jd_task_stack_sp
	IMPORT jd_task_next_stack_sp
	IMPORT jd_task_entry
	IMPORT jd_task_exit_entry
	IMPORT jd_mem_space
		
	IMPORT Stack_Mem
	IMPORT Stack_Size
						AREA |.text|, CODE, READONLY, ALIGN=3

jd_asm_task_first_switch 	PROC	;进入main
							EXPORT  jd_asm_task_first_switch
								CPSID i ;关中断

								;设置PendSV的优先级为255
								LDR R3,=JD_PRI_14
								LDR R2,=0X000000FF
								STR R2,[R3]

								;第一次进入任务，主要目的是定位堆栈，进入程序入口，其他数据无用
								LDR R0,[R0]
								MSR PSP, R0 ; 用户程序堆栈指针
								MOV LR,R1				
								
								; 清除SysTick悬起状态
								LDR R0, =JD_ICRS ; ICSR寄存器的地址
								LDR R1, [R0]        ; 读取当前ICSR寄存器的值
								ORR R1, R1, #0x04000000 ; 设置PENDSTCLR位
								STR R1, [R0]        ; 写回ICSR寄存器
								
								MOV R0, #0x2 ; 设置CONTROL寄存器，让用户程序使用PSP
								MSR CONTROL,R0	
								CPSIE i ;开中断	
								
								BX LR
							ENDP
								
jd_asm_task_exit_switch 	PROC	;任务结束运行（没有while），切换下一个任务
							EXPORT  jd_asm_task_exit_switch
								CPSID i ;关中断
								
								; 此处应该硬性改变堆栈中LR与PC的，相当于回复第一次执行的程序入口数据
								LDR R1,=jd_task_entry ;此次程序的入口地址
								LDR R1,[R1]
								
								LDR R2,=jd_task_exit_entry ;此次程序的exit的入口地址
								LDR R2,[R2]
								
								LDR R0,=jd_task_stack_sp ;此次任务的栈指针
								LDR R0,[R0]
								LDR R0,[R0]
								STR R1,[R0,#56]
								STR R2,[R0,#52]
								
								;取下一个任务的堆栈指针,恢复现场
								LDR R1,=jd_task_next_stack_sp
								LDR R1,[R1]
								LDR R0,[R1]
								LDMFD R0!,{R4-R11}
								MSR PSP, R0 ; 用户程序堆栈指针
								
								MOV R0, #0x2 ; 设置CONTROL寄存器，让用户程序使用PSP
								MSR CONTROL,R0	
								CPSIE i ;开中断	
								
								BX LR
							ENDP	


jd_asm_svc_handler			PROC	;SVC处理
							EXPORT  jd_asm_svc_handler		

								TST LR, #0x4 ; 测试EXC_RETURN的比特2
								ITE EQ ; 如果为0,
								MRSEQ R0, MSP ; 则使用的是主堆栈，故把MSP的值取出
								MRSNE R0, PSP ; 否则, 使用的是进程堆栈，故把MSP的值取出
								;  获取返回地址 (原理是与发生异常时硬件压栈的顺序相关)
								;  这里获得返回地址的原因是为了定位产生异常前执行的最后一条指令，也就是SVC指令
								LDR R1, [R0, #24] 
								
								; 获取SVC指令的低8位，也就是系统调用号，返回地址的上一条就是SVC指令，获取的是机器码
								LDRB R1, [R1, #-2] 
								
								;svc_0服务，任务自动切换下一个任务
								CMP R1, #0
								BEQ svc_handler_0
								
								;svc_1服务，任务中没有while循环，执行完成后退出，需要系统对任务进行操作
								CMP R1, #1
								BEQ svc_handler_1
								BX LR
svc_handler_0
								B jd_asm_pendsv_handler ;正常切换
								BX LR
svc_handler_1					
								B jd_asm_task_exit_switch
								BX LR
							ENDP		

jd_asm_svc_task_switch	PROC	;SVC call
						EXPORT  jd_asm_svc_task_switch
						CPSIE i ;开中断
						SVC 0
						BX LR
						ENDP

jd_asm_svc_task_exit	PROC	;SVC call
						EXPORT  jd_asm_svc_task_exit
						CPSIE i ;开中断
						SVC 1
						BX LR
						ENDP


jd_asm_systick_init			PROC	;systick初始化，hal库已经初始化，这里不可调用
							EXPORT jd_asm_systick_init
								CPSID i ;关中断
								
								LDR R0, =JD_SYSTICK_CTRL ; 加载JD_SYSTICK_CTRL的地址
								MOV R1, #0
								STR R1, [R0] ; 先停止SysTick，以防意外产生异常请求
								
								LDR R1, =72000 ; 让SysTick每72000周期计完一次,主频为72Mhz，计时时间为1ms
								STR R1, [R0,#4] ; 写入重装载的值
								STR R1, [R0,#8] ; 往STCVR中写任意的数，以确保清除COUNTFLAG标志
								
								MOV R1, #0x7 ; 选择FCLK作为时钟源，并使能SysTick及其异常请求
								STR R1, [R0] ; 写入数值，开启定时器
								
								CPSIE i ;开中断
								BX LR
							ENDP
								
jd_asm_cps_disable			PROC	;除能在 NMI 和硬 fault 之外的所有异常
							EXPORT jd_asm_cps_disable
								CPSID i ;关中断
								BX LR;
							ENDP
																
jd_asm_cps_enable			PROC	;使能中断
							EXPORT jd_asm_cps_enable
								CPSIE i ;开中断
								BX LR;
							ENDP

jd_asm_pendsv_putup 		PROC	;悬挂PendSV异常
							EXPORT jd_asm_pendsv_putup
								CPSID i ;关中断

								LDR R0,=JD_ICRS
								LDR R1,=0X10000000
								STR R1,[R0]

								CPSIE i ;开中断
								BX LR
							ENDP
							
jd_asm_pendsv_handler   	PROC	;切换上下文
							EXPORT  jd_asm_pendsv_handler 
								CPSID i ;关中断

								MRS R0,PSP ; 用户程序堆栈指针
								
								STMFD R0!,{R4-R11}	
								
								;保护现场，将堆栈指针传出
								LDR R1,=jd_task_stack_sp
								LDR R1,[R1]
								STR R0,[R1]
								
								;取下一个任务的堆栈指针,恢复现场
								LDR R1,=jd_task_next_stack_sp
								LDR R1,[R1]
								LDR R0,[R1]
								LDMFD R0!,{R4-R11}
								MSR PSP, R0 ; 用户程序堆栈指针
								
								MOV R0, #0x2 ; 设置CONTROL寄存器，让用户程序使用PSP
								MSR CONTROL,R0	
								CPSIE i ;开中断	
								
								
								BX LR
							ENDP

jd_asm_power_sleep 	PROC
					EXPORT jd_asm_power_sleep

					;SLEEPDEEP = 0;进入轻度睡眠，内核停止，外设不停止
					;SLEEP-NOW：如果SLEEPONEXIT位被清除，当WRI或WFE被执行时，微控制器立即进入睡眠模式。
					;SLEEP-ON-EXIT：如果SLEEPONEXIT位被置位，系统从最低优先级的中断处理程序中退出时，微控制器就立即进入睡眠模式。
					
					LDR R0,=JD_POWER_SLEEP
					LDR R1,[R0]
					AND R1,#0xF9
					STR R1,[R0]
					WFI
					
					BX LR
					ENDP
						
jd_asm_dwt_init 	PROC	;cpu利用率初始化
					EXPORT jd_asm_dwt_init
						
					LDR R0,=JD_DEMCR ;使能DWT
					LDR R1,[R0]
					ORR R1,#0x1000000
					STR R1,[R0]
					
					BX LR
					ENDP
				

jd_asm_dwt_set0	PROC
						EXPORT	jd_asm_dwt_set0
							
						LDR R0,=JD_DWT_CYCCNT ;DWT清0
						MOV R1,#0
						STR R1,[R0]
						
						BX LR
						ENDP

jd_asm_dwt_start	PROC	;DWT计时开始
						EXPORT 	jd_asm_dwt_start

						LDR R0,=JD_DWT_CTRL	;DWT启动计时
						LDR R1,[R0]
						ORR R1,#0x1
						STR R1,[R0]
	
						BX LR
						ENDP
							
jd_asm_dwt_stop	PROC	;	DWT停止计时
						EXPORT	jd_asm_dwt_stop
							
						LDR R0,=JD_DWT_CTRL	;DWT启动计时
						LDR R1,[R0]
						AND R1,#0xFFFFFFFE
						STR R1,[R0]
						
						BX LR
						ENDP

jd_asm_dwt_get		PROC	;DWT计时获取
						EXPORT	jd_asm_dwt_get
							
						LDR R0,=JD_DWT_CYCCNT
						LDR R0,[R0]
	
						BX LR
						ENDP
							
jd_initial_sp_get	PROC	
					EXPORT jd_initial_sp_get
						
					LDR R0,=Stack_Mem
					LDR R1,=Stack_Size
					ADD R0,R1
						
					BX LR
					ENDP

	;防止编译器报警
	NOP
	END