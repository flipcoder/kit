#ifndef UTIL_H_SNIHUCWC
#define UTIL_H_SNIHUCWC

#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <thread>
#include <utility>
#include <map>
#include <atomic>
#include <future>
#include <chrono>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/bimap.hpp>
#include <boost/any.hpp>
#include <boost/logic/tribool.hpp>

// lazy range for entire container
#define ENTIRE(blah) (blah).begin(), (blah).end()

/*
 * marking unused parameter to supress warnings, but also keep the name
 * use like this:
 *  void my_function(int UNUSED(keep_this_name)) {} // no warnings
 */
#define UNUSED(blah)

#ifdef K_USE_BRANCH_OPT
    #define K_LIKELY(expr) __builtin_expect((expr),true)
    #define K_UNLIKELY(expr) __builtin_expect((expr),false)
#else
    #define K_LIKELY(expr) (expr)
    #define K_UNLIKELY(expr) (expr)
#endif

namespace kit
{
    template<class T>
    void clear(T& t)
    {
        t = T();
    }

    //enum class thread_safety {
    //    unsafe=0,
    //    safe
    //};

    template<class T, class _ = void>
    struct is_vector {
        static const bool value = false;
    };
    template<class T>
    struct is_vector<std::vector<T>> {
        static bool const value = true;
    };
    template<class T, class _ = void>
    struct is_shared_ptr {
        static const bool value = false;
    };
    template<class T>
    struct is_shared_ptr<std::shared_ptr<T>> {
        static bool const value = true;
    };

    template<class T>
    T mod(T a, T b)
    {
        return (a%b+b)%b;
    }

    //template<class Class=c>
    //class mutex_wrapper<>
    //{
    //}

    //template<class T>
    //struct move_on_copy
    //{
    //    public:
    //        move_on_copy() = default;
            
    //        move_on_copy(T&& rhs):
    //            m_Data(std::forward<T>(rhs)) {}
            
    //        move_on_copy(move_on_copy&& rhs):
    //            m_Data(std::forward<T>(rhs.m_Data)) {}
            
    //        move_on_copy(move_on_copy& rhs):
    //            m_Data(std::move(rhs.m_Data)) {}

    //        move_on_copy& operator=(move_on_copy&& rhs){
    //            m_Data = rhs.m_Data;
    //            return *this;
    //        }

    //        T& get() const {
    //            return m_Data;
    //        }
    //        //const T& get() const {
    //        //    return m_Data;
    //        //}
            
    //        T&& move() {
    //            return std::move(m_Data);
    //        }

    //        const T& operator*() const {
    //            return m_Data;
    //        }
    //        T& operator*() {
    //            return m_Data;
    //        }

    //        operator T() const {
    //            return m_Data;
    //        }

    //    private:
    //        mutable T m_Data;
    //};
    
    struct dummy_mutex
    {
        void lock() {}
        bool try_lock() { return true; }
        void unlock() {}
    };
    
    template<class Mutex=std::mutex>
    struct optional_mutex
    {
        optional_mutex():
            mutex(std::make_shared<Mutex>())
        {}
        optional_mutex(const optional_mutex<Mutex>&) = default;
        optional_mutex(optional_mutex<Mutex>&&) = default;
        optional_mutex& operator=(const optional_mutex<Mutex>&) = default;
        optional_mutex& operator=(optional_mutex<Mutex>&&) = default;
        void lock() {
            if(mutex)
                mutex->lock();
        }
        bool try_lock() {
            if(mutex)
                return mutex->try_lock();
            return true;
        }
        void unlock() {
            if(mutex)
                mutex->unlock();
        }
        std::shared_ptr<Mutex> mutex;
    };
    
    template<class Mutex=std::mutex>
    class mutexed
    {
        public:

            mutexed() = default;

            // never copy mutex data
            mutexed(const mutexed&) {}

            template<class Lock=std::unique_lock<Mutex>>
            Lock lock() const {
                return Lock(m_Mutex);
            }
            template<class Lock=std::unique_lock<Mutex>, class Strategy>
            Lock lock(Strategy strategy) const {
                return Lock(m_Mutex, strategy);
            }

