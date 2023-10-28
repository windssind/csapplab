movq 0x51,%rdi # 将string注入到test的代码区中
pushq $0x4018fa
ret # todo:将这些转化为字节吗指令，然后再将string填入缓冲区中
