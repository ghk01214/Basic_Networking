#include <iostream>
#include <string>
#include <chrono>
#include <queue>
#include "conn.h"

constexpr int MAX_NODES = 16;
extern const unsigned int NUM_NODES;

using namespace std;

constexpr int NIC_STATE_IDLE = 0;
constexpr int NIC_STATE_HAVE_CMD = 1;
constexpr int NIC_STATE_DONE = 2;

string message[4]{};
const chrono::microseconds CLOCK{ 100000 };

struct NIC {						// g_conn을 관리하기 위한 관리 객체
	string m_nic_name;
	volatile int g_send_state;		// NIC_STATE_IDLE : nothing to do,   
									// NIC_STATE_HAVE_CMD : node sent cmd to NIC,  
									// NIC_STATE_DONE : NIC sent reply to node
	queue<string> messageQueue;
	int padding[64];				// 멀티코어 프로그래밍 시간에 다룰 내용, 성능 향상을 위해서 존재.
};

NIC g_nic[4];

bool send_message(chrono::high_resolution_clock::time_point& tp, int num, unsigned int c, CONN& g_conn);

char recieve_message(chrono::high_resolution_clock::time_point& tp, int num, int cmd, CONN& g_conn, string& recieveMessage)
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

void send_command_to_NIC(NIC& nic)
{
	//while (active);
	//if (!active)
	//{
	//	nic.g_send_state = NIC_STATE_HAVE_CMD;
	//}

	//while (nic.g_send_state != NIC_STATE_DONE);

	//nic.g_send_state = NIC_STATE_IDLE;
}

void do_hub()
{
	//wcout << L"\n허브로 동작합니다. Port들을 초기화 합니다\n";
	cout << "Hello World. I am HUB." << endl;

	// HUB NIC 초기화
	for (unsigned int i = 0; i < NUM_NODES; ++i)
	{
		g_nic[i].m_nic_name = string{ "Port " };
		g_nic[i].m_nic_name += ('A' + i);
		g_nic[i].g_send_state = NIC_STATE_IDLE;
	}

	while (true);
}

void do_hub_NIC0(CONN& g_conn)
{
	bool startReading{ false };
	bool readingFinished{ false };
	string temp{};

	constexpr int PORT_NUM = 0;
	NIC& nic = g_nic[PORT_NUM];

	while (true)
	{
		while (nic.g_send_state == NIC_STATE_IDLE)
		{
			auto time = chrono::high_resolution_clock::now();

			if ((message[PORT_NUM].size() > 0) && !(message[PORT_NUM].front() >= 'A' && message[PORT_NUM].front() <= 'D'))
			{
				cout << "clear!" << endl;
				message[PORT_NUM].clear();
				startReading = false;
			}

			if (!startReading)
			{
				startReading = g_conn.get();
			}
			else
			{
				// 메세지 수신
				recieve_message(time, 8, 1, g_conn, message[PORT_NUM]);
				recieve_message(time, 1, 0, g_conn, message[PORT_NUM]);

				startReading = false;

				//cout << "A : " << message[PORT_NUM] << endl;

				if (message[PORT_NUM].back() == '\0')
				{
					if (message[PORT_NUM][1] == 'A')
					{
						readingFinished = true;
					}
					else
					{
						message[PORT_NUM].clear();
						cout << "clear!" << endl;
					}
				}
			}

			// queue에 메세지가 존재하고 A 노드로부터 전송받고 있는 메세지가 없을 때
			if (!nic.messageQueue.empty() && message[PORT_NUM].empty())
			{
				message[PORT_NUM].clear();
				message[PORT_NUM] = nic.messageQueue.front();

				nic.messageQueue.pop();

				nic.g_send_state = NIC_STATE_HAVE_CMD;
			}

			if (readingFinished)
			{
				temp = message[PORT_NUM];

				cout << endl << "Node " << message[PORT_NUM][1] << " sent [";

				temp.erase(0, 2);
				temp.erase(temp.end() - 1, temp.end());

				cout << temp << "] to Node" << message[PORT_NUM][0] << endl;

				nic.g_send_state = NIC_STATE_HAVE_CMD;
				readingFinished = false;
			}
		}

		if (nic.g_send_state == NIC_STATE_HAVE_CMD)
		{
			auto time = chrono::high_resolution_clock::now();

			// 수신자가 A 노드이면
			if (message[PORT_NUM][0] == 'A')
			{
				// 메세지 전송
				for (int i{ 0 }; i < message[PORT_NUM].length(); ++i)
				{
					send_message(time, 1, 1, g_conn);
					send_message(time, 8, message[PORT_NUM][i], g_conn);
					send_message(time, 1, 0, g_conn);
				}

				// 전송 종료 알림 전송
				send_message(time, 1, 1, g_conn);
				send_message(time, 8, '\0', g_conn);
				send_message(time, 1, 0, g_conn);

				cout << "Complete Sending" << endl;
			}
			else
			{
				cout << "Send Message from " << nic.m_nic_name << ", to Port " << message[PORT_NUM][0] << " : " << temp << endl;

				g_nic[message[PORT_NUM][0] - 'A'].messageQueue.push(message[PORT_NUM]);
			}

			message[PORT_NUM].clear();
			temp.clear();

			nic.g_send_state = NIC_STATE_IDLE;
		}
	}
}