            //template<class Lock=std::unique_lock<Mutex>, class DeferStrategy>
            //std::optional<Lock> try_lock() {
            //}

            Mutex& mutex() const {
                return m_Mutex;
            }

        private:

            mutable Mutex m_Mutex;
    };

    /*
     * Lockless freezable concept
     * To enforce tread-safety on objects that start as mutable, but become
     * effectively immutable once they need to be thread safe
     * 
     * This is a one-way operation
     * Copies of frozen objects are unfrozen (maybe set behavior as policy?)
     *
     * Inherit as: virtual public freezable
     */
    class freezable
    {
        public:

            // copy/assign should not copy frozen state
            freezable() = default;
            freezable(freezable&&) {}
            freezable(const freezable&) {}
            freezable& operator=(const freezable&) {
                return *this;
            }
            freezable& operator=(freezable&&) {
                return *this;
            }
            virtual ~freezable() {}

            /*
             * Call freeze() when object should no longer be mutable
             */
            void freeze() {
                m_Frozen = true;
            }

            /*
             * use assert(!frozen()) in non-const accessors, mutator,
             *   move ctors, and unmutexed caches where necessary
             * Optionally use assert(frozen()) before thread usage
             */
            bool frozen() const {
                return m_Frozen;
            }

        private:
            
            std::atomic<bool> m_Frozen = ATOMIC_VAR_INIT(false);
    };

    /*
     * The above two class combined
     * Automatic freeze on lock() acquistions
     */
    template<class Mutex=std::mutex>
    class mutexed_freezable
    {
        public:
            void freeze() {
                m_Frozen=true;
            }
            bool frozen() const {
                return m_Frozen;
            }
            
            std::unique_lock<Mutex> lock() {
                //if(safety == kit::thread_safety::safe)
                //    freeze();

                //if(safety == kit::thread_safety::safe)
                    return std::unique_lock<Mutex>(m_Mutex);
                //return xstd::optional<std::unique_lock<Mutex>>();
            }

            Mutex& mutex() const {
                return m_Mutex;
            }

        private:
            bool m_Frozen=false;
            mutable Mutex m_Mutex;
    };

    template<class Classify, class... Args>
    class decision_tree {
        public:

            // empty trees throw on decide()
            decision_tree() = default;
            decision_tree(std::function<unsigned(Args...)> classify):
                m_Classification(classify)
            {}
            //virtual ~decision_tree() {}

            /*
             * may throw out_of_range or bad_function_call
             */
            Classify decide(Args... args) {
                Classify c = m_Classification(args...); // may throw
                if(m_Children.empty())
                    return c;
                return m_Children.at(c).decide(args...); // may throw
            }

            // TODO: need some way of iterating through each recusion above
            // if needed, allowing assumptions to be made during it...

            void push_back(const std::shared_ptr<
                decision_tree<Classify, Args...>
            >& node) {
                m_Children.push_back(node);
            }

            //void store(const T& data) {
            //    m_Data=data;
            //}

            //void clear_data() {
            //    //m_Data = xstd::optional<T>();
            //}

            //T& data() { return m_Data; }
            //const T& data() const { return m_Data; }

        private:
            std::function<Classify(Args...)> m_Classification;
            std::vector<std::shared_ptr<decision_tree<Classify, Args...>>> m_Children;
            //xstd::optional<T> m_Data;
    };

    template<
        class T,
        class Mutex=std::mutex
    >
    struct mutex_wrap:
        kit::mutexed<Mutex>
    {
        T data = T();

        mutex_wrap():
            data(T())
        {}
        mutex_wrap(T&& v):
            data(v)
        {}
        //mutex_wrap(const T& v):
        //    data(std::forward(v))
        //{}

        T get() {
            auto l = this->lock();
            return data;
        }

        friend bool operator==(const mutex_wrap& rhs, const T& lhs){
            auto l = rhs.lock();
            return rhs.data == lhs;
        }
        friend bool operator==(const T& rhs, const mutex_wrap& lhs){
            auto l = lhs.lock();
            return lhs.data == rhs;
        }
        
        //mutex_wrap& operator=(T&& rhs){
        //    auto l = this->lock();
        //    data = rhs;
        //    return *this;
        //}

        template<class R = void>
        R with(std::function<R(T&)> cb) {
            auto l = this->lock();
            return cb(data);
        }
        template<class R = void>
        R with(std::function<R(const T&)> cb) const {
            auto l = this->lock();
            return cb(data);
        }
        
        void set(T val){
            auto l = this->lock();
            data = val;
        }
    };

