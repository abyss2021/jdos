JD_ICRS EQU 0XE000ED04		;中断控制及状态寄存器
JD_PRI_14 EQU 0XE000ED23	;PendSV的优先级设置寄存器
						AREA |.text|, CODE, READONLY, ALIGN=3
jd_asm_task_switch   	PROC		;线程切换，早期测试版本
						EXPORT  jd_asm_task_switch
						;保护现场，将堆栈指针传出
						PUSH {R0-R12,LR}
						MRS R2,PSR
						PUSH {R2}
						MOV R2,SP
						STR R2,[R0]
						
						;取下一个任务的堆栈指针,恢复现场
						LDR R1,[R1]
						MOV SP,R1
						POP {R2}
						MSR PSR,R2
						POP {R0-R12,LR}
						BX LR
						ENDP

jd_asm_task_first_switch 	PROC	;进入main线程，早期测试版本
							EXPORT  jd_asm_task_first_switch
							;设置PendSV的优先级为最低优先级
							LDR R1,=JD_PRI_14
							LDR R2,=0X000000FF
							STR R2,[R1]

							;保存堆栈指针地址
							MOV R1,R0
							LDR R0,[R0]
							LDMFD  R0!,{R4-R11}
							LDMFD  R0!,{R2-R5}
							LDMFD  R0!,{R12}
							LDMFD  R0!,{LR}
							LDMFD  R0!,{R2}
							LDMFD  R0!,{R2}
							STR R0,[R1]
							BX LR
							ENDP

jd_asm_pendsv_putup 	PROC	;触发PendSV异常
						EXPORT jd_asm_pendsv_putup
						LDR R0,=JD_ICRS
						LDR R1,=0X10000000
						STR R1,[R0]
						BX LR
						ENDP

PendSV_Handler   	PROC	;切换上下文
					EXPORT  PendSV_Handler 
					MOV R0,#01
					ENDP
