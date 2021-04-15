#include <iostream>
#include <string>
#include <chrono>
#include "conn.h"

using namespace std;

volatile int g_send_state = 0;			// 0 : nothing to do,   1: node sent cmd to NIC,  2: NIC sent reply to node
constexpr int NIC_STATE_IDLE = 0;
constexpr int NIC_STATE_HAVE_CMD = 1;
constexpr int NIC_STATE_DONE = 2;

bool active{ false };
string sendMessage{};
string recieveMessage{};
const chrono::microseconds CLOCK{ 100000 };

void send_command_to_NIC()
{
	while (active);
	if (!active)
	{
		g_send_state = NIC_STATE_HAVE_CMD;
	}

	while (g_send_state != NIC_STATE_DONE);

	g_send_state = NIC_STATE_IDLE;
}

void do_node(char node_id)
{
	cout << "Hello World, I am node " << node_id << "." << endl << endl;

	while (true)
	{
		// 전송할 노드와 메세지를 입력
		cout << "Enter destination node with a message to send : ";
		getline(cin, sendMessage);

		send_command_to_NIC();
	}
}

bool send_message(chrono::high_resolution_clock::time_point& tp, int num, unsigned int c, CONN& g_conn)
{
	bool collision{ false };

	for (int i{ 0 }; i < num; ++i)
	{
		bool bit{ (c & (1 << i)) != 0 };
		g_conn.set(bit);

		while (chrono::high_resolution_clock::now() < tp + CLOCK)
		{
			if (g_conn.get() != bit)
				collision = true;
		}

		tp += CLOCK;
	}

	return collision;
}

char recieve_message(chrono::high_resolution_clock::time_point& tp, int num, int cmd, CONN& g_conn)
{
	char c{};

	for (int i{ 0 }; i < num; ++i)
	{
		while (chrono::high_resolution_clock::now() < tp + CLOCK);

		if (chrono::high_resolution_clock::now() > tp + CLOCK)
		{
			if (cmd == 1)
			{
				if (g_conn.get())
				{
					c |= 1 << i;
				}
			}

			tp += CLOCK;
		}
	}

	if (cmd == 1)
	{
		recieveMessage.push_back(c);
	}

	return c;
}

void do_node_NIC(char node_id, CONN& g_conn)
{
	bool read{ false };
	bool finish{ false };

	while (true)
	{
		auto time = chrono::high_resolution_clock::now();

		if (g_send_state == NIC_STATE_HAVE_CMD)
		{
			cout << "You entered [" << sendMessage << "]" << endl << endl;

			// 수신 노드 정보 전송
			send_message(time, 1, 1, g_conn);
			send_message(time, 8, sendMessage[0], g_conn);
			send_message(time, 1, 0, g_conn);

			// 송신 노드 정보 전송
			send_message(time, 1, 1, g_conn);
			send_message(time, 8, node_id, g_conn);
			send_message(time, 1, 0, g_conn);

			// 메세지 전송
			for (int i{ 1 }; i < sendMessage.length(); ++i)
			{
				send_message(time, 1, 1, g_conn);
				send_message(time, 8, sendMessage[i], g_conn);
				send_message(time, 1, 0, g_conn);
			}

			// 전송 종료 알림 전송
			send_message(time, 1, 1, g_conn);
			send_message(time, 8, '\0', g_conn);
			send_message(time, 1, 0, g_conn);

			sendMessage.clear();

			g_send_state = NIC_STATE_DONE;
		}
		else if (g_send_state == NIC_STATE_IDLE)
		{
			if (!read)
			{
				read = g_conn.get();
			}
			else
			{
				active = true;

				// 메세지 수신
				recieve_message(time, 8, 1, g_conn);
				recieve_message(time, 1, 0, g_conn);

				read = false;

				if (recieveMessage.back() == '\0')
				{
					if (recieveMessage[0] == node_id)
					{
						finish = true;
					}
					else
					{
						recieveMessage.clear();

						active = false;
					}
				}
			}

			if (finish)
			{
				cout << endl << "Node " << recieveMessage[1] << " sent [";
				recieveMessage.erase(0, 2);
				recieveMessage.erase(recieveMessage.end() - 1, recieveMessage.end());
				cout << recieveMessage << "]" << endl;

				recieveMessage.clear();

				finish = false;
				active = false;

				cout << endl << "Enter destination node with a message to send : ";
			}
		}
	}
}