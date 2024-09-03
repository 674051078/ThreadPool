#include <iostream>
#include "threadpool.h"
#include <functional>
#include<thread>

const int TASK_MAX_THRESHHOLD = INT32_MAX;//最大任务数
const int Thread_MAX_THRESHHOLD = 1024;//最大线程数
const int Thread_MAX_IDLE_TIME = 10;//线程最大的空闲时间  单位秒


//线程池构造
ThreadPool::ThreadPool()
	: initThreadSize_(0)
	, taskSize_(0)
	, idleThreadSize_(0)
	, curThreadSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	, threadSizeThreadHol_(Thread_MAX_THRESHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
	, isPoolRunning_(false)
{}

//线程池析构
ThreadPool::~ThreadPool()
{
	isPoolRunning_ = false; 
	//notEmpty_.notify_all();//会产生死锁问题   
	// threadfunc里面的notEmpty_.wait()就不能被通知唤醒了  然而下面的wait也会死锁了

	//等待线程池里的所有线程返回  有两种状态 阻塞 &  正在执行任 务中
	std::unique_lock<std::mutex> lock(taskQueMtx_) ;
	notEmpty_.notify_all();//这样在加完锁之后  提前通知threadfunc里面的notEmpty_.wait()
	//进入阻塞状态  下面的wait条件不符合就会释放锁  然后threadfunc里面的notEmpty_.wait()就可以继续工作了
	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
}

// 设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
	if (isPoolRunning_)
	{
		return;
	}
	poolMode_ = mode;
}

//设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	if (isPoolRunning_)
	{
		return;
	}
	taskQueMaxThreshHold_ = threshhold;
}

//设置task任务队列上限阈值
void ThreadPool::setThreadSizeThreshHold(int threshhold)
{
	if (isPoolRunning_)
	{
		return;
	}
	if (poolMode_ == PoolMode::MODE_CACHED)
	{
			threadSizeThreadHol_ = threshhold;
	}
	
}

//给线程池提交任务  用户调用该接口 传入任务 生产任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	//获取锁
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	//线程的通信 等待任务队列有空余  如果满了就阻塞住
	//用户提交任务  最长不能阻塞超过1s，否则判断提交任务失败，返回
	/*while (taskQue_.size() == taskQueMaxThreshHold_)
	{
		notFull_.wait(lock);
	}*///等价下行的 lambda表达式写的
	if (!notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
	{
		std::cerr << "任务队列为满的，提交失败"<<std::endl;
		//return task->getResult(); //不能让Result对象依赖task   线程执行完task，task对象就被析构掉了
		return Result(sp,false);
	}
	//如果未满，则把任务加入到队列中
	taskQue_.emplace(sp);
	taskSize_++;
	//因为放了新任务，任务队列肯定不空了 在noEmpty_上通知
	notEmpty_.notify_all();

	//cache 模式 任务处理比较紧急
	//需要根据任务的数量和空闲线程的时间，判断是否需要创建新线程出来
	if (poolMode_ == PoolMode::MODE_CACHED
		&&taskSize_  > idleThreadSize_
		&& curThreadSize_ < threadSizeThreadHol_)
	{
		//创建新线程对象
		std::cout << "create new thread..." << std::endl;
		std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		//启动线程
		threads_[threadId]->start();
		//修改线程个数相关变量
		curThreadSize_++;
		idleThreadSize_++;
	}

	//返回任务的Result对象
	return Result(sp);//不写 默认为Ture
	//return task->getResult();
}

