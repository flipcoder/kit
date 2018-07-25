#ifndef _KIT_SCHEMA_H_PQUEFVOM
#define _KIT_SCHEMA_H_PQUEFVOM

#include <memory>
#include "../meta/meta.h"
#include "../log/log.h"
#include <kit/smart_ptr.hpp>

template<class Mutex, template <class> class Storage, template <typename> class This>
class SchemaBase
{
    public:
        using mutex_type = Mutex;
        
        SchemaBase() = default;

        SchemaBase(const Storage<MetaBase<Mutex,Storage,This>>& m);
        
        SchemaBase(const std::string& fn):
            SchemaBase(kit::make<Storage<MetaBase<Mutex,Storage,This>>>(fn))
        {}
        
        // validate throws on error, check just returns false
        template<class TMutex, template <class> class TStorage, template <class> class TThis>
        void validate(const TStorage<MetaBase<TMutex,TStorage,TThis>>& m) const;
        
        template<class TMutex, template <class> class TStorage, template <class> class TThis>
        bool check(const TStorage<MetaBase<TMutex,TStorage,TThis>>& m) const {
            try{
                Log::Silencer ls;
                validate(m);
                return true;
            }catch(Error&){
            }catch(std::out_of_range&){
            }catch(boost::bad_any_cast&){
            }
            return false;
        }

        // adds all schema fields with default values
        template<class TMutex, template <class> class TStorage, template <class> class TThis>
        void add_missing_fields(TStorage<MetaBase<TMutex,TStorage,TThis>>& m) const;

        // ignores fields marked as optional
        template<class TMutex, template <class> class TStorage, template <class> class TThis>
        void add_required_fields(TStorage<MetaBase<TMutex,TStorage,TThis>>& m) const;
        
        template<class TMutex, template <class> class TStorage, template <class> class TThis>
        TThis<MetaBase<TMutex,TStorage,TThis>> make_default() const;

        Storage<MetaBase<Mutex,Storage,This>> meta() {
            return m_pSchema;
        }
        Storage<const MetaBase<Mutex,Storage,This>> meta() const {
            return m_pSchema;
        }

    private:
        Storage<MetaBase<Mutex,Storage,This>> m_pSchema;
};

using Schema = SchemaBase<kit::dummy_mutex, kit::local_shared_ptr, kit::enable_shared_from_this>;
using SchemaMT = SchemaBase<std::recursive_mutex, std::shared_ptr, std::enable_shared_from_this>;

#include "schema.inl"

#endif

