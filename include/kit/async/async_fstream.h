#ifndef FILE_H_VSWBGI51
#define FILE_H_VSWBGI51

#include <fstream>
#include <string>
#include <future>
#include <atomic>
#include "async.h"
#include "../kit.h"

#ifndef MX_FILE_CIRCUIT
#define MX_FILE_CIRCUIT 0
#endif

class async_fstream
{
    public:

        async_fstream() = default;
        async_fstream(std::string fn){open(fn);}
        ~async_fstream() {
            close().get();
        }

        async_fstream(const async_fstream&) = delete;
        async_fstream(async_fstream&&) = default;
        async_fstream& operator=(const async_fstream&) = default;
        async_fstream& operator=(async_fstream&&) = default;

        std::future<void> open(std::string fn);
        std::future<void> close();
        std::future<std::string> filename() const;

        template<class T>
        std::future<T> with(std::function<T(std::fstream& f)> func) {
            return MX.circuit(Circuit).task<T>([this,func]{ return func(m_File); });
        }
        
    private:

        const unsigned Circuit = MX_FILE_CIRCUIT;
        std::fstream m_File;
        std::string m_Filename;
};

#endif

