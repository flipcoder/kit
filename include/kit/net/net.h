#ifndef NET_H_QTJHVVWD
#define NET_H_QTJHVVWD

#ifdef _MSC_VER
    #ifndef __WIN32__
        #define __WIN32__
    #endif
#endif

#ifdef __WIN32__
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <winsock.h>
    typedef int socklen_t;
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <errno.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #define closesocket close
    #define ioctlsocket ioctl
    typedef int SOCKET;
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET -1
    #define MTU 576
#endif

#include "../async/async.h"

class socket_exception:
    public std::runtime_error
{
    public:
        socket_exception(const std::string& msg):
            std::runtime_error(msg)
        {}
        virtual ~socket_exception() throw() {}
};
    

class TCPSocket
{
    public:
        
        TCPSocket() = default;
        explicit TCPSocket(SOCKET rhs):
            m_Socket(rhs),
            m_bOpen(true)
        {}
        TCPSocket(TCPSocket&& rhs):
            m_Socket(rhs.m_Socket),
            m_bOpen(rhs.m_bOpen)
        {
            rhs.m_bOpen = false;
        }
        TCPSocket(const TCPSocket& rhs) = delete;
        TCPSocket& operator=(TCPSocket&& rhs) {
            if(m_bOpen)
                close();
            m_Socket = std::move(rhs.m_Socket);
            m_bOpen = rhs.m_bOpen;
            rhs.m_bOpen = false;
            return *this;
        }
        TCPSocket& operator=(const TCPSocket& rhs) = delete;
        //TCPSocket(Protocol p):
        //    m_Protocol(p)
        //{}
        virtual ~TCPSocket(){
            if(m_bOpen)
                ::closesocket(m_Socket);
        }
        operator bool() const {
            return m_bOpen;
        }
        void open() {
            if(m_bOpen)
                close();
            
            m_Socket = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            unsigned long SOCKET_BLOCK = 1L;
            int SOCKET_YES = 1;
            ioctlsocket(m_Socket, FIONBIO, &SOCKET_BLOCK);
            setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&SOCKET_YES, sizeof(int));
            
            //else if(m_Protocol == UDP)
                //m_Socket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

