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

使用`-h`参数查看用法。
