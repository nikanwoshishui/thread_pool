#include"public.h"
#include"public_thread_pool.hpp"

void *p(void *val)
{
	int a = (long)val;
	char str[1024];
	memset(str,0,sizeof(str));

	sprintf(str,"%d::thread::%lu\n",a,pthread_self());
	CIULOG::LOG_INFO(str);		//写入日志系统

	sleep(1);
	return NULL;
}

int main()
{
	//初始化日志系统,启动多线程模式
	CIULOG::init_ciulog(true,true);

	TASK<int> task;
	task.p_main = p;

	//初始化线程池,最小线程数3,最大线程数10
	THREAD_POOL<int> pool(3,10);

	for(int i=0; i<100; i++)
	{
		task.val = i;
		pool.add_pool(task);
	}
		sleep(20);
	return 0;
}
