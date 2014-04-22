#include "schema.h"

template<class Mutex>
template<class TMutex>
Schema<Mutex> :: Schema(const std::shared_ptr<Meta<TMutex>>& m):
    m_pMeta(m)
{
    assert(m);
}

template<class Mutex>
template<class TMutex>
void Schema<Mutex> :: validate(const std::shared_ptr<const Meta<TMutex>>& m) const
{
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

