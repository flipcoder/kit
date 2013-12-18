#ifndef _ICACHE_H
#define _ICACHE_H

//template<class Class, class T>
class ICache
{
    public:
        virtual ~ICache() {}
        //virtual std::shared_ptr<Class> cache(const T& arg) = 0;
        virtual void optimize() = 0;
        virtual void clear() = 0;
        virtual bool empty() const = 0;
        virtual size_t size() const = 0;
};

#endif

