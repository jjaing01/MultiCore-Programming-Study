#include <iostream>
#include <chrono>
#include <vector>
#include <mutex>

using namespace std;
using namespace chrono;

// 1¾ï
#define N 50'000'000
#define CORE 8

mutex my_l;
volatile int sum = 0;
void worker(const int& number)
{
	for (auto i = 0; i < number; ++i)
	{
		my_l.lock();
		sum += 2;
		my_l.unlock();
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