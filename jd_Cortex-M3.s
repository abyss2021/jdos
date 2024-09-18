JD_ICRS				EQU 0XE000ED04	;中断控制及状态寄存器
JD_PRI_14 			EQU 0XE000ED23	;PendSV的优先级设置寄存器
JD_SYSTICK_CTRL 	EQU 0xE000E010	;SysTick控制及状态寄存器
	
	IMPORT jd_task_stack_sp
	IMPORT jd_task_next_stack_sp
						AREA |.text|, CODE, READONLY, ALIGN=3
							
							

jd_asm_task_first_switch 	PROC	;进入main
							EXPORT  jd_asm_task_first_switch

								;设置PendSV的优先级为255
								LDR R3,=JD_PRI_14
								LDR R2,=0X000000FF
								STR R2,[R3]

								;第一次进入任务，主要目的是定位堆栈，进入程序入口，其他数据无用
								LDR R0,[R0]
								MOV SP,R0
								MOV LR,R1

								;MOV R0, #0x3 ; 设置CONTROL寄存器，让用户程序使用PSP，没有内存管理，PSP模式下无法分配系统空间
								;MSR CONTROL,R0
								;ORR LR,#0XD ;	切换到用户栈指针
								CPSIE i ;开中断
								BX LR
							ENDP
								
jd_asm_task_exit_switch 	PROC	;任务结束运行（没有while），切换下一个任务
							EXPORT  jd_asm_task_exit_switch
								CPSID i ;关中断
								
								;取下一个任务的堆栈指针,恢复现场
								LDR R1,=jd_task_next_stack_sp
								LDR R1,[R1]
								LDR R0,[R1]
								LDMFD R0!,{R4-R11}
								MOV SP,R0

								;MOV R0, #0x3 ; 设置CONTROL寄存器，让用户程序使用PSP，没有内存管理，PSP模式下无法分配系统空间
								;MSR CONTROL,R0
								;ORR LR,#0XD ;	切换到用户栈指针
								CPSIE i ;开中断
								BX LR
							ENDP	


jd_asm_svc_handler			PROC	;SVC处理
							EXPORT  jd_asm_svc_handler		
								CPSID i ;关中断
								
								MOV R0,SP
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
								MOV SP,R0
								
								;ORR LR,#0XD ;	切换到用户栈指针
								CPSIE i ;开中断
								BX LR
							ENDP		

jd_asm_svc_call		PROC	;SVC call
					EXPORT  jd_asm_svc_call
						SVC 0
					ENDP															
								

jd_asm_pendsv_putup 		PROC	;悬挂PendSV异常
							EXPORT jd_asm_pendsv_putup
								LDR R0,=JD_ICRS
								LDR R1,=0X10000000
								STR R1,[R0]
								BX LR
							ENDP
									
									

jd_asm_systick_init			PROC	;systick初始化，hal库已经初始化，这里不可调用
							EXPORT jd_asm_systick_init
								CPSID i ;关中断
								
								LDR R0, =JD_SYSTICK_CTRL ; 加载JD_SYSTICK_CTRL的地址
								MOV R1, #0
								STR R1, [R0] ; 先停止SysTick，以防意外产生异常请求
								
								LDR R1, =0x3FF ; 让SysTick每1024周期计完一次。
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

							
jd_asm_pendsv_handler   	PROC	;切换上下文
							EXPORT  jd_asm_pendsv_handler 
								CPSID i ;关中断
								
								MOV R0,SP
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
								MOV SP,R0
								
								;ORR LR,#0XD ;	切换到用户栈指针
								CPSIE i ;开中断
								BX LR
							ENDP
	;防止编译器报警
	NOP
	END