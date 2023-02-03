/*
*@desc:		public.h
*@author:	me
*@date:		2023.1.26
*/

#ifndef PUBLC_H
#define PUBLIC_H 1

//socket服务端
#include<iostream>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

//在p2前面或则后面添加字符串
bool insert_str(const char* p1 = NULL, char* p2 = NULL, const char* p3 = NULL);


class SOCKET_FWD
{
public:
	SOCKET_FWD();
	~SOCKET_FWD();
	bool init_socket(int port_val, const char* ip);	//初始化
	bool accept_socket();	//开始监听等待连接
	bool send_socket(const char* str);
	bool recv_socket(char* str);
	int sock;	//用于监听等待连接的socket
	int sock_khd;	//连接成功后用于通讯的socket
};

//socket客户端
class SOCKET_KHD
{
public:
	SOCKET_KHD();
	~SOCKET_KHD();
	bool init_socket(int port_val, const char* ip);	//初始化并发起连接
	bool send_socket(const char* str);
	bool recv_socket(char* str);
	int sock;
};

/**********************mysql数据库*****************************/
//mysql数据库
#include<mysql/mysql.h>

class MY_MYSQL
{
public:
	MY_MYSQL();
	~MY_MYSQL();
	//初始化MySQL
	bool init_mysql(const char *ip, const char *name, const char *passwd,const char *mysql_name);
	bool sql_mysql(const char *sql);	//执行sql语句
	void select_mysql(const char *sql, MYSQL_ROW *row);	//返回查询结果
	bool print_mysql(const char *sql);	//打印查询结果
private:
	MYSQL mysql, *p_mysql, *p_sock;
};

/*********************日志系统*******************************/
#include<time.h>
#include<sys/time.h>
#include<sys/syscall.h>
#include<fstream>

#define gettid()	syscall(SYS_gettid)

//extern std::ofstream fp;	//外部变量声明如此使用

enum LOG_LEVEL	//日志级别
{
	LOG_LEVEL_INFO,		//信息
	LOG_LEVEL_WARNING,	//警告
	LOG_LEVEL_ERROR		//错误
};

//__FILE__::获取当前文件名, __FUNCTION__::获取当前函数名, __LINE__::获取当前所在行数, __VA_ARGS__::可变参数列表
#define LOG_INFO(...)           CIULOG::log(LOG_LEVEL_INFO,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
#define LOG_WARNING(...)        CIULOG::log(LOG_LEVEL_WARNING,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
#define LOG_ERROR(...)          CIULOG::log(LOG_LEVEL_ERROR,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)

//日志系统
class CIULOG
{
private:
	CIULOG(){}
	~CIULOG();
	CIULOG(const CIULOG &ciulog);
	const CIULOG &operator = (const CIULOG &ciulog);

	//CIULOG() = delete;	//阻止构造
	//~CIULOG() = delete;	//阻止析构
	//CIULOG(const CIULOG& rhs) = delete;	//阻止拷贝
	//CIULOG& operator = (const CIULOG& rhs) = delete;	//阻止赋值

private:
	static bool		m_btoFile;	//日志写入文件还是控制台
	static bool		mutex_val;	//是否为多线程模式
	static LOG_LEVEL	m_nloglevel;	//日志级别
	static char		*fpname;	//日志文件名
	static CIULOG		*ciulog;	//唯一单实列对象指针
	static pthread_mutex_t	log_mutex;	//互斥锁
	static std::ofstream	fp;
public:
	static CIULOG *getInstance()	{return ciulog;}	//获取单列实例
	static void deleteInstance();	//释放单列实例
	//日志初始化,bool输出到文件还是控制台,bool长日志是否截断,PCTSTR统一字符串
	static void init_ciulog(bool btofile, bool bTruncateLongLog);
	static void setlevel(LOG_LEVEL level);	//修改日志级别
	static bool log(LOG_LEVEL level, const char* FileName, const char* FunctionName, int Row, const char* logstr,...);

};

#endif
