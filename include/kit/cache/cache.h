#ifndef _CACHE_H_QB0MZK5J
#define _CACHE_H_QB0MZK5J

#include <unordered_map>
#include "../factory/factory.h"
#include "../log/log.h"
#include "icache.h"

template<class Class, class T>
class Cache:
    public Factory<Class, std::tuple<T, ICache*>>,
    public ICache
{
    public:
        
        virtual ~Cache() {}
        
        virtual void optimize() override {
            for(auto itr = m_Resources.begin();
                itr != m_Resources.end();)
            {
                if(itr->second.unique())
                    itr = m_Resources.erase(itr);
                else
                    ++itr;
            }
        }
        
        virtual std::shared_ptr<Class> cache(T arg) {
            if(m_Transformer)
                arg = m_Transformer(arg);
            try{
                return m_Resources.at(arg);
            }catch(const std::out_of_range&){
                return (m_Resources[arg] =
                    Factory<Class, std::tuple<T, ICache*>>::create(
                        std::tuple<T, ICache*>(arg, this)
                    )
                );
            }
        }
        template<class Cast>
        std::shared_ptr<Cast> cache_as(const T& arg) {
            return std::dynamic_pointer_cast<Cast>(cache(arg));
        }
        
        virtual size_t size() const override {
            return m_Resources.size();
        }
        virtual bool empty() const override {
            return m_Resources.empty();
        }

        virtual void clear() override {
            m_Resources.clear();
        }

        void register_transformer(std::function<T(const T&)> f) {
            m_Transformer=f;
        }
        
    private:

        std::unordered_map<T, std::shared_ptr<Class>> m_Resources;
        std::function<T(const T&)> m_Transformer;
};

#endif

