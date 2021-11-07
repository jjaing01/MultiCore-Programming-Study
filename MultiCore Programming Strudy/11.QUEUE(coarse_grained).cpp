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

class CQUEUE
{
public:
    NODE* head;
    NODE* tail;

    mutex enq_lock, deq_lock;

public:
    CQUEUE() { head = tail = new NODE(0); }
    ~CQUEUE() { Init(); }

    void Init()
    {
        while (head != tail)
        {
            NODE* p = head;
            head = head->next;
            delete p;
        }
    }

    void Enq(int x)
    {
        enq_lock.lock();
        NODE* new_node = new NODE(x);
        tail->next = new_node;
        tail = new_node;
        enq_lock.unlock();
    }

    int Deq()
    {
        deq_lock.lock();
        if (head->next == nullptr)
        {
            deq_lock.unlock();
            return -1;
        }

        int result = head->next->value;
        NODE* temp = head;
        head = head->next;
        deq_lock.unlock();
        delete temp;

        return result;
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

CQUEUE myqueue;

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