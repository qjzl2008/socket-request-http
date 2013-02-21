// Copyright 2012.  All rights reserved.
// http://skycpp.cn
// Author: zhaolei
// E-mail: qjzl2008@163.com
#include "CRequestHttp.h"

CRequestHttp::CRequestHttp(void)
:_part_request_headers(""),_response_headers(""),_cookies("")
,_err_info(""),_post_content(""),_response_content(NULL),_content_size(30000)
,_content_pos(0),_status(0),_is_post(false),_set_cookies(false),care_referer(false)
{
	_response_content = (unsigned char*)malloc(sizeof(unsigned char)*_content_size);
	init();
}

CRequestHttp::~CRequestHttp(void)
{
	if(_response_content)
		free(_response_content);
}
void
CRequestHttp::init(void)
{
	_default_headers.push_back("Accept: image/gif, image/x-xbitmap," 
	" image/jpeg, image/pjpeg, application/vnd.ms-excel," 
	" application/msword, application/vnd.ms-powerpoint," 
	" */*\r\n");
	_default_headers.push_back("Accept-Charset: GBK,utf-8;q=0.7,*;q=0.3\r\n");
	_default_headers.push_back("Accept-Language: en-us\r\n");
	_default_headers.push_back("User-Agent: sky-socket/1.0\r\n");
}

bool 
CRequestHttp::request_web(bool is_post,const char* url)
{
	_is_post = is_post;
	std::string host,sub_directory;
	int port =0;
	if(!parse_url(url,host,sub_directory,port))
		return false;
	bool flag =socket_request(host.c_str(),sub_directory.c_str(),port);
	//设置响应消息头中的cookies
	set_cookies(_response_headers.c_str());
	return flag;
}

bool 
CRequestHttp::parse_url(const char*url,std::string& host,std::string& sub_directory,int& port)
{	
	std::string url_str(url);
	size_t begin_pos,end_pos;
	begin_pos = url_str.find("//",0);
	begin_pos += 2;
	end_pos = url_str.find("/",begin_pos);
	if(end_pos != std::string::npos)
	{
		host = url_str.substr(begin_pos,end_pos-begin_pos);
		sub_directory = url_str.substr(end_pos,url_str.length()-end_pos);
	}
	else
	{
		host = url_str.substr(begin_pos,url_str.length()-begin_pos);
		sub_directory ="/";
	}
	begin_pos = host.find(":",0);
	if(begin_pos !=std::string::npos)
	{
		port = atoi(host.substr(begin_pos+1,host.length()-begin_pos-1).c_str());
		host = host.substr(0,begin_pos);

	}
	else
		port =80;	
	return check_host_valid(host);
}
bool
CRequestHttp::socket_request(const char* host,const char* sub_directory,const int port) 
{
#ifdef WINDOWS
	WSADATA         WsaData;  
	int err = WSAStartup (0x0101, &WsaData);                            // Init Winsock 
	if(err!=0)  
		return false;
#endif
	sockaddr_in     sin;  
	SOCKET          sock;  
	char buffer[MAXBUFFERSIZE];
 

	sock = socket (AF_INET, SOCK_STREAM, 0);  
#ifdef WINDOWS
	if (sock == INVALID_SOCKET)  
		return false;
#else
	if (sock == -1)  
		return false;
#endif
	sin.sin_family = AF_INET;                                       //Connect to web sever  
	sin.sin_port = htons( (unsigned short)port ); 
	sin.sin_addr.s_addr = get_host_address(host);  
	if(sin.sin_addr.s_addr == 0)
	{
		return false;
	}
	if(connect(sock,(sockaddr*)&sin, sizeof(sockaddr_in)))  
	{  
		set_error("connect failed");
		return false;  
	}	
	if(_is_post)
	{
		send_string(sock,"POST ");
		_part_request_headers="POST ";
	}
	else
	{
		send_string(sock,"GET ");
		_part_request_headers="GET ";
	}
	send_string(sock,sub_directory);
	_part_request_headers += sub_directory;
	send_string(sock," HTTP/1.0\r\n"); 
	_part_request_headers += " HTTP/1.0\r\n";
	send_string(sock,"Host: "); 
	_part_request_headers += "Host: ";
	send_string(sock,host);
	_part_request_headers += host;
	send_string(sock,"\r\n");
	_part_request_headers += "\r\n";
	//send default headers
	std::list<std::string>::iterator it = _default_headers.begin();
	std::list<std::string>::iterator it_end = _default_headers.end();
	for(; it!=it_end; ++it)
	{
		send_string(sock,it->c_str());
	}
	//send added headers
	if(!_add_headers.empty())
	{
		it = _add_headers.begin();
		it_end = _add_headers.end();
		for(; it!=it_end; ++it)
		{
			send_string(sock,it->c_str());
		}
	}
	//send cookies
	if(_set_cookies)
	{
		send_string(sock,"Cookie: "); 
		_part_request_headers += "Cookie: ";
		send_string(sock,_cookies.c_str());
		_part_request_headers += _cookies;
		send_string(sock,"\r\n");
		_part_request_headers += "\r\n";
	}

	if(_is_post)  
	{  
		if(_post_content.length())
		{
			sprintf(buffer,"Content-Length: %ld\r\n",_post_content.length());  
			send_string(sock,buffer); 
			_part_request_headers += std::string(buffer,strlen(buffer));
		}
		send_string(sock,"Content-Type: application/x-www-form-urlencoded\r\n");
		_part_request_headers += "Content-Type: application/x-www-form-urlencoded\r\n";
	} 
	send_string(sock,"\r\n");                                // Send a blank line to signal end of HTTP headerReceive 
	_part_request_headers += "\r\n";
	if(_is_post)
	{
		send_string(sock,_post_content.c_str());
		_part_request_headers += _post_content;
	}

	// recv http headers
	int recv_count =0;
	bool done =false;
	int head_end = 0;
	char* recv_buffer =NULL;
	recv_buffer = buffer;
	unsigned int pos =0;
	_response_headers = "";
	while(!done)
	{
		recv_count =recv(sock,recv_buffer,1,0);
		if(recv_count<0)
			break;
		switch(*recv_buffer)  
		{  
			case '\r': 
				break;  
			case '\n':  
				{
					if(!head_end)  
						done = true;
					head_end=0;
					break;
				}
			default: 
				++head_end;
				break;  
		} 
		if(pos ==(MAXBUFFERSIZE -1))
		{
			_response_headers += std::string(buffer);
			pos =0;
			recv_buffer = buffer;
		}
		else
		{
			++recv_buffer;
			++pos;
		}
	}
	_response_headers += std::string(buffer,pos);

	if(!set_status(_response_headers))
		return false;
	
	//get length from headers,and then update buffer size
	std::string temp_length("Length:");
	size_t begin_pos =_response_headers.find(temp_length,0);
	if(begin_pos!= std::string::npos)
	{
		size_t end_pos = _response_headers.find("\r\n",begin_pos);
		begin_pos += temp_length.length();
		temp_length = _response_headers.substr(begin_pos,end_pos-begin_pos);
		unsigned int content_length = atoi(temp_length.c_str());
		if(content_length >_content_size && content_length < MAXMALLOCSIZE)
		{
			free(_response_content);
			_response_content = (unsigned char*)malloc(sizeof(unsigned char)*(content_length+100));
			_content_size = content_length+100;
		}
	}
	//recv http body
	_content_pos = 0;
	do
	{
		recv_count = recv(sock,buffer,sizeof(buffer)-1,0);
		if(recv_count<0)
			break;
		if((recv_count+_content_pos)>=_content_size)
		{
			unsigned char* temp_buffer = (unsigned char*)malloc(sizeof(unsigned char)*(_content_size*2));
			if(!temp_buffer)
				return false;
			_content_size =_content_size*2;
			memcpy(temp_buffer,_response_content,_content_pos);
			free(_response_content);
			_response_content = temp_buffer;
		}

		memcpy(_response_content+_content_pos,buffer,recv_count);
		_content_pos +=recv_count;
	}while(recv_count>0);
#ifdef WINDOWS
	closesocket(sock);
#else
	close(sock);
#endif
  return true;
}
unsigned long
CRequestHttp::get_host_address(const char* host)
{
	struct hostent *phe;  
	char *p;  
	phe = gethostbyname(host);  
	if(phe==NULL) 
	{
		set_error("failed in gethostbyname");
		return 0;  
	}
	p = *phe->h_addr_list;  
	return *((unsigned long*)p);  
}