    /*
     * A generic map with unused key approximation
     */
    template<
        class T,
        class Mutex=kit::dummy_mutex
    >
    class index:
        public kit::mutexed<Mutex>
    {
        public:

            unsigned add(T&& data) {
                unsigned id = reserve();
                m_Group[id] = data;
                return id;
            }
            unsigned add(const T& data) {
                unsigned id = reserve();
                m_Group[id] = data;
                return id;
            }

            template<class... Args>
            unsigned emplace_hint(unsigned hint, Args&&... args) {
                m_Group[hint] = T(std::forward<Args>(args)...);
                return hint;
            }
            template<class... Args>
            unsigned emplace(Args&&... args) {
                unsigned id = reserve();
                m_Group[id] = T(std::forward<Args>(args)...);
                return id;
            }
            bool erase(unsigned id) {
                bool e = m_Group.erase(id);
                m_Unused = std::min(id, m_Unused);
                return e;
            }
            void clear() {
                m_Unused=0;
                m_Group.clear();
            }
            T& at(unsigned idx) {
                return m_Group.at(idx);
            }
            const T& at(unsigned idx) const {
                return m_Group.at(idx);
            }
            unsigned reserve() {
                while(m_Group.find(m_Unused++) != m_Group.end()) {}
                return m_Unused-1;
            }
            
            // WARNING: obtain lock before iterating
            typedef typename std::map<unsigned, T>::const_iterator
                const_iterator;
            typedef typename std::map<unsigned, T>::iterator
                iterator;
            iterator begin() { return m_Group.begin(); }
            iterator end() { return m_Group.end(); }
            const_iterator begin() const { return m_Group.begin(); }
            const_iterator end() const { return m_Group.end(); }
            const_iterator cbegin() const { return m_Group.begin(); }
            const_iterator cend() const { return m_Group.end(); }

        private:
            unsigned m_Unused=0;
            std::map<unsigned, T> m_Group;
    };

