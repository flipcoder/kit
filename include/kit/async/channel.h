#ifndef CHANNEL_H_FC6YZN5J
#define CHANNEL_H_FC6YZN5J

#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>
#include <iostream>
#include <iterator>
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
        void operator<<(T val) {
            auto l = this->lock(std::defer_lock);
            if(!l.try_lock())
                throw kit::yield_exception();
            if(m_bClosed)
                throw std::runtime_error("channel closed");
            if(!m_Buffered || m_Vals.size() < m_Buffered)
            {
                m_Vals.push_back(std::move(val));
                m_bNewData = true;
                return;
            }
            throw kit::yield_exception();
        }
        template<class Buffer=std::vector<T>>
        void stream(Buffer& vals) {
            auto l = this->lock(std::defer_lock);
            if(!l.try_lock())
                throw kit::yield_exception();
            if(m_bClosed)
                throw std::runtime_error("channel closed");
            size_t capacity = 0;
            size_t buflen = vals.size();
            bool partial = false;
            if(m_Buffered)
            {
                capacity = m_Buffered - m_Vals.size();
                if(buflen > capacity)
                {
                    buflen = capacity;
                    partial = true;
                }
            }
            if(buflen)
            {
                m_Vals.insert(m_Vals.end(),
                    make_move_iterator(vals.begin()),
                    make_move_iterator(vals.begin() + buflen)
                );
                vals.erase(vals.begin(), vals.begin() + buflen);
                m_bNewData = true;
                if(partial)
                    throw kit::yield_exception();
                return;
            }
            throw kit::yield_exception();
        }
        void operator<<(std::vector<T>& vals) {
            stream(vals);
        }

        //void operator<<(std::vector<T> val) {
        //    auto l = this->lock(std::defer_lock);
        //    if(!l.try_lock())
        //        throw kit::yield_exception();
        //    if(m_bClosed)
        //        throw std::runtime_error("channel closed");
        //    if(!m_Buffered || m_Vals.size() < m_Buffered) {
        //        m_Vals.push_back(std::move(val));
        //        m_bNewData = true;
        //        return;
        //    }
        //    throw kit::yield_exception();
        //}
        
        // Get from stream
        void operator>>(T& val) {
            if(not m_bNewData)
                throw kit::yield_exception();
            auto l = this->lock(std::defer_lock);
            if(!l.try_lock())
                throw kit::yield_exception();
            //if(m_bClosed)
            //    throw std::runtime_error("channel closed");
            m_bNewData = false;
            if(!m_Vals.empty())
            {
                val = std::move(m_Vals.front());
                m_Vals.pop_front();
                return;
            }
            throw kit::yield_exception();
        }
        void operator>>(std::vector<T>& vals) {
            get(vals);
        }
        template<class Buffer=std::vector<T>>
        void get(Buffer& vals) {
            if(not m_bNewData)
                throw kit::yield_exception();
            auto l = this->lock(std::defer_lock);
            if(!l.try_lock())
                throw kit::yield_exception();
            //if(m_bClosed)
            //    throw std::runtime_error("channel closed");
            m_bNewData = false;
            if(!m_Vals.empty())
            {
                vals.insert(vals.end(),
                    make_move_iterator(m_Vals.begin()),
                    make_move_iterator(m_Vals.end())
                );
                m_Vals.clear();
                return;
            }
            throw kit::yield_exception();
        }
        template<class Buffer=std::vector<T>>
        Buffer get() {
            Buffer buf;
            get(buf);
            return buf;
        }

        T peek() {
            if(not m_bNewData)
                throw kit::yield_exception();
            auto l = this->lock(std::defer_lock);
            if(!l.try_lock())
                throw kit::yield_exception();
            //if(m_bClosed)
            //    throw std::runtime_error("channel closed");
            if(!m_Vals.empty())
                return m_Vals.front();
            throw kit::yield_exception();
        }
        
        // NOTE: full buffers with no matching tokens will never
        //       trigger this
        template<class R=std::vector<T>>
        R get_until(T token) {
            if(not m_bNewData)
                throw kit::yield_exception();
            auto l = this->lock(std::defer_lock);
            if(!l.try_lock())
                throw kit::yield_exception();
            //if(m_bClosed)
            //    throw std::runtime_error("channel closed");
            for(size_t i=0;i<m_Vals.size();++i)
            {
                if(m_Vals[i] == token)
                {
                    R r(make_move_iterator(m_Vals.begin()),
                        make_move_iterator(m_Vals.begin() + i));
                    m_Vals.erase(
                        m_Vals.begin(),
                        m_Vals.begin() + i + 1
                    );
                    return r;
                }
            }
            throw kit::yield_exception();
        }
        T get() {
            if(not m_bNewData)
                throw kit::yield_exception();
            auto l = this->lock(std::defer_lock);
            if(!l.try_lock())
                throw kit::yield_exception();
            m_bNewData = 0;
            //if(m_bClosed)
            //    throw std::runtime_error("channel closed");
            if(!m_Vals.empty()) {
                auto r = std::move(m_Vals.front());
                m_Vals.pop_front();
                return r;
            }
            throw kit::yield_exception();
        }

        //operator bool() const {
        //    return m_bClosed;
        //}
        bool ready() const {
            return m_bNewData;
        }
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
            //if(sz > m_Buffered)
            //    m_Vals.reserve(sz);
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
        std::deque<T> m_Vals;
        std::atomic<bool> m_bClosed = ATOMIC_VAR_INIT(false);
        std::atomic<bool> m_bNewData = ATOMIC_VAR_INIT(false);
};

#endif

