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

class SPNODE
{
public:
	SPNODE() :value(-1), next(nullptr), removed(false) {}
	SPNODE(int x) :value(x), next(nullptr), removed(false) {}
	~SPNODE() {}

public:
	volatile bool removed;
	int value = 0;
	shared_ptr<SPNODE> next;
	mutex nlock;
};

class SPZLIST
{
public:
	SPZLIST()
	{
		head = make_shared<SPNODE>(0x80000000);
		tail = make_shared<SPNODE>(0x7FFFFFFF);
		head->next = tail;
	}
	~SPZLIST() {
		Init();
	}

public:
	void Init()
	{
		head->next = tail;
	}

	bool validate(const shared_ptr<SPNODE>& pred, const shared_ptr<SPNODE>& curr)
	{
		return ((pred->removed == false) && (curr->removed == false) && (pred->next == curr));
	}

	bool Add(int x)
	{
		shared_ptr<SPNODE> pred, curr;
		while (true)
		{
			pred = head;
			curr = pred->next;

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
					shared_ptr<SPNODE> new_node = make_shared<SPNODE>(x);
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
		shared_ptr<SPNODE> pred, curr;
		while (true)
		{
			pred = head;
			curr = pred->next;

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
					curr->removed = true;
					atomic_thread_fence(memory_order_seq_cst);
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
		shared_ptr<SPNODE> curr = head;
		while (curr->value < x)
		{
			curr = curr->next;
		}
		return ((curr->value == x) && (curr->removed == false));
	}

	void Print20()
	{
		shared_ptr<SPNODE> p = head->next;

		for (int i = 0; i < 20; ++i)
		{
			if (p == tail)
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
	shared_ptr<SPNODE> head;
	shared_ptr<SPNODE> tail;
};

SPZLIST spzlist;
void Benchmark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
	{
		int x = rand() % RANGE;
		switch (rand() % 3)
		{
		case 0:
		{
			spzlist.Add(x);
		}
		break;
		case 1:
		{
			spzlist.Remove(x);
		}
		break;
		case 2:
		{
			spzlist.Contains(x);
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

		spzlist.Init();

		auto start_time = system_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(Benchmark, i);

		for (auto& th : workers)
			th.join();

		auto end_time = system_clock::now();
		auto exec_t = end_time - start_time;

		spzlist.Print20();

		cout << i << " threads ";
		cout << "exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
	}
	return 0;
}