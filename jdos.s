					AREA |.text|, CODE, READONLY, ALIGN=3
jd_asm_task_switch   PROC
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

jd_asm_task_first_switch PROC
						EXPORT  jd_asm_task_first_switch
						MOV R1,R0;保存堆栈指针地址
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