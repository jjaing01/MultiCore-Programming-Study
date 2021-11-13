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
	[���]
	lock�� ���°ź��� ������尡 ����.
	������, ������ �ö��� �ʴ´�. ���ļ��� ������.
	�����尡 ���������� ������ ������. --> CAS ������� ����.

	ť�� ����� ������ ������ �ȿö�.
	CAS�� ���� top�� ���� --> top���� �浹�ϱ� ������ ���ÿ� �Ϸ���� �ʴ´�.
	�ڷᱸ�� ��ü�� ���ļ��� ����.
	lock free �ڷᱸ���� ������ �ƴϴ� --> stack�� Ư�� �����̴�.
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

class LFSTACK
{
public:
	NODE* volatile top;
	bool CAS_TOP(NODE* old_node, NODE* new_node)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_int*>(&top),
			reinterpret_cast<int*>(&old_node),
			reinterpret_cast<int>(new_node)
		);
	}

	LFSTACK()
		: top(nullptr)
	{
	}
	~LFSTACK()
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
		NODE* new_node = new NODE(x);

		while (true)
		{
			NODE* p = top;
			new_node->next = p;
			if (CAS_TOP(p, new_node))
				return;
		}
	}

	int Pop()
	{
		while (true)
		{
			NODE* p = top;
			if (nullptr == p)
				return -2;

			NODE* next = p->next;
			int ret_value = p->value;

			if (CAS_TOP(p, next))
				return ret_value;
		}

		return -2;
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

LFSTACK mystack;

// ���� �׽�Ʈ
void Benchmark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
	{

		if ((0 == rand() % 2) || (i < 1'000 / num_threads))
		{
			mystack.Push(i);
		}
		else
		{
			mystack.Pop();
		}
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