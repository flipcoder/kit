# kit
Really cool C++ stuff, including modern async

Open-source under MIT License

Copyright (c) 2013 Grady O'Connell

## async
- Coroutines w/ YIELD(), AWAIT(), and SLEEP()
- Channels
- Async Sockets
- Event Multiplexer

```c++
// MX thread 0, void future
MX[0].coro<void>([]{
    // do async stuff
    auto foo = AWAIT(bar);

    // async sleep yield
    SLEEP(chrono::milliseconds(100));
});
```

```c++
// socket example
MX[0].coro<void>([&]{
    for(;;)
    {
        auto client = make_shared<TCPSocket>(AWAIT(server->accept()));
        
        // coroutine per client
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

```

## reactive
signals, reactive values (signal-paired vars), and lazy evaluation

## meta
JSON-compatible serializable meta-objects, property trees

## freq
Timelines, alarms, animation/easing, waypoints/keyframes, interpolation

## log
Logger w/ error handling, thread-safe scoped indent, silencing, and capturing

## math
some math stuff to use with glm

## common (kit.h)
Common stuff used by other modules, including:

- freezable: freeze objects as immutable
- make_unique: clone of c++14 function
- dummy_mutex
- ENTIRE() range macro
- bit() and mask()
- null_ptr_exception
- scoped_unlock
- thread-safe singleton
- timed function auto-retry
- index data structures w/ unused ID approximation

