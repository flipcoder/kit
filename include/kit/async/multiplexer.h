#ifndef MULTIPLEXER_H_DY7UUXOY
#define MULTIPLEXER_H_DY7UUXOY

//#include "async.h"
#include "../kit.h"
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <algorithm>
#include <atomic>
#include <deque>
#include "../kit.h"
#include "task.h"

class Multiplexer//:
    //public IAsync
    //virtual public kit::freezable
{
    public:

        struct Unit
        {
            Unit(
                std::function<bool()> rdy,
                std::function<void()> func
            ):
                m_Ready(rdy),
                m_Func(func)
            {}
            
            // only a hint, assume ready if functor is 'empty'
            std::function<bool()> m_Ready; 
            Task<void()> m_Func;
            //std::packaged_task<void()> m_Func;
            // TODO: timestamp of last use / avg time idle?
            //unsigned m_Strand = nullptr;
        };

        class Strand:
            //public IAsync,
            public kit::mutexed<std::mutex>
        {
        public:
            
            Strand() {
                run();
            }
            virtual ~Strand() {}
            
            template<class T = void>
            std::future<T> when(std::function<bool()> cond, std::function<T()> cb) {
                //std::packaged_task<T()> task(std::move(cb));
                //auto r = task.get_future();
                while(true) {
                    boost::this_thread::interruption_point();
                    {
                        auto l = lock();
                        if(!m_Buffered || m_Units.size() < m_Buffered) {
                            auto cbt = Task<T()>(std::move(cb));
                            auto fut = cbt.get_future();
                            auto cbc = kit::move_on_copy<Task<T()>>(std::move(cbt));
                            m_Units.emplace_back(cond, [cbc]{
                                cbc.get()();
                            });
                            return fut;
                        }
                    }
                    boost::this_thread::yield();

                    //return std::future<T>();
                    //break;
                }
                //m_Units.emplace_back([]{
                //    return
                //}, cb);
                //return r;
            }
            
            template<class T = void>
            std::future<T> task(std::function<T()> cb) {
                return when(std::function<bool()>(), cb);
            }
            
            //template<class R>
            //std::future<R> when(std::function<bool()> cond, std::function<R()> cb) {
            //    while(true) {
            //        auto l = lock();
            //        if(m_Buffered && m_Units.size() >= m_Buffered)
            //            continue;
            //        auto cbt = Task<T()>(std::move(cb));
            //        auto fut = cbt.get_future();
            //        auto cbc = kit::move_on_copy<Task<T()>>(std::move(cbt));
            //        m_Units.emplace_back(cond, []{
                        
            //        });
            //        break;
            //    }
            //}

            // TODO: handle single-direction channels that may block
            //template<class T>
            //std::shared_ptr<Channel<T>> channel(
            //    std::function<void(std::shared_ptr<Channel<T>>)> worker
            //) {
            //    auto chan = std::make_shared<Channel<>>();
            //    // ... inside lambda if(chan->closed()) remove?
            //}
            
            // TODO: handle multi-direction channels that may block

            //template<class R(T)>
            //std::future<R> then(std::future<T>& fut, std::function<R(T)> cb) {
            template<class R, class T>
            std::future<R> when(std::future<T>& fut, std::function<R(std::future<T>&)> cb) {
                //auto task = std::packaged_task<R(T)>(cb);
                //auto r = task.get_future();
                //auto futc = kit::move_on_copy<std::future<T>>(std::move(fut));
                kit::move_on_copy<std::future<T>> futc(std::move(fut));
                //task(std::function<void()>([cb,futc]{}));
                //std::future<T>* ref = nullptr;
                return task<void>([cb, futc]() {
                    if(futc.get().wait_for(std::chrono::seconds(0)) ==
                        std::future_status::ready)
                    {
                        cb(futc.get());
                    }
                    else
                        throw;
                });
                //task(std::bind([cb](std::future<T> fut){
                //    if(fut.wait_for(std::chrono::seconds(0)) ==
                //        std::future_status::ready)
                //        cb(fut.get());
                //    else
                //        throw;
                //}, std::move(fut)));
                //return r;
            }

            void run() {
                m_Thread = boost::thread([this]{
                    unsigned idx = 0;
                    while(true) {
                        boost::this_thread::interruption_point();
                        next(idx); // poll iterates once index each call
                        if(m_Finish && empty())
                            return;
                        boost::this_thread::yield();
                    }
                });
            }
            void finish() {
                m_Finish = true;
                if(m_Thread.joinable()) {
                    m_Thread.join();
                }
            }
            void stop() {
                if(m_Thread.joinable()) {
                    m_Thread.interrupt();
                    m_Thread.join();
                }
            }
            bool empty() const {
                auto l = lock();
                return m_Units.empty();
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
            //virtual bool poll_once() override { assert(false); }
            //virtual void run() override { assert(false); }
            //virtual void run_once() override { assert(false); }
            
        private:

            virtual void next(unsigned& idx) {
                while(true)
                {
                    auto l = this->lock(std::defer_lock);
                    if(l.try_lock())
                    {
                        if(m_Units.empty() || idx >= m_Units.size())
                        {
                            idx = 0;
                            return;
                        }
                        
                        auto& task = m_Units[idx];
                        if(!task.m_Ready || task.m_Ready()) {
                            l.unlock();
                            try{
                                task.m_Func();
                            }catch(...){
                                l.lock();
                                idx = std::min<unsigned>(idx+1, m_Units.size());
                                continue;
                            }
                            l.lock();
                            m_Units.erase(m_Units.begin() + idx);
                        }
                        //auto task = std::move(m_Units.front());
                        //m_Units.pop_front();
                        //l.unlock();
                        //task();
                    }
                    else
                        return; // task queue empty is or locked
                }
            }
            
            std::deque<Unit> m_Units;
            boost::thread m_Thread;
            size_t m_Buffered = 0;
            std::atomic<bool> m_Finish = ATOMIC_VAR_INIT(false);
        };
        
        Multiplexer():
            m_Concurrency(std::max<unsigned>(1U,boost::thread::hardware_concurrency()))
        {
            for(unsigned i=0;i<m_Concurrency;++i)
                m_Strands.emplace_back(kit::make_unique<Strand>());
        }
        virtual ~Multiplexer() {
            finish();
        }
        //void join() {
        //    for(auto& s: m_Strands)
        //        s.join();
        //}
        void stop() {
            for(auto& s: m_Strands)
                s->stop();
        }

        //Strand& this_strand() {
        //}
        Strand& strand(unsigned idx) {
            return *(m_Strands[idx % m_Strands.size()]);
        }

        size_t size() const {
            return m_Concurrency;
        }

        void finish() {
            for(auto& s: m_Strands)
                s->finish();
        }

    private:
        
        const unsigned m_Concurrency;
        std::vector<std::unique_ptr<Strand>> m_Strands;
};

#endif

