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
#include <sys/socket.h>
#include <unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>

int info_request(char*,char*,int,char*);
int login(char*,int,char*,char*,char*);


char redirect_request_str[]="GET / HTTP/1.1\r\nHost: 123.123.123.123\r\nUser-Agent: Python Socket\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nAccept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\nAccept-Encoding: gzip, deflate\r\nConnection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\n\r\n";
char redirect_host[]="123.123.123.123";
int redirect_port=80;

char login_host[]="172.18.18.60";
int login_port=8080;

char id[]="";
char pwd[]="";


int main(){

    char querystr[1024]={0};

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


    exit(1);
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
    
    if(connect(socket_desc,(struct sockaddr *)&redirect,sizeof(redirect))<0){
        printf("[!]Error Connecting\n");
        return 0;

    }else {
        printf("[*]Connected to redirection server successfully\n");
    }



    printf("[*]requesting redirection : \r\n");
    while(1){
        printf("[*]Sending request...\n");
        if(send(socket_desc,redirect_request_str,strlen(redirect_request_str),0)<0){
            printf("[!]Error sending\n");
            continue;
        }
        //接收
        if(recv(socket_desc,response,1024,0)<0){
            printf("[!]Error receiving\n");
            continue;
        }

        //处理报文获得querystr
        if(NULL==strstr(response,"wlanuserip")){
            flag++;
            if(flag==3){printf("[!]Trouble requesting querystr\n");close(socket_desc);return 0;}
        }
        else{

            *(strstr(response,"\'</script>"))='\0';
            
            strcpy(querystr,strstr(response,"wlanuserip"));
            if(strlen(querystr)>10){
                printf("[*]QueryString is as below:\r\n%s\n",querystr);
                close(socket_desc);
                return 1;
            }else{
                flag++;
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
    
    if(connect(socket_desc,(struct sockaddr *)&login,sizeof(login))<0){
        printf("[!]Error Connecting\n");
        return 0;

    }else {
        printf("[*]Connected to authenticate server successfully\n");
    }

    //连接成功，下面生成请求
    //先把querystr里的等号和与符号变成两次url格式，再根据content的长度修改headers里的长度。
    //最后拼起来
    char login_str[1024]={0},content[1024]={0};

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
        "Host: 172.18.18.60:8080\r\nUser-Agent: Python Socket\r\nAccept: */*\r\n"
        "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"
        "Accept-Encoding: gzip, deflate\r\nContent-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"
        "Content-Length: %d\r\nOrigin: http://172.18.18.60:8080\r\nConnection: keep-alive\r\n\r\n%s",\
        strlen(content),content);

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