void
CRequestHttp::send_string(SOCKET& socket,const char* content)
{
	send(socket,content,strlen(content),0);
}
void 
CRequestHttp::set_error(const char* content)
{
	_err_info = content;
}
void
CRequestHttp::set_cookies(const char* cookies)
{
	std::string temp(cookies);
	size_t begin_pos = 0, end_pos =0;
	std::string cookies_str("Cookie:");
	begin_pos = temp.find(cookies_str,0);
	if(begin_pos == std::string::npos)
		return;
	else
		begin_pos+=cookies_str.length();
	end_pos = temp.find(";",begin_pos);
	_cookies = temp.substr(begin_pos,end_pos-begin_pos+1);
}

bool
CRequestHttp::set_status(std::string& headers)
{
	size_t begin_pos = 0,end_pos =0;
	begin_pos = headers.find("HTTP",0);
	if(begin_pos == std::string::npos)
		return false;
	begin_pos = headers.find(" ",begin_pos);
	if(begin_pos == std::string::npos)
		return false;
	++begin_pos;
	end_pos = headers.find(" ",begin_pos);
	std::string code = headers.substr(begin_pos,end_pos-begin_pos);
	_status = atoi(code.c_str());
	return true;
}
int
CRequestHttp::get_status(void) const
{
	return _status;
}
unsigned char* 
CRequestHttp::get_response_content(unsigned int &length) const
{
	length = _content_pos;
	return _response_content;
}
bool 
CRequestHttp::check_host_valid(const std::string& host)
{

	return true;
}
std::string 
CRequestHttp::get_response_headers(void)const
{
	return _response_headers;
}
std::string 
CRequestHttp::get_request_headers(void)const
{
	std::string temp(_part_request_headers);
	if(!_default_headers.empty())
	{
		

		std::list<std::string>::const_iterator it = _default_headers.begin();
		std::list<std::string>::const_iterator it_end = _default_headers.end();
		for(;it!= it_end; ++it)
			temp += (*it);
	}
	if(!_add_headers.empty())
	{
		std::list<std::string>::const_iterator it = _add_headers.begin();
		std::list<std::string>::const_iterator it_end = _add_headers.end();
		for(;it!= it_end; ++it)
			temp += (*it);
	}
	return temp;
}
void
CRequestHttp::save_cookies(bool flag)
{
	_set_cookies = flag;
}

void
CRequestHttp::clear_addheaders(void)
{
	_add_headers.clear();
}
std::string
CRequestHttp::get_error_info(void)const
{
	return _err_info;
}