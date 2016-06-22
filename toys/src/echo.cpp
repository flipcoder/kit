#include "../../kit/net/net.h"
#include "../../kit/log/log.h"
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

    int client_ids = 0;

    auto fut = MX[0].coro<void>([&]{
        for(;;)
        {
            LOG("awaiting connection");
            auto client = make_shared<TCPSocket>(AWAIT(server->accept()));
            MX[0].coro<void>([&, client]{
                int client_id = client_ids++;
                LOGf("client %s connected", client_id);
                try{
                    for(;;)
                        AWAIT(client->send(AWAIT(client->recv())));
                }catch(const socket_exception& e){
                    LOGf("client %s disconnected (%s)", client_id % e.what());
                }
            });
        }
    });
    
    fut.get();
    return 0;
}