            m_bOpen = m_Socket != INVALID_SOCKET;
            if(not m_bOpen)
                throw socket_exception(
                    std::string("TCPSocket::open failed (")+
                    std::to_string(errno)+")"
                );
        }
        bool is_open() const {
            return m_bOpen;
        }
        void close() {
            if(m_bOpen){
                ::closesocket(m_Socket);
                m_bOpen = false;
            }
        }
        SOCKET socket() const { return m_Socket; }
        TCPSocket accept() {
            SOCKET socket;
            sockaddr_storage addr;
            socklen_t addr_sz = sizeof(addr);
            socket = ::accept(m_Socket, (struct sockaddr*)&addr, &addr_sz);
            if(socket == INVALID_SOCKET)
            {
                if(errno == EWOULDBLOCK || errno == EAGAIN)
                    throw kit::yield_exception();
                throw socket_exception(
                    std::string("TCPSocket::accept failed (")+
                    std::to_string(errno)+")"
                );
            }
            return TCPSocket(socket);
        }
        void bind(unsigned short port = 0) {
            sockaddr_in sAddr;

            sAddr.sin_family = AF_INET;
            sAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            sAddr.sin_port = htons(port);
            memset(sAddr.sin_zero, '\0', sizeof(sAddr.sin_zero));

            if(::bind(m_Socket, (sockaddr*)&sAddr, sizeof(sAddr)) == SOCKET_ERROR)
                throw socket_exception(
                    std::string("TCPSocket::bind failed (")+std::to_string(errno)+")"
                );
        }
        void listen(int backlog = 0) {
            if(::listen(m_Socket, backlog) == -1)
                throw socket_exception(
                    std::string("TCPSocket::listen failed (")+
                    std::to_string(errno)+")"
                );
        }
        void connect(std::string ip, short port)
        {
            sockaddr_in destAddr;
            hostent* he;
            char* connectip;
            char* ipp = (char*)ip.c_str();

            //he = gethostbyname(ipp);
            //if(not he)
            //{
            //    // use normal IP
            //    connectip = ipp;
            //}
            //else
            //{
            //    // use hostname IP
            //    //connectip = (char*)inet_ntoa(*(in_addr*)he->h_addr);
            //}

            destAddr.sin_family = AF_INET;
            destAddr.sin_port = htons(port);
            destAddr.sin_addr.s_addr = inet_addr(ipp);
            memset(&(destAddr.sin_zero), '\0', sizeof(destAddr.sin_zero));

            if(::connect(m_Socket, (sockaddr*)&destAddr, sizeof(destAddr))==SOCKET_ERROR)
                throw socket_exception(
                    std::string("TCPSocket::connect failed (")+
                    std::to_string(errno)+")"
                );
                //return false;
            //return true;
        }
        
        // Async usage (inside coroutine or repeated circuit unit):
        //      YIELD_UNTIL(socket.select());
        bool select() const
        {
            fd_set fds_read;
            int fdscount = 0;

            timeval tv = {0,0};
            //tv.tv_sec = 0;
            //tv.tv_usec = 0;

            FD_ZERO(&fds_read);
            FD_SET(m_Socket, &fds_read);

            #ifndef __WIN32__
                fdscount = m_Socket + 1;
            #endif

            switch(::select(fdscount, &fds_read, (fd_set*)0, (fd_set*)0, &tv))
            {
                case 0:
                    return false;
                    break;
                case SOCKET_ERROR:
                    // TODO: throw socket select error
                    return false;
                default:
                    //return true;
                    break;
            }

            return (bool)FD_ISSET(m_Socket, &fds_read);
        }

        void send(const uint8_t* buf, int sz) {
            if(not m_bOpen)
                throw socket_exception("TCPSocket::send socket not open");
            int sent = 0;
            int left = sz;
            int n = 0;
            while(sent < sz)
            {
                n = ::send(m_Socket, (char*)(buf + sent), left, 0);
                if(n==SOCKET_ERROR)
                    throw socket_exception(
                        std::string("TCPSocket::send socket error (")+
                        std::to_string(errno)+")"
                    );
                sent += n;
                left -= n;
            }
        }
        void send(const std::string& buf) {
            send((const uint8_t*)buf.c_str(), (int)buf.size());
        }
        int recv(uint8_t* buf, int sz) {
            if(sz <= 0)
                throw socket_exception("TCPSocket::recv buffer has no space");
            if(not m_bOpen)
                throw socket_exception("TCPSocket::recv socket not open");
            int n = 0;
            n = ::recv(m_Socket, (char*)buf, sz, 0);
            if(n == SOCKET_ERROR){
                auto err = errno;
                if(err == EWOULDBLOCK || err == EAGAIN)
                {
                    throw kit::yield_exception();
                }
                else
                {
                    throw socket_exception(
                        std::string("TCPSocket::recv socket error (")+
                        std::to_string(err)+")"
                    );
                }
            }
            else if(n==0){
                m_bOpen = false;
                throw socket_exception(
                    "TCPSocket::send disconnected"
                );
            }
            return n;
        }
        std::string recv() {
            uint8_t buf[1024];
            int r = recv(buf, sizeof(buf) - 1);
            buf[r] = '\0';
            return std::string((char*)buf);
        }
        
    private:
        
#ifdef __WIN32__
        struct WinSockIniter {
            WinSockIniter() {
                WSADATA wsaData;
                if(WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
                    cerr << "WinSock failed to intialize." << endl;
                    exit(1);
                }
            }
        };
        struct WinSockInitOnce {
            WinSockInitOnce() {
                static WinSockIniter init_once;
            }
        } m_WinSockInit;
#endif
        
    protected:
        
        SOCKET m_Socket;
        bool m_bOpen = false;
};

//class UDPSocket
//{
//};

#endif

