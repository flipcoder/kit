#ifndef _KIT_SCHEMA_H_PQUEFVOM
#define _KIT_SCHEMA_H_PQUEFVOM

#include <memory>
#include "../meta/meta.h"
#include "../log/log.h"
#include <kit/smart_ptr.hpp>

template<class Mutex, template <class> class Ptr, template <typename> class This>
class SchemaBase
{
    public:
        using mutex_type = Mutex;
        
        SchemaBase() = default;

        SchemaBase(const Ptr<Meta_<Mutex,Ptr,This>>& m);
        
        SchemaBase(const std::string& fn):
            SchemaBase(kit::make<Ptr<Meta_<Mutex,Ptr,This>>>(fn))
        {}
        
        // validate throws on error, check just returns false
        template<class TMutex, template <class> class TPtr, template <class> class TThis>
        void validate(const TPtr<Meta_<TMutex,TPtr,TThis>>& m) const;
        
        template<class TMutex, template <class> class TPtr, template <class> class TThis>
        bool check(const TPtr<Meta_<TMutex,TPtr,TThis>>& m) const {
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
        template<class TMutex, template <class> class TPtr, template <class> class TThis>
        void add_missing_fields(TPtr<Meta_<TMutex,TPtr,TThis>>& m) const;

        // ignores fields marked as optional
        template<class TMutex, template <class> class TPtr, template <class> class TThis>
        void add_required_fields(TPtr<Meta_<TMutex,TPtr,TThis>>& m) const;
        
        template<class TMutex, template <class> class TPtr, template <class> class TThis>
        TThis<Meta_<TMutex,TPtr,TThis>> make_default() const;

        Ptr<Meta_<Mutex,Ptr,This>> meta() {
            return m_pSchema;
        }
        Ptr<const Meta_<Mutex,Ptr,This>> meta() const {
            return m_pSchema;
        }

    private:
        Ptr<Meta_<Mutex,Ptr,This>> m_pSchema;
};

using Schema = SchemaBase<kit::dummy_mutex, kit::local_shared_ptr, kit::enable_shared_from_this>;
using SchemaMT = SchemaBase<std::recursive_mutex, std::shared_ptr, std::enable_shared_from_this>;

#include "schema.inl"

#endif

