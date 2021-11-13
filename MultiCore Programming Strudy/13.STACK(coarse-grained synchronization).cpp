#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <set>

using namespace std;
using namespace chrono;

const int NUM_TEST = 10'000'000;

/*
	[결과]
	mutex 오버헤드 때문에 성능저하.
	lock으로 인한 병렬성 감소로 성능 향상이 없다.
*/

class nullmutex
{
public:
	void lock() {}
	void unlock() {}
};

class NODE
{
public:
	int value;
	NODE* volatile next;

	NODE() { }
	NODE(int input_value)
	{
		value = input_value;
		next = nullptr;
	}
	~NODE() { }
};

class CSTACK
{
public:
	NODE* top;
	mutex top_lock;

	CSTACK()
		: top(nullptr)
	{
	}
	~CSTACK()
	{
		Init();
	}

	void Init()
	{
		while (top != nullptr)
		{
			NODE* p = top;
			top = top->next;
			delete p;
		}
	}

	void Push(int x)
	{
		NODE* e = new NODE(x);
		top_lock.lock();

		e->next = top;
		top = e;

		top_lock.unlock();
	}

	int Pop()
	{
		top_lock.lock();
		if (nullptr == top)
		{
			top_lock.unlock();
			return -2;
		}

		int ret_value = top->value;
		NODE* ptr = top;
		top = top->next;

		top_lock.unlock();
		delete ptr;

		return ret_value;
	}

	void Print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i)
		{
			if (p == nullptr)
				break;

			cout << p->value << ", ";
			p = p->next;
		}
		cout << endl;
	}
};

CSTACK mystack;

// 성능 테스트
void Benchmark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
	{

		if ((0 == rand() % 2) || (i < 1'000 / num_threads))		
			mystack.Push(i);	
		else		
			mystack.Pop();
	}
}

int main()
{
	for (int i = 1; i <= 8; i *= 2)
	{
		vector<thread> workers;
		mystack.Init();

		auto start_time = system_clock::now();

		for (int j = 0; j < i; ++j)
			workers.emplace_back(Benchmark, i);

		for (auto& th : workers)
			th.join();

		auto end_time = system_clock::now();
		auto exec_t = end_time - start_time;

		mystack.Print20();

		cout << i << " threads ";
		cout << "exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
	}

	return 0;
}