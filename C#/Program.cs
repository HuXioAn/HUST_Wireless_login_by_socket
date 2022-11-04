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
*/


namespace HUSTwireless{

    public class account{
        public string id{set; get;}
        public string password{set; get;}
        public bool encrypt{set; get;} = false;
        public TimeSpan? availableMoment{set; get;} = new TimeSpan(0,0,0);
        public TimeSpan? availableSpan{set; get;} = new TimeSpan(24,0,0);

        public bool isAvailable(){
            var time = DateTime.Now - DateTime.Today;
            if((time > availableMoment) && ((time - availableMoment) < availableSpan))return true;
            else return false;
        }
    }
    public class client{

        static public int commandLineHandler(bool logout, string id, string pwd, string redirectHost, int redirectPort, string loginHost, int loginPort){
            if(logout == true){
                return client.logout(loginHost,loginPort);
            }else{
                WriteLine($"[*]Going to login with:\n    Host:{loginHost}:{loginPort}\n    Redirect:{redirectHost}:{redirectPort}\n    ID:{id}");
                string redirectRequestStr = $"GET / HTTP/1.1\r\nHost: {redirectHost}:{redirectPort}\r\n"+
                "User-Agent: C Socket\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"+
                "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\nAccept-Encoding: gzip, deflate\r\n"+
                "Connection: keep-alive\r\nUpgrade-Insecure-Requests: 1\r\n\r\n";
            
                string? queryStr = infoRequest(redirectHost,redirectPort,redirectRequestStr);
                if(queryStr != null){
                    if(0 == login(loginHost,loginPort,queryStr,id,pwd))return 0;
                }
                return -1;
            }
        }
        


        static public int Main(string[] arg){

            var optionLogout = new Option<bool>(name: "--logout",description: "登出本机或上游设备上的认证",getDefaultValue: ()=> false);
            var optionUser = new Option<string>("-u","账户");
            var optionPwd = new Option<string>("-p","认证密码");
            var optionRhost = new Option<string>(name: "--redirect_host",description: "重定向服务地址", getDefaultValue: ()=>"123.123.123.123");
            var optionRport = new Option<int>(name: "--redirect_port",description: "重定向服务端口",getDefaultValue: ()=>80);
            var optionLhost = new Option<string>(name: "--login_host",description: "认证服务地址",getDefaultValue: ()=>"172.18.18.60");
            var optionLport = new Option<int>(name: "--login_port",description: "认证服务端口",getDefaultValue: ()=> 8080);

            var rootCommand = new RootCommand{
                optionLogout,
                optionUser,
                optionPwd,
                optionRhost,
                optionRport,
                optionLhost,
                optionLport
            };

            rootCommand.Description = "华科校园网web认证程序";
            rootCommand.TreatUnmatchedTokensAsErrors = true;

            rootCommand.SetHandler((logout, id, pwd, redirectHost, redirectPort, loginHost, loginPort)=>
                {commandLineHandler(logout, id, pwd, redirectHost, redirectPort, loginHost, loginPort);},
                optionLogout,
                optionUser,
                optionPwd,
                optionRhost,
                optionRport,
                optionLhost,
                optionLport
                );

            rootCommand.Invoke(arg);

            return 0;
        }

        static public Socket? createSocket(string ip, int port){

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

        static public string? infoRequest(string ip, int port ,string requestStr){
            byte[] response = new byte[2048];
            Socket? querySocket = createSocket(ip,port);
            if(querySocket == null)return null;
            
            WriteLine("[*]requesting redirection :");

            try{
                WriteLine("[*]Sending request...");
                byte[] requestByte = Encoding.ASCII.GetBytes(requestStr);
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

        static public int login(string ip, int port, string queryStr, string id, string pwd){
            byte[] response = new byte[2048];
            WriteLine("[*]Trying to login");
            Socket? loginSocket = createSocket(ip,port);
            if(loginSocket == null)return -1;

            queryStr = queryStr.Replace("=","%253D").Replace("&","%2526");

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

        static public int logout(string ip, int port){
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

        static public int logoutWithUserIndex(string ip, int port, string userIndex){
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
