/*
步骤分析：
一. 获取querystr：

1.建立socket，发送请求连接重定向网址
2.获取返回内容，截取querystr

二. 认证

1.创建socket
2.连接认证地址
3.根据账户密码和querystr生成请求内容
4.获取返回报文，查看成功与否
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define HELP_STR "[*]Usage:使用 \"-u id -p password\"来登录,使用\"--logout\"下线已登录的账户.\
\n[*]Further usage:--redirect_host xx.xx.xx.xx --redirect_port xx --login_host xx.xx.xx.xx --login_port xx"

int info_request(char *, const char *, const int, char *);
int login(const char *, const int, char *, char *, char *);
int logout(void);
int logout_with_userIndex(char *userIndex);
int create_socket(const char *host, const int port, int retry, const char *title);

char redirect_host[20] = "123.123.123.123";
int redirect_port = 80;

char login_host[20] = "172.18.18.60";
int login_port = 8080;

char id[20] = "";
char pwd[40] = "";

int main(int argc, char **argv)
{
    //使用getopt解析参数
    int opt;
    char *optstr = "u:p:h"; // user\pwd\help
    int long_option_flag = 0;

    struct option long_option_list[] = {
        {"login_host", required_argument, &long_option_flag, 1},
        {"login_port", required_argument, &long_option_flag, 2},
        {"redirect_host", required_argument, &long_option_flag, 3},
        {"redirect_port", required_argument, &long_option_flag, 4},
        {"logout", no_argument, &long_option_flag, 10}};
    //参数解析
    while ((opt = getopt_long(argc, argv, optstr, long_option_list, NULL)) != -1)
    {
        switch (opt)
        {
        case 'u':
            if (strlen(optarg) < 20)
            { //避免不合理参数
                strcpy(id, optarg);
            }
            else
            {
                printf("[!]Is your ID really that long??? \n");
                exit(-1);
            }
            // printf("[*]Using ID: %s\n",id);
            break;
        case 'p':
            if (strlen(optarg) < 40)
            {
                strcpy(pwd, optarg);
            }
            else
            {
                printf("[!]Long long password, good good memory!\n");
                exit(-1);
            }
            // printf("[*]Using password: %s\n",pwd);
            break;
        case 'h':
            printf("%s\n", HELP_STR);
            exit(0);
            break;
        case 0: // long_optin
            switch (long_option_flag)
            {
            case 1:
                if (strlen(optarg) < 20)
                {
                    strcpy(login_host, optarg);
                }
                else
                {
                    printf("[!]Please check the IP address!\n");
                    exit(-1);
                }
                break;
            case 2:
                login_port = atoi(optarg);
                break;
            case 3:
                if (strlen(optarg) < 20)
                {
                    strcpy(redirect_host, optarg);
                }
                else
                {
                    printf("[!]Please check the IP address!\n");
                    exit(-1);
                }
                break;
            case 4:
                redirect_port = atoi(optarg);
                break;
            case 10:
                // TODO:LOGOUT
                if (0 == logout())
                    exit(0);
                else
                    exit(-1);
            default:
                printf("[!]Unsupported argument, '-h' may help you\n");
                exit(-1);
                break;
            }
            break;
        default:
            printf("[!]Unsupported argument, '-h' may help you\n");
            exit(-1);
            break;
        }
    }

    // general description output
    printf("[*]Going to login with:\n    Host:%s:%d\n    Redirect:%s:%d\n    ID:%s\n",
           login_host, login_port, redirect_host, redirect_port, id);

    char querystr[1024] = {0};
    char redirect_request_str[512] = {0};

    sprintf(redirect_request_str, "GET / HTTP/1.1\r\nHost: %s:%d\r\n\
    User-Agent: C Socket\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n\
    Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\nAccept-Encoding: gzip, deflate\r\n\
    Connection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\n\r\n",
            redirect_host, redirect_port);

    if (!info_request(querystr, redirect_host, redirect_port, redirect_request_str))
        ;
    else
    {
        printf("[!]Error requesting QueryStr\n");
        exit(1);
    }

    if (!login(login_host, login_port, querystr, id, pwd))
        ;
    else
    {
        printf("[!]Error authentication\n");
        exit(1);
    }

    exit(0);
}

int info_request(char *querystr, const char *redirect_host, const int redirect_port, char *redirect_request_str)
{
    //获取querystr
    int flag = 0;
    char response[1024] = {0};

    int socket_desc = create_socket(redirect_host, redirect_port, 3, "INFO REQUEST");
    if(-1==socket_desc)return -1;

    printf("[*]requesting redirection : \r\n");
    while (flag < 3)
    {
        flag++;
        printf("[*]Sending request...\n");
        if (send(socket_desc, redirect_request_str, strlen(redirect_request_str), 0) < 0)
        {
            printf("[!]Error sending\n");
            continue;
        }
        //接收
        if (recv(socket_desc, response, 1024, 0) <= 0)
        {
            printf("[!]Error receiving\n");
            continue;
        }

        //处理报文获得querystr
        if (NULL == strstr(response, "wlanuserip"))
        {//找不到
            if (flag == 3)
            {
                printf("[!]Trouble requesting querystr\n");
                close(socket_desc);
                return -1;
            }
        }
        else
        {

            *(strstr(response, "\'</script>")) = '\0';
            strcpy(querystr, strstr(response, "wlanuserip"));
            if (strlen(querystr) > 10)
            {
                printf("[*]QueryString is as below:\r\n%s\n", querystr);
                close(socket_desc);
                return 0;
            }
        }
    }
    close(socket_desc);
    return -1;
}

int login(const char *login_host, const int login_port, char *querystr, char *id, char *pwd)
{

    printf("[*]Trying to login\n");
    char response[1024] = {0};

    int socket_desc = create_socket(login_host, login_port, 3, "LOGIN");
    if(-1==socket_desc)return -1;

    //连接成功，下面生成请求
    //先把querystr里的等号和与符号变成两次url格式，再根据content的长度修改headers里的长度。
    //最后拼起来
    char login_str[2048] = {0}, content[1024] = {0};

    char *place = NULL;
    while (place = strstr(querystr, "="))
    {
        //临时借用login_str
        strcpy(login_str, place + 1);
        strcpy(place, "%253D");
        strcpy(place + 5, login_str);
    }
    while (place = strstr(querystr, "&"))
    {
        //临时借用login_str
        strcpy(login_str, place + 1);
        strcpy(place, "%2526");
        strcpy(place + 5, login_str);
    }
    //生成content
    sprintf(content, "userId=%s&password=%s&service=&queryString=%s"
                     "&operatorPwd=&operatorUserId=&validcode=&passwordEncrypt=false",
            id, pwd, querystr);

    //替换header里的长度
    sprintf(login_str, "POST /eportal/InterFace.do?method=login HTTP/1.1\r\n"
                       "Host: %s:%d\r\nUser-Agent: C Socket\r\nAccept: */*\r\n"
                       "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"
                       "Accept-Encoding: gzip, deflate\r\nContent-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"
                       "Content-Length: %d\r\nOrigin: http://%s:%d\r\nConnection: keep-alive\r\n\r\n%s",
            login_host, login_port, (int)strlen(content), login_host, login_port, content);

    printf("[*]Login request:\n%s\n", login_str);

    printf("[*]Sending request...\n");
    if (send(socket_desc, login_str, strlen(login_str), 0) < 0)
    {
        printf("[!]Error sending\n");
        return -1;
    }
    //接收
    if (recv(socket_desc, response, 1024, 0) < 0)
    {
        printf("[!]Error receiving\n");
        return -1;
    }
    close(socket_desc);
    if (strstr(response, "success"))
    {
        printf("[*]login Successfully \n");
        return 0;
    }

    return -1;
}

