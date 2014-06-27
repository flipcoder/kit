#ifndef CHANNEL_H_FC6YZN5J
#define CHANNEL_H_FC6YZN5J

#include <boost/thread.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <future>
#include <memory>
#include "../kit.h"

template<class T>
class Channel:
    public kit::mutexed<std::mutex>
    //public std::enable_shared_from_this<Channel<T>>
{
    public:

        //Channel& operator<<(const T& val) {
        //    return *this;
        //}

        // WRITE
        Channel& operator<<(T&& val) {
            do{
                auto l = lock(std::defer_lock);
                if(l.try_lock())
                {
                    if(m_Buffered && m_Vals.size() >= m_Buffered)
                        continue;
                    
                    m_Vals.emplace(val);
                    break;
                }
                boost::this_thread::interruption_point();
                boost::this_thread::yield();
            }while(true);
            return *this;
        }
        
        // READ
        bool operator>>(T& val) {
            auto l = lock();
            if(!m_Vals.empty()) {
                val = std::move(m_Vals.front());
                m_Vals.pop();
                return true;
            }
            return false;
        }
        
        size_t size() const {
            auto l = this->lock();
            return m_Vals.size();
        }
        size_t buffered() const {
            auto l = lock();
            return m_Buffered;
        }
        void unbuffer() {
            auto l = lock();
            m_Buffered = 0;
        }
        void buffer(size_t sz) {
            auto l = lock();
            m_Buffered = sz;
        }

    private:
        
        size_t m_Buffered = 0;
        std::queue<T> m_Vals;
};

#endif

