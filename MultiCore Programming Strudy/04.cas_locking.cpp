#include <iostream>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>

using namespace std;
using namespace chrono;

// 1억 만들기
#define N 50'000'000
#define CORE 8

mutex my_l;
volatile int sum = 0;
volatile int g_lock = 0; // -- CAS LOCK / UNLOCK

const bool& CAS(volatile int* value, int expected, int update)
{
	return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(value), &expected, update);
}

void CAS_LOCK()
{
	while (false == CAS(&g_lock, 0, 1));
}

void CAS_UNLOCK()
{
	//g_lock = 0;
	while (false == CAS(&g_lock, 1, 0));
}

void worker(const int& number, const int& t_id, const int& t_num)
{
	for (auto i = 0; i < number; ++i)
	{
		CAS_LOCK();
		sum += 2;
		CAS_UNLOCK();
	}
}

int main()
{
	for (int i = 1; i <= CORE; i *= 2)
	{
		sum = 0;
		int div = N / i;

		vector<thread> workers;
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(worker, div, j, i);
		for (auto& th : workers)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(end_t - start_t).count();

		cout << "-------------------------------------" << endl;
		cout << "number of threads = " << i << endl;
		cout << "exec time = " << duration << "ms" << endl;
		cout << "sum = " << sum << endl;
	}
	return 0;
}