int logout(void)
{
    //访问http://login_host:login_port/eportal/redirectortosuccess.jsp获取userindex
    //如果获取到重定向地址则未登录，如果得到index继续退出。

    char state_check[1024] = {0};
    char response[1024] = {0};
    char userIndex[256] = {0};

    int socket_desc = create_socket(login_host, login_port, 3, "LOGOUT-USERINDEX");
    if(-1==socket_desc)return -1;

    //替换header里的长度
    sprintf(state_check, "GET /eportal/redirectortosuccess.jsp HTTP/1.1\r\n"
                         "Host: %s:%d\r\nUser-Agent: C Socket\r\n"
                         "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
                         "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"
                         "Accept-Encoding: gzip, deflate\r\n"
                         "Connection: keep-alive\r\n\r\n",
            login_host, login_port);

    if (send(socket_desc, state_check, strlen(state_check), 0) < 0)
    {
        printf("[!]Error sending\n");
        return -1;
    }
    //接收
    if (recv(socket_desc, response, 1024, 0) < 0)
    {
        printf("[!]Error receiving\n");
        return -1;
    }
    close(socket_desc);
    if (strstr(response, "userIndex"))
    {                                                        //获得了userIndex
        char *index_head_p = strstr(response, "userIndex="); //包含了userIndex=，方便后面
        char *index_tail_p = strstr(index_head_p, "\r\n");
        *index_tail_p = '\0';
        if ((index_tail_p - index_head_p) < 256)
        {
            strcpy(userIndex, index_head_p);
        }
        else
        {
            printf("[!]Why is your userIndex longer than 256 characters?\n");
            return -1;
        }
        //得到userIndex，准备登出
        if (logout_with_userIndex(userIndex) == 0)
            return 0;
        else
            return -1;
    }
    else
    { //未登录
        printf("[!]Failed to logout!\n");
        return -1;
    }
}

