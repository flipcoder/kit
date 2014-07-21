#ifndef _META_H_59ROLBDK
#define _META_H_59ROLBDK

#include <boost/signals2.hpp>
#include <boost/any.hpp>
#include "../kit.h"
#include "../log/log.h"
#include <boost/bimap.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread.hpp>
#include <jsoncpp/json/json.h>
//#ifndef BSON_BYTE_ORDER
//#define BSON_BYTE_ORDER BSON_LITTLE_ENDIAN
//#endif
//#include <bson/bson.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <tuple>
#include <atomic>
#include <boost/type_traits.hpp>

struct MetaType {

    MetaType() = default;
    MetaType(MetaType&&) = default;
    MetaType(const MetaType&) = default;
    MetaType& operator=(MetaType&&) = default;
    MetaType& operator=(const MetaType&) = default;

    template<class T>
    MetaType(T val) {

        if(kit::is_shared_ptr<T>::value) {
            //if(typeid(val) == typeid(std::shared_ptr<Meta<Mutex>>))
            id = ID::META;
        }
        else if(typeid(val) == typeid(std::string))
            id = ID::STRING;
        else if(boost::is_integral<T>::value)
        {
            id = ID::INT;
            if(boost::is_signed<T>::value)
                flags |= SIGN;
        }
        else if(boost::is_floating_point<T>::value)
            id = ID::REAL;
        //else if(is_pointer<char* const>::value)
        //    type = MetaType::STRING;
        else if(typeid(val) == typeid(std::nullptr_t))
        {
            id = ID::EMPTY;
        }
        else if(typeid(val) == typeid(bool))
        {
            id = ID::BOOL;
        }
        else if(kit::is_vector<T>::value)
            flags |= CONTAINER;
        else if(typeid(val) == typeid(boost::any))
        {
            //if(val)
            //{
                WARNING("adding raw boost::any value");
                id = ID::USER;
            //}
            //else
            //    id = ID::EMPTY;
        }
        else
        {
            WARNINGf("unserializable type: %s", typeid(T).name());
            id = ID::USER;
        }
    }
    
    /*
     * Really should hash these 3 together so we can do a O(1) call
     * instead of branching when serializing, oh well
     */
    enum class ID {
        EMPTY=0, // null
        INT,
        REAL,
        STRING,
        META,
        BOOL,
        USER
    };

    enum Flag {
        SIGN = kit::bit(0),
        CONTAINER = kit::bit(1),
        MASK = kit::mask(2)
    };

    //enum Storage {
    //    STACK = 0,
    //    UNIQUE = 1,
    //    SHARED = 2,
    //    WEAK = 3
    //};

    ID id = ID::EMPTY;
    unsigned flags = 0;
    //Storage storage = Storage::STACK;

    // 0 means platform-sized
    //unsigned bytes = 0; // size hint (optional)
};

struct MetaElement
{
    MetaElement(MetaType type, const std::string& key, boost::any&& value):
        type(type),
        key(key),
        value(value)
    {}

    MetaElement(const MetaElement& e) = default;
    MetaElement(MetaElement&& e) = default;
    MetaElement& operator=(const MetaElement& e) = default;

    template<class Mutex>
    Json::Value serialize_json(unsigned flags = 0) const;
    
    //template<class Mutex>
    //void deserialize_json(const Json::Value&);

    //MetaElement& operator=(MetaElement&& e) = default;

    //template<class T>
    //MetaElement(T&& val):
    //    type(val),
    //    value(boost::any(value))
    //{}
    // type data (reflection)

    /*
     * Deduced type id
     */
    MetaType type; // null

    void nullify() {
        type.id = MetaType::ID::EMPTY;
        value = boost::any();
    }

    explicit operator bool() const {
        return !value.empty();
    }

    void trigger() {
        if(change)
            (*change)();
    }

    // may throw bad_any_cast
    template<class T>
    T as() const {
        return boost::any_cast<T>(value);
    }

    /*
     * Key (if any)
     * Do not modify the key without modifying Meta's m_Key
     * It is stored two places
     */
    std::string key;

    /*
     * Value is safe to modify
    */
    boost::any value;

    // value change listeners
    std::shared_ptr<boost::signals2::signal<void()>> change;
};

