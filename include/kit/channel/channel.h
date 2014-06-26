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
class Multiplexer
{
    public:

        class Consumer:
            public kit::mutexed<std::mutex>,
            public std::enable_shared_from_this<Multiplexer<T>::Consumer>
        {
            public:
                //Consumer(unsigned id):
                //    m_ID(id)
                //{}
                //Consumer(){
                    //auto th = boost::thread([c]{
                    //    (*c)();
                    //});
                    //th.detach();
                //}
                ~Consumer() {
                    join();
                }
                void operator()() {
                    //join();
                    
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
                    }
                }
                void operator()(std::packaged_task<T()> func) {
                    auto l = this->lock();
                    //while(true) {
                        //try{
                            m_Cmds.emplace(std::move(func));
                        //}catch(...){}
                    //}
                }
            private:
                void join() {
                    if(m_Thread.joinable()) {
                        m_Thread.interrupt();
                        m_Thread.join();
                    }
                }
                
                boost::thread m_Thread;
                std::queue<std::packaged_task<T()>> m_Cmds;
                //boost::lockfree::queue<std::packaged_task<T()>> m_Cmds;
        };
        
        Multiplexer(unsigned size = 1):
            m_Size(size),
            m_Consumers(size)
        {
            for(auto& c: m_Consumers) {
                c = std::make_shared<Multiplexer<T>::Consumer>();
                (*c)();
            }
            //}
        }
        //~Multiplexer();
        
        std::future<T> operator()(unsigned id, std::function<T()>);
        std::future<T> operator()(std::function<T()>);

        unsigned size() const {
            return m_Size;
        }
        
    private:
        
        const unsigned m_Size;
        std::vector<std::shared_ptr<
            Multiplexer<T>::Consumer
        >> m_Consumers;
};

template<class T>
std::future<T> Multiplexer<T> :: operator()(unsigned id, std::function<T()> func)
{
    std::packaged_task<T()> task(std::move(func));
    auto fut = task.get_future();
    (*m_Consumers[id])(std::move(task));
    return fut;
}
template<class T>
std::future<T> Multiplexer<T> :: operator()(std::function<T()> func)
{
    return (*this)(0, func);
}

//template<class T>
//Multiplexer<T>::~Multiplexer()
//{
//    //m_Consumer;
//}

#endif

