#ifndef SIGNAL_H_GQBDAQXV
#define SIGNAL_H_GQBDAQXV

#include <boost/scope_exit.hpp>
#include <boost/optional.hpp>
#include "../kit.h"

namespace kit
{
    template<class R, class ...Args>
    class signal;

    // a single-threaded signal
    template<class R, class ...Args>
    class signal<R (Args...)>
    {
        public:

            // TODO: slot class with some sort of scope test function or maybe filter id?
            
            typedef std::function<R (Args...)> function_type;
            typedef std::vector<std::function<R (Args...)>> container_type;
           
            void connect(function_type func) {
                if(not m_Recursion)
                    m_Slots.push_back(std::move(func));
                else
                    m_PendingOperations.push_back([this, func]{
                        m_Slots.push_back(std::move(func));
                    });
            }

            operator bool() const {
                return not m_Slots.empty();
            }
            
            void operator()(Args... args) const {
                bool recur = not m_Recursion++;
                for(auto&& slot: m_Slots) {
                    slot(args...);
                }
                if(recur)
                    run_pending_ops();
                m_Recursion--;
            }
            
            std::vector<R> accumulate(Args... args) {
                if(m_Slots.empty())
                    return std::vector<R>();
                std::vector<R> r;
                r.reserve(m_Slots.size());
                for(auto&& slot: m_Slots)
                    r.push_back(slot(args...));
                return r;
            }

            void sync(const std::function<void()>& func){
                if(m_Recursion)
                    func();
                else
                    m_PendingOperations.push_back(func);
            }
            bool recursion() const
            {
                return m_Recursion;
            }
            void clear()
            {
                if(m_Recursion)
                    m_PendingOperations.push_back([this]{
                        m_Slots.clear();
                    });
                else
                    m_Slots.clear();
            }

            bool empty() const {
                return m_Slots.empty();
            }
            size_t size() const {
                return m_Slots.size();
            }
            
            typedef typename container_type::iterator iterator;
            typedef typename container_type::const_iterator const_iterator;
            const_iterator cbegin() const { return m_Slots.cbegin(); }
            const_iterator cend() const { return m_Slots.cend(); }
            iterator begin() const { return m_Slots.begin(); }
            iterator end() const { return m_Slots.end(); }
            iterator begin() { return m_Slots.begin(); }
            iterator end() { return m_Slots.end(); }
            
        private:

            void run_pending_ops() const
            {
                for(auto&& op: m_PendingOperations)
                    op();
            }
            
            container_type m_Slots;
            
            mutable std::vector<std::function<void()>> m_PendingOperations;
            mutable unsigned m_Recursion = 0;
    };
    
}

#endif