    /*
     * A generic map with reusable uint keys and faster insertion
     * Values are wrapped in shared_ptrs to allow discarding unshared data
     *   by calling optimize()
     */
    template<
        class T,
        class Mutex=kit::dummy_mutex
    >
    class shared_index:
        public kit::mutexed<Mutex>
    {
        public:
            template<class... Args>
            unsigned emplace_hint(unsigned hint, Args&&... args) {
                auto l = this->lock();
                m_Group[hint] = std::make_shared<T>(
                    std::forward<Args>(args)...
                );
                return hint;
            }
            template<class... Args>
            unsigned emplace(Args&&... args) {
                auto l = this->lock();
                unsigned id = next();
                m_Group[id] = std::make_shared<T>(
                    std::forward<Args>(args)...
                );
                return id;
            }
            bool erase(unsigned id) {
                auto l = this->lock();
                bool e = m_Group.erase(id);
                m_Unused = std::min(id, m_Unused);
                return e;
            }

            unsigned add(std::shared_ptr<T> val) {
                auto l = this->lock();
                unsigned id = next();
                m_Group[id] = val;
                return id;
            }
            
            bool add(unsigned id, std::shared_ptr<T> val) {
                auto l = this->lock();
                m_Group[id] = val;
                return true;
            }
            
            unsigned remove_if(std::function<bool(const std::shared_ptr<T>&)> func)
            {
                auto l = this->lock();
                unsigned count = 0;
                for(auto itr = m_Group.begin();
                    itr != m_Group.end();
                ){
                    if(func(itr->second)){
                        itr = m_Group.erase(itr);
                        ++count;
                    }
                    else
                        ++itr;
                }
                return count;
            }
            void clear(unsigned idx) {
                auto l = this->lock();
                try{
                    m_Reserved.erase(idx);
                }catch(const std::out_of_range&){}
                try{
                    erase(idx);
                }catch(const std::out_of_range&){}
            }
            void clear() {
                auto l = this->lock();
                m_Unused=0;
                m_Group.clear();
            }
            std::shared_ptr<T>& at(unsigned idx) {
                auto l = this->lock();
                return m_Group.at(idx);
            }
            const std::shared_ptr<T>& at(unsigned idx) const {
                auto l = this->lock();
                return m_Group.at(idx);
            }

            void optimize() {
                auto l = this->lock();
                remove_if([](const std::shared_ptr<T>& i){
                    return !i || i.unique();
                });
            }
            unsigned next() {
                auto l = this->lock();
                while(true){
                    if(m_Reserved.find(m_Unused) == m_Reserved.end() &&
                        m_Group.find(m_Unused) == m_Group.end()
                    ){
                        ++m_Unused;
                        break;
                    }
                    ++m_Unused;
                }
                return m_Unused-1;
            }
            
            unsigned reserve() {
                auto l = this->lock();
                return next();
            } // DEPRECATED
            
            void reserve(unsigned idx){
                auto l = this->lock();
                m_Reserved.insert(idx);
            }
            unsigned reserve_next() {
                auto l = this->lock();
                unsigned id = next();
                reserve(id);
                return id;
            }
            bool is_reserved(unsigned idx) const {
                auto l = this->lock();
                return m_Reserved.find(idx) != m_Reserved.end();
            }

            bool has(unsigned idx) {
                auto l = this->lock();
                return m_Group.find(idx) != m_Group.end();
            }
            
            // WARNING: obtain lock before iterating
            typedef typename std::map<unsigned, std::shared_ptr<T>>::const_iterator
                const_iterator;
            typedef typename std::map<unsigned, std::shared_ptr<T>>::iterator
                iterator;
            iterator begin() { return m_Group.begin(); }
            iterator end() { return m_Group.end(); }
            const_iterator begin() const { return m_Group.begin(); }
            const_iterator end() const { return m_Group.end(); }
            const_iterator cbegin() const { return m_Group.begin(); }
            const_iterator cend() const { return m_Group.end(); }

        private:
            unsigned m_Unused=0;
            std::map<unsigned, std::shared_ptr<T>> m_Group;
            std::set<unsigned> m_Reserved;
    };

    /*
     * A bimap (id <-> string) with unused ID approximation
     *
     * Used for when you need both "stringly" types and speed
     *
     * Would be nice to be able to "hook" values in here? (prevent IDs from being overridden with new data)
     */
    class string_index
    {
        public:
            //virtual ~string_index() {}
            std::string at(unsigned id) const {
                return m_Index.left.at(id);
            }
            unsigned at(const std::string& id) const {
                return m_Index.right.at(id);
            }
            unsigned ensure(std::string s) {
                unsigned idx;
                try{
                    idx = m_Index.right.at(s);
                }catch(const std::out_of_range&){
                    idx = store(s);
                }
                return idx;
            }
            unsigned store(std::string s) {
                unsigned idx = reserve();
                m_Index.insert(boost::bimap<unsigned,std::string>::value_type(
                    idx,
                    s
                ));
                return idx;
            }
            void store(std::vector<std::string> v) {
                for(auto&& e: v)
                    store(e);
            }
            unsigned reserve() {
                while(m_Index.left.find(m_Unused++) != m_Index.left.end()) {}
                return m_Unused-1;
            }


            // TODO: extract strings from m_Index

            //void remove(unsigned id) {
            //}
            //void remove(const std::string& s) {
            //}

            typedef boost::bimap<
                unsigned, std::string
            >::left_map::const_iterator const_iterator;
            const_iterator begin() const { return m_Index.left.begin(); }
            const_iterator end() const { return m_Index.left.end(); }

        private:
            boost::bimap<unsigned, std::string> m_Index;
            unsigned m_Unused = 0;
    };

