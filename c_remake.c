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
#include<netinet/in.h>
#include<arpa/inet.h>
#include <netinet/tcp.h>

#define HELP_STR "[*]Usage: -u Youraccount -p Yourpassword\n\
[*]Further usage:--redirect_host xx.xx.xx.xx --redirect_port xx --login_host xx.xx.xx.xx --login_port xx\n"

int info_request(char*,char*,int,char*);
int login(char*,int,char*,char*,char*);


char redirect_host[20]="123.123.123.123";
int redirect_port=80;

char login_host[20]="172.18.18.60";
int login_port=8080;

char id[20]="";
char pwd[40]="";


int main(int argc,char ** argv){
    //使用getopt解析参数
    int opt;
    char * optstr = "u:p:h";//user\pwd\help
    int long_option_flag=0;


    struct option long_option_list[]={
        {"login_host",required_argument,&long_option_flag,1},
        {"login_port",required_argument,&long_option_flag,2},
        {"redirect_host",required_argument,&long_option_flag,3},
        {"redirect_port",required_argument,&long_option_flag,4},
        {"logout",no_argument,&long_option_flag,10}
    };
    //参数解析
    while((opt = getopt_long(argc,argv,optstr,long_option_list,NULL)) != -1){
        switch (opt)
        {
        case 'u':
            if(strlen(optarg)<20){//避免不合理参数
                strcpy(id,optarg);
            }else{
                printf("[!]Is your ID really that long??? \n");
                exit(-1);
            }
            //printf("[*]Using ID: %s\n",id);
            break;
        case 'p':
            if(strlen(optarg)<40){
                strcpy(pwd,optarg);
            }else{
                printf("[!]Long long password, good good memory!\n");
                exit(-1);
            }
            //printf("[*]Using password: %s\n",pwd);
            break;
        case 'h':
            printf("%s\n",HELP_STR);
            exit(0);
            break;
        case 0://long_optin
            switch (long_option_flag)
            {
            case 1:
                if(strlen(optarg)<20){
                    strcpy(login_host,optarg);
                }else{
                    printf("[!]Please check the IP address!\n");
                    exit(-1);
                }
                break;
            case 2:
                login_port=atoi(optarg);
                break;
            case 3:
                if(strlen(optarg)<20){
                    strcpy(redirect_host,optarg);
                }else{
                    printf("[!]Please check the IP address!\n");
                    exit(-1);
                }
                break;
            case 4:
                redirect_port=atoi(optarg);
                break;
            case 10:
                //TODO:LOGOUT
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



    //general description output
    printf("[*]Going to login with:\n    Host:%s:%d\n    Redirect:%s:%d\n    ID:%s\n",\
    login_host,login_port,redirect_host,redirect_port,id);



    char querystr[1024]={0};
    char redirect_request_str[512]={0};

    sprintf(redirect_request_str,"GET / HTTP/1.1\r\nHost: %s:%d\r\n\
    User-Agent: C Socket\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n\
    Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\nAccept-Encoding: gzip, deflate\r\n\
    Connection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\n\r\n",redirect_host,redirect_port);

    if(info_request(querystr,redirect_host,redirect_port,redirect_request_str));
    else {
        printf("[!]Error requesting QueryStr\n");
        exit(1);
    }

    if(login(login_host,login_port,querystr,id,pwd));
    else{
        printf("[!]Error authentication\n");
        exit(1);
    }


    exit(0);
}


int info_request(char* querystr,char* redirect_host,int redirect_port,char* redirect_request_str){
//获取querystr
    int flag=0;
    struct sockaddr_in redirect;
    char response[1024]={0};

    //创建socket
    int socket_desc=socket(AF_INET,SOCK_STREAM,0);
    if(socket_desc==-1){
        printf("[!]Error creating socket\n");
        return 0;

    }

    redirect.sin_addr.s_addr=inet_addr(redirect_host);
    redirect.sin_family=AF_INET;
    redirect.sin_port=htons(redirect_port);
   //连接
    //设置连接超时应对已经连接的情况
    int syncnt = 2;
    setsockopt(socket_desc, IPPROTO_TCP, TCP_SYNCNT, &syncnt, sizeof(syncnt));

    if(connect(socket_desc,(struct sockaddr *)&redirect,sizeof(redirect))<0){
        printf("[!]Error Connecting redirection, chances are that you've already been online.\n");
        return 0;

    }else {
        printf("[*]Connected to redirection server successfully\n");
    }

    struct timeval timeout = {3,0}; 

//设置接收超时
    setsockopt(socket_desc,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));



    printf("[*]requesting redirection : \r\n");
    while(flag<3){
        flag++;
        printf("[*]Sending request...\n");
        if(send(socket_desc,redirect_request_str,strlen(redirect_request_str),0)<0){
            printf("[!]Error sending\n");
            continue;
        }
        //接收
        if(recv(socket_desc,response,1024,0)<=0){
            printf("[!]Error receiving\n");
            continue;
        }

        //处理报文获得querystr
        if(NULL==strstr(response,"wlanuserip")){
            if(flag==3){printf("[!]Trouble requesting querystr\n");close(socket_desc);return 0;}
        }
        else{

            *(strstr(response,"\'</script>"))='\0';
            strcpy(querystr,strstr(response,"wlanuserip"));
            if(strlen(querystr)>10){
                printf("[*]QueryString is as below:\r\n%s\n",querystr);
                close(socket_desc);
                return 1;
            }
        }

    }
    
    return 0;




}


int login(char* login_host,int login_port,char* querystr,char* id,char* pwd){

    printf("[*]Trying to login\n");

    struct sockaddr_in login;
    char response[1024]={0};

    //创建socket
    int socket_desc=socket(AF_INET,SOCK_STREAM,0);
    if(socket_desc==-1){
        printf("[!]Error creating socket\n");
        return 0;

    }

    login.sin_addr.s_addr=inet_addr(login_host);
    login.sin_family=AF_INET;
    login.sin_port=htons(login_port);
   //连接
    
    int syncnt = 2;
    setsockopt(socket_desc, IPPROTO_TCP, TCP_SYNCNT, &syncnt, sizeof(syncnt));

    if(connect(socket_desc,(struct sockaddr *)&login,sizeof(login))<0){
        printf("[!]Error Connecting\n");
        return 0;

    }else {
        printf("[*]Connected to authenticate server successfully\n");
    }

    //连接成功，下面生成请求
    //先把querystr里的等号和与符号变成两次url格式，再根据content的长度修改headers里的长度。
    //最后拼起来
    char login_str[2048]={0},content[1024]={0};

    char* place=NULL;
    while(place=strstr(querystr,"=")){
        //临时借用login_str
        strcpy(login_str,place+1);
        strcpy(place,"%253D");
        strcpy(place+5,login_str);
    }
    while(place=strstr(querystr,"&")){
        //临时借用login_str
        strcpy(login_str,place+1);
        strcpy(place,"%2526");
        strcpy(place+5,login_str);
    }
    //生成content
    sprintf(content,"userId=%s&password=%s&service=&queryString=%s"
            "&operatorPwd=&operatorUserId=&validcode=&passwordEncrypt=false"\
            ,id,pwd,querystr);

    //替换header里的长度
    sprintf(login_str,"POST /eportal/InterFace.do?method=login HTTP/1.1\r\n"
        "Host: %s:%d\r\nUser-Agent: C Socket\r\nAccept: */*\r\n"
        "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"
        "Accept-Encoding: gzip, deflate\r\nContent-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"
        "Content-Length: %d\r\nOrigin: http://%s:%d\r\nConnection: keep-alive\r\n\r\n%s",\
        login_host,login_port,(int)strlen(content),login_host,login_port,content);

    printf("[*]Login request:\n%s\n",login_str);

    printf("[*]Sending request...\n");
        if(send(socket_desc,login_str,strlen(login_str),0)<0){
            printf("[!]Error sending\n");
            return 0;
        }
        //接收
        if(recv(socket_desc,response,1024,0)<0){
            printf("[!]Error receiving\n");
            return 0;
        }
        close(socket_desc);
        if(strstr(response,"success")){
            printf("[*]login Successfully \n");
            return 1;
        }
        
    return 0;


}




