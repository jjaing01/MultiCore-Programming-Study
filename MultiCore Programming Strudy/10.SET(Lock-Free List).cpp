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

class LFNODE;
class MPTR
{
public:
	MPTR() :V(0) {}
	~MPTR() {}

public:
	void set_ptr(LFNODE* p)
	{
		V = reinterpret_cast<int>(p);
	}

	LFNODE* get_ptr()
	{
		return reinterpret_cast<LFNODE*>(V & 0xFFFFFFFE);
	}

	bool get_removed(LFNODE*& p) // attempt mark(marking)
	{
		int curr_v = V;
		p = reinterpret_cast<LFNODE*>(curr_v & 0xFFFFFFFE);
		return ((curr_v & 0x1) == 1); // 1이면 removed 된거
	}

	bool CAS(LFNODE* old_p, LFNODE* new_p, bool old_removed, bool new_removed)
	{
		int old_v = reinterpret_cast<int>(old_p);
		if (true == old_removed) old_v++;
		int new_v = reinterpret_cast<int>(new_p);
		if (true == new_removed) new_v++;
		return atomic_compare_exchange_strong(&V, &old_v, new_v);
	}

private:
	atomic_int V;
};

class LFNODE
{
public:
	LFNODE() :value(0) {
		next.set_ptr(nullptr);
	}
	LFNODE(int val) {
		next.set_ptr(nullptr);
		value = val;
	}
	~LFNODE() {}

public:
	int value;
	MPTR next;
};

class LFLIST
{
public:
	LFLIST() {
		head.value = 0x80000000;
		tail.value = 0x7fffffff;
		head.next.set_ptr(&tail);
	}

	~LFLIST() {
		Init();
	}

public:
	void Find(int x, LFNODE*& pred, LFNODE*& curr)
	{
		while (true)
		{
		retry:
			pred = &head;
			curr = pred->next.get_ptr();

			while (true)
			{
				LFNODE* succ;
				bool removed = curr->next.get_removed(succ);
				while (removed)
				{
					if (false == pred->next.CAS(curr, succ, false, false))
						goto retry;

					removed = curr->next.get_removed(succ);
				}

				if (curr->value >= x)
					return;
				pred = curr;
				curr = curr->next.get_ptr();
			}
		}
	}

	void Init()
	{
		while (head.next.get_ptr() != &tail)
		{
			LFNODE* p = head.next.get_ptr();
			head.next.set_ptr(p->next.get_ptr());
			delete p;
		}
	}

	bool Add(int x)
	{
		LFNODE* pred, * curr;

		while (true)
		{
			Find(x, pred, curr);

			if (curr->value == x)
			{
				return false;
			}
			else
			{
				LFNODE* new_node = new LFNODE(x);

				new_node->next.set_ptr(curr);
				if (true == pred->next.CAS(curr, new_node, false, false))
					return true;
				delete new_node;
			}
		}
	}

	bool Remove(int x)
	{
		LFNODE* pred, * curr;

		while (true)
		{
			Find(x, pred, curr);

			if (curr->value != x)
				return false;
			else
			{
				LFNODE* succ = curr->next.get_ptr();
				if (true == curr->next.CAS(succ, succ, false, false))
				{
					pred->next.set_ptr(succ);
					return true;
				}
			}
		}
	}

	bool Contains(int x)
	{
		LFNODE* curr = head.next.get_ptr();
		bool removed = false;

		while (curr->value < x)
		{
			curr = curr->next.get_ptr();

			LFNODE* succ;
			removed = curr->next.get_removed(succ);
		}

		return curr->value == x && !removed;
	}

	void Print()
	{
		LFNODE* p = head.next.get_ptr();
		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail) break;

			printf("%d ", p->value);
			p = p->next.get_ptr();
		}
		cout << endl;
	}
public:
	LFNODE head, tail;
};

LFLIST lflist;
void Benchmark(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i)
	{
		int x = rand() % RANGE;
		switch (rand() % 3)
		{
		case 0:
		{
			lflist.Add(x);
		}
		break;
		case 1:
		{
			lflist.Remove(x);
		}
		break;
		case 2:
		{
			lflist.Contains(x);
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

		lflist.Init();

		auto start_time = system_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(Benchmark, i);

		for (auto& th : workers)
			th.join();

		auto end_time = system_clock::now();
		auto exec_t = end_time - start_time;

		lflist.Print();

		cout << i << " threads ";
		cout << "exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
	}
	return 0;
}