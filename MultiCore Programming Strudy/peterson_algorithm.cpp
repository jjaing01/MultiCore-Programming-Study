#include <iostream>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>

using namespace std;
using namespace chrono;

// 1억
#define N 50'000'000
#define CORE 2

mutex my_l;
volatile int sum = 0;
volatile bool flag[CORE] = { false,false };
volatile int victim = 0; // 데드락 탈출용

void p_lock(const int& t_id)
{
	int other = 1 - t_id;
	flag[t_id] = true;
	victim = t_id;
	while ((flag[other] == true) && (t_id == victim));
}

void p_unlock(const int& t_id)
{
	flag[t_id] = false;
}


void worker(const int& number,const int& t_id)
{
	for (auto i = 0; i < number; ++i)
	{
		p_lock(t_id);
		sum += 2;
		p_unlock(t_id);
	}
}
int main()
{
	for (int i = 2; i <= CORE; i *= 2)
	{
		sum = 0;
		int div = N / i;

		vector<thread> workers;
		auto start_t = high_resolution_clock::now();
		for (int j = 0; j < i; ++j)
			workers.emplace_back(worker, div,j);
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