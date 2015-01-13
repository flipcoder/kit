#include "../../include/kit/net/net.h"
#include "../../include/kit/log/log.h"
#include <iostream>
#include <memory>
#include <boost/lexical_cast.hpp>
using namespace std;

int main(int argc, char** argv)
{
    unsigned short port = 1337;
    try{
        if(argc > 1)
            port = boost::lexical_cast<short>(argv[1]);
    }catch(...){}
    
    auto server = make_shared<TCPSocket>();
    server->open();
    server->bind(port);
    server->listen();

    auto fut = MX[0].coro<void>([server]{
        for(;;)
        {
            LOG("awaiting connection");
            auto client = make_shared<TCPSocket>(AWAIT(server->accept()));
            LOG("client connected");
            MX[0].coro<void>([client]{
                LOG("client connected");
                client->send(client->recv());
                LOG("client data echoed");
                LOG("client disconnected");
            });
        }
    });
    
    fut.get();
    return 0;
}

