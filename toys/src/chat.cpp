#include "../../include/kit/net/net.h"
#include "../../include/kit/log/log.h"
#include <iostream>
#include <memory>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
using namespace std;

// This is a basic chat server. To chat with other clients, use netcat:
//  nc localhost 1337
// The first message you type will be your username.

// Notes:
//   - TCP-based
//   - Async
//   - single strand/circuit for simplicity (hence no added locks/atomics)

struct Client
{
    Client(shared_ptr<TCPSocket> socket):
        socket(socket)
    {}
    
    Client(const Client&) = default;
    Client(Client&&) = default;
    Client& operator=(const Client&) = default;
    Client& operator=(Client&&) = default;
    
    string name;
    shared_ptr<TCPSocket> socket;
};

void broadcast(std::string text, vector<shared_ptr<Client>> clients)
{
    LOG(text);
    for(auto&& client: clients)
        AWAIT(client->socket->send(text + "\n"));
}

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
    
    vector<shared_ptr<Client>> clients;

    auto fut = MX[0].coro<void>([&]{
        for(;;)
        {
            auto socket = make_shared<TCPSocket>(AWAIT(server->accept()));
            MX[0].coro<void>([&, socket]{

                // add this client
                auto client = make_shared<Client>(socket);
                clients.push_back(client);

                try{
                    for(;;)
                    {
                        // recv message from client
                        auto msg = AWAIT(socket->recv());

                        boost::algorithm::trim(msg);

                        // set client name
                        if(client->name.empty() && not msg.empty()) {
                            client->name = msg;
                            broadcast(client->name + " connected.", clients);
                            continue;
                        }
                        
                        // send chat message
                        broadcast(client->name + ": " + msg, clients);
                    }
                }catch(const socket_exception& e){
                    if(not client->name.empty())
                        LOGf("%s disconnected (%s)", client->name % e.what());
                }
            });
        }
    });
    
    fut.get();
    return 0;
}

