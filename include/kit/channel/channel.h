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
    public kit::mutexed<std::mutex>,
    public std::enable_shared_from_this<Channel<T>>
{
    public:
        ~Channel() {
            join();
        }
        void operator()() {
            join();
            
            auto self(this->shared_from_this());
            m_Thread = boost::thread([self]{
                self->run();
            });
        }
        void run() {
            while(true)
            {
                {
                    auto l = this->lock();
                    if(!m_Cmds.empty())
                    {
                        auto task = std::move(m_Cmds.front());
                        m_Cmds.pop();
                        task();
                    }
                }
                boost::this_thread::interruption_point();
                boost::this_thread::yield();
            }
        }
        std::future<T> operator()(std::function<T()> f) {
            std::packaged_task<T()> func(std::move(f));
            while(true)
            {
                {
                    auto l = this->lock();
                    if(m_Buffered && m_Buffered < m_Cmds.size()){
                        auto fut = func.get_future();
                        m_Cmds.emplace(std::move(func));
                        return fut;
                    }
                }
                boost::this_thread::interruption_point();
                boost::this_thread::yield();
            }
        }
    private:
        void join() {
            if(m_Thread.joinable()) {
                m_Thread.interrupt();
                m_Thread.join();
            }
        }
        
        unsigned m_Buffered = 0;
        boost::thread m_Thread;
        std::queue<std::packaged_task<T()>> m_Cmds;
};

#endif

