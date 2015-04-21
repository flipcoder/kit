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

#include <boost/lexical_cast.hpp>
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
    

class ISocket
{
    public:
        virtual ~ISocket() {}

        virtual operator bool() const = 0;
        virtual void open() = 0;
        virtual void close() = 0;
        virtual SOCKET socket() = 0;
        virtual void connect(std::string ip, uint16_t port) = 0;
        virtual bool select() const = 0;
        virtual void send(const uint8_t* buf, int sz) = 0;
        virtual void send(std::string buf) = 0;
        virtual int recv(uint8_t* buf, int sz) = 0;
        virtual std::string recv() = 0;

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
};

class Address
{
    public:
        Address() {
            clear();
        }
        explicit Address(const sockaddr_in& info) {
            m_Addr.sin_family = info.sin_family;
            m_Addr.sin_addr.s_addr = info.sin_addr.s_addr;
            m_Addr.sin_port = info.sin_port;
            memset(m_Addr.sin_zero, '\0', sizeof(m_Addr.sin_zero));
        }
        Address(const Address&) = default;
        Address(Address&&) = default;
        Address& operator=(const Address&) = default;
        Address& operator=(Address&&) = default;
        Address(std::string ip_and_port) {
            std::vector<std::string> tokens;
            boost::split(tokens, ip_and_port, boost::is_any_of(":"));
            if(tokens.size() != 2)
                throw std::out_of_range("unable to parse address");
            m_Addr.sin_addr.s_addr = inet_addr(tokens.at(0).c_str());
            m_Addr.sin_port = htons(boost::lexical_cast<uint16_t>(tokens.at(1)));
        }
        Address(std::string ip, uint16_t port) {
            m_Addr.sin_addr.s_addr = inet_addr(ip.c_str());
            m_Addr.sin_port = htons(port);
        }
        ~Address() {}
        
        std::string ip() const {
            return std::string(inet_ntoa(m_Addr.sin_addr));
        }
        uint16_t port() const { return ntohs(m_Addr.sin_port); }
        operator std::string() const {
            return ip() + ":" + std::to_string(port());
        }
        
        socklen_t size() const {
            return sizeof(m_Addr);
        }
        void clear()
        {
            memset(&m_Addr, 0, sizeof(m_Addr));
            m_Addr.sin_family = AF_INET;
            memset(m_Addr.sin_zero, '\0', sizeof(m_Addr.sin_zero));
        }
        sockaddr_in* address() const {
            return &m_Addr;
        }
    private:
        mutable sockaddr_in m_Addr;
};

