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
    #include <winsock2.h>
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
#endif

#include "../async/async.h"

class Socket
{
    public:
        enum Protocol {
            TCP = 0,
            UDP
        };
    protected:
        SOCKET m_Socket;
        bool m_bOpen = false;
        
        Protocol m_Protocol = TCP;
    public:
        Socket() = default;
        Socket(Socket&& rhs):
            m_Socket(rhs.m_Socket),
            m_bOpen(rhs.m_bOpen),
            m_Protocol(rhs.m_Protocol)
        {
            rhs.m_bOpen = false;
        }
        Socket(Protocol p):
            m_Protocol(p)
        {}
        virtual ~Socket(){
            if(m_bOpen)
                ::closesocket(m_Socket);
        }
        operator bool() const {
            return m_bOpen;
        }
        void open() {
            if(m_bOpen)
                close();
            if(m_Protocol == TCP)
                m_Socket = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
            else if(m_Protocol == UDP)
                m_Socket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

            m_bOpen = (m_Socket != INVALID_SOCKET);
        }
        bool is_open() const {
            return m_bOpen;
        }
        void close() {
            if(m_bOpen)
                ::closesocket(m_Socket);
            m_bOpen = false;
        }
        SOCKET socket() const { return m_Socket; }
        Socket accept() { return Socket(); }
        bool bind(short port = 0) {return true;}
        bool connect(std::string ip, short port)
        {
            sockaddr_in destAddr;
            hostent* he;
            char* connectip;
            char* ipp = (char*)ip.c_str();
            connectip = ipp;

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
            destAddr.sin_addr.s_addr = inet_addr(connectip);
            memset(&(destAddr.sin_zero), '\0', sizeof(destAddr.sin_zero));

            if(::connect(m_Socket, (sockaddr*)&destAddr, sizeof(destAddr))==SOCKET_ERROR)
                return false;
            return true;
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
};

#endif

