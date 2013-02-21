// Copyright 2012.  All rights reserved.
// http://skycpp.cn
// Author: zhaolei
// E-mail: qjzl2008@163.com
#define WINDOWS
#include <string>
#include <list>

#ifdef WINDOWS
#include <winsock2.h> 
#pragma comment(lib, "WS2_32")   
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
typedef int SOCKET;
#endif

#define MAXBUFFERSIZE 1024    //maximum size of recv buffer
#define MAXMALLOCSIZE 5000000   //maximum size of content for malloc
class CRequestHttp
{
private:
	CRequestHttp(const CRequestHttp&);
	CRequestHttp& operator=(const CRequestHttp&);

public:
	CRequestHttp(void);
	~CRequestHttp(void);
/*简易调用接口,未传入接收变量，可用get_response_content获取*/
inline bool request_get(const char* url); 
inline bool request_get(const char* url,std::string& response_content);
inline bool request_post(const char* url,const char* post_content);
inline bool request_post(const char* url,const char* post_content,std::string& response_content);
//获取请求返回内容，参数为内容长度
unsigned char* get_response_content(unsigned int &length) const;
//获取请求消息头
std::string get_request_headers(void)const;
//获取请求回应消息头
std::string get_response_headers(void)const;
//获取最后一次错误信息
std::string get_error_info(void)const;
//获取请求状态码
int get_status(void)const;

/*自定义设置*/
//添加请求消息头
void add_part_request_headers(const char* key,const char* value);
//删除添加消息头，恢复到默认消息头
void clear_addheaders(void);
//删除指定请求消息头
void delete_part_request_headers(const char* key);
//设置是否保存cookies
void save_cookies(bool flag);
//是否关心Referer
bool care_referer;

private:
void init(void);
//封装url处理细节
bool request_web(bool is_post,const char* url);
bool socket_request(const char* host,const char* sub_directory,const int port =80);
bool parse_url(const char*url,std::string& host,std::string& sub_directory,int& port);
bool check_host_valid(const std::string& host);
unsigned long get_host_address(const char* host);
//封装send
void send_string(SOCKET& sock,const char* content);
void set_error(const char* content);
//设置响应消息头cookies
void set_cookies(const char* cookies);
//获取消息头中的状态码
bool set_status(std::string& headers);
private:
std::list<std::string> _default_headers; 
std::list<std::string> _add_headers; 
std::string _part_request_headers;
std::string _response_headers;
std::string _cookies;
std::string _err_info;
std::string _post_content;
unsigned char* _response_content;//数据缓冲区
unsigned int _content_size;//缓冲区大小
unsigned int _content_pos;
bool _is_post;
bool _set_cookies;
int _status;
};
/*
eg.
url:
	"http://www.google.com","www.google.com","www.google.com/test"
	"www.google.com:80","www.google.com:80/test","www.google.com:8080/test"
	.....
	如无http:// 默认解析为http
post_content:
	"key=value&key2=value2" ...
	
*/
inline bool
CRequestHttp::request_post(const char* url,const char* post_content)
{
	_post_content = post_content;
	return request_web(true,url);
}

inline bool
CRequestHttp::request_post(const char* url,const char* post_content,std::string& response_content)
{
	_post_content = post_content;
	bool flag = request_web(true,url);
	if(flag)
		response_content = std::string((char*)_response_content,_content_pos);
	return flag;
}

inline bool
CRequestHttp::request_get(const char* url) 
{
	return request_web(false,url);
}

inline bool
CRequestHttp::request_get(const char* url,std::string& response_content) 
{
	bool flag = request_web(false,url);
	if(flag)
		response_content = std::string((char*)_response_content,_content_pos);
	return flag;
}