#include <memory>
#include <deque>
#include <tuple>
#include <boost/algorithm/string.hpp>
#include <kit/smart_ptr.hpp>
#include "schema.h"

template<class Mutex, template <class> class Storage, template <typename> class This>
SchemaBase<Mutex,Storage,This> :: SchemaBase(const Storage<MetaBase<Mutex,Storage,This>>& m):
    m_pSchema(m)
{
    assert(m);
}

template<class Mutex, template <class> class Storage, template <typename> class This>
template<class TMutex, template <class> class TStorage, template <typename> class TThis>
void SchemaBase<Mutex,Storage,This> :: validate(const TStorage<MetaBase<TMutex,TStorage,TThis>>& m) const
{
    // our stack for keeping track of location in tree while iterating
    std::deque<std::tuple<
        TStorage<const MetaBase<TMutex,TStorage,TThis>>,
        std::unique_lock<TMutex>,
        std::string
    >> metastack;
    
    // TODO: loop thru schema, look up required fields in m
    
    m->each_c([this, &metastack](
        //Storage<const MetaBase<TMutex,TStorage,TThis>> item,
        Storage<const MetaBase<Mutex,Storage,This>> parent,
        const MetaElement& e,
        unsigned level
    ){
        //LOGf("level: %s, key: %s", level % e.key);
        //if(boost::algorithm::starts_with(e.key, "."))
        //{
            
        //}
        
        if(e.type.id != MetaType::ID::META) {
        
            std::vector<std::string> path;
            for(auto&& d: metastack) {
                std::string key = std::get<2>(d);
                if(!key.empty())
                    path.push_back(std::get<2>(d));
            }
            path.push_back(e.key);
            if(path.empty())
                return MetaLoop::CONTINUE;
            
        //LOGf("path element: %s", boost::algorithm::join(path,"/"));
        
        Storage<MetaBase<Mutex,Storage,This>> r;
        try{
            auto schema_metadata = m_pSchema->path(path);
            bool valid_value = false;
            //LOGf("json: %s", schema_metadata->serialize(MetaFormat::JSON));
            for(auto&& val: *schema_metadata->meta(".values")) {
                //LOGf("key: %s", e.key);
                if(kit::any_eq(val.value, e.value)) {
                    valid_value = true;
                    break;
                }
            }
            if(!valid_value)
                K_ERROR(PARSE, "schema validation failed");
                    
            //}catch(const boost::bad_any_cast&){
            }catch(const std::out_of_range&){
            }
            
            // metastack -> path
            // path -> schema format
            // look up path->at(".values")->have(element) in schema
            // alternative: dictionary with key=".value" value
            //
            // example:
            // {
            //   "pick_a_number": {
            //     // first way
            //     ".values":[
            //        1,2,3
            //     ]
            //     // second way
            //     ".values:[
            //       {".value": 1},
            //       {".value": 3},
            //       {".value": 2}
            //     ]
            //   }
            // Should be able to mix and match the above ^
            //
            // TODO: other hints:
            //   .min/gte, .max/lte, .gt, .lt
            //
            // throw out_of_range if not
        }
        
        return MetaLoop::STEP;
    },
        (unsigned)MetaBase<>::EachFlag::DEFAULTS |
            (unsigned)MetaBase<>::EachFlag::RECURSIVE,
        &metastack
    );
}

template<class Mutex, template <class> class Storage, template <typename> class This>
template<class TMutex, template <class> class TStorage, template <typename> class TThis>
void SchemaBase<Mutex,Storage,This> :: add_missing_fields(TStorage<MetaBase<TMutex,TStorage,TThis>>& m) const
{
}

template<class Mutex, template <class> class Storage, template <typename> class This>
template<class TMutex, template <class> class TStorage, template <typename> class TThis>
void SchemaBase<Mutex,Storage,This> :: add_required_fields(TStorage<MetaBase<TMutex,TStorage,TThis>>& m) const
{
}


template<class Mutex, template <class> class Storage, template <typename> class This>
template<class TMutex, template <class> class TStorage, template <typename> class TThis>
TThis<MetaBase<TMutex,TStorage,TThis>> SchemaBase<Mutex,Storage,This> :: make_default() const
{
    return TStorage<MetaBase<TMutex>>();
}

