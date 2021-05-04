#include <iostream>
#include <string>
#include <map>
#include <chrono>
#include <mutex>
#include "conn.h"

using namespace std;

enum FrameType
{
	requestARP = 75, responseARP, requestDHCP, responseDHCP, sendMessage
};

struct ARPRequest
{
	char fromMac = 0;					// 0
	char toMac = 0;						// 0
	char frameType = requestARP;		// ARP request = 75
	int nodeAddress;
};

struct ARPResponse
{
	char fromMac = 0;
	char toMac = 0;
	char frameType = responseARP;		// ARP response = 76
	int nodeAddress;
	int macAddress;
};

struct DHCPRequest
{
	char fromMac;
	const char toMac = 'A';				// should be 0
	char frameType = requestDHCP;		// DHCP request = 77
};

struct DHCPResponse
{
	char fromMac;
	char toMac;
	char frameType = responseDHCP;		// DHCP response = 78
	int lanAddress = 1;
	int nodeAddress;
};

struct SendMessage
{
	char fromNode;
	char toNode;
	char frameType = sendMessage;		// MESSAGE packet = 78
	char lanAddress;
	char data[MAX_DATA_SIZE];
};

class AddressManager
{
public:
	int GetMacAddress(int nodeAddress)
	{
		int macAddress = -1;

		tableLock.lock();

		if (node2macTable.count(nodeAddress) != 0)
			macAddress = node2macTable[nodeAddress];

		tableLock.unlock();

		return macAddress;
	}
	void SetAddress(int macAddress, int nodeAddress)
	{
		tableLock.lock();

		node2macTable[nodeAddress] = macAddress;
		mac2nodeTable[macAddress] = nodeAddress;

		tableLock.unlock();
	}
	int AssignNodeAddress(int macAddress)
	{
		int nodeAddress = -1;

		tableLock.lock();

		if (mac2nodeTable.count(macAddress) == 0)
		{
			for (int i = 2; i < 9; ++i)
			{
				if (node2macTable.count(i) == 0)
				{
					node2macTable[i] = macAddress;
					mac2nodeTable[macAddress] = i;
					nodeAddress = i;

					break;
				}
			}
		}

		tableLock.unlock();

		return nodeAddress;
	}
private:
	mutex tableLock;
	map<char, char> mac2nodeTable;
	map<char, char> node2macTable;
};

volatile bool endState = false;
volatile int sendState = 0;			// 0 : Idle, 1 : Have Message, 2 : Message Sent
const chrono::microseconds CLOCK{ 100000 };
AddressManager addressManager;
DHCPResponse resDHCP;

void DHCP(const char mac_addr, NIC& g_nic)
{
	DHCPRequest reqDHCP;
	reqDHCP.fromMac = mac_addr;

	if (reqDHCP.fromMac != 'A')
	{
		g_nic.SendFrame(sizeof(reqDHCP), &reqDHCP);

		sendState = 1;

		for (int i = 0;; ++i)
		{
			if ((sendState != 2) && (i % 250000000 == 0))
			{
				cout << "Waiting for receiving DHCP response." << endl;
			}
			else if (sendState == 2)
				break;
		}

		sendState = 0;
	}
	else
		resDHCP.nodeAddress = addressManager.AssignNodeAddress(reqDHCP.fromMac);
}

void do_node(NIC& g_nic)
{
	const char mac_addr = g_nic.GetMACaddr();

	cout << "Hello World, I am a node with MAC address [" << mac_addr << "].\n";

	DHCP(mac_addr, g_nic);

	cout << "My LAN address  = " << resDHCP.lanAddress << ",\t My Node address = " << resDHCP.nodeAddress << endl;

	SendMessage message;
	message.fromNode = resDHCP.nodeAddress;

	while (true)
	{
		char tempStr[MAX_DATA_SIZE + 2];
		char* tempLanAddress = tempStr;
		char* tempToNode = tempStr + 1;
		char* tempMessage = tempStr + 2;

		cout << "\nEnter Message to Send : ";
		cin.getline(tempStr, MAX_DATA_SIZE + 2);

		message.lanAddress = *tempLanAddress;
		message.toNode = *tempToNode;
		strcpy_s(message.data, tempMessage);

		int node = atoi(&message.toNode);

		if (message.lanAddress != '1')
		{
			cout << "You are not connected to LAN " << message.lanAddress << endl;
			continue;
		}
		else if (addressManager.GetMacAddress(node) == -1)
		{
			ARPRequest reqARP;
			reqARP.nodeAddress = node;

			g_nic.SendFrame(sizeof(reqARP), &reqARP);

			auto startTime = chrono::high_resolution_clock::now();
			while (chrono::high_resolution_clock::now() < startTime + CLOCK * 50);

			if (addressManager.GetMacAddress(node) == -1)
			{
				cout << "Node " << node << "is not connected to LAN " << message.lanAddress << endl;
				continue;
			}
		}

		g_nic.SendFrame(sizeof(message), &message);
	}
}

void interrupt_from_link(NIC& g_nic, int recv_size, char* frame)
{
	const char mac_addr = g_nic.GetMACaddr();

	switch (frame[2])
	{
	case requestARP:
	{
		ARPRequest* p = (ARPRequest*)frame;

		if (p->nodeAddress == resDHCP.nodeAddress)
		{
			ARPResponse rp;
			rp.nodeAddress = resDHCP.nodeAddress;
			rp.macAddress = mac_addr;

			g_nic.SendFrame(sizeof(rp), &rp);
		}

		break;
	}
	case responseARP:
	{
		ARPResponse* p = (ARPResponse*)frame;
		addressManager.SetAddress(p->macAddress, p->nodeAddress);

		break;
	}
	case requestDHCP:
	{
		DHCPRequest* p = (DHCPRequest*)frame;

		if (mac_addr == 'A')
		{
			DHCPResponse rp;

			rp.nodeAddress = addressManager.AssignNodeAddress(p->fromMac);
			rp.fromMac = mac_addr;
			rp.toMac = p->fromMac;

			g_nic.SendFrame(sizeof(rp), &rp);
		}

		break;
	}
	case responseDHCP:
	{
		DHCPResponse* p = (DHCPResponse*)frame;

		if (mac_addr != p->toMac)
			break;

		resDHCP = *p;
		sendState = 2;

		break;
	}
	case sendMessage:
	{
		SendMessage* p = (SendMessage*)frame;
		char nodeAddress[10];

		_itoa_s(resDHCP.nodeAddress, nodeAddress, 10);

		if (p->toNode != *nodeAddress)
			return;

		cout << "\n\nMessage from Node " << (int)p->fromNode << " : ";
		cout << p->data << endl;
		cout << "\nEnter Message to Send : ";

		break;
	}
	default:
		break;
	}
}