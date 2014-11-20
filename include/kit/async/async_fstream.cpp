#include "async_fstream.h"
using namespace std;

future<void> async_fstream :: open(std::string fn)
{
    return MX.circuit(Circuit).task<void>([this, fn]{
        m_Filename = fn;
    });
}

future<void> async_fstream :: close()
{
    return MX.circuit(Circuit).task<void>([this]{
        m_Filename = ""; 
    });
}

std::future<std::string> async_fstream :: filename() const
{
    //return kit::make_future<string>(m_Filename.get());
    return MX.circuit(Circuit).task<std::string>([this]{return m_Filename;});
}

