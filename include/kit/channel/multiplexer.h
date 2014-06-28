#ifndef MULTIPLEXER_H_M3QWRHEY
#define MULTIPLEXER_H_M3QWRHEY

#include <boost/signals2.hpp>

class Multiplexer
{
    public:
        //Multiplexer();
        //virtual ~Multiplexer();

        boost::signals2::signal<void()> poll;
        
};

#endif