void do_hub_NIC1(CONN& g_conn)
{
	bool startReading{ false };
	bool readingFinished{ false };
	string temp{};

	constexpr int PORT_NUM = 1;
	NIC& nic = g_nic[PORT_NUM];

	while (true)
	{
		while (nic.g_send_state == NIC_STATE_IDLE)
		{
			auto time = chrono::high_resolution_clock::now();

			if ((message[PORT_NUM].size() > 0) && !(message[PORT_NUM].front() >= 'A' && message[PORT_NUM].front() <= 'D'))
			{
				cout << "clear!" << endl;
				message[PORT_NUM].clear();
				startReading = false;
			}

			if (!startReading)
			{
				startReading = g_conn.get();
			}
			else
			{
				// 메세지 수신
				recieve_message(time, 8, 1, g_conn, message[PORT_NUM]);
				recieve_message(time, 1, 0, g_conn, message[PORT_NUM]);

				startReading = false;

				//cout << "B : " << message[PORT_NUM] << endl;

				if (message[PORT_NUM].back() == '\0')
				{
					if (message[PORT_NUM][1] == 'B')
					{
						readingFinished = true;
					}
					else
					{
						message[PORT_NUM].clear();
						cout << "clear!" << endl;
					}
				}
			}

			// queue에 메세지가 존재하고 A 노드로부터 전송받고 있는 메세지가 없을 때
			if (!nic.messageQueue.empty() && message[PORT_NUM].empty())
			{
				message[PORT_NUM].clear();
				message[PORT_NUM] = nic.messageQueue.front();

				nic.messageQueue.pop();

				nic.g_send_state = NIC_STATE_HAVE_CMD;
			}

			if (readingFinished)
			{
				temp = message[PORT_NUM];

				cout << endl << "Node " << message[PORT_NUM][1] << " sent [";

				temp.erase(0, 2);
				temp.erase(temp.end() - 1, temp.end());

				cout << temp << "] to Node" << message[PORT_NUM][0] << endl;

				nic.g_send_state = NIC_STATE_HAVE_CMD;
				readingFinished = false;
			}
		}

		if (nic.g_send_state == NIC_STATE_HAVE_CMD)
		{
			auto time = chrono::high_resolution_clock::now();

			// 수신자가 B 노드이면
			if (message[PORT_NUM][0] == 'B')
			{
				// 메세지 전송
				for (int i{ 0 }; i < message[PORT_NUM].length(); ++i)
				{
					send_message(time, 1, 1, g_conn);
					send_message(time, 8, message[PORT_NUM][i], g_conn);
					send_message(time, 1, 0, g_conn);
				}

				// 전송 종료 알림 전송
				send_message(time, 1, 1, g_conn);
				send_message(time, 8, '\0', g_conn);
				send_message(time, 1, 0, g_conn);

				cout << "Complete Sending" << endl;
			}
			else
			{
				cout << "Send Message from " << nic.m_nic_name << ", to Port " << message[PORT_NUM][0] << " : " << temp << endl;

				g_nic[message[PORT_NUM][0] - 'A'].messageQueue.push(message[PORT_NUM]);
			}

			message[PORT_NUM].clear();
			temp.clear();

			nic.g_send_state = NIC_STATE_IDLE;
		}
	}
}

