#include "SocketHelper.h"
#include "SocketHelperEvents.h"
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <mutex>

using namespace std;

#ifndef SH_LOG
#define SH_LOG(_fmt, ...) printf(_fmt, ##__VA_ARGS__)
#endif // !SH_LOG

static const auto sleepTime = chrono::milliseconds(200);
static const timeval tv{ 0, 200000 };

namespace BlueOrigin
{
	SocketHelper::SocketHelper() :
		SocketAccpted(bind(&SocketHelper::DefaultSocketAccptedHandler, this, placeholders::_1, placeholders::_2)),
		SocketReceived(bind(&SocketHelper::DefaultSocketReceivedHandler, this, placeholders::_1, placeholders::_2)),
		ListenThread(&SocketHelper::ListenHandler, this),
		ReceiveThread(&SocketHelper::ReceiveHandler, this)
	{
		auto wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		if (::WSAStartup(wVersionRequested, &wsaData))
			throw exception("WSAStartup failed!");

		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
		{
			::WSACleanup();
			throw exception("WSAStartup failed!");
		}
	}

	SocketHelper::~SocketHelper()
	{
		SH_LOG("~SocketHelper() begin\n");
		if (ListenThread.joinable()) ListenThread.join();
		if (ReceiveThread.joinable()) ReceiveThread.join();
		WSACleanup();
		SH_LOG("~SocketHelper()  end \n");
	}

	bool SocketHelper::StartListen(Port_t port)
	{
		auto sockSrv = ::socket(AF_INET, SOCK_STREAM, 0);
		SOCKADDR_IN addrSrv = { AF_INET, ::htons(port), 0UL };

		AtomLock lock(AtomOperation);
		if (::bind(sockSrv, reinterpret_cast<SOCKADDR*>(&addrSrv), sizeof SOCKADDR) == SOCKET_ERROR
			|| ::listen(sockSrv, 10) == SOCKET_ERROR)
		{
			::closesocket(sockSrv);
			return false;
		}

		FD_SET(sockSrv, &ListenerSet);
		SH_LOG("Listen started, socket: %d\n", sockSrv);
		return true;
	}

	void SocketHelper::StopListen(SOCKET listener)
	{
		AtomLock lock(AtomOperation);
		FD_CLR(listener, &ListenerSet);
		closesocket(listener);
	}

	void SocketHelper::StopAllListen()
	{
		AtomLock lock(AtomOperation);
		for (auto& sock : ListenerSet.fd_array)
		{
			closesocket(sock);
		}
		FD_ZERO(&ListenerSet);
	}

	void SocketHelper::ListenHandler()
	{
		FD_ZERO(&ListenerSet);
		fd_set fds;
		auto count = 0;
		SOCKADDR_IN addrCli;
		SOCKET sock;
		static int saLen = sizeof SOCKADDR;
		while (Loop)
		{
			if (ListenerSet.fd_count == 0)
			{
				this_thread::sleep_for(sleepTime);
				continue;
			}
			FDSet(ListenerSet, fds);
			count = ::select(fds.fd_count, &fds, nullptr, nullptr, &tv);
			for (size_t i = 0; count > 0 && Loop && i < ListenerSet.fd_count; i++)
			{
				if (FD_ISSET(ListenerSet.fd_array[i], &fds))
				{
					sock = ::accept(ListenerSet.fd_array[i], reinterpret_cast<SOCKADDR*>(&addrCli), &saLen);
					if (SocketAccpted)
					{
						SocketAccpted(this, SocketAcceptedEvent{ ListenerSet.fd_array[i], sock, addrCli });
					}
					--count;
				}
			}
		}
		SH_LOG("Listen loop break.\n");
	}

