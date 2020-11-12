#ifndef _ARGS_H_59ROLBDK
#define _ARGS_H_59ROLBDK

#include <vector>
#include <map>
#include <string>
#include <set>
#include "../kit.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <cassert>

class Args
    //public kit::mutexed<>
{
    public:
        Args() = default;
        Args(const Args&) = default;
        Args(Args&&) = default;
        Args& operator=(const Args&) = default;
        Args& operator=(Args&&) = default;

        Args(const std::vector<std::string>& args, std::string docstring = ""):
            m_Args(args)
        {
            analyze();
            schema(docstring);
            validate();
        }

        Args(std::vector<std::string>&& args, std::string docstring = ""):
            m_Args(args)
        {
            analyze();
            schema(docstring);
            validate();
        }

        Args(const std::string& lines, std::string docstring = ""):
            m_Args(kit::lines(lines))
        {
            analyze();
            schema(docstring);
            validate();
        }

        Args(int argc, const char** argv, std::string docstring = ""):
            m_Args(argv+1, argv+argc),
            m_Filename(argv[0])
        {
            analyze();
            schema(docstring);
            validate();
        }

        std::vector<std::string>& get() { return m_Args; }
        const std::vector<std::string>& get() const { return m_Args; };

        size_t size() const { return m_Args.size(); }

        bool empty() const { return m_Args.empty(); }

        bool any(const std::vector<std::string>& v) const {
            for(auto&& s: v)
                if(kit::has(m_Args, s))
                    return true;
            return false;
        }
        
        // [OLD]
        //bool has(const std::string& s) const {
        //    return kit::has(m_Args, s);
        //}
        
        bool has_before(
            const std::string& match, const std::string& sep
        ) const {
            for(auto& s: m_Args){
                if(s==sep)
                    return false;
                if(s==match)
                    return true;
            }
            return false;
        }
        bool has_after(
            const std::string& match, const std::string& sep
        ) const {
            for(auto& s: m_Args){
                if(s==sep)
                    continue;
                if(s==match)
                    return true;
            }
            return false;

        }

        std::vector<std::string> get_matches(std::string reg) const {
            boost::regex expr(reg);
            std::vector<std::string> matches;
            for(const auto& s: m_Args)
                if(boost::regex_search(s, expr))
                    matches.push_back(s);
            return matches;
        }

        std::vector<std::string> get_matches_after(
            std::string reg,
            std::string sep
        ) const {
            bool after = false;
            boost::regex expr(reg);
            std::vector<std::string> matches;
            for(const auto& s: m_Args) {
                if(s==sep) {
                    after=true;
                    continue;
                }
                if(after)
                    if(boost::regex_search(s, expr))
                        matches.push_back(s);
            }
            return matches;
        }

        std::vector<std::string> get_matches_before(
            std::string reg,
            std::string sep
        ) const {
            //bool after = false;
            boost::regex expr(reg);
            std::vector<std::string> matches;
            for(const auto& s: m_Args) {
                if(s==sep)
                    return matches;
                //if(!after)
                if(boost::regex_search(s, expr))
                    matches.push_back(s);
            }
            return matches;
        }


        //std::vector<std::tuple<std::string, boost::smatch>> get_match_groups(boost::regex expr) const {
        //    std::vector<std::tuple<std::string, boost::smatch>> matches;
        //    boost::smatch results;
        //    for(const auto& s: m_Args)
        //        if(boost::regex_match(s, results, expr))
        //            matches.push_back(std::tuple<std::string, boost::smatch>(s, results));
        //    return matches;
        //}


        size_t num_matches(std::string reg) const {
            boost::regex expr(reg);
            size_t count = 0;
            for(const auto& s: m_Args) {
                if(boost::regex_search(s, expr))
                    ++count;
            }
            return count;
        }

        void set(const std::string& key, const std::string& value) {
            // TODO: check key and value formats (no spaces, etc.)
            m_Values[key] = value;
            m_Args.push_back((boost::format("--%s=%s") % key % value).str());
        }
        //void value(const std::string& key, const std::string& value) {
        //    // TODO: check key and value formats (no spaces, etc.)
        //    m_Values[key] = value;
        //    m_Args.push_back((boost::format("--%s=%s") % key % value).str());
        //}
        std::string value(const std::string& key) const {
            try{
                return m_Values.at(key);
            }catch(const std::out_of_range&){
                return std::string();
            }
        }
        std::string value_or(
            const std::string& key,
            const std::string& def = std::string()
        ) const {
            try{
                return m_Values.at(key);
            }catch(const std::out_of_range&){
                return def;
            }
        }

        std::string data() const {
            return boost::algorithm::join(m_Args, "\n");
        }

        // TODO: tokenize based on arg separators
        // std::vector<Args> separate() const {
        // }

        //unsigned find(std::string reg) const {
        //}


        // TODO: I'm not liking the below functions at all, im going to use
        //   a generator (coroutine-style) to do iterations throught this stuff
        //   and add better functions later

        std::string get(unsigned pos) {
            try{
                return m_Args.at(pos);
            }catch(const std::out_of_range&){
                return std::string();
            }
        }
        std::string after(unsigned pos, unsigned offset) {
            try{
                return m_Args.at(pos + offset);
            }catch(const std::out_of_range&){
                return std::string();
            }
        }
        std::string before(unsigned pos, unsigned offset) {
            try{
                if(offset>pos)
                    return std::string();
                return m_Args.at(pos - offset);
            }catch(const std::out_of_range&){
                return std::string();
            }
        }

        // [OLD] tests for a switch and its associated char
        //bool has(char c, std::string s) const {
        //    if(boost::starts_with(s, "--"))
        //        s = s.substr(2); // remove --
        //    else if(boost::starts_with(s, "-"))
        //        s = s.substr(1);
        //    if(has(c) || has(std::string("--")+s)
        //        return true;
        //    if(option(c,s))
        //        return true;
        //    return false;
        //}
        
        // [DEPRECATED]
        //bool has(char c) const {
        //    std::string chs = chars();
        //    return chs.find(c) != std::string::npos;
        //}
                
        // [DEPRECATED] returns string containing all provided char switches
        std::string chars() const {
            std::string r;
            for(auto&& a: m_Args)
                if(boost::starts_with(a, "-") && not boost::starts_with(a, "--"))
                    r += a.substr(1);
            return r;
        }
        
        //std::string after(std::string s, unsigned offset) {
        //}
        //std::string before(std::string s, unsigned offset) {
        //}
        //std::string after_match(std::string s, unsigned offset) {
        //}
        //unsigned find(std::string s) {}

        std::string at(int idx, std::string def="") const {
            try{
                if(idx >= 0)
                    return m_Args.at(idx);
                return m_Args.at(m_Args.size() + idx);
            }catch(const std::out_of_range&){
                return def;
            }
        }
        std::string filenames(int idx, std::string def="") const {
            try{
                if(idx >= 0)
                    return m_Filenames.at(idx);
                return m_Filenames.at(m_Filenames.size() + idx);
            }catch(const std::out_of_range&){
                return def;
            }
        }

        typedef std::vector<std::string>::iterator iterator;
        iterator begin() { return m_Args.begin(); }
        iterator end() { return m_Args.end(); }
        typedef std::vector<std::string>::const_iterator const_iterator;
        const_iterator begin() const { return m_Args.begin(); }
        const_iterator end() const { return m_Args.end(); }
        const_iterator cbegin() const { return m_Args.begin(); }
        const_iterator cend() const { return m_Args.end(); }
        
        std::string filename() const {
            return m_Filename;
        }
        void filename(std::string fn) {
            m_Filename = fn;
        }

        bool has(std::string op) const;
        bool has(char ch, std::string full = std::string()) const;

    private:

        static std::string remove_dashes(std::string s, bool* success = nullptr);

        struct Schema
        {
            std::vector<std::string> allowed;
        };
        
        void analyze();
        void schema(std::string docstring);
        void validate() const;
        
        std::vector<std::string> m_Args;
        std::vector<std::string> m_Filenames;
        //std::vector<std::pair<std::string, std::string>> m_Values;
        std::map<std::string, std::string> m_Values;
        std::string m_Filename;
        boost::optional<Schema> m_Schema;

        // all options broken down separately
        //   ./program -abc --test
        // will contain -a, -b, -c, and --test
        // use has() to check this
        std::set<std::string> m_Switches;
};

#endif

