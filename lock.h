#ifndef LOCK_H
#define LOCK_H
#include<semaphore.h>
#include<pthread.h>

class Sem
{
public:
	Sem()
	{
		sem_init(&sem,0,0);
	}
	int wait()
	{
		return sem_wait(&sem);
	}
	int post()
	{
		return sem_post(&sem);
	}
	~Sem()
	{
		sem_destroy(&sem);
	}
private:
	sem_t sem;	
};
class Mutex
{
public:
	Mutex()
	{
		pthread_mutex_init(&mutex,0);
	}
	int lock()
	{
	      return pthread_mutex_lock(&mutex);
	}
	int unlock()
	{
		return pthread_mutex_unlock(&mutex);
	}
	~Mutex()
	{
		pthread_mutex_destroy(&mutex);
	}
	pthread_mutex_t* getmutex()
	{
		return &mutex;
	}

private:
	pthread_mutex_t mutex;
};
class Cond
{
public:
	Cond(Mutex &mutex):mutex(mutex)
	{
		pthread_cond_init(&m_cond,0);
	}
	int  wait()
	{
		return pthread_cond_wait(&m_cond,mutex.getmutex());
	}
	int signal()
	{
		return pthread_cond_signal(&m_cond);
	}
	int broadcast()
	{
		return pthread_cond_broadcast(&m_cond);
	}
	~Cond()
	{
		 pthread_cond_destroy(&m_cond);
	}
private:
	Mutex &mutex;
	pthread_cond_t m_cond;
};

#endif
