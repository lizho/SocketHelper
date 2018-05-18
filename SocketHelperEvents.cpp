#include "SocketHelperEvents.h"

namespace BlueOrigin
{
	SocketReceivedEvent::SocketReceivedEvent(SocketReceivedEvent&& other) :
		Receiver(other.Receiver),
		BufferSize(other.BufferSize),
		Buffer(other.Buffer)
	{
		other.Receiver = 0ULL;
		other.BufferSize = 0;
		other.Buffer = nullptr;
	}

	SocketReceivedEvent::SocketReceivedEvent(SOCKET s, int bufferSize, const char* buffer) :
		Receiver(s),
		BufferSize(bufferSize),
		Buffer(new char[BufferSize+1])
	{
		memcpy_s(Buffer, BufferSize, buffer, BufferSize);
		Buffer[BufferSize] = 0;
	}

	SocketReceivedEvent::SocketReceivedEvent(SOCKET s, const char* buffer) :
		SocketReceivedEvent::SocketReceivedEvent(s, strlen(buffer), buffer)
	{
	}

	//SocketReceivedEvent::SocketReceivedEvent(const SocketReceivedEvent& other) :
	//	SocketReceivedEvent::SocketReceivedEvent(other.Receiver, other.BufferSize, other.Buffer)
	//{
	//}


	SocketReceivedEvent::~SocketReceivedEvent()
	{
		delete[] Buffer;
		Buffer = nullptr;
	}
}