#ifndef _KIT_SCHEMA_H_PQUEFVOM
#define _KIT_SCHEMA_H_PQUEFVOM

#include <memory>
#include "../meta/meta.h"
#include "../log/log.h"

template<class Mutex=kit::dummy_mutex>
class Schema
{
    public:
        using mutex_type = Mutex;
        
        Schema() = default;

        //template<class TMutex>
        Schema(const std::shared_ptr<Meta<Mutex>>& m);
        
        Schema(const std::string& fn):
            Schema(std::make_shared<Meta<Mutex>>(fn))
        {}
        
        // validate throws on error, check just returns false
        template<class TMutex>
        void validate(const std::shared_ptr<Meta<TMutex>>& m) const;
        template<class TMutex>
        bool check(const std::shared_ptr<Meta<TMutex>>& m) const {
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
        void add_missing_fields(std::shared_ptr<Meta<TMutex>>& m) const;

        // ignores fields marked as optional
        template<class TMutex>
        void add_required_fields(std::shared_ptr<Meta<TMutex>>& m) const;
        
        template<class TMutex>
        std::shared_ptr<Meta<TMutex>> make_default() const;

        std::shared_ptr<Meta<Mutex>> meta() {
            return m_pSchema;
        }
        std::shared_ptr<const Meta<Mutex>> meta() const {
            return m_pSchema;
        }

    private:
        std::shared_ptr<Meta<Mutex>> m_pSchema;
};

#include "schema.inl"

#endif

