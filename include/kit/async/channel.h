#ifndef CHANNEL_H_FC6YZN5J
#define CHANNEL_H_FC6YZN5J

#include <boost/thread.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <future>
#include <memory>
#include <utility>
#include "../kit.h"
#include "task.h"

template<class T, class Mutex=std::mutex>
class Channel:
    public kit::mutexed<Mutex>
    //public std::enable_shared_from_this<Channel<T>>
{
    public:

        virtual ~Channel() {}

        // Put into stream
        bool operator<<(T val) {
            do{
                boost::this_thread::interruption_point();
                auto l = this->lock(std::defer_lock);
                if(l.try_lock())
                {
                    if(m_bClosed)
                        throw std::runtime_error("channel closed");
                    if(!m_Buffered || m_Vals.size() < m_Buffered) {
                        m_Vals.push(std::move(val));
                        break;
                    }
                }
                boost::this_thread::yield();
            }while(true);
            return true;
        }
        
        // Get from stream
        bool operator>>(T& val) {
            auto l = this->lock();
            if(!m_Vals.empty()) {
                val = std::move(m_Vals.front());
                m_Vals.pop();
                return true;
            }
            return false;
        }

        T get() {
            auto l = this->lock(std::defer_lock);
            if(!l.try_lock())
                throw RetryTask();
            if(m_bClosed)
                throw std::runtime_error("channel closed");
            if(!m_Vals.empty()) {
                auto r = std::move(m_Vals.front());
                m_Vals.pop();
                return r;
            }else{
                throw RetryTask();
            }
        }

        //operator bool() const {
        //    return m_bClosed;
        //}
        bool empty() const {
            auto l = this->lock();
            return m_Vals.empty();
        }
        size_t size() const {
            auto l = this->lock();
            return m_Vals.size();
        }
        size_t buffered() const {
            auto l = this->lock();
            return m_Buffered;
        }
        void unbuffer() {
            auto l = this->lock();
            m_Buffered = 0;
        }
        void buffer(size_t sz) {
            auto l = this->lock();
            m_Buffered = sz;
        }
        void close() {
            // atomic
            m_bClosed = true;
        }
        bool closed() const {
            // atomic
            return m_bClosed;
        }

    private:
        
        size_t m_Buffered = 0;
        std::queue<T> m_Vals;
        std::atomic<bool> m_bClosed = ATOMIC_VAR_INIT(false);
};

#endif

