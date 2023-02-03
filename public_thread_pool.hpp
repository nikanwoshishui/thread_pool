#include<iostream>
#include<pthread.h>
#include<queue>
#include<unistd.h>
#include<string>
#include<string.h>

template <class T>
struct TASK		//任务
{
	void* (*p_main)(void *val);	//函数指针
	T val;
};

template <class T>
class QUEUE_TASK	//任务队列
{
public:
	QUEUE_TASK();
	~QUEUE_TASK();
	void add_task(TASK<T> val);	//添加任务
	TASK<T> out_task();	//取出任务
	inline int  val_task()	//获取当前任务个数
	{
		return q_task.size();
	}
private:
	std::queue<TASK<T>> q_task;
	pthread_mutex_t mutex;
};

template <class T>
QUEUE_TASK<T>::QUEUE_TASK()
{
	pthread_mutex_init(&mutex,NULL);
}

template <class T>
QUEUE_TASK<T>::~QUEUE_TASK()
{
	pthread_mutex_destroy(&mutex);
}

template <class T>
void QUEUE_TASK<T>::add_task(TASK<T> val)	//添加任务
{
	pthread_mutex_lock(&mutex);
	q_task.push(val);
	pthread_mutex_unlock(&mutex);
}

template <class T>
TASK<T> QUEUE_TASK<T>::out_task()	//取出任务
{
	TASK<T> task;
	task.val = 0;

	if(!q_task.empty())
	{
		pthread_mutex_lock(&mutex);
		task = q_task.front();
		q_task.pop();
		pthread_mutex_unlock(&mutex);
	}
	return task;
}

//线程池
template <class T>
class THREAD_POOL
{
public:
	THREAD_POOL(int min = 3, int max = 10);	//初始化
	~THREAD_POOL();	//销毁线程池
	void add_pool(TASK<T> task);	//增加任务
	int  wrok_val();	//获取正在工作线程的个数
	int  live_val();	//获取活着线程的个数
	//void shutpool;
private:
	static void* Manager(void* args);	//管理者线程
	static void* worker(void* args);	//工作者线程
	void single_exit();	//单个线程退出

private:
	QUEUE_TASK<T> *q_t;		//任务队列

	int minnum;		//最小线程池数量
	int maxnum;		//最大线程池数量
	int live;		//活着的线程数量
	int work;		//工作着的线程数量
	int exitnum; 		//要销毁的线程数量
	static const int NUM = 2;	//管理者线程每次创建工作者线程的数量

	pthread_mutex_t	mutex;	//互斥锁
	pthread_cond_t	cond;	//条件变量

	pthread_t thread_man;	//管理者线程
	pthread_t *thread_work;	//工作者线程

	bool shutpool;		//关闭线程池
};

template <class T>
THREAD_POOL<T>::THREAD_POOL(int min, int max)
{

	do
	{
		q_t = new QUEUE_TASK<T>;
		if(q_t == nullptr)
		{
			std::cout << "q_t new error\n";
		}
		thread_work = new pthread_t[max];
		if(thread_work == nullptr)
		{
			std::cout << "thread_work new error\n";
			break;
		}
		memset(thread_work,0,sizeof(pthread_t) * max);
		
		if(pthread_cond_init(&cond,NULL) != 0)	break;
		if(pthread_mutex_init(&mutex,NULL) != 0) break;

		//创建管理者线程
		if(pthread_create(&thread_man,0,Manager,this) != 0)	break;

		//最少要有三个工作者线程
		for(int i=0; i<min; i++)
			pthread_create(&thread_work[i],0,worker,this);

		minnum = min;	//默认最小3个
		maxnum = max;	//默认最大10个
		live = min;
		work = 0;
		exitnum = 0;
		shutpool = false;

		return;
	}while(0);

	if(q_t)	delete q_t;
	if(thread_work) delete[]thread_work;
}