	void SocketHelper::ReceiveHandler()
	{
		FD_ZERO(&ReceiverSet);
		fd_set fds;
		auto count = 0;
		static const auto bufferCapcity = 9192;
		char buffer[bufferCapcity + 1];
		auto bufferSize = 0;
		while (Loop)
		{
			if (ReceiverSet.fd_count == 0)
			{
				this_thread::sleep_for(sleepTime);
				continue;
			}
			FDSet(ReceiverSet, fds);
			count = ::select(fds.fd_count, &fds, nullptr, nullptr, &tv);
			for (size_t i = 0; count > 0 && Loop && i < ReceiverSet.fd_count; i++)
			{
				if (FD_ISSET(ReceiverSet.fd_array[i], &fds))
				{
					bufferSize = ::recv(ReceiverSet.fd_array[i], buffer, bufferCapcity, 0);
					if (SocketReceived)
					{
						SocketReceived(this, SocketReceivedEvent{ ReceiverSet.fd_array[i], bufferSize, buffer });
					}
					--count;
				}
			}
		}
		SH_LOG("Receive loop break.\n");
	}

	void SocketHelper::StartReceive(SOCKET receiver)
	{
		AtomLock lock(AtomOperation);
		FD_SET(receiver, &ReceiverSet);
	}

	void SocketHelper::StopReceive(SOCKET receiver)
	{
		AtomLock lock(AtomOperation);
		FD_CLR(receiver, &ReceiverSet);
		closesocket(receiver);
	}

	void SocketHelper::StopAllReceive()
	{
		AtomLock lock(AtomOperation);
		auto count = ReceiverSet.fd_count;
		FD_ZERO(&ReceiverSet.fd_count);
		for (size_t i = 0; i < count; i++)
		{
			::closesocket(ReceiverSet.fd_array[i]);
		}
	}

	void SocketHelper::DefaultSocketAccptedHandler(void*, SocketAcceptedEvent& e)
	{
		StartReceive(e.Receiver);
		SH_LOG("Connected socket: %d, from %d.%d.%d.%d:%d\n",
			e.Receiver,
			e.Client.sin_addr.S_un.S_un_b.s_b1,
			e.Client.sin_addr.S_un.S_un_b.s_b2,
			e.Client.sin_addr.S_un.S_un_b.s_b3,
			e.Client.sin_addr.S_un.S_un_b.s_b4,
			e.Client.sin_port);
	}

	void SocketHelper::DefaultSocketReceivedHandler(void*, SocketReceivedEvent& e)
	{
		if (e.BufferSize == 0)
		{
			StopReceive(e.Receiver);
			SH_LOG("Disconnected socket: %d\n", e.Receiver);
			return;
		}
		if (*e.Buffer == '*')
			::send(e.Receiver, "+PONG\r\n", 7, 0);
		SH_LOG(" Received sock %d, data: \n%s\n", e.Receiver, e.Buffer);
	}

	int SocketHelper::FDSet(const SocketSet& set, fd_set& fds)
	{
		AtomLock lock(AtomOperation);
		//fds.fd_count = 0;
		//auto itSock = set.cbegin();
		//while (fds.fd_count < set.size())
		//{
		//	fds.fd_array[fds.fd_count++] = *itSock++;
		//}

		fds.fd_count = set.fd_count;
		memcpy_s(fds.fd_array, FD_SETSIZE  * sizeof SOCKET, set.fd_array, fds.fd_count * sizeof SOCKET);

		return fds.fd_count;
	}

	void SocketHelper::Shutdown()
	{
		Loop = false;
		StopAllListen();
		StopAllReceive();
	}

	SOCKET SocketHelper::ConnectTo(const char* ip, Port_t port)
	{
		auto sockCli = ::socket(AF_INET, SOCK_STREAM, 0);
		SOCKADDR_IN addrSrv = { AF_INET, ::htons(port) };
		addrSrv.sin_addr.s_addr = ::inet_addr(ip);
		//::bind(sockCli, reinterpret_cast<SOCKADDR*>(&addrSrv), sizeof SOCKADDR);
		if (connect(sockCli, reinterpret_cast<SOCKADDR*>(&addrSrv), sizeof SOCKADDR) != SOCKET_ERROR)
		{
			AddReceiver(sockCli);
			return sockCli;
		}
		auto eorrno = GetLastError();
		closesocket(sockCli);
		return 0;
	}

	int SocketHelper::Send(SOCKET sock, const char* msg, size_t size)
	{
		return ::send(sock, msg, size, 0);
	}
}