#pragma once
//#pragma once
//�ñ�����ȷ��ͷ�ļ�ֻ������һ�Σ������ڱ�������б�ֱ�ӻ��Ӱ������ٴΡ�
//������������ #pragma once ʱ�������¼��ͷ�ļ������ں���������ͷ�ļ�ʱ���������ݣ��Ӷ���ֹ���ذ�����

#ifndef THREDPOOL_H
#define THREDPOOL_H

#include <iostream>
#include <string>
#include <queue>
#include <memory>//��������������������ڣ��Զ��ͷ���Դ
#include<atomic>
#include<mutex>
#include<condition_variable>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>
//ģ����Ĵ���д��ͷ�ļ���

// Any���ͣ����Խ����������ݵ�����
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
//	// ������캯��������Any���ͽ�����������������
//	template<typename T>  // T:int    Derive<int>
//	Any(T data) : base_(std::make_unique<Derive<T>>(data))
//	{}
//
//	// ��������ܰ�Any��������洢��data������ȡ����
//	template<typename T>
//	T cast_()
//	{
//		// ������ô��base_�ҵ�����ָ���Derive���󣬴�������ȡ��data��Ա����
//		// ����ָ�� =�� ������ָ��   RTTI
//		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
//		if (pd == nullptr)
//		{
//			throw "type is unmatch!";
//		}
//		return pd->data_;
//	}
//private:
//	// ��������
//	class Base
//	{
//	public:
//		virtual ~Base() = default;
//	};
//
//	// ����������
//	template<typename T>
//	class Derive : public Base
//	{
//	public:
//		Derive(T data) : data_(data)
//		{}
//		T data_;  // �������������������
//	};
//
//private:
//	// ����һ�������ָ��
//	std::unique_ptr<Base> base_;
//};
//
////Task����������ǰ������
//class Task;
//
////ʵ��һ���ź�����
//class Semaphore
//{
//public:
//	Semaphore(int limit = 0)
//		:resLimit_(limit)
//	{}
//	~Semaphore() = default;
//
//	//��ȡһ���ź�����Դ
//	void wait()
//	{
//		std::unique_lock<std::mutex>lock(mtx_);
//		//�ȴ��ź�������Դ��û����Դ�Ļ�����������ǰ�߳�
//		cond_.wait(lock, [&]()-> bool {return resLimit_ > 0; });
//		resLimit_--;
//	}
//	//����һ���ź�����Դ
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

//ʵ�ֽ����ύ���̳߳ص�task����ִ����ɺ�ķ���ֵ���͵�Result
//class Result
//{
//public:
//	Result(std::shared_ptr<Task> task, bool isValid = true);
//	~Result() = default;
//
//	//����һ��setVal��������ȡ����ִ����ķ���ֵ
//	void setVal(Any any);
//	//�������get�������û��������������ȡtask�ķ���ֵ
//	Any get();
//private:
//	Any any_;						//�洢����ķ���ֵ
//	Semaphore sem_;					//�߳�ͨ���ź���
//	std::shared_ptr<Task> task_;	//ָ����ڻ�ȡ����ֵ���������  ���ü�����Ϊ0��ʱ�� ������������ ����һ����
//	std::atomic_bool isValid_;		//����ֵ��Ч
//};
//
////using namespace std; //��ֹȫ��������Ⱦ  ��Ҫд
////����������
//class Task
//{
//public:
//	Task();
//	~Task() = default;
//	void exec();
//	void setResult(Result* res);
//	//�û������Զ��������������ͣ���Task�̳У���дrn������ʵ���Զ���������
//	virtual Any run() = 0;  //virtual T run() = 0; �麯����ģ�岻�ܷ���һ��ʹ��
//private:
//	Result* result_;//�������Ҳʹ��shared_ptr ���������ָ�����ü��� �������õ�����
//	//Result������������� ���� Task����������
//};
//
////�̳߳�֧�ֵ�ģʽ
//enum class PoolMode
//{
//	MODE_FIXED,//�̶��������߳�
//	MODE_CACHED,//������̬���������߳�
//};

const int TASK_MAX_THRESHHOLD = INT32_MAX;//���������
const int Thread_MAX_THRESHHOLD = 1024;//����߳���
const int Thread_MAX_IDLE_TIME = 10;//�߳����Ŀ���ʱ��  ��λ��

//�̳߳�֧�ֵ�ģʽ
enum class PoolMode
{
	MODE_FIXED,//�̶��������߳�
	MODE_CACHED,//������̬���������߳�
};

//�߳�����
class Thread
{
public:
	//�̺߳�����������
	using ThreadFunc = std::function<void(int)>;
	//�̹߳���
	Thread(ThreadFunc func)
		:func_(func)
		, threadId_(genernateId_++)
	{}
	//�߳�����
	~Thread() = default;

	//�����߳�
	void start()
	{
		//����һ���߳���ִ��һ���̺߳���
		std::thread t(func_, threadId_);//C++11��˵ �̶߳���t ���߳��̺߳���func_
		t.detach();//���÷����߳�  �൱��linux��pthread_detach  pthread_t ����Ϊ�����߳�
	}

