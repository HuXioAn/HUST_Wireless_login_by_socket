using System;
using static System.Console;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;

/*
Author:胡小安
Date:2022年7月14日
华科校园网（锐捷）Web认证程序-C#版
请使用"-h"参数获取使用方法
最后修改：2022年11月6日

2022年11月6日：
登入登出改为两个子命令，请使用 login -h 与 logout -h 查看两者的提示 

2023年7月1日：
准备针对强制认证进行更新，实现RSA加密。
由认证界面前端JS源码可知，加密流程大致如下
1. 从QueryStr中提取本机mac
2. 构造字符串‘passwordMac’，其内容为"密码>mac"
3. 对passwordMac进行加密
4. 将加密后的字符串作为password字段进行认证

而加密流程如下：
1. 将passwordMac翻转，即“12345>abcd”翻转后得到“dcba>54321”
2. 获取RSA公钥
3. 加密翻转后的字符串，并返回结果


*/


namespace HUSTwireless{

    public class authAccount{
        public string id{set; get;}
        public string password{set; get;}
        public bool encrypt{set; get;}
        TimeSpan availableMoment = new TimeSpan(0,0,0);
        TimeSpan availableSpan = new TimeSpan(24,0,0);

        public string availableTime{
            set{
                TimeSpan.TryParse(value, out availableMoment);
            }
        }
        public string? Span{
            set{
                TimeSpan.TryParse(value, out availableSpan);
            }
        }


        public bool isAvailable(){
            var time = DateTime.Now - DateTime.Today;
            if((time > availableMoment) && ((time - availableMoment) < availableSpan))return true;
            else return false;
        }
    }

    public class authServer{
        public string redirectHost{set; get;}
        public int redirectPort{set; get;}
        public string loginHost{set; get;}
        public int loginPort{set; get;}

        string redirectRequestStrTemplate = "GET / HTTP/1.1\r\nHost: {redirectHost}:{redirectPort}\r\n"+
                "User-Agent: C Socket\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"+
                "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\nAccept-Encoding: gzip, deflate\r\n"+
                "Connection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\n\r\n";

        public string redirectRequestStr{
            get{
                return redirectRequestStrTemplate
                .Replace("{redirectHost}", redirectHost)
                .Replace("{redirectPort}", redirectPort.ToString());
            }
        }

        public string? queryStr;

    }
    public class client{
        public static authServer? server = new authServer();
        public static authAccount[]? accounts = new authAccount[1];


        static int commandLineHandlerLogin(string accountConfig, string serverConfig,
                                            string id, string pwd, string redirectHost, 
                                            int redirectPort, string loginHost, int loginPort){
            
        
            if(!string.IsNullOrWhiteSpace(serverConfig)){
                try{
                    var serverText = File.ReadAllText(serverConfig);
                    server = JsonSerializer.Deserialize<authServer>(serverText);
                    if(server == null){
                        throw new Exception("Unable to find server info.");
                    }
                }catch(Exception e){
                    WriteLine("[!]Error parsing JSON file: {0}, {1}",serverConfig,e.Message);
                    return -1;
                }
                
            }else{
                server!.redirectHost = redirectHost;
                server.redirectPort = redirectPort;
                server.loginHost = loginHost;
                server.loginPort = loginPort;
            }

            if(!string.IsNullOrWhiteSpace(accountConfig)){
                try{
                    var accountText = File.ReadAllText(accountConfig);
                    accounts = JsonSerializer.Deserialize<authAccount[]>(accountText)!;
                    if(accounts == null || accounts.Length < 1){
                        throw new Exception("Unable to find account info.");
                    }
                }catch(Exception e){
                    WriteLine("[!]Error parsing JSON file: {0}, {1}",accountConfig,e.Message);
                    return -1;
                }
                
            }else{
                accounts![0] = new authAccount();
                accounts[0].id = id;
                accounts[0].password = pwd;
                accounts[0].encrypt = false;
            }

            WriteLine($"[*]Going to login with:\n    Host:{server!.loginHost}:{server.loginPort}\n    Redirect:{server.redirectHost}:{server.redirectPort}\n ");
            server.queryStr = infoRequest();

            foreach(var account in accounts!){
                if(account!.isAvailable()){
                    if(0 == login(account))Environment.Exit(0);
                    else{
                        WriteLine($"[!]Failed to login with: {account.id}.");
                    }
                }
            }
            return -1;
            
        }
        


