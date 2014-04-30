#include <memory>
#include <deque>
#include <tuple>
#include "schema.h"

template<class Mutex>
Schema<Mutex> :: Schema(const std::shared_ptr<Meta<Mutex>>& m):
    m_pMeta(m)
{
    assert(m);
}

template<class Mutex>
template<class TMutex>
void Schema<Mutex> :: validate(const std::shared_ptr<Meta<TMutex>>& m) const
{
    // our stack for keeping track of location in tree while iterating
    std::deque<std::tuple<
        std::shared_ptr<Meta<TMutex>>,
        std::unique_lock<TMutex>,
        std::string
    >> metastack;
    
    m->each([this](
        const std::shared_ptr<Meta<TMutex>>& m,
        MetaElement& e,
        unsigned
    ){
        return MetaLoop::STEP;
    },
        (unsigned)Meta<>::EachFlag::DEFAULTS &
            ~(unsigned)Meta<>::EachFlag::RECURSIVE,
        &metastack
    );
}

template<class Mutex>
template<class TMutex>
void Schema<Mutex> :: add_missing_fields(std::shared_ptr<Meta<TMutex>>& m) const
{
}

template<class Mutex>
template<class TMutex>
void Schema<Mutex> :: add_required_fields(std::shared_ptr<Meta<TMutex>>& m) const
{
}

template<class Mutex>
template<class TMutex>
std::shared_ptr<Meta<TMutex>> Schema<Mutex> :: make_default() const
{
    return std::shared_ptr<Meta<TMutex>>();
}