int logout_with_userIndex(char *userIndex)
{

    char logout_str[1024] = {0};
    char response[1024] = {0};

    int socket_desc = create_socket(login_host, login_port, 3, "LOGOUT");

    if(-1==socket_desc)return -1;

    //替换header里的长度
    sprintf(logout_str, "POST /eportal/InterFace.do?method=logout HTTP/1.1\r\n"
                        "Host: %s:%d\r\nUser-Agent: C Socket\r\n"
                        "Accept: */*\r\n"
                        "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"
                        "Accept-Encoding: gzip, deflate\r\n"
                        "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"
                        "Content-Length: %d\r\n"
                        "Origin: http://172.18.18.60:8080\r\n"
                        "Connection: keep-alive\r\n\r\n%s",
            login_host, login_port, (int)strlen(userIndex), userIndex);

    if (send(socket_desc, logout_str, strlen(logout_str), 0) < 0)
    {
        printf("[!]Error sending\n");
        return -1;
    }
    //接收
    if (recv(socket_desc, response, 1024, 0) < 0)
    {
        printf("[!]Error receiving\n");
        return -1;
    }
    close(socket_desc);
    if (strstr(response, "success"))
    {
        printf("[*]Logout Successfully!\n");
        return 0;
    }
    else
    {
        printf("[!]Failed to logout, due to unknown reasons...\n");
        return -1;
    }
}

int create_socket(const char *host, const int port, int retry, const char *title)
{
    struct sockaddr_in addr;
    int count = 0;

    //创建socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("[!]%s:Error creating socket\n", title);
        return -1;
    }

    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    //连接
    //设置连接超时应对已经连接的情况
    int syncnt = 2;
    setsockopt(socket_desc, IPPROTO_TCP, TCP_SYNCNT, &syncnt, sizeof(syncnt));

    struct timeval timeout = {3, 0};
    //设置接收超时
    if (-1 == setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval)))
    {
        return -1;
    }

    while (count < retry)
    {
        if (connect(socket_desc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            count++;
            printf("[!]%s:Error Connecting. %d time(s) left.\n", title, 3 - count);
        }
        else
        {
            printf("[*]%s:Connected to %s:%d successfully.\n", title, host, port);
            return socket_desc;
        }
    }
    printf("[!]%s:Error Connecting to %s:%d\n", title, host, port);
    return -1;
}
