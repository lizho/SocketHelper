#pragma once
#include <WinSock2.h>
#include <set>
#include <thread>
#include <list>
#include <mutex>
#include "SocketHelperEvents.h"

#ifndef ADD_FD_SET_WITH_LOCK
#define ADD_FD_SET_WITH_LOCK(_Type) \
	void Add##_Type(SOCKET _Type, bool lock = true) { \
		if (lock) AtomOperation.lock(); \
		FD_SET(_Type, &_Type##Set); \
		if (lock) AtomOperation.unlock(); }
#endif // !ADD_FD_SET_WITH_LOCK

#ifndef REMOVE_FD_SET_WITH_LOCK
#define REMOVE_FD_SET_WITH_LOCK(_Type) \
	void Remove##_Type(SOCKET _Type, bool lock = true) { \
		if (lock) AtomOperation.lock(); \
		FD_SET(_Type, &_Type##Set); \
		if (lock) AtomOperation.unlock(); }
#endif // !REMOVE_FD_SET_WITH_LOCK

#ifndef CLEAR_FD_SET
#define CLEAR_FD_SET(_Type) \
	void Clear##_Type() {FD_ZERO(&_Type##Set);}
#endif // !CLEAR_FD_SET



namespace BlueOrigin
{
	class SocketHelper
	{
	public:
		using SocketSet = fd_set;// std::set<SOCKET, std::greater<>>;
		using Port_t = USHORT;

		using SocketAccptedHandler = std::function<void(void*, SocketAcceptedEvent&)>;
		using SocketReceivedHandler = std::function<void(void*, SocketReceivedEvent&)>;

		SocketAccptedHandler SocketAccpted;
		SocketReceivedHandler SocketReceived;

		SocketHelper();
		~SocketHelper();
		
		bool StartListen(Port_t port);
		void StopListen(SOCKET listener);
		void StopAllListen();

		void StartReceive(SOCKET receiver);
		void StopReceive(SOCKET receiver);
		void StopAllReceive();

		void Shutdown();
		void Wait()
		{
			while (Loop)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}

		SOCKET ConnectTo(const char* ip, Port_t port);
		int Send(SOCKET sock, const char* msg, size_t size);

		ADD_FD_SET_WITH_LOCK(Listener);
		ADD_FD_SET_WITH_LOCK(Receiver);
		REMOVE_FD_SET_WITH_LOCK(Listener);
		REMOVE_FD_SET_WITH_LOCK(Receiver);
		CLEAR_FD_SET(Listener);
		CLEAR_FD_SET(Receiver);


	protected:
		int FDSet(const SocketSet& set, fd_set& fds);
		void ListenHandler();
		void ReceiveHandler();

	private:
		void DefaultSocketAccptedHandler(void*, SocketAcceptedEvent&);
		void DefaultSocketReceivedHandler(void*, SocketReceivedEvent&);
		SocketSet ListenerSet;
		SocketSet ReceiverSet;
		std::thread ListenThread;
		std::thread ReceiveThread;
		std::mutex AtomOperation;
		bool Loop = true;

		using AtomLock = std::lock_guard<std::mutex>;
	};
}

