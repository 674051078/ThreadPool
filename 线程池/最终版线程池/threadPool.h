#pragma once
//#pragma once
//让编译器确保头文件只被包含一次，无论在编译过程中被直接或间接包含多少次。
//当编译器遇到 #pragma once 时，它会记录该头文件，并在后续遇到该头文件时跳过其内容，从而防止多重包含。

#ifndef THREDPOOL_H
#define THREDPOOL_H

#include <iostream>
#include <string>
#include <queue>
#include <memory>//保持拉长对象的生命周期，自动释放资源
#include<atomic>
#include<mutex>
#include<condition_variable>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>
//模板类的代码写在头文件中

// Any类型：可以接收任意数据的类型
//class Any
//{
//public:
//	Any() = default;
//	~Any() = default;
//	Any(const Any&) = delete;
//	Any& operator=(const Any&) = delete;
//	Any(Any&&) = default;
//	Any& operator=(Any&&) = default;
//
//	// 这个构造函数可以让Any类型接收任意其它的数据
//	template<typename T>  // T:int    Derive<int>
//	Any(T data) : base_(std::make_unique<Derive<T>>(data))
//	{}
//
//	// 这个方法能把Any对象里面存储的data数据提取出来
//	template<typename T>
//	T cast_()
//	{
//		// 我们怎么从base_找到它所指向的Derive对象，从它里面取出data成员变量
//		// 基类指针 =》 派生类指针   RTTI
//		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
//		if (pd == nullptr)
//		{
//			throw "type is unmatch!";
//		}
//		return pd->data_;
//	}
//private:
//	// 基类类型
//	class Base
//	{
//	public:
//		virtual ~Base() = default;
//	};
//
//	// 派生类类型
//	template<typename T>
//	class Derive : public Base
//	{
//	public:
//		Derive(T data) : data_(data)
//		{}
//		T data_;  // 保存了任意的其它类型
//	};
//
//private:
//	// 定义一个基类的指针
//	std::unique_ptr<Base> base_;
//};
//
////Task类型声明的前置声明
//class Task;
//
////实现一个信号量类
//class Semaphore
//{
//public:
//	Semaphore(int limit = 0)
//		:resLimit_(limit)
//	{}
//	~Semaphore() = default;
//
//	//获取一个信号量资源
//	void wait()
//	{
//		std::unique_lock<std::mutex>lock(mtx_);
//		//等待信号量有资源，没有资源的话，会阻塞当前线程
//		cond_.wait(lock, [&]()-> bool {return resLimit_ > 0; });
//		resLimit_--;
//	}
//	//增加一个信号量资源
//	void post()
//	{
//		std::unique_lock<std::mutex>lock(mtx_);
//		resLimit_++;
//		cond_.notify_all();
//	}
//private:
//	int resLimit_;
//	std::condition_variable  cond_;
//	std::mutex mtx_;
//};

//实现接受提交到线程池的task任务执行完成后的返回值类型的Result
//class Result
//{
//public:
//	Result(std::shared_ptr<Task> task, bool isValid = true);
//	~Result() = default;
//
//	//问题一：setVal方法，获取任务执行完的返回值
//	void setVal(Any any);
//	//问题二：get方法，用户调用这个方法获取task的返回值
//	Any get();
//private:
//	Any any_;						//存储任务的返回值
//	Semaphore sem_;					//线程通信信号量
//	std::shared_ptr<Task> task_;	//指向对于获取返回值的任务对象  引用计数不为0的时候 不会析构掉的 绑定在一起了
//	std::atomic_bool isValid_;		//返回值无效
//};
//
////using namespace std; //防止全局名字污染  不要写
////任务抽象基类
//class Task
//{
//public:
//	Task();
//	~Task() = default;
//	void exec();
//	void setResult(Result* res);
//	//用户可以自定义任意任务类型，从Task继承，重写rn方法，实现自定义任务处理
//	virtual Any run() = 0;  //virtual T run() = 0; 虚函数和模板不能放在一起使用
//private:
//	Result* result_;//这里如果也使用shared_ptr 会造成智能指针引用计数 交叉引用的问题
//	//Result对象的生命周期 大于 Task的生命周期
//};
//
////线程池支持的模式
//enum class PoolMode
//{
//	MODE_FIXED,//固定数量的线程
//	MODE_CACHED,//数量动态可增长的线程
//};

const int TASK_MAX_THRESHHOLD = INT32_MAX;//最大任务数
const int Thread_MAX_THRESHHOLD = 1024;//最大线程数
const int Thread_MAX_IDLE_TIME = 10;//线程最大的空闲时间  单位秒

//线程池支持的模式
enum class PoolMode
{
	MODE_FIXED,//固定数量的线程
	MODE_CACHED,//数量动态可增长的线程
};

//线程类型
class Thread
{
public:
	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;
	//线程构造
	Thread(ThreadFunc func)
		:func_(func)
		, threadId_(genernateId_++)
	{}
	//线程析构
	~Thread() = default;

	//启动线程
	void start()
	{
		//创建一个线程来执行一个线程函数
		std::thread t(func_, threadId_);//C++11来说 线程对象t 和线程线程函数func_
		t.detach();//设置分离线程  相当于linux中pthread_detach  pthread_t 设置为分离线程
	}

	//获取线程id
	int getId() const
	{
		return threadId_;
	}

private:
	ThreadFunc func_;
	static int genernateId_;//静态成员变量 需要类外初始化
	int threadId_; //保存线程id
};

//////线程方法的实现 
int Thread::genernateId_ = 0;