//开启线程池
void ThreadPool::start(int initThreadSize)
{
	//设置线程池的运行状态
	isPoolRunning_ = true;

	//初始化线程池的个数 和当前线程池的数量
	initThreadSize_ = initThreadSize;
	curThreadSize_  = initThreadSize;

	//创建线程对象
	for (int i = 0; i < initThreadSize_; i++) 
	{
		//创建thread线程对象的时候，把线程函数给到thread线程对象上

		std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		//threads_.emplace_back(std::move(ptr));
	}

	//创建所有线程  std::vector<Thread*> threads_;
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();//需要执行一个线程函数
		idleThreadSize_++;  //每启动一个线程就记录初始空闲线程的数量
	}
}
//定义线程函数  线程池的所有线程从任务队列中消费任务
// 定义线程函数   线程池的所有线程从任务队列里面消费任务
void ThreadPool::threadFunc(int threadid)  // 线程函数返回，相应的线程也就结束了
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	// 所有任务必须执行完成，线程池才可以回收所有线程资源 
	for(;;)
	//while(isPoolRunning_)  线程运行/不运行都要执行任务
	{
		std::shared_ptr<Task> task;
		{
			// 先获取锁
			std::unique_lock<std::mutex> lock(taskQueMtx_);

			std::cout << "tid:" << std::this_thread::get_id()
				<< "尝试获取任务..." << std::endl;

			// cached模式下，有可能已经创建了很多的线程，但是空闲时间超过60s，应该把多余的线程
			// 结束回收掉（超过initThreadSize_数量的线程要进行回收）
			// 当前时间 - 上一次线程执行的时间 > 60s

			// 每一秒中返回一次   怎么区分：超时返回？还是有任务待执行返回
			// 锁 + 双重判断
			while (taskQue_.size() == 0)//任务都执行完了进入循环
			//while (isPoolRunning_&&taskQue_.size() == 0)//任务数>0 要做任务  不用判断是否有线程在运行
			{
				 //线程池要结束，回收线程资源
				if (!isPoolRunning_)
				{
					threads_.erase(threadid); // std::this_thread::getid()
					std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
						<< std::endl;
					exitCond_.notify_all();
					return; // 线程函数结束，线程结束
				}

				if (poolMode_ == PoolMode::MODE_CACHED)
				{
					// 条件变量，超时返回了
					if (std::cv_status::timeout ==
						notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= Thread_MAX_IDLE_TIME
							&& curThreadSize_ > initThreadSize_)
						{
							// 开始回收当前线程
							// 记录线程数量的相关变量的值修改
							// 把线程对象从线程列表容器中删除   没有办法 threadFunc《=》thread对象
							// threadid => thread对象 => 删除
							threads_.erase(threadid); // std::this_thread::getid()
							curThreadSize_--;
							idleThreadSize_--;

							std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
								<< std::endl;
							return;
						}
					}
				}
				else
				{
					// 等待notEmpty条件
					notEmpty_.wait(lock);
				}
			}
			//线程池要结束，回收线程资源
			/*if (!isPoolRunning_)
			{
				break;
			}*/
			idleThreadSize_--;

			std::cout << "tid:" << std::this_thread::get_id()
				<< "获取任务成功..." << std::endl;

			// 从任务队列种取一个任务出来
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			// 如果依然有剩余任务，继续通知其它得线程执行任务
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}
			// 取出一个任务，进行通知，通知可以继续提交生产任务
			notFull_.notify_all();
		} // 就应该把锁释放掉

		// 当前线程负责执行这个任务
		if (task != nullptr)
		{
			// task->run(); // 执行任务；把任务的返回值setVal方法给到Result
			task->exec();
		}

		idleThreadSize_++;
		lastTime = std::chrono::high_resolution_clock().now(); // 更新线程执行完任务的时间
	}
	//线程池要结束，回收线程资源
	/*threads_.erase(threadid);
	std::cout << "threadid:" << std::this_thread::get_id() << " exit " << std::endl;
	exitCond_.notify_all();*/

}
bool ThreadPool::checkRunningState()const
{
	return isPoolRunning_;
}

//////线程方法的实现 
int Thread::genernateId_ = 0;

//线程构造
Thread::Thread(ThreadFunc func)
	:func_(func)
	, threadId_(genernateId_++ )
{}
//线程析构
Thread::~Thread()
{}

//启动线程
void Thread::start()
{
	//创建一个线程来执行一个线程函数
	std::thread t(func_,threadId_);//C++11来说 线程对象t 和线程线程函数func_
	t.detach();//设置分离线程  相当于linux中pthread_detach  pthread_t 设置为分离线程
}

//获取线程id
int Thread::getId()const
{
	return threadId_;
}

//////////////////////// Task方法实现
Task::Task()
	:result_(nullptr)
	{}

//////////////////////////// Task方法的实现
void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run());//这里发生多态
	}
	//result_.setVal(run());//这里发生多态
}

void Task::setResult(Result* res)
{
	result_ = res;
	
}

//////////////////////////// Result方法的实现
Result::Result(std::shared_ptr<Task> task, bool isValid)
	:isValid_(isValid)
	,task_(task)
{
	task_->setResult(this);
}

Any Result::get()
{
	if (!isValid_)
	{
		return " ";
	}
		sem_.wait();
		return std::move(any_);//shared_ptr使用的是右值需要使用move()
}

void Result::setVal(Any any)
{
	//存储task的返回值
	this->any_ = std::move(any);
	sem_.post();

}