        static public int Main(string[] arg){

            var optionLogout = new Option<bool>(name: "--logout",description: "登出本机或上游设备上的认证",getDefaultValue: ()=> false);
            var optionAConfig = new Option<string>("--ac", "指定账户信息配置文件");
            var optionSConfig = new Option<string>("--sc", "指定认证服务器信息配置文件");
            var optionUser = new Option<string>("-u","账户");
            var optionPwd = new Option<string>("-p","认证密码");
            var optionRhost = new Option<string>(name: "--redirect_host",description: "重定向服务地址", getDefaultValue: ()=>"123.123.123.123");
            var optionRport = new Option<int>(name: "--redirect_port",description: "重定向服务端口",getDefaultValue: ()=>80);
            var optionLhost = new Option<string>(name: "--login_host",description: "认证服务地址",getDefaultValue: ()=>"172.18.18.60");
            var optionLport = new Option<int>(name: "--login_port",description: "认证服务端口",getDefaultValue: ()=> 8080);
            
            

            var rootCommand = new RootCommand("校园网认证");
            rootCommand.TreatUnmatchedTokensAsErrors = true;

            var logoutCommand = new Command("logout", "登出本机或上游设备上的认证");
            rootCommand.AddCommand(logoutCommand);

            logoutCommand.AddOption(optionLhost);
            logoutCommand.AddOption(optionLport);

            var loginCommand = new Command("login", "进行网络认证");
            rootCommand.AddCommand(loginCommand);

            loginCommand.AddOption(optionAConfig);
            loginCommand.AddOption(optionSConfig);
            loginCommand.AddOption(optionUser);
            loginCommand.AddOption(optionPwd);
            loginCommand.AddOption(optionRhost);
            loginCommand.AddOption(optionRport);
            loginCommand.AddOption(optionLhost);
            loginCommand.AddOption(optionLport);

            logoutCommand.SetHandler(
                (host, port)=>{
                    if(0 == logout(host,port)){
                        Environment.Exit(0);
                    }else{
                        Environment.Exit(-1);
                    }
                    },
                optionLhost,optionLport
            );

            loginCommand.SetHandler(
                (accountConfig, serverConfig, id, 
                pwd, redirectHost, redirectPort, 
                loginHost, loginPort)=>{
                    commandLineHandlerLogin(
                        accountConfig, serverConfig,
                        id, pwd, redirectHost, 
                        redirectPort, loginHost, loginPort);
                },
                optionAConfig,optionSConfig,
                optionUser,optionPwd,optionRhost,
                optionRport,optionLhost,optionLport
            );

            rootCommand.InvokeAsync(arg);

            return -1;
        }

        static  Socket? createSocket(string ip, int port){

            try{
            IPAddress ipAddress = IPAddress.Parse(ip);
            IPEndPoint remoteEP = new IPEndPoint(ipAddress, port);

            Socket sender = new Socket(ipAddress.AddressFamily,SocketType.Stream, ProtocolType.Tcp);
            sender.Connect(remoteEP);
            return sender;
            }catch(Exception e){
                WriteLine($"[!]Error creating socket:{e.ToString()}");
                return null;
            }
            
        }

        static  string? infoRequest(){
            byte[] response = new byte[2048];
            Socket? querySocket = createSocket(server.redirectHost,server.redirectPort);
            if(querySocket == null)return null;
            
            WriteLine("[*]requesting redirection :");

            try{
                WriteLine("[*]Sending request...");
                byte[] requestByte = Encoding.ASCII.GetBytes(server.redirectRequestStr);
                querySocket.Send(requestByte);

                int byteReceive = querySocket.Receive(response);
                string queryString = Encoding.ASCII.GetString(response, 0, byteReceive);

                if( queryString.Contains("wlanuserip")){
                    //正确内容
                    queryString = queryString.Substring(queryString.IndexOf("wlanuserip"));
                    string result = queryString.Remove(queryString.IndexOf("\'</script>"));

                    if(result.Length > 10){
                        WriteLine($"[*]QueryString is as below:\r\n{result}");
                        querySocket.Shutdown(SocketShutdown.Both);
                        querySocket.Close();
                        return result;
                    }
                }
                throw new Exception("[!]Error requesting queryString");
            }catch(Exception e){
                querySocket.Shutdown(SocketShutdown.Both);
                querySocket.Close();
                WriteLine($"[!]Error requesting queryString:{e.ToString()}");
                return null;
            }


        }

