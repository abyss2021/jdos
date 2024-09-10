					AREA |.text|, CODE, READONLY, ALIGN=3
jd_hw_task_switch   PROC
					EXPORT  jd_hw_task_switch
					;保护现场，将堆栈指针传出
					MRS R2,PSP
					LDMFD R2!,{R4-R11}
					;LDR R0,[R0]
					STR R2,[R0]
					;取下一个任务的堆栈指针,恢复现场
					LDR R1,[R1]
					STMFD R1!,{R4-R11}
					MSR PSP,R1
					BX LR
					ENDP

jd_hw_task_first_switch PROC
						EXPORT  jd_hw_task_first_switch
						LDR R0,[R0]
						STMFD R0!,{R4-R11}
						MSR PSP,R0
						BX LR
						ENDP