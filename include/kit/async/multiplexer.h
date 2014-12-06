#ifndef MULTIPLEXER_H_DY7UUXOY
#define MULTIPLEXER_H_DY7UUXOY

//#include "async.h"
#include "../kit.h"
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/coroutine/all.hpp>
#include <algorithm>
#include <atomic>
#include <deque>
#include "../kit.h"
#include "task.h"

#define MX Multiplexer::get()

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

typedef boost::coroutines::coroutine<void>::pull_type pull_coro_t;
typedef boost::coroutines::coroutine<void>::push_type push_coro_t;

// async await a future (continously yield until future is available or exception)
#define AWAIT_MX(MUX, EXPR) \
    [&]{\
        while(true){\
            try{\
                return (EXPR);\
            }catch(const kit::yield_exception&){\
                MUX.yield();\
            }\
        }\
    }()
#define AWAIT(EXPR) AWAIT_MX(MX, EXPR)

// async await a condition (continously yield until condition is true)
#define YIELD_UNTIL_MX(MUX, EXPR) \
    [&]{\
        while(not (EXPR)){\
            MUX.yield();\
        }\
    }()
#define YIELD_UNTIL(EXPR) AWAIT_MX(MX, EXPR)

#define YIELD_MX(MUX) MUX.yield();
#define YIELD() YIELD_MX(MX)

#define AWAIT_HINT_MX(MUX, HINT, EXPR) \
    [&]{\
        bool once = false;\
        while(true)\
        {\
            try{\
                return (EXPR);\
            }catch(const kit::yield_exception&){\
                if(not once) {\
                    Multiplexer::get().this_circuit().this_unit().cond = [=]{\
                        return (HINT);\
                    };\
                    once = true;\
                }\
                MX.yield();\
            }\
            if(once)\
            {\
                kit::clear(Multiplexer::get().this_circuit().this_unit().cond);\
            }\
        }\
    }()

#define AWAIT_HINT(HINT, EXPR) AWAIT_HINT_MX(MUX, HINT, EXPR)

