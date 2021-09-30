#include <iostream>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>

using namespace std;
using namespace chrono;

// 1¾ï
#define N 50'000'000
#define CORE 8

mutex my_l;
atomic<int> sum = 0;
void worker(const int& number)
{
	volatile int local_sum = 0;
	for (auto i = 0; i < number; ++i)
	{
		local_sum = local_sum + 2;
	}
	sum += local_sum;
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
			workers.emplace_back(worker, div);
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