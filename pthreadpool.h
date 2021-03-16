#ifndef PTHREAD_POOL_H
#define PTHREAD_POOL_H
#include<pthread.h>
#include<list>
#include<exception>
#include"lock.h"
template<typename T>
class Pthreadpool
{
public:
	Pthreadpool(int number,int max_request);
	bool push(T* src);
	static void * work(void *arg);
	void run();
private:
	pthread_t *p_tid=nullptr;//线程id
	std::list<T*> list;//用来存放http类的链表
	Sem p_sem;//信号量判断里面有没有东西
	Mutex p_mutex;//用来锁住链表里面的东西
	int p_number;//线程数目
	int max_request;//最大请求数量
};

template<typename T>
bool Pthreadpool<T>::push(T*src)
{
	p_mutex.lock();
	list.push_back(src);
	p_mutex.unlock();
	p_sem.post();
	return true;
}
template<typename T>
Pthreadpool<T>::Pthreadpool(int number,int max_request):p_number(number),max_request(max_request)
{
	if(number<=0||max_request<=0)
		throw std::exception();
	p_tid=new pthread_t[number];
	if(!p_tid)
		throw std::exception();
	for(int i=0;i<number;i++)
	{
		if(pthread_create(p_tid+i,NULL,work,(void*)this)!=0)
		{
			delete []p_tid;
			throw std::exception();
		}
		if(pthread_detach(p_tid[i])!=0)
		{
			delete []p_tid;
			throw std::exception();
		}
	}
}
template<typename T>
void* Pthreadpool<T>::work(void *arg)
{
	Pthreadpool *pool=static_cast<Pthreadpool*> (arg);
	pool->run();
	return pool;
}
template<typename T>
void Pthreadpool<T>::run()
{
	while(1)
	{
		p_sem.wait();
		p_mutex.lock();
		T*src=list.back();
		list.pop_back();
		p_mutex.unlock();
		if(src==nullptr)
			continue;
		src->process();
	}
}
#endif
