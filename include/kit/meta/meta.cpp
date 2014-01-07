#include <boost/filesystem.hpp>
#include <sstream>
#include "meta.h"
#include "../log/log.h"
using namespace std;

Meta :: Meta(const string& fn):
    m_Filename(fn)
{
    //deserialize();
}

Meta :: Meta(Meta::Format fmt, const std::string& data)
{
    deserialize(fmt, data);
}

Meta::Loop Meta::each(
    std::function<Loop(
        const std::shared_ptr<Meta>&, Element&, unsigned
    )> func,
    std::deque<std::tuple<
        std::shared_ptr<Meta>,
        std::unique_lock<std::recursive_mutex>
    >>* metastack,
    unsigned flags, // use EachFlag enum
    unsigned level
){
    auto l = lock();

    // only keep one instance of this, will be null if metastack isn't
    std::shared_ptr<Meta> spthis_ = shared_from_this();
    std::shared_ptr<Meta>* spthis = &spthis_;
    if(metastack) {
        metastack->push_back(std::make_tuple(move(spthis_), move(l)));
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

    Loop status = Loop::STEP;
    bool repeat;
    bool break_outer;

    for(auto&& e: elements_ref()) // outer
    {
        break_outer = false;

        do{ // inner
            repeat = false;

            if(!(flags & (unsigned)EachFlag::POSTORDER))
            {
                status = func(*spthis, e, level+1);
                if(status == Loop::BREAK) {
                    break_outer = true;
                    break;
                }
                if(status == Loop::REPEAT) {
                    repeat = true;
                    continue;
                }
                if(status == Loop::CONTINUE) {
                    // skip subtree recursion
                    continue;
                }
            }

            // reset status just in case we don't enter this block (*)
            status = Loop::STEP;
            if(flags & (unsigned)EachFlag::RECURSIVE &&
                e.type.id == Type::ID::META
            ){
                // TODO: catch flags from each() call here
                //
                // NOTE: keep the metastack pointer as the exclusive
                // shared_ptr during the each() call
                // so dumping the stack allows other threads
                // to erase the pointer)
                Meta* mp = boost::any_cast<std::shared_ptr<Meta>>(e.value).get();
                status =
                    mp->each(
                        func,
                        metastack,
                        flags,
                        level+1
                    );

                // Allows for user to pop items off the BOTTOM of
                // the stack if another thread wants in
                // This invalidates iterators, so we have to break if
                // this is the lowest valid level
                if(metastack && level>0 && metastack->size() == 1) {
                    break_outer = true;
                    break;
                }
                if(status == Loop::BREAK) {
                    // metastack might have been modified
                    break_outer = true;
                    break;
                }
                if(status == Loop::REPEAT) {
                    repeat = true;
                    continue;
                }

                //lc = loop_controller();
                //if(lc == LoopController::BREAK)
                //    break;
            }

            // the CONTINUE check here is why we reset status above (*)
            if(flags & (unsigned)EachFlag::POSTORDER
                && status != Loop::CONTINUE // warning: untested
            ){
                status = func(*spthis, e, level+1);
                if(status == Loop::BREAK) {
                    break_outer = true;
                    break;
                }
                if(status == Loop::REPEAT) {
                    repeat = true;
                    continue;
                }

            }

        }while(repeat);

        if(break_outer)
        {
            // dismiss the scope guard
            metastack = nullptr;
            return Loop::BREAK;
        }
    }

    return Loop::STEP; // EachResult flags here
}

//  use type checks with typeid() before attempting conversion
//   or boost::any cast?
//  POD types should also work if behind smart ptrs
//  only shared_ptr's to other trees should serialize (not weak)

/*
 * Will recurse into nested Metas
 * Might throw bad_any_cast
 */
Json::Value Meta::Element::serialize_json(
    unsigned flags
) const {

    Json::Value v;

    if(type.id == Type::ID::META)
    {
        boost::any_cast<shared_ptr<Meta>>(value)->serialize_json(v);
    }
    // TODO: boost::any -> value (and key?)
    else if(type.id == Type::ID::INT) {
        //if(type.storage == Type::Storage::STACK) {
            if(type.flags & Type::Flag::SIGN)
                v = Json::Int(boost::any_cast<int>(value));
            else
                v = Json::Int(boost::any_cast<unsigned>(value));
        //} else if(type.storage == Type::Storage::SHARED) {
        //    v = Json::Int(
        //        *kit::safe_ptr(boost::any_cast<shared_ptr<int>>(value))
        //    );
        //}
    }
    else if(type.id == Type::ID::REAL)
    {
        //if(type.storage == Type::Storage::STACK) {
            v = Json::Value(boost::any_cast<double>(value));
        //}
    }
    else if(type.id == Type::ID::STRING)
    {
        //if(type.storage == Type::Storage::STACK) {
            v = Json::Value(boost::any_cast<string>(value));
        //}
    }
    else if(type.id == Type::ID::EMPTY)
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
    if((flags & SerializeFlag::STORE_KEY) && !key.empty())
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
void Meta :: serialize_json(Json::Value& v) const
{
    if(is_map())
    {
        v = Json::Value(Json::objectValue);

        for(auto&& e: m_Elements)
        {
            try{
                v[e.key] = e.serialize_json();
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
                v.append(e.serialize_json(Element::SerializeFlag::STORE_KEY));
                //else
                //    keys[e.key] = e.serialize_json();
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
void Meta :: deserialize_json(
    const Json::Value& v
){
    //unsigned flags = empty() ? 0 : AddFlag::UNIQUE;
    unsigned flags = 0;

    for(auto i = v.begin();
        i != v.end();
        ++i)
    {
        auto&& e = *i;

        string k;
        if(i.memberName())
            k = i.memberName();

        // TODO: move this stuff to Element::deserialize_json()
        if(e.isArray() || e.isObject())
        {
            auto nested = make_shared<Meta>();
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
            set<string>(k, e.asString());
        else if(e.isBool())
            set<bool>(k, e.asBool());
        else if(e.isDouble())
            set<double>(k, e.asDouble());
        else
            WARNINGf("unparsable value at key = %s", k);
    }
}

string Meta :: serialize(Meta::Format fmt) const
{
    auto l = lock();

    string data;
    if(fmt == Format::JSON)
    {
        Json::Value root;
        serialize_json(root);
        return root.toStyledString();
    }
    else if (fmt == Format::BSON)
    {
        
    }
    else if (fmt == Format::HUMAN)
        assert(false);
    else
        assert(false);

    return data;
}

void Meta :: deserialize(Meta::Format fmt, const string& data, const std::string& fn)
{
    istringstream iss(data);
    deserialize(fmt, iss, fn);
}

void Meta :: deserialize(Meta::Format fmt,  istream& data, const std::string& fn)
{
    auto l = lock();

    if(fmt == Format::JSON)
    {
        Json::Value root;
        Json::Reader reader;

        if(!reader.parse(data, root))
            if(fn.empty())
                ERROR(PARSE, "Meta input stream data")
            else
                ERROR(PARSE, fn);

        deserialize_json(root);
    }
    else if (fmt == Format::HUMAN)
    {
        // human deserialization unsupported
        assert(false);
    }
    else if (fmt == Format::BSON)
    {
        
    }
    else
    {
        WARNING("Unknown serialization format");
    }
}

void Meta :: deserialize(const string& fn)
{
    auto l = lock();
    //assert(!frozen());
    if(fn.empty())
        ERROR(READ, "no filename specified");

    ifstream file(fn);
    if(!file.good())
        ERROR(READ, fn);


    // may throw, but dont catch
    deserialize(filename_to_format(fn), file, fn);

    m_Filename = fn;
}

void Meta :: serialize(const string& fn) const
{
    auto l = lock();

    if(fn.empty())
        ERROR(WRITE, "no filename specified");

    fstream file;
    file.open(
        fn,
        ios_base::in | ios_base::out | ios_base::trunc
    );
    if(!file.is_open() || file.bad())
        ERROR(WRITE, fn);

    file << serialize(filename_to_format(fn));
}

tuple<shared_ptr<Meta>, bool> Meta :: ensure_path(
    const vector<string>& path,
    unsigned flags
){
    // don't lock until you have the root

    shared_ptr<Meta> base;

    {
        // TODO; redo this try locks like in parent()
        if(flags & (unsigned)EnsurePathFlags::ABSOLUTE_PATH)
        {
            if(!(base = root()))
                base = shared_from_this();
        }
        else
            base = shared_from_this();
    }

    // [ASSUMPTION] if the root of *this changes,
    // we use the old root instead.
    //
    // Why the hassle? Well..
    // We have to lock from root-to-this, but need to find the root
    // from this using this->root(), so we have to let go of the lock
    // above to reacquire in the right order!
    // In the rare case that root is disconnected from *this during
    // this period, we prefer to use the discovered root.

    // TODO: replace this with the backwards try-locks like in parent()

    bool created = false;

    list<unique_lock<recursive_mutex>> locks;
    for(auto&& p: path)
    {
        locks.push_back(base->lock());
        try{
            auto child = base->at<shared_ptr<Meta>>(p);
            // TODO: add flags
            base = child;
            continue;
        }catch(...){
            auto child = make_shared<Meta>();
            base->set(p, child);
            base = child;
            created = true;
            continue;
        }
    }
    locks.clear();

    return tuple<shared_ptr<Meta>,bool>(base, !created);
}

void Meta :: merge(
    const shared_ptr<Meta>& t,
    WhichCallback_t which,
    unsigned flags,
    TimeoutCallback_t timeout
){
    assert(t);

    unique_lock<recursive_mutex> al(mutex(), defer_lock);
    unique_lock<recursive_mutex> bl(t->mutex(), defer_lock);
    if(!timeout)
        std::lock(al, bl);
    else
    {
        if(std::try_lock(al, bl) != -1) // failed to lock
        {
            // incremental merges only try subtrees once and only if they have
            // timeouts
            auto spthis = shared_from_this();
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

    //unsafe_merge(t, flags);

    // not all elements have to have keys, so use Elements size

    //for(auto&& k: t.keys_ref())
    //    k.second += keys_offset;

    //// TODO: this will not do overwrites
    //m_Keys.insert(
    //    t.keys_ref().begin(),
    //    t.keys_ref().end()
    //);

    //unsigned keys_offset = m_Elements.size();

    //for(const auto& k: m_Keys)
    //    m_Keys[k.first] = k.second + keys_offset;

    for(auto&& e: t->elements_ref())
    {
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
            if(this_e.type.id == Type::ID::META &&
                    e.type.id == Type::ID::META// &&
                //this_e.type.storage == Type::Storage::SHARED &&
                //     e.type.storage == Type::Storage::SHARED
            ){
                if(flags & (unsigned)MergeFlags::RECURSIVE)
                {
                    //try{
                        at<shared_ptr<Meta>>(this_id)->merge(
                            t->at<shared_ptr<Meta>>(e.key),
                            which,
                            flags,
                            timeout
                        );
                    //}catch(const kit::null_ptr_exception&){}
                }
                else
                {
                    auto r = which(this_e, e);
                    Meta::Which* w = boost::get<Meta::Which>(&r);
                    if(!w)
                    {
                        Meta::Element* a = boost::get<Meta::Element>(&r);
                        auto preserved_key = this_e.key;
                        if(flags & (unsigned)MergeFlags::INCREMENTAL)
                            this_e = move(*a);
                        else
                            this_e = *a;
                        this_e.key = preserved_key;
                    }
                    else if(*w == Which::RECURSE)
                    {
                        auto m = at<shared_ptr<Meta>>(this_id);
                        assert(m);
                        m->merge(
                            t->at<shared_ptr<Meta>>(e.key),
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

                    // TODO: other Meta::Which values (except Which::THIS,
                    //  that can be treated simply by continue; below)
                    // Meta::Which values have preference over flags below
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

void Meta :: merge(
    const shared_ptr<Meta>& t,
    unsigned flags
){
    if(flags & (unsigned)MergeFlags::REPLACE)
        merge(t, [](const Meta::Element& a, const Meta::Element& b) {
            return Which::OTHER;
        }, flags);
    else
        merge(t, [](const Meta::Element& a, const Meta::Element& b) {
            return Which::THIS;
        }, flags);
}

/*static*/ Meta::Format  Meta :: filename_to_format(const string& fn)
{
    boost::filesystem::path p(fn);
    return p.extension().string() == ".json" ? Format::JSON : Format::UNKNOWN;
}

