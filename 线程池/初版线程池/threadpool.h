//#pragma once
//�ñ�����ȷ��ͷ�ļ�ֻ������һ�Σ������ڱ�������б�ֱ�ӻ��Ӱ������ٴΡ�
//������������ #pragma once ʱ�������¼��ͷ�ļ������ں���������ͷ�ļ�ʱ���������ݣ��Ӷ���ֹ���ذ�����

#ifndef THREDPOOL_H
#define THREDPOOL_H

#include <string>
#include <queue>
#include <memory>//��������������������ڣ��Զ��ͷ���Դ
#include<atomic>
#include<mutex>
#include<condition_variable>
#include <functional>
#include <unordered_map>
//ģ����Ĵ���д��ͷ�ļ��� 

// Any���ͣ����Խ����������ݵ�����
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	// ������캯��������Any���ͽ�����������������
	template<typename T>  // T:int    Derive<int>
	Any(T data) : base_(std::make_unique<Derive<T>>(data))
	{}

	// ��������ܰ�Any��������洢��data������ȡ����
	template<typename T>
	T cast_()
	{
		// ������ô��base_�ҵ�����ָ���Derive���󣬴�������ȡ��data��Ա����
		// ����ָ�� =�� ������ָ��   RTTI
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch!";
		}
		return pd->data_;
	}
private:
	// ��������
	class Base
	{
	public:
		virtual ~Base() = default;
	};

	// ����������
	template<typename T>
	class Derive : public Base
	{
	public:
		Derive(T data) : data_(data)
		{}
		T data_;  // �������������������
	};

private:
	// ����һ�������ָ��
	std::unique_ptr<Base> base_;
};

//Task����������ǰ������
class Task;

//ʵ��һ���ź�����
class Semaphore
{
public:
	Semaphore(int limit = 0)
	:resLimit_(limit)
	{}
	~Semaphore() = default;

	//��ȡһ���ź�����Դ
	void wait()
	{
		std::unique_lock<std::mutex>lock(mtx_);
		//�ȴ��ź�������Դ��û����Դ�Ļ�����������ǰ�߳�
		cond_.wait(lock, [&]()-> bool {return resLimit_ > 0; });
		resLimit_--;
	}
	//����һ���ź�����Դ
	void post()
	{
		std::unique_lock<std::mutex>lock(mtx_);
		resLimit_++;
		cond_.notify_all();
	}
private:
	int resLimit_;
	std::condition_variable  cond_;
	std::mutex mtx_;
};

//ʵ�ֽ����ύ���̳߳ص�task����ִ����ɺ�ķ���ֵ���͵�Result
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;
	 
	//����һ��setVal��������ȡ����ִ����ķ���ֵ
	void setVal(Any any);
	//�������get�������û��������������ȡtask�ķ���ֵ
	Any get();
private:
	Any any_;						//�洢����ķ���ֵ
	Semaphore sem_;					//�߳�ͨ���ź���
	std::shared_ptr<Task> task_;	//ָ����ڻ�ȡ����ֵ���������  ���ü�����Ϊ0��ʱ�� ������������ ����һ����
	std::atomic_bool isValid_;		//����ֵ��Ч
};

//using namespace std; //��ֹȫ��������Ⱦ  ��Ҫд
//����������
class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);
	//�û������Զ��������������ͣ���Task�̳У���дrn������ʵ���Զ���������
	virtual Any run() = 0;  //virtual T run() = 0; �麯����ģ�岻�ܷ���һ��ʹ��
private:
	Result* result_;//�������Ҳʹ��shared_ptr ���������ָ�����ü��� �������õ�����
	//Result������������� ���� Task����������
};

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
	Thread(ThreadFunc func);
	//�߳�����
	~Thread();

	//�����߳�
	void start();

	//��ȡ�߳�id
	int getId() const;
private:
	ThreadFunc func_;
	static int genernateId_;//��̬��Ա���� ��Ҫ�����ʼ��
	int threadId_; //�����߳�id
};

//�̳߳�����  ����źܶ���߳�
class ThreadPool
{
public:
	//�̳߳ع���
	ThreadPool();

	//�̳߳�����
	~ThreadPool();

	// �����̳߳صĹ���ģʽ
	void setMode(PoolMode mode);

	//����task�������������ֵ
	void setTaskQueMaxThreshHold(int threshhold);

	//����task�������������ֵ
	void setThreadSizeThreshHold(int threshhold);

	//���̳߳��ύ����
	Result submitTask(std::shared_ptr<Task> sp);

	//�����̳߳�
	void start(int initThreadSize = std::thread::hardware_concurrency());//��ʼΪ������CPU��������

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;


private:
	//�����̺߳���
	void threadFunc(int threadid);

	//���pool����״̬
	bool checkRunningState() const;
private:
	//std::vector<Thread*> threads_;   ��ָ���Ϊ��������ָ��
	//std::vector<std::unique_ptr<Thread>> threads_;//�߳��б�
	std::unordered_map<int,std::unique_ptr<Thread>> threads_;//�߳��б�
	int initThreadSize_;//��ʼ���̸߳���   int��ʾ�з��ŵģ������������������ﲻ����Ϊ���� ����дunsigned int
	int threadSizeThreadHol_;//�߳��������޵���ֵ
	std::atomic_int curThreadSize_;//��¼��ǰ�̵߳�������
	std::atomic_int idleThreadSize_;//��¼�����̵߳�����

	//std::queue<Task*> ��������ͻᱻ�ͷţ�������ΪҰָ�� ���ܴ�����ָ��
	std::queue<std::shared_ptr<Task>> taskQue_;//�������  
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
