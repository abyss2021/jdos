
#### 介绍
jdos-简单OS！
玩单片机到现在已有5 6个年头了，学了很多知识，见识过很多大佬的代码，但心里一直挂念着要做一个简单的RTOS，于是便有了这个工程。

#### 软件架构
只有任务创建删除，系统每1ms切换任务，暂时没用优先级等其他功能。
![输入图片说明](https://foruda.gitee.com/images/1726026645705536659/dbdd12c6_8205780.png "9ce12694a0930e38b169a15c6dec2b8.png")


#### 使用说明
目前只有KEIL下的工程，KEIL工程通过Stm32CubeMX生成的最小工程，没用添加其他功能，使用的单片机为stm32f103rct6，移植仅仅只需要修改几行代码，说明如下：
1.jdos文件
jdos非常简单，只有3个文件：jdos.c、jdos.h、jdos.s，将这3个文件加入到自己的工程当中。
![输入图片说明](https://foruda.gitee.com/images/1726031536940488956/2dcfcdb6_8205780.png "在这里输入图片标题")

2.修改启动文件
修改堆大小，按需修改。
![输入图片说明](https://foruda.gitee.com/images/1726031626855695340/b5ed2fbc_8205780.png "1074457ee661164b272144a1de152e3.png")

关闭中断，防止系统未初始化导致异常。
![输入图片说明](https://foruda.gitee.com/images/1726031729329982080/48d2ee8d_8205780.png "8efeaff06365f8a0ab9a3cc0bf4c948.png")

3.修改main文件
加入系统初始化函数。
![输入图片说明](https://foruda.gitee.com/images/1726031780589223911/564e10b4_8205780.png "cd3938d782d761bcab55d0108aa0698.png")

4.修改异常处理函数
在stm32f1xx_it.c中修改PendSV异常处理函数，在其中添加代码`jd_asm_pendsv_handler(); //切换上下文`
![输入图片说明](https://foruda.gitee.com/images/1726032055320490792/47010d47_8205780.png "58876165b1d34bb4f58c9cab86a6a2a.png")

5.设置keil
勾选Use MicroLIB。
![输入图片说明](https://foruda.gitee.com/images/1726032143415200813/37cf47df_8205780.png "85b4cd65a085f282b47238f949ab7dc.png")
