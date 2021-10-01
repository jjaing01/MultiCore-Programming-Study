#include <iostream>
#include <vector>
#include <mutex>
#include <atomic>

using namespace std;

const int SIZE = 50'000'000;
volatile int x, y;
int trace_x[SIZE];
int trace_y[SIZE];

void worker1()
{
	for (int i = 0; i < SIZE; ++i)
	{
		x = i;
		atomic_thread_fence(memory_order_seq_cst);
		trace_y[i] = y;
	}
}
void worker2()
{
	for (int i = 0; i < SIZE; ++i)
	{
		y = i;
		atomic_thread_fence(memory_order_seq_cst);
		trace_x[i] = x;
	}
}

int main()
{
	thread t1{ worker1 };
	thread t2{ worker2 };
	t1.join();
	t2.join();

	int iCnt = 0;
	for (int i = 0; i < SIZE - 1; ++i) // i == y
	{
		// 연속된 값이 아니라면 검사 X
		if (trace_x[i] != trace_x[i + 1]) continue;

		int x_value = trace_x[i];
		if (trace_y[x_value] != trace_y[x_value + 1]) continue;

		if (i != trace_y[x_value]) continue;

		//cout << "x =" << x_value << ", y =" << trace_y[x_value] << endl;
		iCnt++;
	}

	cout << "Memory Consistency Violation: " << iCnt << endl;

	return 0;
}