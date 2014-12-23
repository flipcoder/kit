#include "../include/kit/net/net.h"
#include "../include/kit/log/log.h"
#include <iostream>
#include <memory>
#include <boost/lexical_cast.hpp>
using namespace std;

int main(int argc, char** argv)
{
    unsigned short port = 1337;
    if(argc > 1)
        port = boost::lexical_cast<short>(argv[1]);
    
    auto server = make_shared<TCPSocket>();
    server->open();
    server->bind(port);
    server->listen();

    auto fut = MX[0].coro<void>([server]{
        for(;;)
        {
            LOG("awaiting connection");
            auto client = make_shared<TCPSocket>(AWAIT(server->accept()));
            LOG("connected");
            MX[0].coro<void>([client]{
                LOG("waiting on client");
                auto msg = client->recv();
                client->send(msg);
                LOG("goodbye client!");
            });
        }
    });
    
    fut.get();
    return 0;
}

