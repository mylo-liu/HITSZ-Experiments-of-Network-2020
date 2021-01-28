# HITSZ-Experiments-of-Network-2020
哈工大深圳-计算机网络-实验

## Experiment of ethernet-arp-ip-icmp-udp
在代码框架中补全，注意修改ip地址，环境是Linux

udp实验：  
搭建实验自测环境： 首先将 VS Code 与 Ubuntu 虚拟机连接：几个需要注意的地方：    
（1）注意查看 控制面板\网络和 Internet\网络连接：VMware Virtual Ethernet Adapter for VMnet8 属性中的 ip 地址是否正确，通常情况下重启电脑后 由于虚拟机没有运行，ip 地址是有问题的。   
（2）注意要进入 root 模式才能调试程序：操作包括设置 root 用户密码；用 vim 修改/etc/ssh/sshd_config 文件，将 PermitRootLogin 的值改为 yes；   
程序文件 config.h 中的 ip 地址的网络号需要与虚拟机相同，主机号不同且不能冲突。  

## Experiment of tcp
多线程文件传输：
服务器采用多线程的方式，用 thread 创建任务，能够实现多客户端的连接服务。
客户端连接成功，接收客户端的请求信息，客户端能够从服务器下载指定文件，客户端也能上传本地文件到服务器。
程序中使用socket套接字  
多线程服务器要打印线程号  

编译：   
gcc server.c -o server -lpthread   
gcc client.c -o client  

执⾏：  
./server   
./client  
