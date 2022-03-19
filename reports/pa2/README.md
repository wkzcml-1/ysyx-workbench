## PA2:简单复杂的机器: 冯诺依曼计算机系统

Date:	2022/3/9

#### PA2.1

##### 核心处理思想

* 位的处理：在使用`load`指令取数时会涉及取出的数位的处理，由于RISCV64架构中寄存器是64位的，但是取出的数可能是32位，8位等，而跳转比较指令有需要比较有符号数；由于PA是用程序模拟ISA，所以我采用的方式是将低于64位的数扩展成64位，最后若要进行有符号数比较再进行强转
* 指令方面：根据指令类型设计不同的获取立即数，寄存器号的方式，最后再对每个指令进行更细节的操作

##### 测试DEBUG记录

* sum:	B-type指令读立即数处理错误一位(这个BUG耽误我特别多时间，不堪回首的回忆🤦‍♀️)
* wanshu：补充了`slli`、`srli`、`srai`指令
* unalign：补充了`lbu`、`lhu`指令，此处为了与`lb`之类有符号数指令作对比，专门定义一个新宏`Sr`表示有符号读数
* mov-c：直接pass
* fact：出现BUG，经查是将`mulw`指令识别为`addw`区别位数取少了，此外本次实现了`mulw`，`divw`，`divuw`、`remuw`指令；此外还做了一定数字处理优化，将数字字节处理符号处理集成到一个函数中
* hello-str：有BUG，幸好在文档上看到了提示信息，不然又要de汇编代码bug
* div：直接pass
* shuixianhua：直接pass
* prime：直接pass
* goldbach：直接pass
* fib：直接pass
* movsx：BUG，完成`slliw`、`srliw`、`sraiw`指令；发现一个严重BUG，把`sra`和`srl`逻辑搞反了
* bubble-sort：有BUG，检查后发现是因为64位的寄存器数，所以移位指令立即数偏移应该是6位，而我只给了5位
* select-sort：直接pass
* switch：直接pass
* if-else：直接pass
* pascal：直接pass
* leap-year：直接pass
* mul-longlong：BUG，补充了`rv32m`和`rv64i`
* quick-sort：直接pass
* shift：BUG，发现在处理32位字移位有一定逻辑错误
* to-lower-case：BUG，漏掉一个`BLTU`指令
* recursion：直接pass
* bit：直接pass
* load-store：直接pass
* max：直接pass
* add：直接pass
* min3：直接pass
* sub-longlong：直接pass
* add-longlong：直接pass
* 

