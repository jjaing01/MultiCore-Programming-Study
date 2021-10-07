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

class NODE
{
public:
	NODE() :next(nullptr), value(0) {};
	NODE(int val) :value(val), next(nullptr) {}
	~NODE() {}

public:
	int value;
	NODE* next;
	mutex nlock;
};

class FLIST
{
public:
	FLIST() {
		head.value = 0x80000000;
		tail.value = 0x7fffffff;
		head.next = &tail;
	}

	~FLIST() {
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

		pred->nlock.lock();
		curr = pred->next;

		curr->nlock.lock();
		while (curr->value < x)
		{
			pred->nlock.unlock();
			pred = curr;
			curr = curr->next;
			curr->nlock.lock();
		}

		if (curr->value == x)
		{
			curr->nlock.unlock();
			pred->nlock.unlock();
			return false;
		}
		else
		{
			NODE* new_node = new NODE(x);
			pred->next = new_node;
			new_node->next = curr;
			curr->nlock.unlock();
			pred->nlock.unlock();
			return true;
		}
	}

	bool Remove(int x)
	{
		NODE* pred{ nullptr };
		NODE* curr{ nullptr };

		pred = &head;

		pred->nlock.lock();
		curr = pred->next;

		curr->nlock.lock();
		while (curr->value < x)
		{
			pred->nlock.unlock();
			pred = curr;
			curr = curr->next;
			curr->nlock.lock();
		}

		if (curr->value != x)
		{
			curr->nlock.unlock();
			pred->nlock.unlock();

			return false;
		}
		else
		{
			pred->next = curr->next;
			curr->nlock.unlock();
			pred->nlock.unlock();
			delete curr;

			return true;
		}
	}

	bool Contains(int x)
	{
		NODE* curr{ nullptr };

		head.nlock.lock();
		curr = head.next;
		head.nlock.unlock();

		curr->nlock.lock();
		while (curr->value < x)
		{
			NODE* temp = curr;
			curr = curr->next;
			temp->nlock.unlock();
			curr->nlock.lock();
		}

		if (curr->value != x)
		{
			curr->nlock.unlock();
			return false;
		}
		else
		{
			curr->nlock.unlock();
			return true;
		}
	}

	void Print()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail) break;

			printf("%d ", p->value);
			p = p->next;
		}
	}
public:
	NODE head, tail;
};


FLIST flist;
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
			flist.Add(x);
			break;
		case 1:
			flist.Remove(x);
			break;
		case 2:
			flist.Contains(x);
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

		flist.Init();

		auto start_t = system_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(Benchmark, i);

		for (auto& th : workers)
			th.join();

		auto end_t = system_clock::now();
		auto duration = duration_cast<milliseconds>(end_t - start_t).count();

		flist.Print();

		cout << endl;
		cout << i << " Threads ";
		cout << "exec Time = " << duration << "ms\n";
		cout << endl;
	}
	return 0;
}
