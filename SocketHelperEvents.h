#pragma once
#include <WinSock2.h>

namespace BlueOrigin
{
	class SockAddrInfo
	{
	public:
		SOCKET Sock;
		USHORT Port;
		SOCKADDR_IN Addr;
	};

	class SocketAcceptedEvent
	{
	public:
		SOCKET Listener;
		SOCKET Receiver;
		SOCKADDR_IN Client;
	};

	class SocketReceivedEvent
	{
	public:
		SocketReceivedEvent(SocketReceivedEvent&& other);
		SocketReceivedEvent(SOCKET s, int bufferSize, const char* buffer);
		SocketReceivedEvent(SOCKET s, const char* buffer);
		SocketReceivedEvent(const SocketReceivedEvent&) = delete;
		~SocketReceivedEvent();
		SOCKET Receiver;
		int BufferSize;
		char* Buffer;
	};
}