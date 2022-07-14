# HUST_Wireless_login_by_socket
使用Socket模拟浏览器进行华科校园网认证，使ESP32等也能联网。

----
使用Python3，Win10和ESP32+micropython1.14测试通过
注释很少哈，非常简短估计也不需要注释了。
**文章链接：**
https://blog.csdn.net/qq_28039135/article/details/118573714

**2021/9/6**
更新Linux下C语言版本登录程序，并为R6300v2编译运行通过可用。在CentOS8下也可用。
文章链接：
https://www.cnblogs.com/huxiaoan/p/15235849.html
C语言版本严谨了一些。。。毕竟要在路由器上一直跑。

----
2022/3/5:
添加命令行参数，通过命令行修改用户、密码、认证\重定向地址、端口。

`-h`：帮助

``` shell
 user@localhost:~$./a.out -h
[*]Usage:使用 "-u id -p password"来登录,使用"--logout"下线已登录的账户.
[*]Further usage:--redirect_host xx.xx.xx.xx --redirect_port xx --login_host xx.xx.xx.xx --login_port xx
```

使用`--redirect_host`、`--redirect_port`、`--login_host`、`--login_port`来更改相应地址与端口。

----
2022/3/6：

添加登出功能，通过`--logout`可以下线本机或上游网络设备（主机、路由器）上已经登陆的校园网账户。

----
2022/7/11:

新增C#版本，Win10、Linux测试可用。

2022/7/11:
C#版本命令行功能完成，并入主分支，请使用`-h`参数查看用法。



