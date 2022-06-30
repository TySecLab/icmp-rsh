# icmp-rsh
icmp-rsh 是一个使用 icmp 协议直连的 remote shell 工具
+ 使用 raw socket icmp 协议直连
+ 数据分块传输，支持 netstat -ano 这种回显数据量大的命令
+ 数据通信加密
+ 支持交互式命令
+ 支持 Windows 系统和常用的 linux 发行版：CentOS、Ubuntu、kali
+ 支持多个客户端访问服务端

## Todo
- [ ] 反弹模式
- [ ] 加入 FEC 和 ARQ 协议，实现可靠传输

## Usege
直连模式：
> ./server  

> ./client [dest-ip]