//线程池类型  里面放很多个线程
class ThreadPool
{
public:
	//线程池构造
	ThreadPool()
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
	~ThreadPool()
	{
		isPoolRunning_ = false;
		//notEmpty_.notify_all();//会产生死锁问题   
		// threadfunc里面的notEmpty_.wait()就不能被通知唤醒了  然而下面的wait也会死锁了

		//等待线程池里的所有线程返回  有两种状态 阻塞 &  正在执行任 务中
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		notEmpty_.notify_all();//这样在加完锁之后  提前通知threadfunc里面的notEmpty_.wait()
		//进入阻塞状态  下面的wait条件不符合就会释放锁  然后threadfunc里面的notEmpty_.wait()就可以继续工作了
		exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
	}

	// 设置线程池的工作模式
	void setMode(PoolMode mode)
	{
		if (isPoolRunning_)
		{
			return;
		}
		poolMode_ = mode;
	}

	//设置task任务队列上限阈值
	void setTaskQueMaxThreshHold(int threshhold)
	{
		if (isPoolRunning_)
		{
			return;
		}
		taskQueMaxThreshHold_ = threshhold;
	}

	//设置task任务队列上限阈值
	void setThreadSizeThreshHold(int threshhold)
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

	//给线程池提交任务
	//Result submitTask(std::shared_ptr<Task> sp);
	//使用可变参模板杉函数，让submitTask可以接受任意任务函数和任意数量的参数
	//pool.submitTask(sum1,10,20);    右值引用+引用折叠
	//返回值  future<>
	template<typename Func,typename... Args>
	auto sbumitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>
	{
		//打包任务，放入任务队列里面
		using RType = decltype(func(args...));//给类型重命名
		auto task = std::make_shared<std::packaged_task<RType()>>(
		std::bind(std::forward<Func>(func), std::forward<Args>(args)...)); 
		std::future<RType> result = task->get_future();

		//获取锁
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		//用户提交任务  最长不能阻塞超过1s，否则判断提交任务失败，返回
		if (!notFull_.wait_for(lock, std::chrono::seconds(1),
			[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
		{
			std::cerr << "任务队列为满的，提交失败" << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>(
				[&]()->RType { return RType(); });
			//return task->getResult(); //不能让Result对象依赖task   线程执行完task，task对象就被析构掉了
			(*task)();//执行任务
			return task->get_future();
		}
		//如果未满，则把任务加入到队列中
		//taskQue_.emplace(sp);
		//using Task = std::function<void()>;
		taskQue_.emplace([task]() { (*task)(); });
		taskSize_++;
		//因为放了新任务，任务队列肯定不空了 在noEmpty_上通知
		notEmpty_.notify_all();

		//cache 模式 任务处理比较紧急
		//需要根据任务的数量和空闲线程的时间，判断是否需要创建新线程出来
		if (poolMode_ == PoolMode::MODE_CACHED
			&& taskSize_ > idleThreadSize_
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
		return result;//不写 默认为Ture
		//return task->getResult();
		
	}

	//开启线程池
	void start(int initThreadSize = std::thread::hardware_concurrency())//初始为机器的CPU核心数量
	{
		//设置线程池的运行状态
		isPoolRunning_ = true;

		//初始化线程池的个数 和当前线程池的数量
		initThreadSize_ = initThreadSize;
		curThreadSize_ = initThreadSize;

		//创建线程对象
		for (int i = 0; i < initThreadSize_; i++)
		{
			//创建thread线程对象的时候，把线程函数给到thread线程对象上

			std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
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

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;


private:
	//定义线程函数
	void threadFunc(int threadid)
	{
		auto lastTime = std::chrono::high_resolution_clock().now();

		// 所有任务必须执行完成，线程池才可以回收所有线程资源 
		for (;;)
			//while(isPoolRunning_)  线程运行/不运行都要执行任务
		{
			Task task;
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
				task();//执行function<void()>
			}

			idleThreadSize_++;
			lastTime = std::chrono::high_resolution_clock().now(); // 更新线程执行完任务的时间
		}
		//线程池要结束，回收线程资源
		/*threads_.erase(threadid);
		std::cout << "threadid:" << std::this_thread::get_id() << " exit " << std::endl;
		exitCond_.notify_all();*/

	}

	//检查pool运行状态
	bool checkRunningState() const
	{
		return isPoolRunning_;
	}
private:
	//std::vector<Thread*> threads_;   裸指针改为智能智能指针
	//std::vector<std::unique_ptr<Thread>> threads_;//线程列表
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;//线程列表
	int initThreadSize_;//初始的线程个数   int表示有符号的，包含负数，但是这里不可能为负数 可以写unsigned int
	int threadSizeThreadHol_;//线程数量上限的阈值
	std::atomic_int curThreadSize_;//记录当前线程的总数量
	std::atomic_int idleThreadSize_;//记录空闲线程的数量


	//std::queue<Task*> 出作用域就会被释放，反而变为野指针 不能传入裸指针

	//Task任务=》函数对象
	using Task = std::function<void()>;
	std::queue<Task> taskQue_;//任务队列  
	//std::queue<std::shared_ptr<Task>> taskQue_;//任务队列  
	std::atomic_uint taskSize_;//任务数量
	int taskQueMaxThreshHold_; //任务队列数量上限的阈值

	std::mutex taskQueMtx_;//保证任务队列的线程安全
	std::condition_variable notFull_;//表示队列不满，可以继续生产
	std::condition_variable notEmpty_;//表示队列不空，可以继续消费
	std::condition_variable exitCond_;//等待资源全全部回收

	PoolMode poolMode_;	//当前线程池的工作模式

	//表示当前线程池的启动状态
	std::atomic_bool isPoolRunning_;

};

#endif // THREDPOOL_H