template <class T>
THREAD_POOL<T>::~THREAD_POOL()
{
	shutpool = true;	//关闭线程池

	//阻塞回收管理者线程
	pthread_join(thread_man, NULL);
	//唤醒阻塞的消费者线程
	for(int i=0; i<live; i++)
		pthread_cond_signal(&cond);

	//释放堆内存
	if(q_t)		delete q_t;
	if(thread_work)	delete[]thread_work;

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

template <class T>
void THREAD_POOL<T>::add_pool(TASK<T> task)//增加任务
{
	if(shutpool)	return;
	q_t->add_task(task);
	pthread_cond_signal(&cond);
}

template <class T>
int  THREAD_POOL<T>::wrok_val()		//获取正在工作线程的个数
{
	pthread_mutex_lock(&mutex);
	int busy = work;
	pthread_mutex_unlock(&mutex);
	return busy;
}

template <class T>
int  THREAD_POOL<T>::live_val()		//获取活着线程的个数
{
	pthread_mutex_lock(&mutex);
	int livenum = live;
	pthread_mutex_unlock(&mutex);
	return livenum;
}

template <class T>
void* THREAD_POOL<T>::Manager(void* args )		//管理者线程
{
	THREAD_POOL* pool = static_cast<THREAD_POOL*>(args);

	while(!pool->shutpool)
	{
		sleep(3);	//每3秒检测一次

		pthread_mutex_lock(&pool->mutex);
		int queuesize = pool->q_t->val_task();
		int livenum = pool->live;
		int busynum = pool->work;
		pthread_mutex_unlock(&pool->mutex);

		//创建线程	任务数大于线程存活数 且 线程存活数小于线程总数
		if(queuesize >  livenum && livenum < pool->maxnum)
		{
			pthread_mutex_lock(&pool->mutex);
			int count = 0;

			//i小于线程总数 且 每次只创建两个线程 且 线程存活数小于线程总数
			for(int i=0; i<pool->maxnum && count < NUM && pool->live < pool->maxnum; i++)
			{
				if(pool->thread_work[i] == 0)
				{
					pthread_create(&pool->thread_work[i],0,pool->worker,pool);
					count++;
					pool->live++;
					std::cout << pool->thread_work[i] << "::thread add success\n";
				}
			}
			pthread_mutex_unlock(&pool->mutex);
				
		}

		//销毁线程	忙碌的线程小于活着的线程 且 线程存活数量大于最小线程存活数量
		if(busynum*2 < livenum && livenum > pool->minnum)
		{
			pthread_mutex_lock(&pool->mutex);
			pool->exitnum = NUM;
			pthread_mutex_unlock(&pool->mutex);
			for(int i=0; i<NUM; ++i)
			{
				pthread_cond_signal(&pool->cond);
			}
		}
	}

	return NULL;
}

template <class T>
void* THREAD_POOL<T>::worker(void* args)		//工作者线程
{
	THREAD_POOL* pool = static_cast<THREAD_POOL*>(args);

	while(1)
	{
		pthread_mutex_lock(&pool->mutex);	//只有一个线程可以获取锁,其它加入mutex等待队列

		//判断队列内任务是否为空且线程池不被关闭
		while(pool->q_t->val_task() == 0 && !pool->shutpool)
		{
			pthread_cond_wait(&pool->cond,&pool->mutex);	//加入cond等待队列并释放锁

			if(pool->exitnum > 0 )	//判断是否需要减少线程数量
			{
				pool->exitnum--;
				if(pool->live > pool->minnum)	//判断存活数量是否大于最小数量
				{
					pool->live--;
					pthread_mutex_unlock(&pool->mutex);
					pool->single_exit();
				}
			}
		}

		if(pool->shutpool)	//判断线程池是否要关闭
		{
			pthread_mutex_unlock(&pool->mutex);
			pool->single_exit();
		}

		TASK<T> task;
		task = pool->q_t->out_task();	//取出一个任务
		pool->work++;	//工作线程加一
		pthread_mutex_unlock(&pool->mutex);	//取出任务后即可解锁

		
		task.p_main((void*)(long)task.val);	//执行任务

		pthread_mutex_lock(&pool->mutex);
		pool->work--;
		pthread_mutex_unlock(&pool->mutex);
	}

	return NULL;
}
template <class T>
void THREAD_POOL<T>::single_exit()		//单个线程退出
{
	pthread_t tid = pthread_self();

	for(int i=0; i<maxnum; i++)
	{
		if(thread_work[i] == tid)
		{
			thread_work[i] = 0;
			std::cout << "threadExit() called " << std::to_string(tid) << "exiting...\n";
			break;
		}
	}
	pthread_exit(NULL);
	return;
}