        static  int login(authAccount account){

            if(server.queryStr == null)return -1;
            
            var ip = server.loginHost;
            var port = server.loginPort;
            var id = account.id;
            var pwd = account.password;

            byte[] response = new byte[2048];
            WriteLine("[*]Trying to login");
            Socket? loginSocket = createSocket(ip,port);
            if(loginSocket == null)return -1;

            string queryStr = server.queryStr.Replace("=","%253D").Replace("&","%2526");

            string content = $"userId={id}&password={pwd}&service=&queryString={queryStr}"
                +"&operatorPwd=&operatorUserId=&validcode=&passwordEncrypt=false";

            string loginStr = "POST /eportal/InterFace.do?method=login HTTP/1.1\r\n"+
                       $"Host: {ip}:{port}\r\nUser-Agent: C Socket\r\nAccept: */*\r\n"+
                       "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"+
                       "Accept-Encoding: gzip, deflate\r\nContent-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"+
                       $"Content-Length: {content.Length}\r\nOrigin: http://{ip}:{port}\r\nConnection: keep-alive\r\n\r\n{content}";
            
            WriteLine($"[*]Login request:\n{loginStr}\n[*]Sending request...");            
            
            try{
                loginSocket.Send(Encoding.ASCII.GetBytes(loginStr));
                int byteReceive = loginSocket.Receive(response);
                string loginResult = Encoding.ASCII.GetString(response, 0, byteReceive);
            
                if(loginResult.Contains("success")){
                    WriteLine("[*]login Successfully");
                    return 0;
                }
                throw new Exception("Error communicating with server");
                
            }catch(Exception e){
                loginSocket.Shutdown(SocketShutdown.Both);
                loginSocket.Close();
                WriteLine($"[*]Error logining:{e.ToString()}");
                return -1;
            }

        }

        static  int logout(string ip, int port){
            byte[] response = new byte[2048];
            string userIndex;
            Socket? logoutSocket = createSocket(ip,port);
            if(logoutSocket == null)return -1;

            string logoutStr = "GET /eportal/redirectortosuccess.jsp HTTP/1.1\r\n"+
                         $"Host: {ip}:{port}\r\nUser-Agent: C Socket\r\n"+
                         "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"+
                         "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"+
                         "Accept-Encoding: gzip, deflate\r\n"+
                         "Connection: keep-alive\r\n\r\n";

            try{
            logoutSocket.Send(Encoding.ASCII.GetBytes(logoutStr));
            int byteReceive = logoutSocket.Receive(response);
            string logoutResult = Encoding.ASCII.GetString(response, 0, byteReceive);

            if(logoutResult.Contains("userIndex")){
                logoutResult = logoutResult.Substring(logoutResult.IndexOf("userIndex="));
                userIndex = logoutResult.Remove(logoutResult.IndexOf("\r\n"));

                if(userIndex.Length > 256)throw new Exception("Incorrect response");
                logoutSocket.Shutdown(SocketShutdown.Both);
                logoutSocket.Close();

            }else{
                WriteLine("[*]Error requesting userIndex from {0}:{1}",ip,port);
                throw new Exception("Incorrect response");
            }

            }catch(Exception e){
                logoutSocket.Shutdown(SocketShutdown.Both);
                logoutSocket.Close();
                WriteLine($"[!]Error requesting userIndex:{e.ToString()}");
                return -1;
            }

            return logoutWithUserIndex(ip,port,userIndex);


            
        }

        static  int logoutWithUserIndex(string ip, int port, string userIndex){
            byte[] response = new byte[2048];
            Socket? logoutSocket = createSocket(ip,port);
            if(logoutSocket == null)return -1;

            string logoutStr = "POST /eportal/InterFace.do?method=logout HTTP/1.1\r\n"+
                        $"Host: {ip}:{port}\r\nUser-Agent: C Socket\r\n"+
                        "Accept: */*\r\n"+
                        "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"+
                        "Accept-Encoding: gzip, deflate\r\n"+
                        "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"+
                        $"Content-Length: {userIndex.Length}\r\n"+
                        $"Origin: http://{ip}:{port}\r\n"+
                        $"Connection: keep-alive\r\n\r\n{userIndex}";

            try{
                logoutSocket.Send(Encoding.ASCII.GetBytes(logoutStr));
                int byteReceive = logoutSocket.Receive(response);
                string logoutResult = Encoding.ASCII.GetString(response, 0, byteReceive);

                if(logoutResult.Contains("success")){
                    WriteLine("[*]Logout Successfully!");
                    logoutSocket.Shutdown(SocketShutdown.Both);
                    logoutSocket.Close();
                    return 0;
                }else{
                    WriteLine("[!]Failed to logout");
                    throw new Exception("unexpected response");
                }
            }catch(Exception e){
                logoutSocket.Shutdown(SocketShutdown.Both);
                logoutSocket.Close();
                WriteLine($"[!]Error logging out: {e.ToString()}");
                return -1;
            }
        } 
    }

}
