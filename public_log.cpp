#include"public.h"


pthread_mutex_t		CIULOG::log_mutex;
std::ofstream	CIULOG::fp;
char*		CIULOG::fpname = new char[20];		//初始化静态char变量,静态字符串还是要申明指针
bool		CIULOG::m_btoFile = true;		//默认输入到日志文件内
bool		CIULOG::mutex_val = false;		//默认为单线程不需要使用互斥锁
LOG_LEVEL	CIULOG::m_nloglevel = LOG_LEVEL_INFO;	//默认日志级别为信息输出
//代码一运行就初始化创建实例,本身就线程安全
CIULOG* CIULOG::ciulog = new(std::nothrow)CIULOG();


CIULOG::~CIULOG()
{
	deleteInstance();
	fp.close();
	pthread_mutex_destroy(&log_mutex);
}

//释放单列实例
void CIULOG::deleteInstance()
{
	if(ciulog)
	{
		delete ciulog;
		ciulog = NULL;
	}
}

//日志初始化,bool输出到文件还是控制台,bool判断是否为福哦线程模式
void CIULOG::init_ciulog(bool btofile, bool val)
{
	m_btoFile = btofile;
	mutex_val = val;

	if(m_btoFile == true)
	{
		struct timeval tv;	//把当地时间转换为秒
		struct tm* ptm;		//当地时间(年月日,时分秒)
		memset(fpname,0,sizeof(fpname));

		gettimeofday(&tv,NULL);		//把当地时间转换为秒
		ptm = localtime(&(tv.tv_sec));	//把秒转换为当地时间

		//fpname是指针,指针大小固定为8
		strftime(fpname,sizeof(char[20]), "%Y-%m-%d", ptm);

		fp.open(fpname,std::ios::app);

		if(!fp.is_open())
		{
			std::cout << "current init_ciulog() file open error\n";
			return;
		}

		fp.close();

		if(mutex_val)		//判断是否为多线程模式
		{
			if(pthread_mutex_init(&log_mutex,0) !=0)	//初始化互斥锁
				LOG_ERROR("mutex init error");
		}
	}

}
void CIULOG::setlevel(LOG_LEVEL level)  //修改日志级别
{
	m_nloglevel = level;
}

bool CIULOG::log(LOG_LEVEL level, const char* FileName, const char* FunctionName, int Row, const char* logstr,...)
{
	char str[1024],levelname[10];
	memset(str,0,sizeof(str));
	memset(levelname,0,sizeof(levelname));
	
	if(level == LOG_LEVEL_INFO)		strcpy(levelname, "INFO");
	else if(level == LOG_LEVEL_WARNING)	strcpy(levelname, "WARNING");
	else if(level == LOG_LEVEL_ERROR)	strcpy(levelname, "ERROR");

	if(mutex_val)	//判断是否为多线程
	{
		if(pthread_mutex_lock(&log_mutex) != 0)	//加锁
		{
			mutex_val = false;
			LOG_ERROR("lock error");
			return false;
		}
	}

	struct timeval tv;	//把当地时间转换为秒
	struct tm* ptm;		//当地时间(年月日,时分秒)
	char time_string[40];

	gettimeofday(&tv,NULL);		//把当地时间转换为秒
	ptm = localtime(&(tv.tv_sec));	//把秒转换为当地时间
	strftime(time_string,sizeof(time_string), "%Y-%m-%d %H:%M:%S", ptm);

	if(m_btoFile)
	{
		sprintf(str,"[%s][%s][PID:%u][TID:%u][%s][%s(): %d]::%s\n",time_string,levelname,
		(unsigned int)getpid(),(unsigned int)gettid(),FileName,FunctionName,Row,logstr);
		//每次写日志要打开文件
		fp.open(fpname,std::ios::out | std::ios::app);
		fp << str;
		fp.close();	//写完后关闭文件
	}
	else
	{
		printf("[%s][%s][PID:%u][TID:%u][%s][%s: %d]::%s\n",time_string,levelname,
		(unsigned int)getpid(),(unsigned int)gettid(),FileName,FunctionName,Row,logstr);
	}

	if(mutex_val)
	{
		if(pthread_mutex_unlock(&log_mutex) != 0)
		{
			mutex_val = false;
			LOG_ERROR("unlock error");
			return false;
		}
	}
	return true;
}
