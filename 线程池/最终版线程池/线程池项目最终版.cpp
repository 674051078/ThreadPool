// 线程池项目最终版.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include"threadPool.h"
using namespace std;

/*
如何让线程池提交任务更加方便
1.pool.submiyTask(sum1,10,20)
pool.submitTask(sum1,12,22,33)
submitTask：可编程模板编程

2.我们自己造了一个Result以及相关的类型，代码挺多
C++11线程库threadpackaged21task6function函数对象)  async
使用future来代替Result节省线程池代码
*/
int sum1(int a, int b)
{
	return a + b;
}
int sum2(int a, int b)
{
	return a + b;
}

int main()
{
	ThreadPool pool;
	pool.setMode(PoolMode::MODE_CACHED);
	pool.start(4);
	future<int> r1 = pool.sbumitTask(sum1, 1, 2); 
	future<int> r2 = pool.sbumitTask(sum2, 1, 4);
	future<int> r3 = pool.sbumitTask(sum2, 1, 10);
	future<int> r4 = pool.sbumitTask([](int b,int e) ->int {
		int sum = 0;
		for (int i = b; i < e; i++)
		{
			sum += i;
		}
		return sum;
	}, 1, 100);
	future<int> r5 = pool.sbumitTask([](int b, int e) ->int {
		int sum = 0;
		for (int i = b; i < e; i++)
		{
			sum += i;
		}
		return sum;
		}, 1, 100);
	cout << r1.get() << endl;
	cout << r2.get() << endl;
	cout << r3.get() << endl;
	cout << r4.get() << endl;
	cout << r5.get() << endl;
}