	//��ȡ�߳�id
	int getId() const
	{
		return threadId_;
	}

private:
	ThreadFunc func_;
	static int genernateId_;//��̬��Ա���� ��Ҫ�����ʼ��
	int threadId_; //�����߳�id
};

//////�̷߳�����ʵ�� 
int Thread::genernateId_ = 0;

//�̳߳�����  ����źܶ���߳�
class ThreadPool
{
public:
	//�̳߳ع���
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

	//�̳߳�����
	~ThreadPool()
	{
		isPoolRunning_ = false;
		//notEmpty_.notify_all();//�������������   
		// threadfunc�����notEmpty_.wait()�Ͳ��ܱ�֪ͨ������  Ȼ�������waitҲ��������

		//�ȴ��̳߳���������̷߳���  ������״̬ ���� &  ����ִ���� ����
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		notEmpty_.notify_all();//�����ڼ�����֮��  ��ǰ֪ͨthreadfunc�����notEmpty_.wait()
		//��������״̬  �����wait���������Ͼͻ��ͷ���  Ȼ��threadfunc�����notEmpty_.wait()�Ϳ��Լ���������
		exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
	}

	// �����̳߳صĹ���ģʽ
	void setMode(PoolMode mode)
	{
		if (isPoolRunning_)
		{
			return;
		}
		poolMode_ = mode;
	}

	//����task�������������ֵ
	void setTaskQueMaxThreshHold(int threshhold)
	{
		if (isPoolRunning_)
		{
			return;
		}
		taskQueMaxThreshHold_ = threshhold;
	}

	//����task�������������ֵ
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

