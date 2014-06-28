#ifndef ASYNC_H_TIBVELUE
#define ASYNC_H_TIBVELUE

class IAsync
{
    public:

        virtual ~IAsync() = 0;
        virtual void poll() { assert(false); }
        virtual bool poll_once() { assert(false); }
        virtual void run() { assert(false); }
        virtual void run_once() { assert(false); }
};

IAsync :: ~IAsync() {}

#endif

