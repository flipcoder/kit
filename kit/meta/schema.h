#ifndef _KIT_SCHEMA_H_PQUEFVOM
#define _KIT_SCHEMA_H_PQUEFVOM

#include <memory>
#include "../meta/meta.h"
#include "../log/log.h"

template<class Mutex=kit::dummy_mutex>
class SchemaBase
{
    public:
        using mutex_type = Mutex;
        
        SchemaBase() = default;

        //template<class TMutex>
        SchemaBase(const std::shared_ptr<MetaBase<Mutex>>& m);
        
        SchemaBase(const std::string& fn):
            SchemaBase(std::make_shared<MetaBase<Mutex>>(fn))
        {}
        
        // validate throws on error, check just returns false
        template<class TMutex>
        void validate(const std::shared_ptr<MetaBase<TMutex>>& m) const;
        template<class TMutex>
        bool check(const std::shared_ptr<MetaBase<TMutex>>& m) const {
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
        template<class TMutex>
        void add_missing_fields(std::shared_ptr<MetaBase<TMutex>>& m) const;

        // ignores fields marked as optional
        template<class TMutex>
        void add_required_fields(std::shared_ptr<MetaBase<TMutex>>& m) const;
        
        template<class TMutex>
        std::shared_ptr<MetaBase<TMutex>> make_default() const;

        std::shared_ptr<MetaBase<Mutex>> meta() {
            return m_pSchema;
        }
        std::shared_ptr<const MetaBase<Mutex>> meta() const {
            return m_pSchema;
        }

    private:
        std::shared_ptr<MetaBase<Mutex>> m_pSchema;
};

using Schema = SchemaBase<kit::dummy_mutex>;
using SchemaMT = SchemaBase<std::recursive_mutex>;

#include "schema.inl"

#endif

