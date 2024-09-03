#include <iostream>
#include "threadpool.h"
#include <functional>
#include<thread>

const int TASK_MAX_THRESHHOLD = INT32_MAX;//���������
const int Thread_MAX_THRESHHOLD = 1024;//����߳���
const int Thread_MAX_IDLE_TIME = 10;//�߳����Ŀ���ʱ��  ��λ��


//�̳߳ع���
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

//�̳߳�����
ThreadPool::~ThreadPool()
{
	isPoolRunning_ = false; 
	//notEmpty_.notify_all();//�������������   
	// threadfunc�����notEmpty_.wait()�Ͳ��ܱ�֪ͨ������  Ȼ�������waitҲ��������

	//�ȴ��̳߳���������̷߳���  ������״̬ ���� &  ����ִ���� ����
	std::unique_lock<std::mutex> lock(taskQueMtx_) ;
	notEmpty_.notify_all();//�����ڼ�����֮��  ��ǰ֪ͨthreadfunc�����notEmpty_.wait()
	//��������״̬  �����wait���������Ͼͻ��ͷ���  Ȼ��threadfunc�����notEmpty_.wait()�Ϳ��Լ���������
	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
}

// �����̳߳صĹ���ģʽ
void ThreadPool::setMode(PoolMode mode)
{
	if (isPoolRunning_)
	{
		return;
	}
	poolMode_ = mode;
}

//����task�������������ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	if (isPoolRunning_)
	{
		return;
	}
	taskQueMaxThreshHold_ = threshhold;
}

//����task�������������ֵ
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

//���̳߳��ύ����  �û����øýӿ� �������� ��������
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	//��ȡ��
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	//�̵߳�ͨ�� �ȴ���������п���  ������˾�����ס
	//�û��ύ����  �������������1s�������ж��ύ����ʧ�ܣ�����
	/*while (taskQue_.size() == taskQueMaxThreshHold_)
	{
		notFull_.wait(lock);
	}*///�ȼ����е� lambda���ʽд��
	if (!notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
	{
		std::cerr << "�������Ϊ���ģ��ύʧ��"<<std::endl;
		//return task->getResult(); //������Result��������task   �߳�ִ����task��task����ͱ���������
		return Result(sp,false);
	}
	//���δ�������������뵽������
	taskQue_.emplace(sp);
	taskSize_++;
	//��Ϊ����������������п϶������� ��noEmpty_��֪ͨ
	notEmpty_.notify_all();

	//cache ģʽ ������ȽϽ���
	//��Ҫ��������������Ϳ����̵߳�ʱ�䣬�ж��Ƿ���Ҫ�������̳߳���
	if (poolMode_ == PoolMode::MODE_CACHED
		&&taskSize_  > idleThreadSize_
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
	return Result(sp);//��д Ĭ��ΪTure
	//return task->getResult();
}

//�����̳߳�
void ThreadPool::start(int initThreadSize)
{
	//�����̳߳ص�����״̬
	isPoolRunning_ = true;

	//��ʼ���̳߳صĸ��� �͵�ǰ�̳߳ص�����
	initThreadSize_ = initThreadSize;
	curThreadSize_  = initThreadSize;

	//�����̶߳���
	for (int i = 0; i < initThreadSize_; i++) 
	{
		//����thread�̶߳����ʱ�򣬰��̺߳�������thread�̶߳�����

		std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
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
//�����̺߳���  �̳߳ص������̴߳������������������
// �����̺߳���   �̳߳ص������̴߳��������������������
void ThreadPool::threadFunc(int threadid)  // �̺߳������أ���Ӧ���߳�Ҳ�ͽ�����
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	// �����������ִ����ɣ��̳߳زſ��Ի��������߳���Դ 
	for(;;)
	//while(isPoolRunning_)  �߳�����/�����ж�Ҫִ������
	{
		std::shared_ptr<Task> task;
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
			//�̳߳�Ҫ�����������߳���Դ
			/*if (!isPoolRunning_)
			{
				break;
			}*/
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
			// task->run(); // ִ�����񣻰�����ķ���ֵsetVal��������Result
			task->exec();
		}

		idleThreadSize_++;
		lastTime = std::chrono::high_resolution_clock().now(); // �����߳�ִ���������ʱ��
	}
	//�̳߳�Ҫ�����������߳���Դ
	/*threads_.erase(threadid);
	std::cout << "threadid:" << std::this_thread::get_id() << " exit " << std::endl;
	exitCond_.notify_all();*/

}
bool ThreadPool::checkRunningState()const
{
	return isPoolRunning_;
}

//////�̷߳�����ʵ�� 
int Thread::genernateId_ = 0;

//�̹߳���
Thread::Thread(ThreadFunc func)
	:func_(func)
	, threadId_(genernateId_++ )
{}
//�߳�����
Thread::~Thread()
{}

//�����߳�
void Thread::start()
{
	//����һ���߳���ִ��һ���̺߳���
	std::thread t(func_,threadId_);//C++11��˵ �̶߳���t ���߳��̺߳���func_
	t.detach();//���÷����߳�  �൱��linux��pthread_detach  pthread_t ����Ϊ�����߳�
}

//��ȡ�߳�id
int Thread::getId()const
{
	return threadId_;
}

//////////////////////// Task����ʵ��
Task::Task()
	:result_(nullptr)
	{}

//////////////////////////// Task������ʵ��
void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run());//���﷢����̬
	}
	//result_.setVal(run());//���﷢����̬
}

void Task::setResult(Result* res)
{
	result_ = res;
	
}

//////////////////////////// Result������ʵ��
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
		return std::move(any_);//shared_ptrʹ�õ�����ֵ��Ҫʹ��move()
}

void Result::setVal(Any any)
{
	//�洢task�ķ���ֵ
	this->any_ = std::move(any);
	sem_.post();

}