	//���̳߳��ύ����
	//Result submitTask(std::shared_ptr<Task> sp);
	//ʹ�ÿɱ��ģ��ɼ��������submitTask���Խ������������������������Ĳ���
	//pool.submitTask(sum1,10,20);    ��ֵ����+�����۵�
	//����ֵ  future<>
	template<typename Func,typename... Args>
	auto sbumitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>
	{
		//������񣬷��������������
		using RType = decltype(func(args...));//������������
		auto task = std::make_shared<std::packaged_task<RType()>>(
		std::bind(std::forward<Func>(func), std::forward<Args>(args)...)); 
		std::future<RType> result = task->get_future();

		//��ȡ��
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		//�û��ύ����  �������������1s�������ж��ύ����ʧ�ܣ�����
		if (!notFull_.wait_for(lock, std::chrono::seconds(1),
			[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
		{
			std::cerr << "�������Ϊ���ģ��ύʧ��" << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>(
				[&]()->RType { return RType(); });
			//return task->getResult(); //������Result��������task   �߳�ִ����task��task����ͱ���������
			(*task)();//ִ������
			return task->get_future();
		}
		//���δ�������������뵽������
		//taskQue_.emplace(sp);
		//using Task = std::function<void()>;
		taskQue_.emplace([task]() { (*task)(); });
		taskSize_++;
		//��Ϊ����������������п϶������� ��noEmpty_��֪ͨ
		notEmpty_.notify_all();

		//cache ģʽ ������ȽϽ���
		//��Ҫ��������������Ϳ����̵߳�ʱ�䣬�ж��Ƿ���Ҫ�������̳߳���
		if (poolMode_ == PoolMode::MODE_CACHED
			&& taskSize_ > idleThreadSize_
			&& curThreadSize_ < threadSizeThreadHol_)
		{
			//�������̶߳���
			std::cout << "create new thread..." << std::endl;
			std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
			int threadId = ptr->getId();
			threads_.emplace(threadId, std::move(ptr));
			//�����߳�
			threads_[threadId]->start();
			//�޸��̸߳�����ر���
			curThreadSize_++;
			idleThreadSize_++;
		}
		//���������Result����
		return result;//��д Ĭ��ΪTure
		//return task->getResult();
		
	}

	//�����̳߳�
	void start(int initThreadSize = std::thread::hardware_concurrency())//��ʼΪ������CPU��������
	{
		//�����̳߳ص�����״̬
		isPoolRunning_ = true;

		//��ʼ���̳߳صĸ��� �͵�ǰ�̳߳ص�����
		initThreadSize_ = initThreadSize;
		curThreadSize_ = initThreadSize;

		//�����̶߳���
		for (int i = 0; i < initThreadSize_; i++)
		{
			//����thread�̶߳����ʱ�򣬰��̺߳�������thread�̶߳�����

			std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
			int threadId = ptr->getId();
			threads_.emplace(threadId, std::move(ptr));
			//threads_.emplace_back(std::move(ptr));
		}

		//���������߳�  std::vector<Thread*> threads_;
		for (int i = 0; i < initThreadSize_; i++)
		{
			threads_[i]->start();//��Ҫִ��һ���̺߳���
			idleThreadSize_++;  //ÿ����һ���߳̾ͼ�¼��ʼ�����̵߳�����
		}
	}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;


private:
	//�����̺߳���
	void threadFunc(int threadid)
	{
		auto lastTime = std::chrono::high_resolution_clock().now();

		// �����������ִ����ɣ��̳߳زſ��Ի��������߳���Դ 
		for (;;)
			//while(isPoolRunning_)  �߳�����/�����ж�Ҫִ������
		{
			Task task;
			{
				// �Ȼ�ȡ��
				std::unique_lock<std::mutex> lock(taskQueMtx_);

				std::cout << "tid:" << std::this_thread::get_id()
					<< "���Ի�ȡ����..." << std::endl;

				// cachedģʽ�£��п����Ѿ������˺ܶ���̣߳����ǿ���ʱ�䳬��60s��Ӧ�ðѶ�����߳�
				// �������յ�������initThreadSize_�������߳�Ҫ���л��գ�
				// ��ǰʱ�� - ��һ���߳�ִ�е�ʱ�� > 60s

				// ÿһ���з���һ��   ��ô���֣���ʱ���أ������������ִ�з���
				// �� + ˫���ж�
				while (taskQue_.size() == 0)//����ִ�����˽���ѭ��
					//while (isPoolRunning_&&taskQue_.size() == 0)//������>0 Ҫ������  �����ж��Ƿ����߳�������
				{
					//�̳߳�Ҫ�����������߳���Դ
					if (!isPoolRunning_)
					{
						threads_.erase(threadid); // std::this_thread::getid()
						std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
							<< std::endl;
						exitCond_.notify_all();
						return; // �̺߳����������߳̽���
					}

					if (poolMode_ == PoolMode::MODE_CACHED)
					{
						// ������������ʱ������
						if (std::cv_status::timeout ==
							notEmpty_.wait_for(lock, std::chrono::seconds(1)))
						{
							auto now = std::chrono::high_resolution_clock().now();
							auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
							if (dur.count() >= Thread_MAX_IDLE_TIME
								&& curThreadSize_ > initThreadSize_)
							{
								// ��ʼ���յ�ǰ�߳�
								// ��¼�߳���������ر�����ֵ�޸�
								// ���̶߳�����߳��б�������ɾ��   û�а취 threadFunc��=��thread����
								// threadid => thread���� => ɾ��
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
						// �ȴ�notEmpty����
						notEmpty_.wait(lock);
					}
				}

				idleThreadSize_--;

				std::cout << "tid:" << std::this_thread::get_id()
					<< "��ȡ����ɹ�..." << std::endl;

				// �����������ȡһ���������
				task = taskQue_.front();
				taskQue_.pop();
				taskSize_--;

				// �����Ȼ��ʣ�����񣬼���֪ͨ�������߳�ִ������
				if (taskQue_.size() > 0)
				{
					notEmpty_.notify_all();
				}
				// ȡ��һ�����񣬽���֪ͨ��֪ͨ���Լ����ύ��������
				notFull_.notify_all();
			} // ��Ӧ�ð����ͷŵ�

			// ��ǰ�̸߳���ִ���������
			if (task != nullptr)
			{
				task();//ִ��function<void()>
			}

			idleThreadSize_++;
			lastTime = std::chrono::high_resolution_clock().now(); // �����߳�ִ���������ʱ��
		}
		//�̳߳�Ҫ�����������߳���Դ
		/*threads_.erase(threadid);
		std::cout << "threadid:" << std::this_thread::get_id() << " exit " << std::endl;
		exitCond_.notify_all();*/

	}

	//���pool����״̬
	bool checkRunningState() const
	{
		return isPoolRunning_;
	}
private:
	//std::vector<Thread*> threads_;   ��ָ���Ϊ��������ָ��
	//std::vector<std::unique_ptr<Thread>> threads_;//�߳��б�
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;//�߳��б�
	int initThreadSize_;//��ʼ���̸߳���   int��ʾ�з��ŵģ������������������ﲻ����Ϊ���� ����дunsigned int
	int threadSizeThreadHol_;//�߳��������޵���ֵ
	std::atomic_int curThreadSize_;//��¼��ǰ�̵߳�������
	std::atomic_int idleThreadSize_;//��¼�����̵߳�����


	//std::queue<Task*> ��������ͻᱻ�ͷţ�������ΪҰָ�� ���ܴ�����ָ��

	//Task����=����������
	using Task = std::function<void()>;
	std::queue<Task> taskQue_;//�������  
	//std::queue<std::shared_ptr<Task>> taskQue_;//�������  
	std::atomic_uint taskSize_;//��������
	int taskQueMaxThreshHold_; //��������������޵���ֵ

	std::mutex taskQueMtx_;//��֤������е��̰߳�ȫ
	std::condition_variable notFull_;//��ʾ���в��������Լ�������
	std::condition_variable notEmpty_;//��ʾ���в��գ����Լ�������
	std::condition_variable exitCond_;//�ȴ���Դȫȫ������

	PoolMode poolMode_;	//��ǰ�̳߳صĹ���ģʽ

	//��ʾ��ǰ�̳߳ص�����״̬
	std::atomic_bool isPoolRunning_;

};

#endif // THREDPOOL_H