class TCPSocket:
    public ISocket
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
        virtual operator bool() const override {
            return m_bOpen;
        }
        virtual void open() override {
            if(m_bOpen)
                close();
            
            m_Socket = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            unsigned long SOCKET_BLOCK = 1L;
            int SOCKET_YES = 1;
            ioctlsocket(m_Socket, FIONBIO, &SOCKET_BLOCK);
            setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&SOCKET_YES, sizeof(int));
            
            m_bOpen = m_Socket != INVALID_SOCKET;
            if(not m_bOpen)
                throw socket_exception(
                    std::string("TCPSocket::open failed (")+
                    std::to_string(errno)+")"
                );
        }
        virtual void close() override {
            if(m_bOpen){
                ::closesocket(m_Socket);
                m_bOpen = false;
            }
        }
        virtual SOCKET socket() override { return m_Socket; }
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
        void bind(uint16_t port = 0) {
            sockaddr_in sAddr;

            sAddr.sin_family = AF_INET;
            sAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            sAddr.sin_port = htons(port);
            memset(sAddr.sin_zero, '\0', sizeof(sAddr.sin_zero));

            if(::bind(m_Socket, (struct sockaddr*)&sAddr, sizeof(sAddr)) == SOCKET_ERROR)
                throw socket_exception(
                    std::string("TCPSocket::bind failed (")+std::to_string(errno)+")"
                );
        }
        void listen(int backlog = 5) {
            if(::listen(m_Socket, backlog) == -1)
                throw socket_exception(
                    std::string("TCPSocket::listen failed (")+
                    std::to_string(errno)+")"
                );
        }
        virtual void connect(std::string ip, uint16_t port) override
        {
            sockaddr_in destAddr;
            hostent* he;
            char* connectip;
            char* ipp = (char*)ip.c_str();

            destAddr.sin_family = AF_INET;
            destAddr.sin_port = htons(port);
            destAddr.sin_addr.s_addr = inet_addr(ipp);
            memset(&(destAddr.sin_zero), '\0', sizeof(destAddr.sin_zero));

            if(::connect(m_Socket, (struct sockaddr*)&destAddr, sizeof(destAddr))==SOCKET_ERROR)
                throw socket_exception(
                    std::string("TCPSocket::connect failed (")+
                    std::to_string(errno)+")"
                );
        }
        
        // Async usage (inside coroutine or repeated circuit unit):
        //      YIELD_UNTIL(socket.select());
        virtual bool select() const override
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

        virtual void send(const uint8_t* buf, int sz) override {
            if(not m_bOpen)
                throw socket_exception("TCPSocket::send socket not open");
            int sent = 0;
            int left = sz;
            int n = 0;
            while(sent < sz)
            {
                n = ::send(m_Socket, (char*)(buf + sent), left, 0);
                if(n==SOCKET_ERROR){
                    if(errno == EWOULDBLOCK || errno == EAGAIN)
                        throw kit::yield_exception();
                    else
                        throw socket_exception(
                            std::string("TCPSocket::send socket error (")+
                            std::to_string(errno)+")"
                        );
                }
                sent += n;
                left -= n;
            }
        }
        virtual void send(std::string buf) override {
            send((const uint8_t*)buf.c_str(), (int)buf.size());
        }
        virtual int recv(uint8_t* buf, int sz) override {
            if(sz <= 0)
                throw socket_exception("TCPSocket::recv buffer has no space");
            if(not m_bOpen)
                throw socket_exception("TCPSocket::recv socket not open");
            int n = 0;
            n = ::recv(m_Socket, (char*)buf, sz, 0);
            if(n == SOCKET_ERROR){
                if(errno == EWOULDBLOCK || errno == EAGAIN)
                    throw kit::yield_exception();
                else
                    throw socket_exception(
                        std::string("TCPSocket::recv socket error (")+
                        std::string(strerror(errno))+")"
                    );
            }
            else if(n==0){
                m_bOpen = false;
                throw socket_exception(
                    "TCPSocket::send disconnected"
                );
            }
            return n;
        }
        virtual std::string recv() override {
            uint8_t buf[1024];
            int r = recv(buf, sizeof(buf) - 1);
            buf[r] = '\0';
            return std::string((char*)buf, r);
        }
        
    protected:
        
        SOCKET m_Socket;
        bool m_bOpen = false;
};

