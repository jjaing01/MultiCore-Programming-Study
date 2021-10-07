#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <set>

using namespace std;
using namespace chrono;

const int NUM_TEST = 4000000;
const int RANGE = 1000;

class nullmutex
{
public:
	void lock() {}
	void unlock() {}
};


/*SET*/
class NODE
{
public:
	NODE() :next(nullptr), value(0) {};
	NODE(int val) :value(val), next(nullptr) {}
	~NODE() {}

public:
	int value;
	NODE* next;
};

class CLIST
{
public:
	CLIST() {
		head.value = 0x80000000;
		tail.value = 0x7fffffff;
		head.next = &tail;
	}

	~CLIST() {
		Init();
	}

public:
	void Init()
	{
		while (head.next != &tail)
		{
			NODE* p = head.next;
			head.next = p->next;
			delete p;
			p = nullptr;
		}
	}

	bool Add(int x)
	{
		NODE* pred{ nullptr };
		NODE* curr{ nullptr };
		
		pred = &head;

		ll.lock();
		curr = pred->next;
		
		while (curr->value < x)
		{	
			pred = curr;
			curr = curr->next;	
		}

		if (curr->value == x)
		{
			ll.unlock();
			return false;
		}
		else
		{
			NODE* new_node = new NODE(x);
			pred->next = new_node;
			new_node->next = curr;
			ll.unlock();
			return true;
		}
	}

	bool Remove(int x)
	{
		NODE* pred{ nullptr };
		NODE* curr{ nullptr };

		pred = &head;
		
		ll.lock();
		curr = pred->next;

		while (curr->value < x)
		{
			pred = curr;
			curr = curr->next;
		}

		if (curr->value != x)
		{
			ll.unlock();
			return false;
		}
		else
		{
			pred->next = curr->next;
			delete curr;
			ll.unlock();
			return true;
		}
	}


	bool Contains(int x)
	{
		NODE* curr{ nullptr };

		ll.lock();
		curr = head.next;

		while (curr->value < x)
		{
			curr = curr->next;
		}

		if (curr->value != x)
		{
			ll.unlock();
			return false;
		}
		else
		{
			ll.unlock();
			return true;
		}
	}

	void Print()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail) break;

			cout << p->value << " ";
			p = p->next;
		}

	}

public:
	NODE head, tail;
	mutex ll;
};


CLIST clist;
set<int> set_list;
mutex stll;
// 구현 버전

void Benchmark(int t_num)
{
	for (int i = 0; i < NUM_TEST / t_num; ++i)
	{
		int x = rand() % RANGE;
		switch (rand() % 3)
		{
		case 0:
			clist.Add(x);
			break;
		case 1:
			clist.Remove(x);
			break;
		case 2:
			clist.Contains(x);
			break;
		}
	}
}

// stl 버전
void Benchmark2(int t_num)
{
	for (int i = 0; i < NUM_TEST / t_num; ++i)
	{
		int x = rand() % RANGE;
		switch (rand() % 3)
		{
		case 0:
			stll.lock();
			set_list.insert(x);
			stll.unlock();
			break;
		case 1:
			stll.lock();
			set_list.erase(x);
			stll.unlock();
			break;
		case 2:
			stll.lock();
			set_list.count(x);
			stll.unlock();
			break;
		}
	}
}



int main()
{
	for (int i = 1; i <= 8; i = i * 2)
	{
		vector<thread> workers;

		clist.Init();

		auto start_t = system_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(Benchmark, i);

		for (auto& th : workers)
			th.join();

		auto end_t = system_clock::now();
		auto duration = duration_cast<milliseconds>(end_t - start_t).count();

		clist.Print();

		cout << endl;
		cout << i << " Threads ";
		cout << "exec Time = " << duration << "ms\n";
		cout << endl;
	}
	return 0;
}


// [Coarse-grained(성긴 동기화) 방식]
// 성능 향상이 없다.

/*
[USER::SET - LOCK]
1 Threads - 1060ms
2 Threads - 1174ms
4 Threads - 1449ms
8 Threads - 3769ms

[STL::SET - LOCK]
1 Threads - 583ms
2 Threads - 590ms
4 Threads - 759ms
8 Threads - 1041ms
*/