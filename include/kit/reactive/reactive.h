#ifndef REACTIVE_H_GQBDAQXV
#define REACTIVE_H_GQBDAQXV

#include <boost/optional.hpp>
#include "../kit.h"

namespace kit
{

    template<class T>
    class reactive
    {
        public:

            reactive() = default;
            reactive(const T& t):
                m_Data(t)
            {
                on_change(m_Data);
            }
            reactive(T&& t):
                m_Data(t)
            {
                on_change(m_Data);
            }

            T& operator=(const T& t)
            {
                m_Data = t;
                on_change(t);
                return m_Data;
            }
            T& operator=(T&& t)
            {
                m_Data = t;
                on_change(m_Data);
                return m_Data;
            }

            T& operator*() {
                return m_Data;
            }
            const T& operator*() const {
                return m_Data;
            }
            T& get() {
                return m_Data;
            }
            const T& get() const {
                return m_Data;
            }

            void trigger(){
                on_change(m_Data);
            }

            void clear()
            {
                //on_destroy();
                on_change.disconnect_all_slots();
                //on_destroy.disconnect_all_slots();
            }
            
            boost::signals2::signal<void(const T&)> on_change;
            //boost::signals2::signal<void()> on_destroy;
            
        private:
            
            T m_Data = T();
    };

    template<class T>
    class lazy
    {
        public:
            
            lazy() = default;
            lazy(const lazy&) = default;
            lazy(std::function<T()> rhs):
                m_Getter(rhs)
            {}
            lazy(lazy&&) = default;
            lazy& operator=(const lazy&) = default;
            lazy& operator=(lazy&&) = default;
            lazy& operator=(std::function<T()> rhs)
            {
                m_Value = boost::optional<T>();
                m_Getter = rhs;
                return *this;
            }

            //lazy(reactive<T>& r) {
            //    m_Getter = [&r]() -> T {
            //        return r.get();
            //    };
            //    m_ChangeConnection = r.on_change.connect([this](const T& val){
            //        pend();
            //    });
            //    m_DestroyConnection = r.on_destroy.connect([this]{
            //        m_Getter = std::function<T()>();
            //    });
            //}

            void set(const T& value){
                m_Value = value;
            }

            void pend() {
                m_Value = boost::optional<T>();
            }

            void recache() {
                m_Value = m_Getter();
            }
            
            void ensure() {
                if(!m_Value)
                    m_Value = m_Getter();
            }
            
            bool valid() const {
                return bool(m_Value);
            }

            boost::optional<T> try_get() {
                return m_Value;
            }
            T& get() {
                ensure();
                return *m_Value;
            }

            void getter(std::function<T()> func){
                m_Getter = func;
            }

            T& operator()() {
                return get();
            }

        private:
            boost::optional<T> m_Value;
            std::function<T()> m_Getter;
    };

}

#endif

