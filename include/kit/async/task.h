#ifndef TASK_H_HAPEX5Z1
#define TASK_H_HAPEX5Z1

#include <boost/noncopyable.hpp>
#include <future>
#include <stdexcept>
#include "../kit.h"

template<class R, class ...Args>
class Task;

template<class R, class ...Args>
class Task<R (Args...)>
{
    public:

        template<class ...T>
        explicit Task(T&&... t):
            m_Func(std::forward<T>(t)...)
        {}

        Task(Task&&) = default;
        Task& operator=(Task&&) = default;
        
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;
        
        template<class ...T>
        void operator()(T&&... t) {
            try{
                m_Promise.set_value(m_Func(std::forward<T>(t)...));
            }catch(const kit::yield_exception&){
                throw;
            }catch(const std::exception& e){
                m_Promise.set_exception(std::make_exception_ptr(e));
            }
        }

        std::future<R> get_future() {
            return m_Promise.get_future();
        }

    private:

        std::function<R(Args...)> m_Func;
        std::promise<R> m_Promise;
};

template<class ...Args>
class Task<void(Args...)>
{
    public:

        template<class ...T>
        explicit Task(T&&... t):
            m_Func(std::forward<T>(t)...)
        {}

        Task(Task&&) = default;
        Task& operator=(Task&&) = default;
        
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;
        
        template<class ...T>
        void operator()(T&&... t) {
            try{
                m_Func(std::forward<T>(t)...);
                m_Promise.set_value();
            }catch(const kit::yield_exception& e){
                throw e;
            }catch(const std::exception& e){
                m_Promise.set_exception(std::make_exception_ptr(e));
            }
        }

        std::future<void> get_future() {
            return m_Promise.get_future();
        }

    private:

        std::function<void(Args...)> m_Func;
        std::promise<void> m_Promise;

};

#endif

