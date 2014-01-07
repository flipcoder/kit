#ifndef _FACTORY_H_5UUKWBMC
#define _FACTORY_H_5UUKWBMC

#include <functional>
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
//#include <boost/any.hpp>
#include <cassert>
#include <limits>
#include "ifactory.h"
#include "../kit.h"

// TODO: add some way to have resolvers pass data into constructor instead
//   of relying on unchangable ctor parameters (hmm...)
// TODO: or how about add second callback for after the ctor (?)
//   Let's use signals :D

template<class Class, class T, class ClassName=std::string>
class Factory:
    public IFactory,
    virtual public kit::mutexed<std::recursive_mutex>
{
    private:

        // derived class make_shared() functors
        std::vector<
            std::function<
                std::shared_ptr<Class>(const T&)
            >
        > m_Classes;

        std::unordered_map<ClassName, unsigned> m_ClassNames;

        //boost::signals2::signal<void (Class*)> m_Callback;

        // TODO: add this feature
        //std::map<
        //    std::string name,
        //    size_t index
        //> m_ClassNames;

        // class resolution functor, best not to make this public since we want a threading policy eventually,
        // so still needs encapsulation (also, should be read-only from outside)
        std::function<unsigned(const T&)> m_Resolver;

    public:

        virtual ~Factory() {}

        bool empty() const {
            auto l = lock();
            return m_Classes.empty();
        }
        
        //TOD: register_class should allow ID to set to instead of returning one?
        template <class YourClass>
        unsigned register_class(
            ClassName name = ClassName(),
            unsigned id = std::numeric_limits<unsigned>::max()
        ) {
            // bind subclass's make_shared() to functor and store it in m_Classes list
            // TODO: exceptions?
            auto l = lock();

            const size_t size = m_Classes.size();
            if(id == std::numeric_limits<unsigned>::max()) // don't care about class ID
            {
                m_Classes.push_back(
                    std::bind(
                        &std::make_shared<YourClass, const T&>,
                        std::placeholders::_1
                    )
                );
                if(!name.empty())
                    m_ClassNames[name] = static_cast<unsigned>(size);
                return static_cast<unsigned>(size);
            }
            else
            {
                if(id+1 > size) // not max ID
                    m_Classes.resize(id+1);
                m_Classes[id] = std::bind(
                    &std::make_shared<YourClass, const T&>,
                    std::placeholders::_1
                );
                if(!name.empty())
                    m_ClassNames[name] = static_cast<unsigned>(size);
                return id;
            }
        }

        void register_resolver(std::function<unsigned(const T&)> func) {
            auto l = lock();
            m_Resolver = func;
        }

        unsigned class_id(ClassName name) const {
            auto l = lock();
            try{
                return m_ClassNames.at(name);
            } catch(const std::out_of_range&) {}
            return std::numeric_limits<unsigned>::max();
        }
        static unsigned invalid_id() {
            return std::numeric_limits<unsigned>::max();
        }

        std::shared_ptr<Class> create(unsigned id, const T& args) const {
            auto l = lock();
            return m_Classes.at(id)(args);
        }

        std::shared_ptr<Class> create(const T& args) const {
            auto l = lock();
            unsigned id = m_Resolver(args);
            return create(id, args);
        }

        //std::vector<std::shared_ptr<Class>> create_all(
        //    const std::vector<T>& object_list
        //) const {
        //    std::vector<std::shared_ptr<Class>> objects;
        //    objects.reserve(object_list.size());
        //    for(const auto& params: object_list) {
        //        objects.push_back(create(params));
        //    }
        //    return objects;
        //}
};

//template <class Class> std::vector<
//    std::function<
//        std::shared_ptr<Class>(boost::any&)
//    >
//> Factory<Class> ::m_Classes;

//template<class Class> std::function<
//    unsigned(boost::any&)
//> Factory<Class> ::m_Resolver;

#endif

