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

    int num_clients = 0;

    auto fut = MX[0].coro<void>([&num_clients, server]{
        for(;;)
        {
            LOG("awaiting connection");
            auto client = make_shared<TCPSocket>(AWAIT(server->accept()));
            MX[0].coro<void>([&num_clients, client]{
                int client_id = num_clients;
                LOGf("client %s connected", client_id);
                ++num_clients;
                try{
                    string msg;
                    for(;;)
                    {
                        msg = AWAIT(client->recv());
                        cout << msg << endl;
                        client->send(msg);
                    }
                }catch(const socket_exception& e){
                    LOGf("client %s disconnected (%s)", client_id % e.what());
                }
                --num_clients;
            });
        }
    });
    
    fut.get();
    return 0;
}

