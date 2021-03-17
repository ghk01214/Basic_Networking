#include <iostream>
#include <string>
#include <thread>
#include "conn.h"

using namespace std;

void do_node_a()
{
	bool end_node = false;
	string str{};

	cout << "Hello World, I am node A.\n";
	do {
		cout << "Enter a char to send: ";
		getline(cin, str);
		cout << "You enterd " << str << endl << endl;

		g_conn.set(true);

		int size = str.size();

		for (int i = 0; i < 8; ++i)
		{
			g_conn.set(size & true);

			size >>= 1;

			this_thread::sleep_for(250ms);
		}

		for (int j = 0; j < str.size(); ++j)
		{
			for (int i = 0; i < 8; ++i)
			{
				g_conn.set(str[j] & true);

				str[j] >>= 1;

				this_thread::sleep_for(250ms);
			}
		}
		g_conn.set(false);
	} while (false == end_node);
	cout << "Bye.\n";
}