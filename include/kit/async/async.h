#ifndef ASYNC_H_TIBVELUE
#define ASYNC_H_TIBVELUE

#include "task.h"
#include "multiplexer.h"
#include "channel.h"

template<class T>
class async_wrap
{
private:
    
    Multiplexer::Circuit* const m_pCircuit;
    T m_Data;
    
public:

    async_wrap(Multiplexer::Circuit* circuit = &MX[0]):
        m_pCircuit(circuit),
        m_Data(T())
    {}
    async_wrap(T&& v, Multiplexer::Circuit* circuit = &MX[0]):
        m_pCircuit(circuit),
        m_Data(v)
    {}
    async_wrap(const T& v, Multiplexer::Circuit* circuit = &MX[0]):
        m_pCircuit(circuit),
        m_Data(std::forward(v))
    {}
    template<class R = void>
    std::future<R> with(std::function<R(T&)> cb) {
        return m_pCircuit->task<R>([this, cb]{
            return cb(m_Data);
        });
    }
    template<class R = void>
    std::future<R> with(std::function<R(const T&)> cb) const {
        return m_pCircuit->task<R>([this, cb]{
            return cb(m_Data);
        });
    }
    
    // blocks until getting a copy of the wrapped m_Data
    T operator*() const {
        return get();
    }
    T get() const {
        return m_pCircuit->task<T>([this]{
            return m_Data;
        }).get();
    }

    const Multiplexer::Circuit& circuit() const { return *m_pCircuit; }
};

#endif

