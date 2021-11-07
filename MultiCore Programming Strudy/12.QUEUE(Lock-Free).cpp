#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

const int NUM_TEST = 10'000'000;

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
};

class LFQUEUE
{
public:
    NODE* volatile head;
    NODE* volatile tail;

public:
    LFQUEUE() { head = tail = new NODE(0); }
    ~LFQUEUE() { Init(); }

    void Init()
    {
        while (head != tail)
        {
            NODE* p = head;
            head = head->next;
            delete p;
        }
    }

    bool CAS(NODE* volatile* addr, NODE* old_node, NODE* new_node)
    {
        int a_addr = reinterpret_cast<int>(addr);

        return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(a_addr),
            reinterpret_cast<int*>(&old_node),
            reinterpret_cast<int>(new_node));
    }

    void Enq(int x)
    {
        NODE* new_node = new NODE(x);

        while (true)
        {
            NODE* last = tail;
            NODE* next = last->next;
            if (last != tail) continue;
            if (nullptr == next)
            {
                if (CAS(&(last->next), nullptr, new_node))
                {
                    CAS(&tail, last, new_node);
                    return;
                }
            }
            else
                CAS(&tail, last, next);
        }
    }

    int Deq()
    {
        while (true)
        {
            NODE* first = head;
            NODE* last = tail;
            NODE* next = first->next;

            if (first != head) continue;
            if (nullptr != next) return -1;
            if (first == last)
            {
                CAS(&tail, last, next);
                continue;
            }

            int value = next->value;
            if (false == CAS(&head, first, next)) continue;
            delete first;
            return value;
        }
    }

    void Print20()
    {
        NODE* p = head->next;

        for (int i = 0; i < 20; ++i)
        {
            if (p == nullptr)
                break;
            else
            {
                cout << p->value << ", ";
                p = p->next;
            }
        }
        cout << endl;
    }
};

LFQUEUE myqueue;

// 성능 테스트
void Benchmark(int num_threads)
{
    for (int i = 0; i < NUM_TEST / num_threads; ++i)
    {
        if ((0 == rand() % 2) || (i < 32 / num_threads))
            myqueue.Enq(i);
        else
            myqueue.Deq();
    }
}

int main()
{
    for (int i = 1; i <= 8; i *= 2)
    {
        vector<thread> workers;
        myqueue.Init();

        auto start_time = system_clock::now();

        for (int j = 0; j < i; ++j)
            workers.emplace_back(Benchmark, i);

        for (auto& th : workers)
            th.join();

        auto end_time = system_clock::now();
        auto exec_t = end_time - start_time;

        myqueue.Print20();

        cout << i << " threads ";
        cout << "exec_time = " << duration_cast<milliseconds>(exec_t).count() << "ms" << endl;
    }

    return 0;
}