void do_hub_NIC2(CONN& g_conn)
{
	bool startReading{ false };
	bool readingFinished{ false };
	string temp{};

	constexpr int PORT_NUM = 2;
	NIC& nic = g_nic[PORT_NUM];

	while (true)
	{
		while (nic.g_send_state == NIC_STATE_IDLE)
		{
			auto time = chrono::high_resolution_clock::now();

			if ((message[PORT_NUM].size() > 0) && !(message[PORT_NUM].front() >= 'A' && message[PORT_NUM].front() <= 'D'))
			{
				cout << "clear!" << endl;
				message[PORT_NUM].clear();
				startReading = false;
			}

			if (!startReading)
			{
				startReading = g_conn.get();
			}
			else
			{
				// 메세지 수신
				recieve_message(time, 8, 1, g_conn, message[PORT_NUM]);
				recieve_message(time, 1, 0, g_conn, message[PORT_NUM]);

				startReading = false;

				//cout << "C : " << message[PORT_NUM] << endl;

				if (message[PORT_NUM].back() == '\0')
				{
					if (message[PORT_NUM][1] == 'C')
					{
						readingFinished = true;
					}
					else
					{
						message[PORT_NUM].clear();
						cout << "clear!" << endl;
					}
				}
			}

			// queue에 메세지가 존재하고 A 노드로부터 전송받고 있는 메세지가 없을 때
			if (!nic.messageQueue.empty() && message[PORT_NUM].empty())
			{
				message[PORT_NUM].clear();
				message[PORT_NUM] = nic.messageQueue.front();

				nic.messageQueue.pop();

				nic.g_send_state = NIC_STATE_HAVE_CMD;
			}

			if (readingFinished)
			{
				temp = message[PORT_NUM];

				cout << endl << "Node " << message[PORT_NUM][1] << " sent [";

				temp.erase(0, 2);
				temp.erase(temp.end() - 1, temp.end());

				cout << temp << "] to Node" << message[PORT_NUM][0] << endl;

				nic.g_send_state = NIC_STATE_HAVE_CMD;
				readingFinished = false;
			}
		}

		if (nic.g_send_state == NIC_STATE_HAVE_CMD)
		{
			auto time = chrono::high_resolution_clock::now();

			// 수신자가 C 노드이면
			if (message[PORT_NUM][0] == 'C')
			{
				// 메세지 전송
				for (int i{ 0 }; i < message[PORT_NUM].length(); ++i)
				{
					send_message(time, 1, 1, g_conn);
					send_message(time, 8, message[PORT_NUM][i], g_conn);
					send_message(time, 1, 0, g_conn);
				}

				// 전송 종료 알림 전송
				send_message(time, 1, 1, g_conn);
				send_message(time, 8, '\0', g_conn);
				send_message(time, 1, 0, g_conn);

				cout << "Complete Sending" << endl;
			}
			else
			{
				cout << "Send Message from " << nic.m_nic_name << ", to Port " << message[PORT_NUM][0] << " : " << temp << endl;

				g_nic[message[PORT_NUM][0] - 'A'].messageQueue.push(message[PORT_NUM]);
			}

			message[PORT_NUM].clear();
			temp.clear();

			nic.g_send_state = NIC_STATE_IDLE;
		}
	}
}

void do_hub_NIC3(CONN& g_conn)
{
	bool startReading{ false };
	bool readingFinished{ false };
	string temp{};

	constexpr int PORT_NUM = 3;
	NIC& nic = g_nic[PORT_NUM];

	while (true)
	{
		while (nic.g_send_state == NIC_STATE_IDLE)
		{
			auto time = chrono::high_resolution_clock::now();

			if ((message[PORT_NUM].size() > 0) && !(message[PORT_NUM].front() >= 'A' && message[PORT_NUM].front() <= 'D'))
			{
				cout << "clear!" << endl;
				message[PORT_NUM].clear();
				startReading = false;
			}

			if (!startReading)
			{
				startReading = g_conn.get();
			}
			else
			{
				// 메세지 수신
				recieve_message(time, 8, 1, g_conn, message[PORT_NUM]);
				recieve_message(time, 1, 0, g_conn, message[PORT_NUM]);

				startReading = false;

				//cout << "D : " << message[PORT_NUM] << endl;

				if (message[PORT_NUM].back() == '\0')
				{
					if (message[PORT_NUM][1] == 'D')
					{
						readingFinished = true;
					}
					else
					{
						message[PORT_NUM].clear();
						cout << "clear!" << endl;
					}
				}
			}

			// queue에 메세지가 존재하고 A 노드로부터 전송받고 있는 메세지가 없을 때
			if (!nic.messageQueue.empty() && message[PORT_NUM].empty())
			{
				message[PORT_NUM].clear();
				message[PORT_NUM] = nic.messageQueue.front();

				nic.messageQueue.pop();

				nic.g_send_state = NIC_STATE_HAVE_CMD;
			}

			if (readingFinished)
			{
				temp = message[PORT_NUM];

				cout << endl << "Node " << message[PORT_NUM][1] << " sent [";

				temp.erase(0, 2);
				temp.erase(temp.end() - 1, temp.end());

				cout << temp << "] to Node" << message[PORT_NUM][0] << endl;

				nic.g_send_state = NIC_STATE_HAVE_CMD;
				readingFinished = false;
			}
		}

		if (nic.g_send_state == NIC_STATE_HAVE_CMD)
		{
			auto time = chrono::high_resolution_clock::now();

			// 수신자가 A 노드이면
			if (message[PORT_NUM][0] == 'D')
			{
				// 메세지 전송
				for (int i{ 0 }; i < message[PORT_NUM].length(); ++i)
				{
					send_message(time, 1, 1, g_conn);
					send_message(time, 8, message[PORT_NUM][i], g_conn);
					send_message(time, 1, 0, g_conn);
				}

				// 전송 종료 알림 전송
				send_message(time, 1, 1, g_conn);
				send_message(time, 8, '\0', g_conn);
				send_message(time, 1, 0, g_conn);

				cout << "Complete Sending" << endl;
			}
			else
			{
				cout << "Send Message from " << nic.m_nic_name << ", to Port " << message[PORT_NUM][0] << " : " << temp << endl;

				g_nic[message[PORT_NUM][0] - 'A'].messageQueue.push(message[PORT_NUM]);
			}

			message[PORT_NUM].clear();
			temp.clear();

			nic.g_send_state = NIC_STATE_IDLE;
		}
	}
}