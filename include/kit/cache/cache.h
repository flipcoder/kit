#ifndef _CACHE_H_QB0MZK5J
#define _CACHE_H_QB0MZK5J

#include <unordered_map>
#include "../factory/factory.h"
#include "../log/log.h"
#include "../kit.h"
#include "icache.h"

template<class Class, class T, class Mutex=kit::dummy_mutex>
class Cache:
    public Factory<Class, std::tuple<T, ICache*>, std::string, Mutex>,
    public ICache,
    virtual public kit::mutexed<Mutex>
{
    public:
        
        Cache() = default;
        Cache(std::string fn):
            Factory<Class, std::tuple<T, ICache*>>(fn)
        {}
        Cache(std::shared_ptr<Meta> cfg):
            Factory<Class, std::tuple<T, ICache*>>(cfg)
        {}
        
        virtual ~Cache() {}
        
        virtual void optimize() override {
            auto l = this->lock();
            for(auto itr = m_Resources.begin();
                itr != m_Resources.end();)
            {
                if(itr->second.unique())
                    itr = m_Resources.erase(itr);
                else
                    ++itr;
            }
        }
        
        virtual std::shared_ptr<Class> cache_raw(const T& arg) {
            auto l = this->lock();
            if(m_Preserve && m_Preserve(arg)) {
                return Factory<Class, std::tuple<T, ICache*>, std::string, Mutex>::create(
                    std::tuple<T, ICache*>(arg, this)
                );
            }
            try{
                return m_Resources.at(arg);
            }catch(const std::out_of_range&){
                return (m_Resources[arg] =
                    Factory<Class, std::tuple<T, ICache*>, std::string, Mutex>::create(
                        std::tuple<T, ICache*>(arg, this)
                    )
                );
            }
        }
        
        virtual std::shared_ptr<Class> cache(T arg) {
            auto l = this->lock();
            arg = transform(arg);
            return cache_raw(arg);
        }

        //template<class Cast>
        //virtual void cache(const std::shared_ptr<Cast>& blah) {
        //}
        
        template<class Cast>
        std::shared_ptr<Cast> cache_as(T arg) {
            auto l = this->lock();
            arg = transform(arg);
            std::shared_ptr<Cast> p;
            try{
                p = std::dynamic_pointer_cast<Cast>(cache_raw(arg));
            }catch(...){
                typename std::unordered_map<T, std::shared_ptr<Class>>::iterator itr;
                try{
                    itr = m_Resources.find(arg);
                }catch(const std::out_of_range&){
                    throw;
                }
                if(itr->second.unique())
                    m_Resources.erase(itr);
                throw;
            }
            return p;
        }
        
        virtual size_t size() const override {
            auto l = this->lock();
            return m_Resources.size();
        }
        virtual bool empty() const override {
            auto l = this->lock();
            return m_Resources.empty();
        }

        virtual void clear() override {
            auto l = this->lock();
            m_Resources.clear();
        }

        T transform(const T& t){
            auto l = this->lock();
            if(m_Transformer)
                return m_Transformer(t);
            else
                return t;
        }
        void register_transformer(std::function<T(const T&)> f) {
            auto l = this->lock();
            m_Transformer=f;
        }
        
        void register_preserver(std::function<bool(const T&)> f) {
            auto l = this->lock();
            m_Preserve = f;
        }
        
    private:

        std::unordered_map<T, std::shared_ptr<Class>> m_Resources;
        std::function<T(const T&)> m_Transformer;
        std::function<bool(const T&)> m_Preserve; // cache this?
};

#endif

