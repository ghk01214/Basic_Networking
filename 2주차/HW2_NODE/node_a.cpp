#include <iostream>
#include <string>
#include <thread>
#include "conn.h"
#define LAGTIME 50ms
#define WAIT	200ms

using namespace std;

void do_node(char node_id)
{
	bool startNode{ false };

	// 입력 버퍼 초기화
	getchar();

	while (true)
	{
		if (node_id == 'A' || node_id == 'a')
		{
			if (!startNode)
			{
				cout << "노드 A를 시작합니다." << endl;
				startNode = true ;
			}

			string str;

			cout << "\n문자를 전송할 노드의 ID와 전송할 문자를 같이 입력 : ";
			getline(cin, str);

			// 노드 B~D로 전송 시작을 알림
			g_conn.set(true);

			// 문자를 보낼 노드의 정보를 전송
			for (int i = 0; i < 8; ++i)
			{
				g_conn.set(str[0] & true);
				this_thread::sleep_for(LAGTIME);

				str[0] >>= 1;
			}

			this_thread::sleep_for(WAIT);

			// 문자열의 길이를 전송
			for (int i = 0; i < 8; ++i)
			{
				int size = str.size();
				size >>= i;

				g_conn.set(size & true);
				this_thread::sleep_for(LAGTIME);
			}

			this_thread::sleep_for(WAIT);

			// 문자열 전송
			for (int i = 1; i < str.size() + 1; ++i)
			{
				for (int j = 0; j < 8; ++j)
				{
					g_conn.set(str[i] & true);
					this_thread::sleep_for(LAGTIME);

					str[i] >>= 1;
				}

				this_thread::sleep_for(WAIT);
			}
		}
		// ASCII 코드 값이 대소문자 b~d 사이의 값일 때
		else if ((node_id >= 66 && node_id <= 68) || (node_id >= 98 && node_id <= 100))
		{
			if (g_conn.get())
			{
				//cout << "노드 A의 메세지를 수신 중" << endl;
				
				char nodeID{};

				this_thread::sleep_for(20ms);

				for (int i = 0; i < 8; ++i)
				{
					nodeID |= g_conn.get() << i;

					this_thread::sleep_for(LAGTIME);
				}

				this_thread::sleep_for(WAIT);

				if (node_id != nodeID)
				{
					g_conn.set(false);
					continue;
				}

				cout << "\n노드 A의 메세지를 수신 중" << endl;

				int size{};

				for (int i = 0; i < 8; ++i)
				{
					size |= g_conn.get() << i;

					this_thread::sleep_for(LAGTIME);
				}

				this_thread::sleep_for(WAIT);

				string str{};
				str.resize(size);

				for (int j = 0; j < size; ++j)
				{
					for (int i = 0; i < 8; ++i)
					{
						str[j] |= g_conn.get() << i;

						this_thread::sleep_for(LAGTIME);
					}

					this_thread::sleep_for(WAIT);
				}

				cout << "노드 A로부터 [" << str << "]를 받았습니다." << endl << endl;
				g_conn.set(false);
			}
		}
	}
}