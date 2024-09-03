//线程池项目.cpp 此文件包含main函数 程序执行将在此处开始并结束

#include <iostream>
#include "threadpool.h"
#include<thread>
#include<chrono>

/*
1 + .... +300000000的和
thread1		1 + .... +100000000
thread2		1000000001 + .... +200000000
thread3		2000000001 + .... +300000000

main thread：给每个线程分配计算的区间  并等待计算返回的结果，合并最终的结果
*/

using uLong = unsigned long long;

class MyTask : public Task
{

public:
	MyTask(int begin, int end)
		:begin_(begin)
		,end_(end)
	{}
	//问题一：怎么设计run函数的返回值,可以表示任意的类型
	//Java Python   Object 基类
	//C++17 any
	Any run()//run方法最终在线程池分配的线程中去做执行了
	{
		std::cout << "tid" << std::this_thread::get_id() <<"begin！"<< std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		uLong sum = 0;
		for (uLong i = begin_; i <= end_; i++)
		{
			sum += i;
		}
		std::cout << "tid" << std::this_thread::get_id() << "end！"<<std::endl;
		return sum;
	}
private:
	int begin_;//有符号整形最高存20亿
	int end_;
	 
};
int main() {
	{

	ThreadPool pool;
	pool.setMode(PoolMode::MODE_CACHED);
		pool.start(2); // 开始启动线程池
		//std::this_thread::sleep_for(std::chrono::seconds(10)); // 等待任务执行
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
	//ThreadPolol对象析构以后，把线程池相关的线程资源全部回收
	
	{
		ThreadPool pool;
		//用户设计自己的工作模式
		pool.setMode(PoolMode::MODE_CACHED);
		//开始启动线程池
		pool.start(4);
		std::cout << "Submitting tasks..." << std::endl;

		//如何设计这里的Result机制
		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
		Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		std::this_thread::sleep_for(std::chrono::seconds(10)); // 等待任务执行
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

		//随着task被执行完了，task对象没了，依赖与task对象的result对象也没了
		uLong sum1 = res1.get().cast_<uLong>();//get返回一个Any类型，怎么转为具体的类型呢？
		uLong sum2 = res2.get().cast_<uLong>();
		uLong sum3 = res3.get().cast_<uLong>();

		//Master - slave线程模型
		//Master线程用来分解任务，然后给各个salve线程分配任务
		//等待各个slave线程执行完任务，返回结果
		//Master线程合并各个任务结果，输出

		std::cout << (sum1 + sum2 + sum3) << std::endl;
		//pool.submitTask(std::make_shared<MyTask>());
		//pool.submitTask(std::make_shared<MyTask>());
		//pool.submitTask(std::make_shared<MyTask>());
		//pool.submitTask(std::make_shared<MyTask>());
	}
#endif
	getchar();// 等待用户输入以便查看输出
}