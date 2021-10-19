#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <set>
using namespace std;
using namespace chrono;

const int NUM_TEST = 4'000'000;
const int RANGE = 1'000;

class NODE
{
public:
	NODE() :value(0), next(nullptr) {}
	NODE(int x) :value(x), next(nullptr) {}
	~NODE() {}

public:
	int value = 0;
	NODE* next = nullptr;
	mutex nlock;
};

class OLIST
{
public:
	OLIST()
	{
		head.value = 0x80000000;
		tail.value = 0x7fffffff;
		head.next = &tail;
	}
	~OLIST() {
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
		}
	}

	bool validate(NODE* pred, NODE* curr)
	{
		NODE* node = &head;
		while (node->value <= pred->value)
		{
			if (node == pred)
				return pred->next == curr;
			node = node->next;
		}
		return false;
	}

	bool Add(int x)
	{
		while (true)
		{
			NODE* pred = &head;
			NODE* curr = pred->next;

			while (curr->value < x)
			{
				pred = curr;
				curr = curr->next;
			}

			pred->nlock.lock();
			curr->nlock.lock();
			if (true == validate(pred, curr))
			{
				if (curr->value == x)
				{
					curr->nlock.unlock();
					pred->nlock.unlock();
					return false;
				}
				else
				{
					NODE* new_node = new NODE(x);
					new_node->next = curr;
					pred->next = new_node;
					curr->nlock.unlock();
					pred->nlock.unlock();
					return true;
				}
			}
			curr->nlock.unlock();
			pred->nlock.unlock();
		}
	}

	bool Remove(int x)
	{
		while (true)
		{
			NODE* pred = &head;
			NODE* curr = pred->next;

			while (curr->value < x)
			{
				pred = curr;
				curr = curr->next;
			}

			pred->nlock.lock();
			curr->nlock.lock();
			if (true == validate(pred, curr))
			{
				if (curr->value == x)
				{
					pred->next = curr->next;
					curr->nlock.unlock();
					pred->nlock.unlock();
					return true;
				}
				else
				{
					curr->nlock.unlock();
					pred->nlock.unlock();
					return false;
				}
			}
			curr->nlock.unlock();
			pred->nlock.unlock();
		}
	}

	bool Contains(int x)
	{
		while (true)
		{
			NODE* pred = &head;
			NODE* curr = pred->next;

			while (curr->value < x)
			{
				pred = curr;
				curr = curr->next;
			}

			pred->nlock.lock();
			curr->nlock.lock();
			if (true == validate(pred, curr))
			{
				curr->nlock.unlock();
				pred->nlock.unlock();
				return curr->value == x;
			}
			curr->nlock.unlock();
			pred->nlock.unlock();
		}
	}

	void Print20()
	{
		NODE* p = head.next;

		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail)
			{
				cout << endl;
				break;
			}
			else
			{
				cout << p->value << ", ";
				p = p->next;
			}
		}
		cout << endl;
	}

public:
	NODE head;
	NODE tail;
};

OLIST olist;
void Benchmark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
	{
		int x = rand() % RANGE;
		switch (rand() % 3)
		{
		case 0:
		{
			olist.Add(x);
		}
		break;
		case 1:
		{
			olist.Remove(x);
		}
		break;
		case 2:
		{
			olist.Contains(x);
		}
		break;
		default:
			break;
		}
	}
}



int main()
{
	for (int i = 1; i <= 8; i *= 2)
	{
		vector<thread> workers;

		olist.Init();

		auto start_time = system_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(Benchmark, i);

		for (auto& th : workers)
			th.join();

		auto end_time = system_clock::now();
		auto exec_t = end_time - start_time;

		olist.Print20();

		cout << i << " threads ";
		cout << "exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
	}
	return 0;
}