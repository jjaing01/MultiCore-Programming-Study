#include <iostream>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>

using namespace std;
using namespace chrono;

// 1억
#define N 50'000'000
#define CORE 8

mutex my_l;
volatile int sum = 0;
atomic_bool* g_flag;
atomic_int* g_label;

const int& maxLable(const int& t_num)
{
	int ret = g_label[0];

	for (int i = 0; i < t_num; ++i)
	{
		if (ret < g_label[i])
			ret = g_label[i];
	}
	return ret + 1;
}

const bool& compareLabel(const int& other, const int& me)
{
	if (g_label[other] < g_label[me]) return true;
	else if (g_label[other] > g_label[me]) return false;
	else
	{
		if (other < me) return true;
		else if (other > me) return false;
		else return false;
	}
}

void bakery_lock(const int& t_id, const int& t_num)
{
	// 번호표 발급
	g_flag[t_id] = true;
	g_label[t_id] = maxLable(t_num);
	g_flag[t_id] = false;

	// 다른 스레드와 번호표 비교
	for (int i = 0; i < t_num; ++i)
	{
		while (g_flag[i]);
		while ((g_label[i] != 0) && compareLabel(i, t_id));
	}

}

void bakery_unlock(const int& t_id)
{
	g_label[t_id] = 0;
}

void worker(const int& number, const int& t_id, const int& t_num)
{
	for (auto i = 0; i < number; ++i)
	{
		bakery_lock(t_id, t_num);
		sum += 2;	
		bakery_unlock(t_id);
	}
}
int main()
{
	for (int i = 1; i <= CORE; i *= 2)
	{
		sum = 0;
		int div = N / i;

		g_flag = new atomic_bool[i]{};
		g_label = new atomic_int[i]{};

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

		delete[] g_flag;
		g_flag = nullptr;
		delete[] g_label;
		g_label = nullptr;
	}
	return 0;
}