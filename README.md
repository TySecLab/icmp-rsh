# icmp-rsh
icmp-rsh 是一个使用 icmp 协议直连的 remote shell 工具
+ 1、使用 raw socket icmp 协议直连
+ 2、数据分块传输，支持 netstat -ano 这种回显数据量大的命令
+ 3、数据通信加密
+ 4、支持交互式命令
+ 5、支持 Windows 系统和常用的 linux 发行版：CentOS、Ubuntu、kali
+ 6、支持多个客户端访问服务端

## Todo
+ icmp 反弹的模式，从服务端发起请求
+ 未来可能加入 FEC 和 ARQ 协议，在实现可靠传输的基础上，实现文件上传/下载功能