    /*
     * Allow mutable data to become const once for thread-safety reasons
     * TODO: allow for copy-on-write to make mutable again
     */
    //class freezable
    //{
    //    public:
    //        void freeze() {
    //            m_Frozen = true;
    //        }
    //        bool frozen() const {
    //            return m_Frozen;
    //        }
    //    private;
    //        bool m_Frozen=false;
    //};

    // TODO: would be nice to have a cache here

    //template<class T, class Container>
    //T peel_front(boost::any& c) {
    //    Container ab = boost::any_cast<Container>(c);
    //    if(ab.empty())
    //        return T();

    //    auto a = ab.front();
    //    c.pop_front();

    //    return a;
    //}

    constexpr uint32_t bit(uint32_t x) {
        return 1<<x;
    }
    constexpr uint32_t mask(uint32_t x) {
        return (1<<x) - 1;
    }

    inline int round_int(double r) {
        return (int)( (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5) );
    }
    inline int round_int(float r){
        return (int)( (r > 0.0f) ? floor(r + 0.5f) : ceil(r - 0.5f) );
    }
    inline unsigned int round_uint(double r){
        return (int)( (r > 0.0f) ? floor(r + 0.5f) : ceil(r - 0.5f) );
    }
    inline unsigned int round_uint(float r){
        return (int)( (r > 0.0f) ? floor(r + 0.5f) : ceil(r - 0.5f) );
    }

    template<class T>
    bool ready(std::future<T>& fut)
    {
        return fut.wait_for(std::chrono::seconds(0)) ==
            std::future_status::ready;
    }
    
    template<class Container, class Object>
    bool has(const Container& c, const Object& o) {
        return std::find(ENTIRE(c), o) != c.end();
    }
    
    template<class Container, class Pred>
    void remove_if(Container& c, Pred&& p) {
        c.erase(std::remove_if(ENTIRE(c), p), c.end());
    }

    template<class Container, class Object>
    void remove(Container& c, const Object& o)
    {
        c.erase(std::remove(ENTIRE(c), o), c.end());
    }

    template<class Container>
    Container slice(const Container& c, int start, int end, int step = 1)
    {
        if(start < 0)
            start = std::max<int>(0,c.size() + start);
        else
            start = std::min<int>(c.size(), start);
        
        if(end < 0)
            end = std::max<int>(0, c.size() + end);
        else
            end = std::min<int>(c.size(), end);
        
        if(end <= start)
            return Container();
        
        auto r = Container(
            c.begin() + start,
            c.begin() + end
        );
        
        if(step != 1){
            int i = step;
            kit::remove_if(r, [&i,step](const typename Container::value_type& v){
                bool b = (i % step);
                ++i;
                return b;
            });
        }
        return r;
    }
    
    template<class Container>
    Container slice(const Container& c, int start)
    {
        return slice(c, start, c.size());
    }

    template<class Container>
    Container idx(const Container& c, int start)
    {
        if(start < 0)
            c.at(c.size() + start);
        return c.at(start);
    }

    
    template<class T, class ...Args>
    std::future<T> make_future(Args&&... args)
    {
        std::promise<T> tmp;
        tmp.set_value(std::forward<Args>(args)...);
        return tmp.get_future();
    }

