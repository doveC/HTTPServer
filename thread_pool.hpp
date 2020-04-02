#pragma once
#include <iostream>                                                                                                                     
#include <queue> 
#include <pthread.h>
#include <unistd.h>
#include <time.h>
            
#define MAX_THREAD 5
typedef void(*handler_t)(void *);

// 线程任务类
// 为了通用，函数指针的参数设为 void*
class ThreadTask {
private:
    void* _arg;
    handler_t _handler;
public:
    ThreadTask()
        : _arg(nullptr)
        , _handler(nullptr) {}

    ThreadTask(void* arg, handler_t handler)
        : _arg(arg)
        , _handler(handler) {}

    void SetTask(void* arg, handler_t handler) {
        _arg = arg;
        _handler = handler;
    }
        
    void Run() {
        _handler(_arg);
    }
};  

// 线程池类
// 封装线程池接口
class ThreadPool {
private:
    int _thread_max;
    int _thread_cur;
    bool _tp_quit;
    std::queue<ThreadTask *> _task_queue;
    pthread_mutex_t _lock;
    pthread_cond_t _cond;
private:
    void LockQueue() {
        pthread_mutex_lock(&_lock);
    }
    void UnLockQueue() {
        pthread_mutex_unlock(&_lock);
    }
    void WakeUpOne() {
        pthread_cond_signal(&_cond);
    }
    void WakeUpAll() {
        pthread_cond_broadcast(&_cond);
    }
    void ThreadQuit() {
        _thread_cur--;
        UnLockQueue();
        pthread_exit(nullptr);
    }
    void ThreadWait() {
        if(_tp_quit) {
            ThreadQuit();
        }
        pthread_cond_wait(&_cond, &_lock);
    }
    bool IsEmpty() {                                                                                                                    
        return _task_queue.empty();
    }

    static void *thr_start(void *arg) {  // 每个线程的启动函数
        ThreadPool *tp = (ThreadPool*)arg;
        while(1) {
            tp->LockQueue();
            while(tp->IsEmpty()) {
                tp->ThreadWait();
            }
            ThreadTask *tt;
            tp->PopTask(&tt);
            tp->UnLockQueue();
            tt->Run();
        }

        return nullptr;
    }
public:
    ThreadPool(int th_count = MAX_THREAD)
        : _thread_max(th_count)
        , _thread_cur(th_count)
        , _tp_quit(false) {
            pthread_mutex_init(&_lock, nullptr);
            pthread_cond_init(&_cond, nullptr);
        }
    ~ThreadPool() {
        pthread_mutex_destroy(&_lock);
        pthread_cond_destroy(&_cond);
    }

    bool PoolInit() {
        pthread_t tid;
        for(int i = 0; i < _thread_max; i++) {
            int ret = pthread_create(&tid, nullptr, thr_start, this);
            if(ret != 0) {                                                                                                              
                std::cout << "Error: pthread_create()" << std::endl;
                return false;
            }
        }

	    return true;
	}

	bool PushTask(ThreadTask *tt) {
		LockQueue();
		if(_tp_quit) {
			UnLockQueue();
			return false;
		}
		_task_queue.push(tt);
		WakeUpOne();
		UnLockQueue();

		return true;
	}

    bool PopTask(ThreadTask **tt) {
        *tt = _task_queue.front();
        _task_queue.pop();

        return true;
    }

    bool PoolQuit() {
        LockQueue();
        _tp_quit = true;
        UnLockQueue();
        while(_thread_cur > 0) {
            WakeUpAll();
            usleep(1000);
        }                                                                                                                               

        return true;
    }
};

