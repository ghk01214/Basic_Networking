#include <iostream>
#include <thread>
#include <string>
#include "conn.h"

using namespace std;

void do_node_b()
{
	bool end_node = false;

	cout << "Hello World, I am node B.\n";
	cout << "Waiting for a Message from Node A" << endl;
	do {
		bool gcinet_bit = false;
		string str{};
		int size{};

		if (g_conn.get())
		{
			for (int i = 0; i < 8; ++i)
			{
				this_thread::sleep_for(250ms);

				size |= g_conn.get() << i;
			}

			for (int j = 0; j < size; ++j)
			{
				for (int i = 0; i < 8; ++i)
				{
					this_thread::sleep_for(250ms);

					str[j] |= g_conn.get() << i;
				}
			}

			cout << "Node A Send a char [" << str << "]" << endl << endl;
			cout << "Waiting for a Message from Node A" << endl;
		}
	} while (false == end_node);
	cout << "Bye.\n";
}