    template<class T, class... Args>
    std::unique_ptr<T> make_unique(Args&&... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
    template<class T, class... Args>
    std::unique_ptr<T> init_unique(Args&&... args)
    {
        auto p = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        p->init();
        return p;
    }
    template<class T, class... Args>
    std::shared_ptr<T> init_shared(Args&&... args)
    {
        auto p = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
        p->init();
        return p;
    }
    template<class T, class... Args>
    std::unique_ptr<T>& make(std::unique_ptr<T>& p, Args&&... args)
    {
        p = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        return p;
    }
    template<class T, class... Args>
    std::shared_ptr<T>& make(std::shared_ptr<T>& p, Args&&... args)
    {
        p = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
        return p;
    }
    template<class T, class... Args>
    std::unique_ptr<T>& init(std::unique_ptr<T>& p, Args&&... args)
    {
        try {
            p = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
            p->init();
        }catch(...){
            p = std::unique_ptr<T>();
            throw;
        }
        return p;
    }
    template<class T, class... Args>
    std::shared_ptr<T>& init(std::shared_ptr<T>& p, Args&&... args)
    {
        try {
            p = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
            p->init();
        }catch(...){
            p = std::shared_ptr<T>();
            throw;
        }
        return p;
    }

    inline std::vector<std::string> lines(const std::string& s)
    {
        std::vector<std::string> v;
        return boost::algorithm::split(v, s, boost::is_any_of("\r\n"));
    }
    
    template<class T>
    boost::tribool any_eq_type(const boost::any& a, const boost::any& b) {
        if (a.type() == typeid(T) && b.type() == typeid(T))
            return boost::any_cast<T>(a) ==
                boost::any_cast<T>(b);
        return boost::tribool::indeterminate_value;
    }

    inline bool any_eq(const boost::any& a, const boost::any& b) {
        if(a.type() != b.type())
            return false;
        {auto r = any_eq_type<std::string>(a,b); if(r||!r) return r;}
        {auto r = any_eq_type<int>(a,b); if(r||!r) return r;}
        {auto r = any_eq_type<unsigned>(a,b); if(r||!r) return r;}
        {auto r = any_eq_type<float>(a,b); if(r||!r) return r;}
        {auto r = any_eq_type<double>(a,b); if(r||!r) return r;}
        {auto r = any_eq_type<bool>(a,b); if(r||!r) return r;}

        throw std::runtime_error("unable to compare values");
    }
    
    inline void ensure_starts_with(std::string& s, std::string t){
        if(not boost::starts_with(s,t))
            s=t+s;
    }
    inline void ensure_ends_with(std::string& s, std::string t){
        if(not boost::ends_with(s,t))
            s+=t;
    }
    
    //std::string lines(const std::vector<std::string>& s, std::string sep="")
    //{
    //    return boost::algorithm::join(s, "\n");
    //}

    class yield_exception:
        public std::runtime_error
    {
        public:
            yield_exception():
                std::runtime_error("blocking operation requested yield")
            {}
            virtual ~yield_exception() throw() {}
    };

    class timeout:
        public std::runtime_error
    {
        public:
            timeout():
                std::runtime_error("timeout")
            {}
            virtual ~timeout() throw() {}
    };
    
    class interrupt:
        public std::runtime_error
    {
        public:
            interrupt():
                std::runtime_error("interrupt")
            {}
            virtual ~interrupt() throw() {}
    };

    class null_ptr_exception:
        public std::runtime_error
    {
        public:
            null_ptr_exception():
                std::runtime_error("null pointer exception")
            {}
            virtual ~null_ptr_exception() throw() {}
    };

    template<class T>
    T safe_ptr(T ptr)
    {
        if(ptr == nullptr){
            throw null_ptr_exception();
        }
        return ptr;
    }

    // Be stupidly careful with this class
    // The mutex is only stored as a pointer, so make sure
    // it's guarenteed to exist throughout
    template<class Lockable>
    class scoped_unlock
    {
        public:
            scoped_unlock() = delete;
            explicit scoped_unlock(Lockable* l):
                m_Lock(l)
            {
                assert(l);
                m_Lock->unlock();
            }
            scoped_unlock<Lockable>& operator=(const scoped_unlock<Lockable>& rhs) = default;
            scoped_unlock<Lockable>& operator=(scoped_unlock<Lockable>&& rhs) = default;
            scoped_unlock(scoped_unlock<Lockable>&& rhs) {
                m_Lock = rhs.m_Lock;
                rhs.resolve();
            }
            ~scoped_unlock() {
                if(m_Lock)
                    m_Lock->lock();
            }
            void resolve() {
                m_Lock = nullptr;
            }
        private:
            Lockable* m_Lock;
    };

    // Warning: double free or open crash?
    template <class T>
    class scoped_dtor {
        private:
            T* m_object;
        public:
            scoped_dtor(T* o):
                m_object(o)
            {}

            ~scoped_dtor(){
                call();
            }

            void call() {
                if(m_object)
                    m_object->~T();
            }

            void resolve() {
                m_object = nullptr;
            }

            void now() {
                call();
                resolve();
            }
    };

    //template<class Func>
    //class scope_exit {
    //    public:
    //        scope_exit(Func func):
    //            m_Func(std::move(func))
    //        {}
    //        scope_exit() = delete;
    //        scope_exit(const scope_exit&) = delete;
    //        scope_exit& operator=(const scope_exit&) = delete;
    //        scope_exit(scope_exit&& o):
    //            m_Func(std::move(o.m_Func)),
    //            m_Active(std::move(o.m_Active))
    //        {
    //            o.resolve();
    //        }
    //        ~scope_exit() {
    //            if(m_Active)
    //                m_Func();
    //        }
    //        void resolve() {
    //            m_Active=false;
    //        }
    //    private:
    //        bool m_Active=true;
    //        Func m_Func;
    //};
    //template<class Func>
    //scope_exit<Func> on_scope_exit(Func func)
    //{
    //    return scope_exit<Func>(std::move(func));
    //}

    template <class Time>
    bool retry(int count, Time delay, std::function<bool()> func)
    {
        for(int i=0; i<count || count<0; count<0 || ++i)
        {
            if(func())
                return true;
            std::this_thread::sleep_for(delay);
        }
        return false;
    }

    // only thread-safe in c++11
    template<class T>
    class singleton
    {
        public:
            static T& get()
            {
                static T instance;
                return instance;
            }
    };

    template<typename T>
    struct push_back_values
    {
        typedef T result_type;
        template<class Iterator>
        T operator()(Iterator first, Iterator last) const {
            T r;
            while(first != last) {
                r.push_back(*first);
                ++first;
            }
            return r;
        }
    };

    template<class Function>
    bool threw(Function f)
    {
        try{
            f();
        }catch(...){
            return true;
        }
        return false;
    }

    template<class T>
    T clamp(T val, T minval, T maxval)
    {
        if(val < minval)
            return minval;
        if(val > maxval)
            return maxval;
        return val;
    }
    
    template<class T>
    T saturate(T val)
    {
        return clamp(val, T(0.0), T(1.0));
    }

    inline int sign(int v)
    {
        if(v > 0)
            return 1;
        else if(v < 0)
            return -1;
        return 0;
    }

    template<class T>
    T mix(T a, T b, float t) {
        return a + (b-a)*t;
    }
    
    #define TRY(expr) try{ expr; } catch(...) {}
    #define TRY_OR(expr, alt) [&]{try{ return (expr); } catch(...) {return (alt);}}()
    #define IF_OR(expr, alt) [&]{try{ auto r = (expr);if(r)return r; } catch(...) {}return (alt);}()

    //template<class T>
    //class local_singleton
    //{
    //    public:
    //        static T& get()
    //        {
    //            thread_local T instance;
    //            return instance;
    //        }
    //};

    #define K_GET(TYPE, NAME, MEMBER) \
        TYPE NAME() const { return MEMBER; }
    #define K_SET(TYPE, NAME, MEMBER) \
        void NAME(TYPE val) { MEMBER = val; }
    #define K_GET_SET(TYPE, NAME, MEMBER) \
        TYPE NAME() const { return MEMBER; } \
        void NAME(TYPE val) { MEMBER = val; }
    #define K_GETR(TYPE, NAME, MEMBER) \
        TYPE& NAME() { return MEMBER; } \
        const TYPE& NAME() const { return MEMBER; }
    #define K_CGETR(TYPE, NAME, MEMBER) \
        const TYPE& NAME() const { return MEMBER; }
    #define K_GETR_SET(TYPE, NAME, MEMBER) \
        K_GETR(TYPE, NAME, MEMBER) \
        K_SET(TYPE, NAME, MEMBER)
    #define K_CGETR_SET(TYPE, NAME, MEMBER) \
        K_CGETR(TYPE, NAME, MEMBER) \
        K_SET(TYPE, NAME, MEMBER)
    #define K_S_GET_SET(TYPE, NAME, MEMBER) \
        static TYPE NAME() { return MEMBER; } \
        static void NAME(TYPE val) { MEMBER = val; }
}

#endif

