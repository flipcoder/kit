#ifndef MULTIPLEXER_H_DY7UUXOY
#define MULTIPLEXER_H_DY7UUXOY

#include "async.h"
#include "../kit.h"
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <algorithm>
#include <atomic>
#include "../kit.h"

class Multiplexer:
    public IAsync
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
            std::function<bool()> m_Ready; // only a hint
            std::function<void()> m_Func;
            //std::packaged_task<void()> m_Func;
            // TODO: timestamp of last use / avg time idle?
            //unsigned m_Strand = nullptr;
        };

        struct Strand:
            public IAsync,
            public kit::mutexed<std::mutex>
        {
            Strand() {
                run();
            }
            
            //template<class T = void>
            /*std::future<T>*/ void task(std::function<void()> cb) {
                //std::packaged_task<T()> task(std::move(cb));
                //auto r = task.get_future();
                auto l = lock();
                m_Units.emplace_back(std::function<bool()>(), cb);
                //m_Units.emplace_back([]{
                //    return
                //}, cb);
                //return r;
            }
            
            void when(std::function<bool()> cond, std::function<void()> cb) {
                auto l = lock();
                m_Units.emplace_back(cond, cb);
            }

            //template<class R(T)>
            //std::future<R> then(std::future<T>& fut, std::function<R(T)> cb) {
            template<class T>
            void when(std::future<T>& fut, std::function<void(T)> cb) {
                //auto task = std::packaged_task<R(T)>(cb);
                //auto r = task.get_future();
                //auto futc = kit::move_on_copy<std::future<T>>(std::move(fut));
                kit::move_on_copy<std::future<T>> futc(std::move(fut));
                //task(std::function<void()>([cb,futc]{}));
                //std::future<T>* ref = nullptr;
                /*return */task([cb, futc]() {
                    if(futc.get().wait_for(std::chrono::seconds(0)) ==
                        std::future_status::ready)
                        cb(futc.get().get());
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
            
            virtual void poll() override {
                while(true)
                {
                    auto l = this->lock(std::defer_lock);
                    if(l.try_lock() && !m_Units.empty())
                    {
                        auto& task = m_Units.front();
                        if(!task.m_Ready || task.m_Ready()) {
                            l.unlock();
                            try{
                                task.m_Func();
                            }catch(...){
                                continue;
                            }
                            l.lock();
                            m_Units.pop_front();
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

            void run() {
                m_Thread = boost::thread([this]{
                    while(true) {
                        poll();
                        if(m_Finish && empty())
                            return;
                        boost::this_thread::interruption_point();
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
            //virtual bool poll_once() override { assert(false); }
            //virtual void run() override { assert(false); }
            //virtual void run_once() override { assert(false); }
            
            std::list<Unit> m_Units;
            boost::thread m_Thread;
            std::atomic<bool> m_Finish = ATOMIC_VAR_INIT(false);
        };
        
        Multiplexer():
            m_Concurrency(std::min<unsigned>(1U,boost::thread::hardware_concurrency()))
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

        void finish() {
            for(auto& s: m_Strands)
                s->finish();
        }

    private:
        
        const unsigned m_Concurrency;
        std::vector<std::unique_ptr<Strand>> m_Strands;
};

#endif

