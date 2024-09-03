//�̳߳���Ŀ.cpp ���ļ�����main���� ����ִ�н��ڴ˴���ʼ������

#include <iostream>
#include "threadpool.h"
#include<thread>
#include<chrono>

/*
1 + .... +300000000�ĺ�
thread1		1 + .... +100000000
thread2		1000000001 + .... +200000000
thread3		2000000001 + .... +300000000

main thread����ÿ���̷߳�����������  ���ȴ����㷵�صĽ�����ϲ����յĽ��
*/

using uLong = unsigned long long;

class MyTask : public Task
{

public:
	MyTask(int begin, int end)
		:begin_(begin)
		,end_(end)
	{}
	//����һ����ô���run�����ķ���ֵ,���Ա�ʾ���������
	//Java Python   Object ����
	//C++17 any
	Any run()//run�����������̳߳ط�����߳���ȥ��ִ����
	{
		std::cout << "tid" << std::this_thread::get_id() <<"begin��"<< std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		uLong sum = 0;
		for (uLong i = begin_; i <= end_; i++)
		{
			sum += i;
		}
		std::cout << "tid" << std::this_thread::get_id() << "end��"<<std::endl;
		return sum;
	}
private:
	int begin_;//�з���������ߴ�20��
	int end_;
	 
};
int main() {
	{

	ThreadPool pool;
	pool.setMode(PoolMode::MODE_CACHED);
		pool.start(2); // ��ʼ�����̳߳�
		//std::this_thread::sleep_for(std::chrono::seconds(10)); // �ȴ�����ִ��
		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));

		pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		uLong sum1 = res1.get().cast_<uLong>();
		std::cout << sum1 << std::endl;
		
	}
	std::cout << "main over" << std::endl;

#if 0
	//ThreadPolol���������Ժ󣬰��̳߳���ص��߳���Դȫ������
	
	{
		ThreadPool pool;
		//�û�����Լ��Ĺ���ģʽ
		pool.setMode(PoolMode::MODE_CACHED);
		//��ʼ�����̳߳�
		pool.start(4);
		std::cout << "Submitting tasks..." << std::endl;

		//�����������Result����
		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
		Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		std::this_thread::sleep_for(std::chrono::seconds(10)); // �ȴ�����ִ��
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		//����task��ִ�����ˣ�task����û�ˣ�������task�����result����Ҳû��
		uLong sum1 = res1.get().cast_<uLong>();//get����һ��Any���ͣ���ôתΪ����������أ�
		uLong sum2 = res2.get().cast_<uLong>();
		uLong sum3 = res3.get().cast_<uLong>();

		//Master - slave�߳�ģ��
		//Master�߳������ֽ�����Ȼ�������salve�̷߳�������
		//�ȴ�����slave�߳�ִ�������񣬷��ؽ��
		//Master�̺߳ϲ����������������

		std::cout << (sum1 + sum2 + sum3) << std::endl;
		//pool.submitTask(std::make_shared<MyTask>());
		//pool.submitTask(std::make_shared<MyTask>());
		//pool.submitTask(std::make_shared<MyTask>());
		//pool.submitTask(std::make_shared<MyTask>());
	}
#endif
	getchar();// �ȴ��û������Ա�鿴���
}