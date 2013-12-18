#ifndef _FACTORY_H_5UUKWBMC
#define _FACTORY_H_5UUKWBMC

#include <functional>
#include <vector>
#include <memory>
#include <stdexcept>
//#include <boost/any.hpp>
#include <cassert>
#include <limits>

// TODO: add some way to have resolvers pass data into constructor instead
//   of relying on unchangable ctor parameters (hmm...)
// TODO: or how about add second callback for after the ctor (?)
//   Let's use signals :D

template<class Class, class T>
class Factory
{
    private:

        // derived class make_shared() functors
        std::vector<
            std::function<
                std::shared_ptr<Class>(const T&)
            >
        > m_Classes;

        //boost::signals2::signal<void (Class*)> m_Callback;

        // TODO: add this feature
        //std::map<
        //    std::string name,
        //    size_t index
        //> m_ClassNames;

        // class resolution functor, best not to make this public since we want a threading policy eventually,
        // so still needs encapsulation (also, should be read-only from outside)
        std::function<unsigned int(const T&)> m_Resolver;

    public:

        virtual ~Factory() {}

        bool empty() const {
            return m_Classes.empty();
        }
        
        //TOD: register_class should allow ID to set to instead of returning one?
        template <class YourClass>
        unsigned int register_class(unsigned int id = std::numeric_limits<unsigned int>::max()) {
            // bind subclass's make_shared() to functor and store it in m_Classes list
            // TODO: exceptions?

            const size_t size = m_Classes.size();
            if(id == std::numeric_limits<unsigned int>::max()) // don't care about class ID
            {
                m_Classes.push_back(
                    std::bind(
                        &std::make_shared<YourClass, const T&>,
                        std::placeholders::_1
                    )
                );
                return static_cast<unsigned int>(size);  //return index (-1 is invalid)
            }
            else
            {
                if(id+1 > size) // not max ID
                    m_Classes.resize(id+1);
                m_Classes[id] = std::bind(
                    &std::make_shared<YourClass, const T&>,
                    std::placeholders::_1
                );
                return id;
            }
        }

        void register_resolver(std::function<unsigned int(const T&)> func) {
            m_Resolver = func;
        }

        std::shared_ptr<Class> create(unsigned int id, const T& args) const {
            return m_Classes.at(id)(args);
        }

        std::shared_ptr<Class> create(const T& args) const {
            unsigned int id = m_Resolver(args);
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
//    unsigned int(boost::any&)
//> Factory<Class> ::m_Resolver;

#endif