enum class MetaFormat : unsigned {
    UNKNOWN=0,
    JSON,
    HUMAN,
    //BSON
};

enum class MetaLoop : unsigned {
    STEP = 0,
    BREAK,
    CONTINUE, // skip subtree recursion
    REPEAT // repeat this current iteration
};

enum class MetaSerialize {
    /*
     * JSON data makes a distinction between maps and arrays.
     * Since Meta does not, we can store the kv elements as a
     * separate map {key: value}.
     */
    STORE_KEY = kit::bit(0),
    MINIMIZE = kit::bit(1) 
}; 

template<class Mutex=kit::dummy_mutex>
class MetaBase:
    public std::enable_shared_from_this<MetaBase<Mutex>>,
    public kit::mutexed<Mutex>//,
    //public kit::freezable
{
    public:
        using mutex_type = Mutex;
        friend class MetaElement;
        //typedef Mutex mutex_type;
        
        //struct Iterator
        //{
        //    Iterator(MetaBase* m):
        //        m_MetaBase(m),
        //        m_Value(0)
        //    {}
        //    Iterator& operator++() {
        //        ++m_Value;
        //        return *this;
        //    }
        //    Iterator operator++(int) {
        //        Iterator old(*this);
        //        ++m_Value;
        //        return old;
        //    }

        //    private:
        //        MetaBase* m_MetaBase = nullptr;
        //        unsigned m_Value=0;
        //};

        //Iterator end() {
        //    return Iterator(this, size());
        //}

        enum class EachFlag : unsigned {
            LEAFS_ONLY = kit::bit(0),
            POSTORDER = kit::bit(1), // default is preorder
            RECURSIVE = kit::bit(2), // implied on RecursiveIterator
                                   // used on each()
            //ONLY_META = kit::bit(?),
            //SKIP_META = kit::bit(?),
            DEFAULTS = 0
        };
        
        // to avoid possible race conditions, some functions need lock options
        enum LockFlags: unsigned {
            TRY = kit::bit(0)
        };

        /*
         * Serializable allows any class's objects to act as metas
         */
        class Serializable:
            virtual public kit::mutexed<Mutex>
        {
            public:
                Serializable() = default;
                explicit Serializable(const std::string& fn):
                    m_Cached(std::make_shared<MetaBase<Mutex>>(fn))
                {}
                explicit Serializable(const std::shared_ptr<MetaBase<Mutex>>& m):
                    m_Cached(m)
                {}

                //Serializable(const std::shared_ptr<MetaBase>& meta) {
                //    deserialize(meta);
                //}
                virtual ~Serializable() {}

                /*
                 * Return shared_ptr<MetaBase> representation of object
                 *
                 * Set up necessary change listeners at each level of the meta
                 * if necessary
                 */
                virtual void serialize(std::shared_ptr<MetaBase<Mutex>>& meta) const = 0;

                /*
                 * Return shared_ptr<MetaBase> with placeholder values representing
                 *  the meta's schema
                 */
                //virtual std::shared_ptr<MetaBase> schema() const = 0;

                //enum class SyncFlags : unsigned {
                //    NONE = 0
                //};
                void sync() {
                    try{
                        // TODO: add flags to MetaBase::deserialize() to auto-create
                        m_Cached->deserialize();
                    }catch(const Error&){
                        m_Cached->serialize();
                        return;
                    }

                    deserialize(m_Cached);
                }
                virtual void deserialize(
                    const std::shared_ptr<MetaBase<Mutex>>& meta
                ) = 0;

                /*
                 * Check file modification dates
                 * Use MetaBase's change listeners when syncing
                 *
                 * returns false if the meta's format is no longer compatible
                 */
                //virtual void sync(std::shared_ptr<MetaBase>& cached) {
                //}

                //virtual bool needs_sync() const {
                //    return false;
                //}

                // TODO: methods to move file (to another path)?

                //std::shared_ptr<MetaBase> meta() const;
            private:
                // TODO: last modified date (same timestamp type as sync())
                std::shared_ptr<MetaBase<Mutex>> m_Cached;
        };
        
        MetaBase() = default;
        MetaBase(int argc, const char** argv){
            from_args(std::vector<std::string>(argv+1, argv+argc));
        }
        MetaBase(const std::string& fn);
        MetaBase(MetaFormat fmt, const std::string& data);

        MetaBase(MetaBase&&)= delete;
        MetaBase(const MetaBase&)= delete;
        MetaBase& operator=(const MetaBase&) = delete;
        MetaBase& operator=(MetaBase&&) = delete;

        // TODO: add init list
        
        virtual ~MetaBase() {}
        
        void from_args(std::vector<std::string> args);

        // recursive converter between MetaBases of diff mutex types
        template<class Mutex2>
        static std::shared_ptr<MetaBase<Mutex>> convert(
            std::shared_ptr<Mutex2> meta
        ){
            // TODO: impl
            return std::shared_ptr<Mutex>();
        }

        //// Traversal visits MetaBase objects recursively
        //class Traversal
        //{
        //    public:

        //        Traversal(std::shared_ptr<MetaBase>& m) {
        //            //m_Locks.push_back(m->lock());
        //        }

        //        template<class Fn>
        //        static void each(Fn fn) {
                    
        //        }

        //        void unlock() {
        //            m_Locks.clear();
        //        }

        //    private:

        //        std::list<std::unique_lock<Mutex>> m_Locks;
        //};

        //template<class Func>
        MetaLoop each(
            std::function<MetaLoop(
                const std::shared_ptr<MetaBase<Mutex>>&, MetaElement&, unsigned
            )> func,
            unsigned flags = 0, // EachFlag enum
            std::deque<std::tuple<
                std::shared_ptr<MetaBase<Mutex>>,
                std::unique_lock<Mutex>,
                std::string // key in parent (blank if root or empty key)
            >>* metastack = nullptr,
            unsigned level = 0,
            std::string key = std::string()
        );

        enum Which {
            //RETURNED=0, // its a variant, won't need this
            THIS=0,
            OTHER=1,
            NEITHER=2,
            RECURSE=3 // force a recursion
        };

        //struct Reserved {};

        //using lock = kit::mutexed<Mutex>::lock;
        //std::unique_lock<Mutex> this->lock() const {
        //    return kit::mutexed<Mutex>::lock();
        //}
        
        explicit operator bool() const {
            auto l = this->lock();
            return !m_Elements.empty();
        }

        bool empty() const {
            auto l = this->lock();
            return m_Elements.empty();
        }

        bool all_null() const {
            auto l = this->lock();
            for(auto&& e: elements_ref())
                if(e.type.id != MetaType::ID::EMPTY)
                    return false;
            return true;
        }

        size_t size() const {
            auto l = this->lock();
            return m_Elements.size();
        }

        size_t key_count() const {
            auto l = this->lock();
            return m_Keys.size();
        }
        template<class U>
        std::string first_key_of(U t) {
            auto l = this->lock();
            for(auto&& e: elements_ref()) {
                try{
                    if(boost::any_cast<U>(e.value) == t)
                        return e.key;
                }catch(const boost::bad_any_cast&){}
            }
            throw std::out_of_range("no such element");
        }

        /*
         * May throw on null
         */
        void parent(const std::shared_ptr<MetaBase<Mutex>>& p) {
            auto l = this->lock();
            m_pParent = p.get();
        }
        void parent(MetaBase<Mutex>* p) {
            auto l = this->lock();
            m_pParent = p;
        }

        // warning: O(n)
        std::string key_in_parent() {
            auto l = this->lock();
            auto par = parent();
            return par->first_key_of(this->shared_from_this());
        }

        // TODO: Treespace lock is important for recursion here
        std::shared_ptr<MetaBase<Mutex>> parent(
            unsigned lock_flags=0,
            bool recursion=false
        ){
            auto l = this->lock(std::defer_lock);
            if(lock_flags & (unsigned)LockFlags::TRY)
            {
                if(!l.try_lock())
                    throw kit::lock_exception();
            }
            else
            {
                l.lock();
            }

            if(!recursion)
            {
                //auto p = m_pParent.lock();
                //return p ? p : shared_from_this();
                //return m_pParent.lock(); // allow null for this case
                return m_pParent ? m_pParent->shared() : std::shared_ptr<MetaBase<Mutex>>();
            }
            else
            {
                //auto p = m_pParent.lock();
                return m_pParent ?
                    m_pParent->parent(lock_flags, recursion) :
                    this->shared_from_this();
            }
        }

        std::shared_ptr<MetaBase<Mutex>> shared() {
            return this->shared_from_this();
        }
        std::shared_ptr<const MetaBase<Mutex>> shared() const {
            return this->shared_from_this();
        }

        /*
         * Sync config with a specifc key-value file (ini?)
         *
         * TODO: Make flags handle overwrite case
         */
        //void sync() {
        //}
        //void sync(const std::string& fn) {
        //    m_Filename = fn;
        //}

        void filename(const std::string& fn) {
            auto l = this->lock();
            m_Filename = fn;
        }
        std::string filename() const {
            auto l = this->lock();
            return m_Filename;
        }

        enum class MergeFlags: unsigned
        {
            // other smart methods of replace may require another merge() fn
            //   that takes a bool() fn sort of like a sort()
            REPLACE = kit::bit(0),
            RECURSIVE = kit::bit(1),

            // allow failed locks (one retry w/ timeout callback)
            INCREMENTAL = kit::bit(2),
            MASK = kit::mask(3),

            DEFAULTS = REPLACE | RECURSIVE
        };

        //struct TimeoutResult {
        //    //unsigned attempts = 0; // 0 = infinite // depends on INCREMENTAL flag
        //    unsigned timeout = 0; // delay between tries ms (you may also block in your functor)
        //};
        // return value (bool) indicates whether to retrigger
        typedef std::function<bool(std::shared_ptr<MetaBase<Mutex>>&)> TimeoutCallback_t;

        typedef std::function<boost::variant<Which, MetaElement>(
            const MetaElement&, const MetaElement&
        )> WhichCallback_t;
        
        typedef std::function<void(
            const std::shared_ptr<MetaBase<Mutex>>, const MetaElement&
        )> VisitorCallback_t;

        /*
         * Merge a meta into this one, recursively
         *
         * TODO: timeout callback if we hit a lock
         * TODO: incremental behavior (merge only what you can without locking)
         *  so you can loop this until `t` is empty)
         *
         *  For throw-on-conflict behavior, throw inside of `which`
         */
        template<class Mutex2>
        void merge(
            const std::shared_ptr<MetaBase<Mutex2>>& t,
            WhichCallback_t which,
            unsigned flags = (unsigned)MergeFlags::DEFAULTS, // MergeFlags
            TimeoutCallback_t timeout = TimeoutCallback_t(),
            VisitorCallback_t visit = VisitorCallback_t()
        );

        template<class Mutex2>
        void merge(
            const std::shared_ptr<MetaBase<Mutex2>>& t,
            unsigned flags = (unsigned)MergeFlags::DEFAULTS // MergeFlags
        );

        void merge(
            const std::string& fn,
            WhichCallback_t which,
            unsigned flags = (unsigned)MergeFlags::DEFAULTS, // MergeFlags
            TimeoutCallback_t timeout = TimeoutCallback_t(),
            VisitorCallback_t visit = VisitorCallback_t()
        );

        void merge(
            const std::string& fn,
            unsigned flags = (unsigned)MergeFlags::DEFAULTS // MergeFlags
        );
        
        //void merge(
        //    const MetaBase& t,
        //    unsigned flags = (unsigned)MergeFlags::DEFAULTS,
        //    std::function<
        //        boost::any(const boost::any&, const boost::any&)
        //    > which =
        //    std::function<
        //        boost::any(
        //            const boost::any&,
        //            const boost::any&
        //        )
        //    >()
        //);

        unsigned id(const std::string& key) {
            auto l = this->lock();
            return m_Keys.at(key);
            //TODO return m_Keys[s]
        }

        /*
         * Is the value out of range or empty?
         */
        bool has(unsigned id) const {
            auto l = this->lock();
            try{
                return bool(m_Elements.at(id));
            }catch(const std::out_of_range&){
                return false;
            }
        }
        bool has(const std::string& key) const {
            auto l = this->lock();
            try{
                return m_Keys.find(key) != m_Keys.end();
            }catch(const std::out_of_range&){
                return false;
            }
        }

        //template<class T>
        //bool is_type() const {
        //    // TODO: add type check (if actually possible)
        //    assert(false);
        //    return false;
        //}

        /*
         * Get index of key
         * May throw out_of_range()
         */
        unsigned index(const std::string& key) {
            auto l = this->lock();
            return m_Keys.at(key);
        }

        /*
         * May throw
         */
        
        //template<class T>
        //T at(unsigned idx) {
        //    auto l = this->lock();
        //    return boost::any_cast<T>(m_Elements.at(idx).value);
        //}
        template<class T>
        T at(unsigned idx) const {
            auto l = this->lock();
            return boost::any_cast<T>(m_Elements.at(idx).value);
        }
        template<class T>
        T raw(unsigned idx) {
            auto l = this->lock();
            return m_Elements.at(idx).value;
        }

        std::shared_ptr<MetaBase<Mutex>> meta(const std::string& key) {
            auto l = this->lock();
            return boost::any_cast<std::shared_ptr<MetaBase<Mutex>>>(
                m_Elements.at(m_Keys.at(key)).value
            );
        }
        std::shared_ptr<const MetaBase<Mutex>> meta(const std::string& key) const {
            auto l = this->lock();
            return boost::any_cast<std::shared_ptr<MetaBase<Mutex>>>(
                m_Elements.at(m_Keys.at(key)).value
            );
        }
        
        /*
         * May throw bad_any_cast or out_of_range
         */
        template<class T>
        T at(const std::string& key) const {
            auto l = this->lock();
            return boost::any_cast<T>(
                m_Elements.at(m_Keys.at(key)).value
            );
        }

        /*
         * May throw bad_any_cast or out_of_range
         */
        //template<class T>
        //T at(const std::string& key) {
        //    auto l = this->lock();
        //    return boost::any_cast<T>(
        //        m_Elements.at(m_Keys.at(key)).value
        //    );
        //}
        
        //template<class T>
        //T at(const std::string& key, T def) {
        //    try{
        //        return at<T>(key);
        //    }catch(const std::out_of_range&){
        //    }catch(const boost::bad_any_cast&){
        //    }
        //    return def;
        //}
        template<class T>
        T at(const std::string& key, T def) const {
            try{
                return at<T>(key);
            }catch(const std::out_of_range&){
            }catch(const boost::bad_any_cast&){
            }
            return def;
        }
        /*
         * Gets reference to stored property
         * Warning: You're responsible for triggering change listeners
         *  using trigger_change(id) if you modify this
         */
        //boost::any& ref(unsigned id) {
        //    return m_Change.at(id);
        //}
        //const boost::any& ref(unsigned id) const {
        //    return m_Change.at(id);
        //}

        /*
         * Triggers the change callbacks on the key with the specific id
         */
        void trigger_change(unsigned id) {
            auto l = this->lock();
            m_Elements.at(id).trigger();
            //(*m_Change.at(id))();
        }


        //enum AddFlag {
        //    UNIQUE = kit::bit(0)
        //};
        template<class T>
        unsigned add(
            T val
            //kit::thread_safety safety = kit::thread_safety::safe
        ){
            return set(std::string(), val);
        }

        enum class PathFlags : unsigned {
            ABSOLUTE_PATH = kit::bit(0),
            REPLACE_ON_CONFLICT = kit::bit(1), // default behavior for now
            ENSURE = kit::bit(2)
        };
        // Ensures the path exists in the tree and initializes an element `val`
        // the endpoint.
        std::tuple<std::shared_ptr<MetaBase<Mutex>>, bool> path(
            const std::vector<std::string>& path,
            unsigned flags = 0
        );

        //std::optional<

        /*
         * Adds key with a value if the doesn't already exist
         *
         * TODO: Overwrites conflicting types
         */
        template<class T>
        T ensure(
            const std::string& key,
            const T& val
        ){
            assert(!key.empty());
            auto l = this->lock();
            unsigned idx;
            try{
                // element exists?
                idx = index(key);

                // same type?
                try{
                    return at<T>(key);
                    // already there, done
                    //return idx;
                }catch(const boost::bad_any_cast&){
                    // type conflict, override!
                    set(key,val);
                    return val;
                }
            }catch(const std::out_of_range&){
                // doesn't exist
                set(key,val);
                return val;
            }
        }

        /*
         * Set func with automatic type hints (WIP)
         */
        //enum class SetFlags {
        //    // only "ensures" the value is there and initializes it to default `val`,
        //    // but does not overwrite a preexisting value with the given key
        //    ENSURE = kit::bit(0)
        //};
        template<class T>
        unsigned set(
            const std::string& key,
            T val
            //unsigned flags = 0
        ){
            auto l = this->lock();

            // entry has a key
            if(!key.empty())
            {
                std::unordered_map<std::string, unsigned>::iterator itr;

                // key exists?
                if((itr = m_Keys.find(key)) != m_Keys.end()) {
                    // TODO: if dynamic typing is off, check type compatibility
                    auto& e = m_Elements[itr->second];
                    e.type = MetaType(val); // overwrite type just in case
                    e.value = val;
                    // TODO: trigger change listener?... or maybe only for sync()?
                    return itr->second;
                }
            }
            //else
            //{
            //    // ENSURE is meaningless if there is no key specified
            //    assert(!(flags & (unsigned)SetFlags::ENSURE));
            //}

            // add a new value
            const size_t idx = m_Elements.size();
            if(!key.empty())
                m_Keys[key] = idx;
            m_Elements.emplace_back(MetaType(val), key, boost::any(val));

            if(
                m_Elements[idx].type.id == MetaType::ID::META
                //m_Elements[idx].type.storage == MetaType::Storage::SHARED
            ){
                at<std::shared_ptr<MetaBase<Mutex>>>(idx)->parent(this);
            }

            return idx;

            //unsigned id = m_Keys.ensure(key);
            //if(id >= m_Values.size())
            //    m_Values.resize(id+1);
            //if(id >= m_Types.size())
            //    Types.resize(id+1);

            //m_Types.at(id) = MetaType(val);
            //m_Values.at(id) = std::move(val);
            //trigger_change(id);
            //return id;
        }

        /*
         * Set (update) element already at index (offset)
         *
         * Returns bool indicating success
         */
        template<class T>
        bool set(
            unsigned offset,
            T&& val
        ){
            // Should really check type compatibility here...

            try{
                m_Elements.at(offset).value = val;
            }catch(...){
                return false;
            }
            return true;
        }

        /*
         * Creates property with key and optional value (default is blank)
         * Returns the id used to refer to the property
         */
        //unsigned set(
        //    const std::string& key,
        //    const boost::any& val
        //){
            
        //}

        /*
         * Call trigger_change(id) after
         */
        boost::any& ref(unsigned id) {
            return m_Elements.at(id).value;
        }
        
        //void set(
        //    unsigned id,
        //    const boost::any& value
        //){
        //    // TODO need boost::any compare function for common types,
        //    //   otherwise it's _always_ not equal
        //    //if(value != m_Values.at(id)) {
        //    if(id >= m_Values.size())
        //        m_Values.resize(id+1);
        //    m_Values.at(id) = value;
        //    trigger_change(id);
        //    //}
        //}

        //std::unique_ptr<boost::signals2::scoped_connection> on_change(
        //    unsigned id,
        //    std::function<void()>&& callback
        //){
        //    auto l = this->lock();

        //    //if(id>=m_Change.size())
        //    //    m_Change.resize(id+1);
        //    //if(!m_Change.at(id)) {
        //    //    m_Change.at(id) = std::make_shared<
        //    //        boost::signals2::signal<void()>
        //    //    >();
        //    //}
        //    //return kit::make_unique<boost::signals2::scoped_connection>(
        //    //    m_Change.at(id)->connect(callback)
        //    //);
        //}

        void callbacks(bool enabled) {
            m_bCallbacks = enabled;
        }
        bool callbacks() {
            return m_bCallbacks;
        }

        // May throw out_of_range
        void remove(unsigned id) {
            auto l = this->lock();
            // TODO: remove hooks too?
            if(!m_Elements.at(id).key.empty())
                remove(m_Elements[id].key);
            m_Elements.erase(m_Elements.begin() + id);
        }

        void pop_back() {
            auto l = this->lock();
            if(!m_Elements.empty())
                m_Elements.pop_back();
            else
                throw std::out_of_range("meta pop_back()");
        }
        void pop_front() {
            auto l = this->lock();
            if(m_Elements.empty())
                throw std::out_of_range("meta empty");
            m_Elements.erase(m_Elements.begin());
        }

        // May throw out_of_range
        void remove(const std::string& key) {
            if(key.empty())
                throw std::out_of_range("empty key");

            auto l = this->lock();
            unsigned idx = m_Keys.at(key);
            m_Elements.erase(m_Elements.begin() + idx);
            m_Keys.erase(key);
            //kit::remove_if(m_Elements, [&](Element& e){
            //    // TODO: remove key from hooks
            //    bool b = e.key == key;
            //    if(b)
            //        m_Keys.erase(key);
            //    return b;
            //});
        }

        // Copying internal MetaBase::Element into an existing element of type key
        // may throw std::out_of_range
        void replace(
            unsigned id,
            const MetaElement& e
        ){
            // TODO: add flag to specify whether to replace callbacks?
            auto l = this->lock();
            m_Elements.at(id) = e;
        }
        void replace(
            unsigned id,
            MetaElement&& e
        ){
            // TODO: add flag to specify whether to replace callbacks?
            auto l = this->lock();
            m_Elements.at(id) = e;
        }

        /*
         * Get the full key path of the current node
         * If this is a virtual fs, this is the abs path from the root
         *
         * NOT YET IMPLEMENTED
         */
        //std::vector<std::string> path(
        //    unsigned lock_flags = 0
        //){ // TODO: const vers
        //    auto r = root();
        //    assert(r);
        //    auto rl = r->lock();

        //    std::vector<std::shared_ptr<MetaBase<Mutex>>> metastack;

        //    do{
        //        metastack.clear();
        //        try{
        //            // hooks?
        //            // auto p = parent();
        //        }catch(const kit::lock_exception& e){
        //            if(lock_flags & (unsigned)LockFlags::TRY)
        //                throw e;
        //            continue;
        //        }
        //        break;
        //    }while(true);
            
        //    //std::vector<std::string> p;
        //    //auto parent = parent();
        //    //while(parent())
        //    //{
        //    //    p.push_back();
        //    //}

        //    //std::vector<std::string> abs_path;
        //    //return parent()->path();
        //    return std::vector<std::string>();
        //}

        /*
         * Is meta completely serializable?
         */
        //bool is_serializable() const {
        //    auto l = this->lock();
        //    assert(false);
        //    return false;
        //}

        /*
         * Is value serializable?
         */
        //static bool is_serializable(boost::any value) {
        //    assert(false);
        //    return false;
        //}

        // TODO: make a value factory (store it in typepalette?)

        std::string serialize(MetaFormat fmt, unsigned flags = 0) const;
        void serialize(const std::string& fn, unsigned flags = 0) const;
        void serialize(unsigned flags = 0) const {
            auto l = this->lock();
            serialize(m_Filename, flags);
        }

        // use enforce flags instead
        //enum DeserializeFlag {
            /*
             * Detect [key,value] arrays are treat them as keys
             */
        //    DETECT_KEYS = kit::bit(0)
        //};
        void deserialize(
            MetaFormat fmt,
            const std::string& data,
            const std::string& fn = std::string()
        );
        void deserialize(
            MetaFormat fmt,
            std::istream& data,
            const std::string& fn = std::string()
        );
        
        void deserialize() {
            auto l = this->lock();
            deserialize(m_Filename);
        }
        void deserialize(const std::string& fn);

        static MetaFormat filename_to_format(const std::string& fn);

        void clear() {
            auto l = this->lock();
            kit::clear(m_Elements);
            kit::clear(m_Keys);
            //kit::clear(m_Hooks);
            //kit::clear(m_Values);
            //kit::clear(m_Types);
        }

        // TODO: need iterator for each (id)(key)(type)(value)

        //typedef unsigned offset_t;
        //typedef unsigned hook_t;

        std::vector<MetaElement> elements(
            //kit::thread_safety safety = kit::thread_safety::safe
        ) const {
            auto l = this->lock();
            return m_Elements;
        }

        /*
         * Warning: Remember to hold lock while using this
         */
        std::vector<MetaElement>& elements_ref() {
            return m_Elements;
        }
        const std::vector<MetaElement>& elements_ref() const {
            return m_Elements;
        }

        std::unordered_map<std::string, unsigned>&  keys_ref() {
            return m_Keys;
        }
        const std::unordered_map<std::string, unsigned>& keys_ref() const {
            return m_Keys;
        }

        // WARNING: Direct iteration requires lock if Mutex policy non-dummy
        typedef std::vector<MetaElement>::iterator iterator;
        typedef std::vector<MetaElement>::const_iterator const_iterator;
        const_iterator cbegin() const { return m_Elements.cbegin(); }
        const_iterator cend() const { return m_Elements.cend(); }
        iterator begin() { return m_Elements.begin(); }
        iterator end() { return m_Elements.end(); }

        /*
         * Check if meta is a pure map, array, or another static type
         */
        bool is_map() const {
            auto l = this->lock();
            return m_Keys.size() == m_Elements.size();
        }
        bool is_array() const {
            auto l = this->lock();
            return m_Keys.empty();
            //return m_Keys.size() != m_Elements.size();
        }

        std::shared_ptr<MetaBase<Mutex>> root(unsigned lock_flags = 0) {
            // TODO: lock order is bad here, should be try_lock
            auto l = this->lock();

            std::shared_ptr<MetaBase<Mutex>> m(this->shared_from_this());
            return m->parent(lock_flags, true);
        }

    private:

        //std::optional<Mutex> m_Treespace;

        void serialize_json(Json::Value& v) const;
        void deserialize_json(const Json::Value& v);

        std::string m_Filename;
        bool m_bModified = true; // modified since last save?

        /*
         * Enforce flags
         */
        enum Flag {
            // Don't reseting values to incompatible types thru set()
            STATIC = kit::bit(0),
            // all types in the meta are the same
            TYPED = kit::bit(1),
            // guarentee meta is value array OR purely key-value
            PURE = kit::bit(2),
            //// Merge when deserializing (force UNIQUE flag)
            //MERGE = kit::bit(3)
        };
        unsigned flags = 0;

        // TODO: need type enforcements and schemas
        enum TypeHint {
            DYNAMIC = 0, // allow anything, serialize as array and merge nested maps
            MAP = 1, // all elements are key->value
            ARRAY = 2 // don't allow keys
        } m_TypeHint = DYNAMIC;

        // There's two ways to deal with mixing a map and an array,
        // We could store values with keys as tuples [key,value],
        // or how about storing the non-keyed values in their own array
        // I like the second way better
        
        
        // each value in a meta can have
        // (id) (hook) (key) (type) (value)
        // ------------^  type is optional (for faster parsing)

        //kit::string_index m_Keys;

        // boost::any allows Values to be OTHER MetaBases (in the form of
        //  shared_ptr<MetaBase> for shared or weak_ptr<MetaBase> for links)
        //  so we can nest these to form a property tree

        // hooks (for non-locked indexing into data) (see notes)
        //kit::index<unsigned> m_Hooks;

        // str -> offset (m_Elements)
        // NOTE: the key is stored in both m_Keys and in m_Elements

        // TODO: switch to bimap?
        std::unordered_map<std::string, unsigned> m_Keys;
        //std::unordered_map<unsigned, std::shared_ptr<unsigned>> m_Hooks;
        std::vector<MetaElement> m_Elements; // elements also contain keys

        //std::shared_ptr<boost::shared_mutex> m_Treespace;

        //void unsafe_merge(MetaBase<Mutex>&& t, unsigned flags);

        /*
         * Note: bind() a parameter yourself to get the specific object id
         *
         * Had to be wrapped in ptr since resize() invokes a copy ctor :(
         */
        //std::vector<
        //    std::shared_ptr<boost::signals2::signal<void()>>
        //> m_Change;

        // may be empty / incorrect, search parent to resolve
        //unsigned m_IndexOfSelfInParent = std::numeric_limits<unsigned>::max();

        //std::weak_ptr<MetaBase<Mutex>> m_pParent;
        MetaBase<Mutex>* m_pParent = nullptr;

        bool m_bCallbacks = true;

        //bool m_bQuiet = false; // don't throw on file not found
};

using Meta = MetaBase<kit::dummy_mutex>;
using MetaMT = MetaBase<std::recursive_mutex>;

#include "meta.inl"

#endif