class Multiplexer:
    public kit::singleton<Multiplexer>
    //public IAsync
    //virtual public kit::freezable
{
    public:

        struct Unit
        {
            Unit(
                std::function<bool()> rdy,
                std::function<void()> func,
                std::unique_ptr<push_coro_t>&& push,
                pull_coro_t* pull
            ):
                m_Ready(rdy),
                m_Func(func),
                m_pPush(std::move(push)),
                m_pPull(pull)
            {}
            
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
            std::unique_ptr<push_coro_t> m_pPush;
            pull_coro_t* m_pPull = nullptr;
            // TODO: idletime hints for load balancing?
        };

        class Circuit:
            //public IAsync,
            public kit::mutexed<boost::mutex>
        {
        public:
            
            Circuit(Multiplexer* mx, unsigned idx):
                m_pMultiplexer(mx),
                m_Index(idx)
            {
                run();
            }
            virtual ~Circuit() {}

            void yield() {
                if(m_pCurrentUnit && m_pCurrentUnit->m_pPull)
                    (*m_pCurrentUnit->m_pPull)();
                else
                    throw kit::yield_exception();
                //else
                //    boost::this_thread::yield();
            }
            
            template<class T = void>
            std::future<T> when(std::function<bool()> cond, std::function<T()> cb) {
                while(true) {
                    boost::this_thread::interruption_point();
                    {
                        auto l = lock();
                        if(!m_Buffered || m_Units.size() < m_Buffered) {
                            auto cbt = Task<T()>(std::move(cb));
                            auto fut = cbt.get_future();
                            auto cbc = kit::move_on_copy<Task<T()>>(std::move(cbt));
                            m_Units.emplace_back(cond, [cbc]() {
                                cbc.get()();
                            });
                            m_CondVar.notify_one();
                            return fut;
                        }
                    }
                    boost::this_thread::yield();
                }
            }

            template<class T = void>
            std::future<T> coro(std::function<T()> cb) {
                while(true) {
                    boost::this_thread::interruption_point();
                    {
                        auto l = lock();
                        if(!m_Buffered || m_Units.size() < m_Buffered) {
                            auto cbt = Task<T()>(std::move(cb));
                            auto fut = cbt.get_future();
                            auto cbc = kit::move_on_copy<Task<T()>>(std::move(cbt));
                            m_Units.emplace_back(
                                std::function<bool()>(),
                                std::function<void()>()
                            );
                            auto* pullptr = &m_Units.back().m_pPull;
                            m_Units.back().m_pPush = kit::make_unique<push_coro_t>(
                                [cbc, pullptr](pull_coro_t& sink){
                                    *pullptr = &sink;
                                    cbc.get()();
                                }
                            );
                            auto* coroptr = m_Units.back().m_pPush.get();
                            m_Units.back().m_Ready = std::function<bool()>(
                                [coroptr]() -> bool {
                                    return bool(*coroptr);
                                }
                            );
                            m_Units.back().m_Func = Task<void()>(std::function<void()>(
                                [coroptr]{
                                    (*coroptr)();
                                    if(*coroptr) // not completed?
                                        throw kit::yield_exception(); // continue
                                }
                            ));
                            m_CondVar.notify_one();
                            return fut;
                        }
                    }
                    boost::this_thread::yield();
                }

            }
            
            template<class T = void>
            std::future<T> task(std::function<T()> cb) {
                return when(std::function<bool()>(), cb);
            }
            
            // TODO: handle single-direction channels that may block
            //template<class T>
            //std::shared_ptr<Channel<T>> channel(
            //    std::function<void(std::shared_ptr<Channel<T>>)> worker
            //) {
            //    auto chan = std::make_shared<Channel<>>();
            //    // ... inside lambda if(chan->closed()) remove?
            //}
            
            // TODO: handle multi-direction channels that may block

            template<class R, class T>
            std::future<R> when(std::future<T>& fut, std::function<R(std::future<T>&)> cb) {
                kit::move_on_copy<std::future<T>> futc(std::move(fut));
                
                return task<void>([cb, futc]() {
                    if(futc.get().wait_for(std::chrono::seconds(0)) ==
                        std::future_status::ready)
                    {
                        cb(futc.get());
                    }
                    else
                        throw;
                });
            }

            void run() {
                m_Thread = boost::thread([this]{
                    m_pMultiplexer->m_ThreadToCircuit.with<void>(
                        [this](std::map<boost::thread::id, unsigned>& m){
                            m[boost::this_thread::get_id()] = m_Index;
                        }
                    );
                    unsigned idx = 0;
                    while(next(idx)) {}
                });
            }
            void forever() {
                if(m_Thread.joinable()) {
                    m_Thread.join();
                }
            }
            void finish_nojoin() {
                m_Finish = true;
                {
                    auto l = this->lock<boost::unique_lock<boost::mutex>>();
                    m_CondVar.notify_one();
                }
            }
            void join() {
                if(m_Thread.joinable())
                    m_Thread.join();
            }
            void finish() {
                m_Finish = true;
                {
                    auto l = this->lock<boost::unique_lock<boost::mutex>>();
                    m_CondVar.notify_one();
                }
                if(m_Thread.joinable()) {
                    m_Thread.join();
                }
            }
            void stop() {
                if(m_Thread.joinable()) {
                    m_Thread.interrupt();
                    {
                        auto l = this->lock<boost::unique_lock<boost::mutex>>();
                        m_CondVar.notify_one();
                    }
                    m_Thread.join();
                }
            }
            bool empty() const {
                auto l = this->lock<boost::unique_lock<boost::mutex>>();
                return m_Units.empty();
            }
            void sync() {
                while(true){
                    if(empty())
                        return;
                    boost::this_thread::yield();
                };
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
            
            Unit* this_unit() { return m_pCurrentUnit; }
            
        private:

            // returns false only on empty() && m_Finish
            virtual bool next(unsigned& idx) {
                auto lck = this->lock<boost::unique_lock<boost::mutex>>();
                //if(l.try_lock())
                // wait until task queued or thread interrupt
                while(true){
                    boost::this_thread::interruption_point();
                    if(not m_Units.empty())
                        break;
                    else if(m_Finish) // finished AND empty
                        return false;
                    m_CondVar.wait(lck);
                    continue;
                }
                
                if(idx >= m_Units.size())
                    idx = 0;
                
                auto& task = m_Units[idx];
                if(!task.m_Ready || task.m_Ready()) {
                    lck.unlock();
                    m_pCurrentUnit = &m_Units[idx]; // TODO: fix this garbage
                    try{
                        task.m_Func();
                    }catch(...){
                        lck.lock();
                        //++idx;
                        size_t sz = m_Units.size();
                        idx = std::min<unsigned>(idx+1, sz);
                        if(idx == sz)
                            idx = 0;
                        return true;
                    }
                    m_pCurrentUnit = nullptr;
                    lck.lock();
                    m_Units.erase(m_Units.begin() + idx);
                }
                return true;
            }
            
            Unit* m_pCurrentUnit = nullptr;
            std::deque<Unit> m_Units;
            boost::thread m_Thread;
            size_t m_Buffered = 0;
            std::atomic<bool> m_Finish = ATOMIC_VAR_INIT(false);
            Multiplexer* m_pMultiplexer;
            unsigned m_Index=0;
            boost::condition_variable m_CondVar;
        };
        
        friend class Circuit;
        
        Multiplexer(bool init_now=true):
            m_Concurrency(std::max<unsigned>(1U,boost::thread::hardware_concurrency()))
        {
            if(init_now)
                init();
        }
        virtual ~Multiplexer() {
            finish();
        }
        void init() {
            for(unsigned i=0;i<m_Concurrency;++i)
                m_Circuits.emplace_back(make_tuple(
                    kit::make_unique<Circuit>(this, i), CacheLinePadding()
                ));
        }
        //void join() {
        //    for(auto& s: m_Circuits)
        //        s.join();
        //}
        void stop() {
            for(auto& s: m_Circuits)
                std::get<0>(s)->stop();
        }

        Circuit& any_circuit(){
            // TODO: load balancing would be nice here
            return *std::get<0>((m_Circuits[std::rand() % m_Concurrency]));
        }
        
        Circuit& circuit(unsigned idx) {
            return *std::get<0>((m_Circuits[idx % m_Concurrency]));
        }
        
        void yield(){
            try{
                this_circuit().yield();
            }catch(const std::out_of_range&){
                throw kit::yield_exception();
            }
        }
        Circuit& this_circuit(){
            return *m_ThreadToCircuit.with<Circuit*>(
                [this](std::map<boost::thread::id, unsigned>& m
            ){
                return &circuit(m.at(boost::this_thread::get_id()));
            });
        }

        size_t size() const {
            return m_Concurrency;
        }

        void finish() {
            for(auto& s: m_Circuits)
                std::get<0>(s)->finish_nojoin();
            for(auto& s: m_Circuits)
                std::get<0>(s)->join();
        }

    private:

        struct CacheLinePadding
        {
            volatile int8_t pad[CACHE_LINE_SIZE];
        };

        const unsigned m_Concurrency;
        std::vector<std::tuple<std::unique_ptr<Circuit>, CacheLinePadding>> m_Circuits;

        kit::mutex_wrap<std::map<boost::thread::id, unsigned>> m_ThreadToCircuit;
};

#endif

