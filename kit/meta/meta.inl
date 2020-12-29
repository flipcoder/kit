#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <string>
#include "meta.h"
#include "../log/log.h"
#include "../kit.h"

template<class Mutex, template <class> class Ptr, template <typename> class This>
Meta_<Mutex,Ptr,This> :: Meta_(const std::string& fn, unsigned flags):
    m_Filename(fn)
{
    deserialize(flags);
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
Meta_<Mutex,Ptr,This> :: Meta_(const Ptr<Meta_<Mutex,Ptr,This>>& rhs)
{
    //clear();
    //auto tmp = make_shared<Meta>();
    //tmp->merge(rhs);
    merge(rhs);
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: from_args(std::vector<std::string> args)
{
    // remove whitespace args
    kit::remove_if(args, [](const std::string& s) {
        return (s.find_first_not_of(" \n\r\t") == std::string::npos);
    });
    std::map<std::string, std::string> values;

    // parse key-values
    for(auto&& arg: args)
    {
        if(boost::starts_with(arg, "--") && arg.length()>2)
        {
            kit::remove(args, arg);
            size_t idx = arg.find('=');
            if(idx != std::string::npos)
            {
                if(idx != arg.rfind('='))
                    throw std::exception(); // more than one '='
                std::string key = arg.substr(2, idx-2);
                if(values.find(key) != values.end())
                    throw std::exception(); // duplicate
                std::string value = arg.substr(idx+1);
                if(!value.empty())
                    if(!values.insert({key,value}).second)
                        throw std::exception(); // failed to insert
            }
        }
    }
    
    // populate meta with lone elements and key-values
    for(auto&& arg: args)
        add(arg);
    for(auto&& val: values) 
        set(val.first, val.second);
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
MetaLoop Meta_<Mutex,Ptr,This>::each(
    std::function<MetaLoop(
        //Ptr<Meta_<Mutex,Ptr,This>>,
        Ptr<Meta_<Mutex,Ptr,This>>,
        MetaElement&,
        unsigned
    )> func,
    unsigned flags, // use EachFlag enum
    std::deque<std::tuple<
        Ptr<Meta_<Mutex,Ptr,This>>,
        std::unique_lock<Mutex>,
        std::string // key in parent
    >>* metastack,
    unsigned level,
    std::string key
){
    auto l = this->lock();

    // only keep one instance of this, will be null if metastack isn't
    auto spthis = this->shared_from_this();
    //Ptr<Meta_<Mutex,Ptr,This>> spthis_ = this->shared_from_this();
    //Ptr<Meta_<Mutex,Ptr,This>>* spthis = &spthis_;
    if(metastack) {
        metastack->push_back(std::make_tuple(
            spthis, std::move(l), std::move(key)
        ));
        //spthis = &std::get<0>(metastack->back());
    }

    BOOST_SCOPE_EXIT_TPL((&metastack))
    {
        if(metastack) 
            metastack->pop_back();
    } BOOST_SCOPE_EXIT_END

    //BOOST_SCOPE_EXIT_ALL(&metastack) {
    //    if(metastack) 
    //        metastack->pop_back();
    //};

    MetaLoop status = MetaLoop::STEP;
    bool repeat;
    bool break_outer;

    for(auto&& e: elements_ref()) // outer
    {
        break_outer = false;

        do{ // inner
            repeat = false;

            if(!(flags & POSTORDER))
            {
                status = func(spthis, e, level+1);
                if(status == MetaLoop::BREAK) {
                    break_outer = true;
                    break;
                }
                if(status == MetaLoop::REPEAT) {
                    repeat = true;
                    continue;
                }
                if(status == MetaLoop::CONTINUE) {
                    // skip subtree recursion
                    continue;
                }
            }

            // reset status just in case we don't enter this block (*)
            status = MetaLoop::STEP;
            if((flags & RECURSIVE) &&
                e.type.id == MetaType::ID::META
            ){
                // TODO: catch flags from each() call here
                //
                // NOTE: keep the metastack pointer as the exclusive
                // Ptr during the each() call
                // so dumping the stack allows other threads
                // to erase the pointer)
                auto nextsp = boost::any_cast<Ptr<Meta_<Mutex,Ptr,This>>>(e.value);
                status = nextsp->each(
                    func,
                    flags,
                    metastack,
                    level+1,
                    e.key
                );

                // Allows for user to pop items off the BOTTOM of
                // the stack if another thread wants in
                // This invalidates iterators, so we have to break if
                // this is the lowest valid level
                if(metastack && level>0 && metastack->size() == 1) {
                    break_outer = true;
                    break;
                }
                if(status == MetaLoop::BREAK) {
                    // metastack might have been modified
                    break_outer = true;
                    break;
                }
                if(status == MetaLoop::REPEAT) {
                    repeat = true;
                    continue;
                }

                //lc = loop_controller();
                //if(lc == LoopController::BREAK)
                //    break;
            }

            // the CONTINUE check here is why we reset status above (*)
            if(flags & POSTORDER
                && status != MetaLoop::CONTINUE // warning: untested
            ){
                status = func(spthis, e, level+1);
                if(status == MetaLoop::BREAK) {
                    break_outer = true;
                    break;
                }
                if(status == MetaLoop::REPEAT) {
                    repeat = true;
                    continue;
                }

            }

        }while(repeat);

        if(break_outer)
        {
            // dismiss the scope guard
            metastack = nullptr;
            return MetaLoop::BREAK;
        }
    }

    return MetaLoop::STEP; // EachResult flags here
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
MetaLoop Meta_<Mutex,Ptr,This>::each_c(
    std::function<MetaLoop(
        //Ptr<const Meta_<Mutex,Ptr,This>>,
        Ptr<const Meta_<Mutex,Ptr,This>>,
        const MetaElement&,
        unsigned
    )> func,
    unsigned flags, // EachFlag enum
    std::deque<std::tuple<
        Ptr<const Meta_<Mutex,Ptr,This>>,
        std::unique_lock<Mutex>,
        std::string // key in parent (blank if root or empty key)
    >>* metastack,
    unsigned level,
    std::string key
) const {
    
    auto l = this->lock();
    
    auto spthis = this->shared_from_this();
    //const Ptr<Meta_<Mutex,Ptr,This>>* spthis = &spthis_;

    if(metastack) {
        metastack->push_back(std::make_tuple(
            spthis, std::move(l), std::move(key)
        ));
    }

    BOOST_SCOPE_EXIT_TPL((&metastack))
    {
        if(metastack) 
            metastack->pop_back();
    } BOOST_SCOPE_EXIT_END

    MetaLoop status = MetaLoop::STEP;
    bool repeat;
    bool break_outer;

    for(auto&& e: elements_ref()) // outer
    {
        break_outer = false;

        do{ // inner
            repeat = false;

            if(!(flags & POSTORDER))
            {
                status = func(spthis, e, level+1);
                if(status == MetaLoop::BREAK) {
                    break_outer = true;
                    break;
                }
                if(status == MetaLoop::REPEAT) {
                    repeat = true;
                    continue;
                }
                if(status == MetaLoop::CONTINUE) {
                    // skip subtree recursion
                    continue;
                }
            }

            // reset status just in case we don't enter this block (*)
            status = MetaLoop::STEP;
            if((flags & RECURSIVE) &&
                e.type.id == MetaType::ID::META
            ){
                auto next = boost::any_cast<Ptr<Meta_<Mutex,Ptr,This>>>(e.value).get();
                status = next->each(
                    func,
                    flags,
                    metastack,
                    level+1,
                    e.key
                );

                if(metastack && level>0 && metastack->size() == 1) {
                    break_outer = true;
                    break;
                }
                if(status == MetaLoop::BREAK) {
                    // metastack might have been modified
                    break_outer = true;
                    break;
                }
                if(status == MetaLoop::REPEAT) {
                    repeat = true;
                    continue;
                }

                //lc = loop_controller();
                //if(lc == LoopController::BREAK)
                //    break;
            }

            // the CONTINUE check here is why we reset status above (*)
            if(flags & POSTORDER
                && status != MetaLoop::CONTINUE // warning: untested
            ){
                status = func(spthis, e, level+1);
                if(status == MetaLoop::BREAK) {
                    break_outer = true;
                    break;
                }
                if(status == MetaLoop::REPEAT) {
                    repeat = true;
                    continue;
                }

            }

        }while(repeat);

        if(break_outer)
        {
            // dismiss the scope guard
            metastack = nullptr;
            return MetaLoop::BREAK;
        }
    }

    return MetaLoop::STEP; // EachResult flags here
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
MetaLoop Meta_<Mutex,Ptr,This>::each(
    std::function<MetaLoop(
        //Ptr<const Meta_<Mutex,Ptr,This>>,
        Ptr<const Meta_<Mutex,Ptr,This>>,
        const MetaElement&,
        unsigned
    )> func,
    unsigned flags, // EachFlag enum
    std::deque<std::tuple<
        Ptr<const Meta_<Mutex,Ptr,This>>,
        std::unique_lock<Mutex>,
        std::string // key in parent (blank if root or empty key)
    >>* metastack,
    unsigned level,
    std::string key
) const {
    return each_c(func,flags,metastack,level,key);
}

//  use type checks with typeid() before attempting conversion
//   or boost::any cast?
//  POD types should also work if behind smart ptrs -- not yet impl
//  only Ptr's to other trees should serialize (not weak)

/*
 * Will recurse into nested Meta_s
 * Might throw bad_any_cast
 */
template<class Mutex, template <class> class Ptr, template <typename> class This>
Json::Value MetaElement::serialize_json(
    unsigned flags
) const {

    Json::Value v;

    if((flags & SKIP_SERIALIZE))
        throw SkipSerialize();
        
    if(type.id == MetaType::ID::META)
    {
        boost::any_cast<Ptr<Meta_<Mutex,Ptr,This>>>(value)->serialize_json(v);
    }
    // TODO: boost::any -> value (and key?)
    else if(type.id == MetaType::ID::INT) {
        //if(type.storage == Type::Ptr::STACK) {
            //if(type.flags & MetaType::Flag::SIGN)
                v = Json::Int(boost::any_cast<int>(value));
            //else
            //    v = Json::Int(boost::any_cast<unsigned>(value));
        //} else if(type.storage == MetaType::Ptr::SHARED) {
        //    v = Json::Int(
        //        *kit::safe_ptr(boost::any_cast<Ptr<int>>(value))
        //    );
        //}
    }
    else if(type.id == MetaType::ID::REAL)
        v = Json::Value(boost::any_cast<double>(value));
    else if(type.id == MetaType::ID::STRING)
        v = Json::Value(boost::any_cast<std::string>(value));
    else if(type.id == MetaType::ID::BOOL)
        v = Json::Value(boost::any_cast<bool>(value));
    else if(type.id == MetaType::ID::EMPTY)
        v = Json::Value();

    // if meta...
    // {
    //     Json::Value v;
    //     meta->serialize(v)
    //     return v;
    // }

    // This is the best way to deal with an array that contains
    // key-values but isn't purely a map
    // This allows us to preserve ordering, and merge-on-deserialize
    if((flags & (unsigned)MetaSerialize::STORE_KEY) && !key.empty())
    {
        Json::Value tup;
        //tup.append(key);
        //tup.append(v);
        tup[key] = v;
        return tup;
    }
    return v;
}

/*
 * Private method JSON serialization
 */
template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: serialize_json(Json::Value& v) const
{
    if(is_map())
    {
        v = Json::Value(Json::objectValue);

        for(auto&& e: m_Elements)
        {
            try{
                v[e.key] = e.template serialize_json<Mutex,Ptr,This>();
            }catch(const MetaElement::SkipSerialize&){
            }catch(...){
                WARNING("Unable to serialize element");
            }
        }
    }
    else
    {
        v = Json::Value(Json::arrayValue);

        Json::Value keys;
        for(auto&& e: m_Elements)
        {
            try{
                //if(e.key.empty())
                v.append(e.template serialize_json<Mutex,Ptr,This>((unsigned)MetaSerialize::STORE_KEY));
                //else
                //    keys[e.key] = e.serialize_json();
            }catch(const MetaElement::SkipSerialize&){
            }catch(...){
                WARNING("Unable to serialize element");
            }
        }
        
        //if(!keys.isNull())
        //    v.append(keys);
    }
}

/*
 * Private method JSON deserialization
 */
template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: deserialize_json(
    const Json::Value& v
){
    //unsigned flags = empty() ? 0 : AddFlag::UNIQUE;
    //unsigned flags = 0;

    for(auto i = v.begin();
        i != v.end();
        ++i)
    {
        auto&& e = *i;

        std::string k;
        if(not i.name().empty())
            k = i.name();

        // TODO: move this stuff to Element::deserialize_json()
        if(e.isArray() || e.isObject())
        {
            auto nested = kit::make<Ptr<Meta_<Mutex,Ptr,This>>>();
            nested->deserialize_json(e);
            set(k, nested);
            continue;
        }
        
        // blank keys will be added as array items, so just use set<>()

        if(e.isNull())
            set<std::nullptr_t>(k, nullptr);
        //if(e.isInt())
        else if(e.type() == Json::intValue)
            set<int>(k, e.asInt());
        //else if(e.isUInt())
        //    set<unsigned>(k, e.asUInt());
        else if(e.type() == Json::stringValue)
        //else if(e.isString())
            set<std::string>(k, e.asString());
        else if(e.type() == Json::booleanValue)
        //else if(e.isBool())
            set<bool>(k, e.asBool());
        else if(e.type() == Json::realValue)
        //else if(e.isDouble())
            set<double>(k, e.asDouble());
        else
            WARNINGf("unparsable value at key = %s", k);
    }
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
std::string Meta_<Mutex,Ptr,This> :: serialize(MetaFormat fmt, unsigned flags) const
{
    auto l = this->lock();

    std::string data;
    if(fmt == MetaFormat::JSON)
    {
        Json::Value root;
        serialize_json(root);
        if(flags & (unsigned)MetaSerialize::MINIMIZE)
        {
            Json::StreamWriterBuilder builder;
            auto writer = std::unique_ptr<Json::StreamWriter>(builder.newStreamWriter());
            std::ostringstream ss;
            writer->write(root, &ss);
            return ss.str();
        }
        else
            return root.toStyledString();
    }
    else if (fmt == MetaFormat::INI)
    {
        for(auto&& e: m_Elements)
        {
            Ptr<Meta_<Mutex,Ptr,This>> m;
            try{
                m = boost::any_cast<Ptr<Meta_<Mutex,Ptr,This>>>(e.value);
            }catch(boost::bad_any_cast&){}
            if(m)
            {
                data += "["+e.key+"]\n";
                for(auto&& j: *m){
                    // TODO: write sub category [foo.bar]
                    //if(e.type.id==MetaType::ID::META)
                    //    data += j.key + "=" + boost::any_cast<Ptr<Meta_<Mutex,Ptr,This>>>(j.value) + "\n";
                    if(j.type.id==MetaType::ID::INT)
                        data += j.key + "=" + std::to_string(boost::any_cast<int>(j.value)) + "\n";
                    else if(j.type.id==MetaType::ID::REAL)
                        data += j.key + "=" + std::to_string(boost::any_cast<double>(j.value)) + "\n";
                    else if(j.type.id==MetaType::ID::BOOL)
                        data += j.key + "=" + (boost::any_cast<bool>(j.value)?"true":"false") + "\n";
                    else if(j.type.id==MetaType::ID::STRING)
                        data += j.key + "=" + boost::any_cast<std::string>(j.value) + "\n";
                    else{
                        //LOG(std::to_string(int(j.type.id)));
                        WARNING("warning: cannot serialize value");
                    }
                }
                data += "\n";
            }
            else
            {
                //data += e.key + "=" + kit::any_to_string(e.value) + "\n";
                if(e.type.id==MetaType::ID::INT)
                    data += e.key + "=" + std::to_string(boost::any_cast<int>(e.value)) + "\n";
                else if(e.type.id==MetaType::ID::REAL)
                    data += e.key + "=" + std::to_string(boost::any_cast<double>(e.value)) + "\n";
                else if(e.type.id==MetaType::ID::BOOL)
                    data += e.key + "=" + (boost::any_cast<bool>(e.value)?"true":"false") + "\n";
                else if(e.type.id==MetaType::ID::STRING)
                    data += e.key + "=" + boost::any_cast<std::string>(e.value) + "\n";
                else
                    WARNING("warning: cannot serialize value");
            }
        }
    }
    //else if (fmt == MetaFormat::YAML)
    //    assert(false);
    else
        assert(false);

    return data;
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
Meta_<Mutex,Ptr,This> :: Meta_(MetaFormat fmt, const std::string& data, unsigned flags)
{
    deserialize(
        fmt,
        data,
        std::string(), // fn
        std::string(), // pth
        flags
    );
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: deserialize(
    MetaFormat fmt,
    const std::string& data,
    unsigned flags
){
    deserialize(
        fmt,
        data,
        std::string(), // fn
        std::string(), // pth
        flags
    );
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: deserialize(MetaFormat fmt, const std::string& data, const std::string& fn, const std::string& pth, unsigned flags)
{ 
    std::istringstream iss(data);
    deserialize(fmt, iss, fn, pth, flags);
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: deserialize_json(
    const Json::Value& v,
    const std::vector<std::string>& pth
){
    //if(pth.empty())
    //{
    //    deserialize_json(v);
    //    return;
    //}
    
    for(auto i = v.begin();
        i != v.end();
        ++i)
    {
        auto&& e = *i;

        std::string k;
        if(not i.name().empty())
            k = i.name();

        if(/*e.isArray() || */e.isObject())
        {
            if(pth.size() == 1)
            {
                deserialize_json(e);
                return;
            }
            if(not pth.empty() && *pth.begin() == k)
            {
                std::vector<std::string> cur_pth(pth.begin()+1,pth.end());
                deserialize_json(e, cur_pth);
                return;
            }
        }
    }
    K_ERROR(READ, "subpath");
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: deserialize(MetaFormat fmt, std::istream& data, const std::string& fn, const std::string& pth, unsigned flags)
{
    auto l = this->lock();

    if(not (flags & Meta::F_MERGE))
        clear();
    
    if(fmt == MetaFormat::JSON) 
    {
        Json::Value root;
        Json::CharReaderBuilder builder;

        //if(!reader.parse(data, root)) {
        JSONCPP_STRING errs;
        if(!Json::parseFromStream(builder, data, &root, &errs)) {
            if(fn.empty()) {
                K_ERROR(PARSE, "Meta input stream data")
            } else {
                K_ERROR(PARSE, fn);
            }
        }
        
        if(pth.empty())
            deserialize_json(root);
        else
        {
            //std::vector<std::string> pth_vec;
            //boost::split(pth, pth_vec, boost::is_any_of("."));
            //deserialize_json(root, pth_vec);
        }
    }
    else if (fmt == MetaFormat::YAML)
    {
        assert(false);
    }
    else if (fmt == MetaFormat::INI)
    {
        try{
            Meta_<Mutex,Ptr,This>* m = this;
            std::string line;
            while(std::getline(data, line))
            {
                if(boost::starts_with(line, "["))
                {
                    auto m2 = kit::make<Ptr<Meta_<Mutex,Ptr,This>>>();
                    set(line.substr(1, line.length()-2), m2);
                    m = m2.get();
                }
                else if(not boost::starts_with(line,";") && line.length() > 1)
                {
                    std::vector<std::string> toks;
                    boost::split(toks, line, boost::is_any_of("="));
                    auto k = toks.at(0);
                    auto v = toks.at(1);
                    if(v.find(".") != std::string::npos){
                        try{
                            auto r = boost::lexical_cast<double>(v);
                            m->set<double>(k,r);
                            continue;
                        }catch(...){}
                    }
                    if(v=="false" || v=="true"){
                        m->set<bool>(k,v=="true");
                        continue;
                    }
                    try{
                        auto r = boost::lexical_cast<int>(v);
                        m->set<int>(k,r);
                        continue;
                    }catch(...){ }
                    m->set<std::string>(k,v);
                }
            }
        }catch(...){
            if(fn.empty()) {
                K_ERROR(PARSE, "Meta_ input stream data")
            } else {
                K_ERROR(PARSE, fn);
            }
        }
    }
    else
    {
        WARNING("Unknown serialization format");
    }
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: deserialize(const std::string& fn, unsigned flags)
{
    auto l = this->lock();
    //assert(!frozen());
    if(fn.empty())
        K_ERROR(READ, "no filename specified");

    if(flags & Meta::F_CREATE)
    {
        if(not boost::filesystem::exists(fn))
        {
            boost::filesystem::create_directories(
                boost::filesystem::path(fn).parent_path()
            );
            std::fstream file(fn, std::fstream::out); // create
            if(not file.good())
                K_ERROR(READ, fn);
            if(boost::ends_with(boost::to_lower_copy(fn), ".json"))
                file << "{}\n";
            file.flush();
        }
    }
        
    std::ifstream file(fn);
    if(not file.good())
        K_ERROR(READ, fn);

    // TODO: subdocument path
    std::string pth;
    auto ofs = fn.rfind(':');
    if(ofs != std::string::npos && ofs > 1)
       pth = fn.substr(ofs+1);
    
    // may throw, but dont catch
    deserialize(filename_to_format(fn), file, fn, pth, flags);

    //if(not pth.empty())
    //{
    //    // this is temp, so no need to lock
    //    Ptr<Meta_<Mutex,Ptr,This>> m;
    //    try{
    //        Meta_<Mutex,Ptr,This>* par = parent_ptr();
    //        bool created = false;
    //        m = path(pth, 0, &created);
    //        if(created)
    //            K_ERROR(READ, fn);
    //        *this = std::move(*m);
    //        parent(par);
    //        m = kit::make<Ptr<Meta_<Mutex,Ptr,This>>>();
    //    }catch(...){
    //        K_ERROR(READ, fn);
    //    }
    //}

    m_Filename = fn;
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: serialize(const std::string& fn, unsigned flags) const
{
    auto l = this->lock();

    if(fn.empty())
        K_ERROR(WRITE, "no filename specified");

    std::fstream file;
    file.open(
        fn,
        std::ios_base::in | std::ios_base::out | std::ios_base::trunc
    );
    if(!file.is_open() || file.bad())
        K_ERROR(WRITE, fn);

    file << serialize(filename_to_format(fn), flags);
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
Ptr<Meta_<Mutex,Ptr,This>> Meta_<Mutex,Ptr,This> :: path(
    const std::vector<std::string>& pth,
    unsigned flags,
    bool* created
){
    // don't this->lock until you have the root
    Ptr<Meta_<Mutex,Ptr,This>> spthis = this->shared_from_this();
    Ptr<Meta_<Mutex,Ptr,This>> base;
    
    {
        // TODO; redo this try locks like in parent()
        if(flags & ABSOLUTE_PATH)
        {
            if(!(base = root()))
                base = spthis;
        }
        else
            base = spthis;
    }

    // [ASSUMPTION] if the root of *this changes,
    // we use the old root instead.
    //
    // Why the hassle? Well..
    // We have to this->lock from root-to-this, but need to find the root
    // from this using this->root(), so we have to let go of the this->lock
    // above to reacquire in the right order!
    // In the rare case that root is disconnected from *this during
    // this period, we prefer to use the discovered root.

    // TODO: replace this with the backwards try-locks like in parent()

    std::list<std::unique_lock<Mutex>> locks;
    for(auto&& p: pth)
    {
        locks.push_back(base->lock());
        try{
            auto child = base->template at<Ptr<Meta_<Mutex,Ptr,This>>>(p);
            // TODO: add flags
            base = child;
            continue;
        }catch(const std::exception&){
            if(flags & ENSURE_PATH) {
                auto child = kit::make<Ptr<Meta_<Mutex,Ptr,This>>>();
                base->set(p, child);
                base = child;
                if(created)
                    *created = true;
                continue;
            }else{
                throw std::out_of_range("no such path");
                //return std::tuple<Ptr<Meta_<Mutex,Ptr,This>>,bool>(
                //    nullptr, false
                //);
            }
        }
    }
    //locks.clear();

    return base;
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
template<class TMutex,template <class> class TPtr,template <class> class TThis>
void Meta_<Mutex,Ptr,This> :: merge(
    const TPtr<Meta_<TMutex,TPtr,TThis>>& t,
    WhichCallback_t which,
    unsigned flags,
    TimeoutCallback_t timeout,
    std::function<void(
        const TPtr<Meta_<Mutex,TPtr,TThis>>&, const MetaElement&
    )> visit
){
    assert(t);

    std::unique_lock<Mutex> al(this->mutex(), std::defer_lock);
    std::unique_lock<TMutex> bl(t->mutex(), std::defer_lock);
    if(!timeout)
        std::lock(al, bl);
    else
    {
        if(std::try_lock(al, bl) != -1) // failed to this->lock
        {
            // incremental merges only try subtrees once and only if they have
            // timeouts
            auto spthis = this->shared_from_this();
            if(flags & (unsigned)MergeFlags::INCREMENTAL)
            {
                // bail if no timeout function or if timeout returns false
                if(!timeout || !timeout(spthis))
                    return;
                // timeout told us to retry once more
                if(std::try_lock(al, bl) != -1)
                    return;
            }
            else
            {
                do {
                    if(!timeout(spthis))
                        return; // give up
                }while(std::try_lock(al, bl) == -1);
            }
        }
    }

    for(auto&& e: t->elements_ref())
    {
        if(visit)
            visit(t, e);
        
        if(e.key.empty())
            m_Elements.push_back(e);
        else if(m_Keys.find(e.key)==m_Keys.end())
        {
            m_Elements.push_back(e);
            m_Keys[e.key] = m_Elements.size()-1;
        }
        else
        {
            // already there, merge?
            unsigned this_id = m_Keys[e.key];
            auto&& this_e = m_Elements[this_id];
            if(this_e.type.id == MetaType::ID::META &&
                    e.type.id == MetaType::ID::META// &&
                //this_e.type.storage == MetaType::Ptr::SHARED &&
                //     e.type.storage == MetaType::Ptr::SHARED
            ){
                if(flags & (unsigned)MergeFlags::RECURSIVE)
                {
                    //try{
                        at<Ptr<Meta_<Mutex,Ptr,This>>>(this_id)->merge(
                            t->template at<TPtr<Meta_<TMutex,TPtr>>>(e.key),
                            which,
                            flags,
                            timeout
                        );
                    //}catch(const kit::null_ptr_exception&){}
                }
                else
                {
                    auto r = which(this_e, e);
                    Meta_::Which* w = boost::get<Meta_::Which>(&r);
                    if(!w)
                    {
                        MetaElement* a = boost::get<MetaElement>(&r);
                        auto preserved_key = this_e.key;
                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
                            this_e = std::move(*a);
                        else
                            this_e = *a;
                        this_e.key = preserved_key;
                    }
                    else if(*w == Which::RECURSE)
                    {
                        auto m = at<Ptr<Meta_<Mutex,Ptr,This>>>(this_id);
                        assert(m);
                        m->merge(
                            t->template at<Ptr<Meta_<Mutex,Ptr,This>>>(e.key),
                            which,
                            flags,
                            timeout
                        );
                        // delete entire null trees in destructive merges
                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
                        {
                            if(m->all_empty())
                            {
                                m->clear();
                                e.nullify();
                            }
                        }
                    }
                    else if(*w == Which::OTHER)
                    {
                        replace(this_id, e);
                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
                            e.nullify();
                    }
                    else if(*w == Which::NEITHER)
                    {
                        remove(this_id);
                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
                            e.nullify();
                    }

                    // TODO: other Meta_::Which values (except Which::SELF,
                    //  that can be treated simply by continue; below)
                    // Meta_::Which values have preference over flags below
                    // so we continue; here

                    continue;
                }
            }

            if(flags & (unsigned)MergeFlags::REPLACE) {
                replace(this_id, e);
                e.nullify();
            }
        }
    }
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
template<class TMutex, template <class> class TPtr, template <typename> class TThis>
void Meta_<Mutex,Ptr,This> :: merge(
    const TPtr<Meta_<TMutex,TPtr,TThis>>& t,
    unsigned flags
){
    if(flags & (unsigned)MergeFlags::REPLACE)
        merge(t, [](const MetaElement&,
            const MetaElement&
        ){
            return Which::OTHER;
        }, flags);
    else
        merge(t,
            [](const MetaElement&,
            const MetaElement&
        ){
            return Which::SELF;
        }, flags);
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: merge(
    const std::string& fn,
    unsigned flags
){
    merge(kit::make<Ptr<Meta_<>>>(fn), flags);
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
void Meta_<Mutex,Ptr,This> :: merge(
    const std::string& fn,
    WhichCallback_t which,
    unsigned flags,
    TimeoutCallback_t timeout,
    VisitorCallback_t visit
){
    merge(kit::make<Ptr<Meta_<kit::dummy_mutex,Ptr,This>>>(fn),
        which,
        flags,
        timeout,
        visit
    );
}

template<class Mutex, template <class> class Ptr, template <typename> class This>
/*static*/ MetaFormat Meta_<Mutex,Ptr,This> :: filename_to_format(const std::string& fn)
{
    boost::filesystem::path p(fn);
    if(p.extension().string() == ".json")
        return MetaFormat::JSON;
    else if(p.extension().string() == ".ini")
        return MetaFormat::INI;
    else if(p.extension().string() == ".yaml")
        return MetaFormat::YAML;
    return MetaFormat::UNKNOWN;
}

//template<class Mutex, template <class> class Ptr, template <typename> class This>
//static std::shared_ptr<Meta> Meta_<Mutex,Ptr,This> :: copy()
//{
//    assert(t);

//    auto l = lock();
    
//    if(!timeout)
//        std::lock(al, bl);
//    else
//    {
//        if(std::try_lock(al, bl) != -1) // failed to this->lock
//        {
//            // incremental merges only try subtrees once and only if they have
//            // timeouts
//            auto spthis = this->shared_from_this();
//            if(flags & (unsigned)MergeFlags::INCREMENTAL)
//            {
//                // bail if no timeout function or if timeout returns false
//                if(!timeout || !timeout(spthis))
//                    return;
//                // timeout told us to retry once more
//                if(std::try_lock(al, bl) != -1)
//                    return;
//            }
//            else
//            {
//                do {
//                    if(!timeout(spthis))
//                        return; // give up
//                }while(std::try_lock(al, bl) == -1);
//            }
//        }
//    }

//    for(auto&& e: t->elements_ref())
//    {
//        if(visit)
//            visit(t, e);
        
//        if(e.key.empty())
//            m_Elements.push_back(e);
//        else if(m_Keys.find(e.key)==m_Keys.end())
//        {
//            m_Elements.push_back(e);
//            m_Keys[e.key] = m_Elements.size()-1;
//        }
//        else
//        {
//            // already there, merge?
//            unsigned this_id = m_Keys[e.key];
//            auto&& this_e = m_Elements[this_id];
//            if(this_e.type.id == MetaType::ID::META &&
//                    e.type.id == MetaType::ID::META// &&
//                //this_e.type.storage == MetaType::Ptr::SHARED &&
//                //     e.type.storage == MetaType::Ptr::SHARED
//            ){
//                if(flags & (unsigned)MergeFlags::RECURSIVE)
//                {
//                    //try{
//                        at<Ptr<Meta_<Mutex,Ptr,This>>>(this_id)->merge(
//                            t->template at<TPtr<Meta_<TMutex,TPtr>>>(e.key),
//                            which,
//                            flags,
//                            timeout
//                        );
//                    //}catch(const kit::null_ptr_exception&){}
//                }
//                else
//                {
//                    auto r = which(this_e, e);
//                    Meta_::Which* w = boost::get<Meta_::Which>(&r);
//                    if(!w)
//                    {
//                        MetaElement* a = boost::get<MetaElement>(&r);
//                        auto preserved_key = this_e.key;
//                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
//                            this_e = std::move(*a);
//                        else
//                            this_e = *a;
//                        this_e.key = preserved_key;
//                    }
//                    else if(*w == Which::RECURSE)
//                    {
//                        auto m = at<Ptr<Meta_<Mutex,Ptr,This>>>(this_id);
//                        assert(m);
//                        m->merge(
//                            t->template at<Ptr<Meta_<Mutex,Ptr,This>>>(e.key),
//                            which,
//                            flags,
//                            timeout
//                        );
//                        // delete entire null trees in destructive merges
//                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
//                        {
//                            if(m->all_empty())
//                            {
//                                m->clear();
//                                e.nullify();
//                            }
//                        }
//                    }
//                    else if(*w == Which::OTHER)
//                    {
//                        replace(this_id, e);
//                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
//                            e.nullify();
//                    }
//                    else if(*w == Which::NEITHER)
//                    {
//                        remove(this_id);
//                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
//                            e.nullify();
//                    }

//                    // TODO: other Meta_::Which values (except Which::SELF,
//                    //  that can be treated simply by continue; below)
//                    // Meta_::Which values have preference over flags below
//                    // so we continue; here

//                    continue;
//                }
//            }

//            if(flags & (unsigned)MergeFlags::REPLACE) {
//                replace(this_id, e);
//                e.nullify();
//            }
//        }
//    }

//}

