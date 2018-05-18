#include <WinSock2.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include "SocketHelper.h"
using namespace std;
#pragma comment(lib, "ws2_32.lib")

vector<string> split(const string& str, const string& delim)
{
	vector<string> res;
	if ("" == str) return res;

	auto p = strtok(const_cast<char*>(str.c_str()), delim.c_str());
	while (p)
	{
		res.push_back(p);
		p = strtok(NULL, delim.c_str());
	}

	return res;
}

enum class command
{
	Listen, StopListen, Connect, Disconnect, Send, Exit, None
};

/*constexpr*/ command parseCmd(string& cmd);
void usage();

int main(int, char**)
{
	BlueOrigin::SocketHelper bosh;
	string BufferIn;
	
	SOCKET sock;
	string addr;
	BlueOrigin::SocketHelper::Port_t port;
	auto run = true;

	while (run)
	{
		cout << "=> " << flush;
		getline(cin, BufferIn, '\n');
		
		auto xx = split(BufferIn, " :");
		if (xx.size() == 0) continue;

		try
		{
			switch (parseCmd(xx.front()))
			{
			case command::Listen:
				if (xx.size() < 2) goto exception_tag;
				bosh.StartListen(stoul(xx[1]));
				break;
			case command::StopListen:
				if (xx.size() < 2) goto exception_tag;
				bosh.StopListen(stoul(xx[1]));
				break;
			case command::Connect:
				if (xx.size() < 2) goto exception_tag;
				if (xx.size() < 3) xx.insert(--xx.cend(), "127.0.0.1");
				port = stoul(xx[2]);
				sock = bosh.ConnectTo(xx[1].c_str(), port);
				if (sock)
				{
					cout << "connected to " << xx[1].c_str() << ':' << port
						<< " via SOCKET " << sock << ".\nYou may use\n  send " << sock
						<< " [message]\nto send message to the host you just connected to."
						<< endl;
				}
				else
				{
					cout << "connect failed!" << endl;
				}
				break;
			case command::Disconnect:
				if (xx.size() < 2) goto exception_tag;
				bosh.StopReceive(stoul(xx[1]));
				break;
			case command::Send:
				if (xx.size() < 3) goto exception_tag;
				bosh.Send(stoul(xx[1]), xx[2].c_str(), xx[2].size());
				break;
			case command::Exit:
				run = false;
				break;
			case command::None:
			exception_tag :
				usage();
				 break;
			default:
				break;
			}
		}
		catch (const exception& ex)
		{
			cout << ex.what() << endl;
			cout << "Bad command" << endl;
		}
	}

	bosh.Shutdown();
	bosh.Wait();

	system("pause");
	return 0;
}

command parseCmd(string& cmd)
{
	if (cmd == "listen") return command::Listen;
	if (cmd == "stop") return command::StopListen;
	if (cmd == "conn") return command::Connect;
	if (cmd == "disconn") return command::Disconnect;
	if (cmd == "send") return command::Send;
	if (cmd == "exit" || cmd == "quit") return command::Exit;
	return command::None;
}

void usage()
{
	cout << " Usage:\n\tYou guess!\n" << endl;
}