class UDPSocket:
    public ISocket
{
    public:
        UDPSocket() = default;
        explicit UDPSocket(SOCKET rhs):
            m_Socket(rhs),
            m_bOpen(true)
        {}
        UDPSocket(UDPSocket&& rhs):
            m_Socket(rhs.m_Socket),
            m_bOpen(rhs.m_bOpen)
        {
            rhs.m_bOpen = false;
        }
        UDPSocket(const UDPSocket& rhs) = delete;
        UDPSocket& operator=(UDPSocket&& rhs) {
            if(m_bOpen)
                close();
            m_Socket = std::move(rhs.m_Socket);
            m_bOpen = rhs.m_bOpen;
            rhs.m_bOpen = false;
            return *this;
        }
        UDPSocket& operator=(const UDPSocket& rhs) = delete;
        //UDPSocket(Protocol p):
        //    m_Protocol(p)
        //{}
        virtual ~UDPSocket(){
            if(m_bOpen)
                ::closesocket(m_Socket);
        }
        virtual void open() override {
            if(m_bOpen)
                close();
            
            m_Socket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
            
            unsigned long SOCKET_BLOCK = 1L;
            int SOCKET_YES = 1;
            ioctlsocket(m_Socket, FIONBIO, &SOCKET_BLOCK);
            setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&SOCKET_YES, sizeof(int));
            
            m_bOpen = m_Socket != INVALID_SOCKET;
            if(not m_bOpen)
                throw socket_exception(
                    std::string("UDPSocket::open failed (")+
                    std::to_string(errno)+")"
                );
        }
        virtual void connect(std::string ip, uint16_t port) override {
            sockaddr_in destAddr;
            hostent* he;
            char* connectip;
            char* ipp = (char*)ip.c_str();

            destAddr.sin_family = AF_INET;
            destAddr.sin_port = htons(port);
            destAddr.sin_addr.s_addr = inet_addr(ipp);
            memset(&(destAddr.sin_zero), '\0', sizeof(destAddr.sin_zero));

            if(::connect(m_Socket, (struct sockaddr*)&destAddr, sizeof(destAddr))==SOCKET_ERROR)
                throw socket_exception(
                    std::string("UDPSocket::connect failed (")+
                    std::to_string(errno)+")"
                );

        }
        virtual operator bool() const override {
            return m_bOpen;
        }

        virtual void close() override {
            if(m_bOpen)
            {
                ::closesocket(m_Socket);
                m_bOpen = false;
            }
        }
        virtual SOCKET socket() override { return m_Socket; }
        virtual bool select() const override
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
        void send_to(const Address& addr, const uint8_t* buf, int sz)
        {
            if(not m_bOpen)
                throw socket_exception("UDPSocket::send socket not open");
            int sent = 0;
            int left = sz;
            int n = 0;
            while(sent < sz)
            {
                n = ::sendto(
                        m_Socket, (char*)(buf + sent), left, 0,
                        (struct sockaddr*)addr.address(),
                        addr.size()
                    );
                if(n==SOCKET_ERROR){
                    if(errno == EWOULDBLOCK || errno == EAGAIN)
                        throw kit::yield_exception();
                    else
                        throw socket_exception(
                            std::string("UDPSocket::send socket error (")+
                            std::to_string(errno)+")"
                        );
                }
                sent += n;
                left -= n;
            }

        }
        virtual void send(const uint8_t* buf, int sz) override
        {
            if(not m_bOpen)
                throw socket_exception("UDPSocket::send socket not open");
            int sent = 0;
            int left = sz;
            int n = 0;
            while(sent < sz)
            {
                n = ::send(m_Socket, (char*)(buf + sent), left, 0);
                if(n==SOCKET_ERROR){
                    if(errno == EWOULDBLOCK || errno == EAGAIN)
                        throw kit::yield_exception();
                    else
                        throw socket_exception(
                            std::string("UDPSocket::send socket error (")+
                            std::to_string(errno)+")"
                        );
                }
                sent += n;
                left -= n;
            }
        }
        virtual void send(std::string buf) override {
            send((const uint8_t*)buf.c_str(), (int)buf.size());
        }
        int recv_from(Address& addr, uint8_t* buf, int sz) {
            if(sz <= 0)
                throw socket_exception("UDPSocket::recv buffer has no space");
            if(not m_bOpen)
                throw socket_exception("UDPSocket::recv socket not open");
            int n = 0;
            socklen_t addr_len = addr.size();
            n = ::recvfrom(
                    m_Socket, (char*)buf, sz, 0,
                    (struct sockaddr*)addr.address(),
                    &addr_len
                );
            if(n == SOCKET_ERROR){
                if(errno == EWOULDBLOCK || errno == EAGAIN)
                    throw kit::yield_exception();
                else
                    throw socket_exception(
                        std::string("UDPSocket::recv socket error (")+
                        std::string(strerror(errno))+")"
                    );
            }
            else if(n==0){
                m_bOpen = false;
                throw socket_exception(
                    "UDPSocket::send disconnected"
                );
            }
            return n;
        }

        virtual int recv(uint8_t* buf, int sz) override {
            if(sz <= 0)
                throw socket_exception("UDPSocket::recv buffer has no space");
            if(not m_bOpen)
                throw socket_exception("UDPSocket::recv socket not open");
            int n = 0;
            n = ::recv(m_Socket, (char*)buf, sz, 0);
            if(n == SOCKET_ERROR){
                if(errno == EWOULDBLOCK || errno == EAGAIN)
                    throw kit::yield_exception();
                else
                    throw socket_exception(
                        std::string("UDPSocket::recv socket error (")+
                        std::string(strerror(errno))+")"
                    );
            }
            else if(n==0){
                m_bOpen = false;
                throw socket_exception(
                    "UDPSocket::send disconnected"
                );
            }
            return n;
        }
        virtual std::string recv() override {
            uint8_t buf[1024];
            int r = recv(buf, sizeof(buf) - 1);
            buf[r] = '\0';
            return std::string((char*)buf, r);
        }
        
    protected:
        
        SOCKET m_Socket;
        bool m_bOpen = false;
};

#endif

