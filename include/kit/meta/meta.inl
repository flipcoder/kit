#include <boost/filesystem.hpp>
#include <sstream>
#include "meta.h"
#include "../log/log.h"
#include "../kit.h"

template<class Mutex>
MetaBase<Mutex> :: MetaBase(const std::string& fn):
    m_Filename(fn)
{
    deserialize();
}

template<class Mutex>
MetaBase<Mutex> :: MetaBase(MetaFormat fmt, const std::string& data)
{
    deserialize(fmt, data);
}

template<class Mutex>
void MetaBase<Mutex> :: from_args(std::vector<std::string> args)
{
    // remove whitespace args
    kit::remove_if(args, [](const std::string& s) {
        return (s.find_first_not_of(" \n\r\t") == std::string::npos);
    });
    std::map<std::string, std::string> values;

    // parse key-values
    for(const auto& arg: args)
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

template<class Mutex>
MetaLoop MetaBase<Mutex>::each(
    std::function<MetaLoop(
        const std::shared_ptr<MetaBase<Mutex>>&, MetaElement&, unsigned
    )> func,
    unsigned flags, // use EachFlag enum
    std::deque<std::tuple<
        std::shared_ptr<MetaBase<Mutex>>,
        std::unique_lock<Mutex>,
        std::string // key in parent
    >>* metastack,
    unsigned level,
    std::string key
){
    auto l = this->lock();

    // only keep one instance of this, will be null if metastack isn't
    std::shared_ptr<MetaBase<Mutex>> spthis_ = this->shared_from_this();
    std::shared_ptr<MetaBase<Mutex>>* spthis = &spthis_;
    if(metastack) {
        metastack->push_back(std::make_tuple(
            std::move(spthis_), std::move(l), std::move(key)
        ));
        spthis = &std::get<0>(metastack->back());
    }

    //BOOST_SCOPE_EXIT_TPL((&metastack))
    //{
    //    if(metastack) 
    //        metastack->pop_back();
    //} BOOST_SCOPE_EXIT_END

    BOOST_SCOPE_EXIT_ALL(&metastack) {
        if(metastack) 
            metastack->pop_back();
    };

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
                status = func(*spthis, e, level+1);
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
                // std::shared_ptr during the each() call
                // so dumping the stack allows other threads
                // to erase the pointer)
                MetaBase* mp = boost::any_cast<std::shared_ptr<MetaBase<Mutex>>>(e.value).get();
                status =
                    mp->each(
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
                status = func(*spthis, e, level+1);
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

//  use type checks with typeid() before attempting conversion
//   or boost::any cast?
//  POD types should also work if behind smart ptrs
//  only std::shared_ptr's to other trees should serialize (not weak)

/*
 * Will recurse into nested MetaBases
 * Might throw bad_any_cast
 */
template<class Mutex>
Json::Value MetaElement::serialize_json(
    unsigned flags
) const {

    Json::Value v;

    if(not (flags & SERIALIZE))
        throw SkipSerialize();
        
    if(type.id == MetaType::ID::META)
    {
        boost::any_cast<std::shared_ptr<MetaBase<Mutex>>>(value)->serialize_json(v);
    }
    // TODO: boost::any -> value (and key?)
    else if(type.id == MetaType::ID::INT) {
        //if(type.storage == Type::Storage::STACK) {
            if(type.flags & MetaType::Flag::SIGN)
                v = Json::Int(boost::any_cast<int>(value));
            else
                v = Json::Int(boost::any_cast<unsigned>(value));
        //} else if(type.storage == MetaType::Storage::SHARED) {
        //    v = Json::Int(
        //        *kit::safe_ptr(boost::any_cast<std::shared_ptr<int>>(value))
        //    );
        //}
    }
    else if(type.id == MetaType::ID::REAL)
    {
        //if(type.storage == Type::Storage::STACK) {
            v = Json::Value(boost::any_cast<double>(value));
        //}
    }
    else if(type.id == MetaType::ID::STRING)
    {
        //if(type.storage == MetaType::Storage::STACK) {
            v = Json::Value(boost::any_cast<std::string>(value));
        //}
    }
    else if(type.id == MetaType::ID::BOOL)
    {
        //if(type.storage == MetaType::Storage::STACK) {
            v = Json::Value(boost::any_cast<bool>(value));
        //}
    }

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
template<class Mutex>
void MetaBase<Mutex> :: serialize_json(Json::Value& v) const
{
    if(is_map())
    {
        v = Json::Value(Json::objectValue);

        for(auto&& e: m_Elements)
        {
            try{
                v[e.key] = e.template serialize_json<Mutex>();
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
                v.append(e.template serialize_json<Mutex>((unsigned)MetaSerialize::STORE_KEY));
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
template<class Mutex>
void MetaBase<Mutex> :: deserialize_json(
    const Json::Value& v
){
    //unsigned flags = empty() ? 0 : AddFlag::UNIQUE;
    unsigned flags = 0;

    for(auto i = v.begin();
        i != v.end();
        ++i)
    {
        auto&& e = *i;

        std::string k;
        if(i.memberName())
            k = i.memberName();

        // TODO: move this stuff to Element::deserialize_json()
        if(e.isArray() || e.isObject())
        {
            auto nested = std::make_shared<MetaBase<Mutex>>();
            nested->deserialize_json(e);
            set(k, nested);
            continue;
        }
        
        // blank keys will be added as array items, so just use set<>()

        if(e.isNull())
            set<std::nullptr_t>(k, nullptr);
        if(e.isInt())
            set<int>(k, e.asInt());
        else if(e.isUInt())
            set<unsigned>(k, e.asUInt());
        else if(e.isString())
            set<std::string>(k, e.asString());
        else if(e.isBool())
            set<bool>(k, e.asBool());
        else if(e.isDouble())
            set<double>(k, e.asDouble());
        else
            WARNINGf("unparsable value at key = %s", k);
    }
}

template<class Mutex>
std::string MetaBase<Mutex> :: serialize(MetaFormat fmt, unsigned flags) const
{
    auto l = this->lock();

    std::string data;
    if(fmt == MetaFormat::JSON)
    {
        Json::Value root;
        serialize_json(root);
        if(flags & (unsigned)MetaSerialize::MINIMIZE)
        {
            Json::FastWriter writer;
            return writer.write(root);
        }
        else
            return root.toStyledString();
    }
    //else if (fmt == MetaFormat::BSON)
    //{
        
    //}
    //else if (fmt == MetaFormat::HUMAN)
    //    assert(false);
    else
        assert(false);

    return data;
}

template<class Mutex>
void MetaBase<Mutex> :: deserialize(MetaFormat fmt, const std::string& data, const std::string& fn)
{
    std::istringstream iss(data);
    deserialize(fmt, iss, fn);
}

template<class Mutex>
void MetaBase<Mutex> :: deserialize(MetaFormat fmt, std::istream& data, const std::string& fn)
{
    auto l = this->lock();

    if(fmt == MetaFormat::JSON) 
    {
        Json::Value root;
        Json::Reader reader;

        if(!reader.parse(data, root)) {
            if(fn.empty()) {
                ERROR(PARSE, "MetaBase input stream data")
            } else {
                ERROR(PARSE, fn);
            }
        }

        deserialize_json(root);
    }
    else if (fmt == MetaFormat::HUMAN)
    {
        // human deserialization unsupported
        assert(false);
    }
    //else if (fmt == MetaFormat::BSON)
    //{
        
    //}
    else
    {
        WARNING("Unknown serialization format");
    }
}

template<class Mutex>
void MetaBase<Mutex> :: deserialize(const std::string& fn)
{
    auto l = this->lock();
    //assert(!frozen());
    if(fn.empty())
        ERROR(READ, "no filename specified");

    std::ifstream file(fn);
    if(!file.good())
        ERROR(READ, fn);

    // may throw, but dont catch
    deserialize(filename_to_format(fn), file, fn);

    m_Filename = fn;
}

template<class Mutex>
void MetaBase<Mutex> :: serialize(const std::string& fn, unsigned flags) const
{
    auto l = this->lock();

    if(fn.empty())
        ERROR(WRITE, "no filename specified");

    std::fstream file;
    file.open(
        fn,
        std::ios_base::in | std::ios_base::out | std::ios_base::trunc
    );
    if(!file.is_open() || file.bad())
        ERROR(WRITE, fn);

    file << serialize(filename_to_format(fn), flags);
}

template<class Mutex>
std::tuple<std::shared_ptr<MetaBase<Mutex>>, bool> MetaBase<Mutex> :: path(
    const std::vector<std::string>& path,
    unsigned flags
){
    // don't this->lock until you have the root

    std::shared_ptr<MetaBase<Mutex>> base;

    {
        // TODO; redo this try locks like in parent()
        if(flags & ABSOLUTE_PATH)
        {
            if(!(base = root()))
                base = this->shared_from_this();
        }
        else
            base = this->shared_from_this();
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

    bool created = false;

    std::list<std::unique_lock<Mutex>> locks;
    for(auto&& p: path)
    {
        locks.push_back(base->lock());
        try{
            auto child = base->template at<std::shared_ptr<MetaBase<Mutex>>>(p);
            // TODO: add flags
            base = child;
            continue;
        }catch(const std::exception&){
            if(flags & ENSURE_PATH) {
                auto child = std::make_shared<MetaBase<Mutex>>();
                base->set(p, child);
                base = child;
                created = true;
                continue;
            }else{
                throw std::out_of_range("no such path");
                //return std::tuple<std::shared_ptr<MetaBase<Mutex>>,bool>(
                //    nullptr, false
                //);
            }
        }
    }
    //locks.clear();

    return std::tuple<std::shared_ptr<MetaBase<Mutex>>,bool>(base, !created);
}

template<class Mutex>
template<class Mutex2>
void MetaBase<Mutex> :: merge(
    const std::shared_ptr<MetaBase<Mutex2>>& t,
    WhichCallback_t which,
    unsigned flags,
    TimeoutCallback_t timeout,
    VisitorCallback_t visit
){
    assert(t);

    std::unique_lock<Mutex> al(this->mutex(), std::defer_lock);
    std::unique_lock<Mutex2> bl(t->mutex(), std::defer_lock);
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

    for(auto& e: t->elements_ref())
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
            auto& this_e = m_Elements[this_id];
            if(this_e.type.id == MetaType::ID::META &&
                    e.type.id == MetaType::ID::META// &&
                //this_e.type.storage == MetaType::Storage::SHARED &&
                //     e.type.storage == MetaType::Storage::SHARED
            ){
                if(flags & (unsigned)MergeFlags::RECURSIVE)
                {
                    //try{
                        at<std::shared_ptr<MetaBase<Mutex>>>(this_id)->merge(
                            t->template at<std::shared_ptr<MetaBase<Mutex2>>>(e.key),
                            which,
                            flags,
                            timeout
                        );
                    //}catch(const kit::null_ptr_exception&){}
                }
                else
                {
                    auto r = which(this_e, e);
                    MetaBase::Which* w = boost::get<MetaBase::Which>(&r);
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
                        auto m = at<std::shared_ptr<MetaBase<Mutex>>>(this_id);
                        assert(m);
                        m->merge(
                            t->template at<std::shared_ptr<MetaBase<Mutex>>>(e.key),
                            which,
                            flags,
                            timeout
                        );
                        // delete entire null trees in destructive merges
                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
                        {
                            if(m->all_null())
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

                    // TODO: other MetaBase::Which values (except Which::THIS,
                    //  that can be treated simply by continue; below)
                    // MetaBase::Which values have preference over flags below
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

template<class Mutex>
template<class Mutex2>
void MetaBase<Mutex> :: merge(
    const std::shared_ptr<MetaBase<Mutex2>>& t,
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
            return Which::THIS;
        }, flags);
}

template<class Mutex>
void MetaBase<Mutex> :: merge(
    const std::string& fn,
    unsigned flags
){
    // TODO: Could use null mutex for temp meta here
    merge(std::make_shared<MetaBase<kit::dummy_mutex>>(fn), flags);
}

template<class Mutex>
void MetaBase<Mutex> :: merge(
    const std::string& fn,
    WhichCallback_t which,
    unsigned flags,
    TimeoutCallback_t timeout,
    VisitorCallback_t visit
){
    merge(std::make_shared<MetaBase<kit::dummy_mutex>>(fn),
        which,
        flags,
        timeout,
        visit
    );
}

template<class Mutex>
/*static*/ MetaFormat MetaBase<Mutex> :: filename_to_format(const std::string& fn)
{
    boost::filesystem::path p(fn);
    return p.extension().string() == ".json" ?
        MetaFormat::JSON:
        MetaFormat::UNKNOWN;
}

