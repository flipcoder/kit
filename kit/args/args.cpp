#include "args.h"
#include "../log/log.h"
#include "../kit.h"
#include <boost/filesystem.hpp>
using namespace std;

void Args :: analyze()
{
    // remove whitespace args
    kit::remove_if(m_Args, [](const string& s) {
        return (s.find_first_not_of(" \n\r\t") == string::npos);
    });

    // parse key-values
    bool after_sep = false;
    for(const auto& arg: m_Args)
    {
        if(boost::starts_with(arg, "--"))
        {
            if(arg.length()==2)
                after_sep = true; // empty -- is a separator
            
            if(!after_sep)
            {
                size_t idx = arg.find('=');
                if(idx != string::npos)
                {
                    if(idx != arg.rfind('='))
                        throw exception(); // more than one '='
                    string key = arg.substr(2, idx-2);
                    if(m_Values.find(key) != m_Values.end())
                        throw exception(); // duplicate
                    string value = arg.substr(idx+1);
                    if(!value.empty())
                        if(!m_Values.insert({key,value}).second)
                            throw exception(); // failed to insert
                    
                    m_Switches.insert(remove_dashes(key)); // insert key as switch
                }
                else
                {
                    m_Switches.insert(remove_dashes(arg)); // just a switch, --test is test
                }
            }
        }
        else if(boost::starts_with(arg, "-")) // only one dash? (above check is two)
        {
            string flags = arg.substr(1); // remove one dash
            for(auto&& ch: flags)
                m_Switches.insert(string()+ch); // add chars separately
        }
        else
        {
            // no - or --, assume its a filename
            m_Filenames.push_back(arg);
        }
    }
}

string Args :: remove_dashes(string s, bool* success) // static
{
    if(success)
        *success = false;
    
    if(boost::starts_with(s,"--"))
    {
        s = s.substr(2);
        if(success)
            *success = true;
    }
    else if(boost::starts_with(s,"-"))
    {
        s = s.substr(1);
        if(success)
            *success = true;
    }
    return s;
}

bool Args :: has(std::string op) const
{
    bool removed;
    string flag = remove_dashes(op, &removed);
    if(not removed)
        return kit::has(m_Args, op);
    
    if(not flag.empty())
        return kit::has(m_Switches, flag);
    
    assert(false);
}

bool Args :: has(char ch, std::string op) const
{
    if(boost::starts_with(op, "--"))
        op = op.substr(2); // remove --
    else if(boost::starts_with(op, "-"))
        op = op.substr(1);
    auto flag = string()+ch;
    return (kit::has(m_Switches, flag) ||
       (not op.empty() && kit::has(m_Switches, op))
    );
}

void Args :: schema(std::string docstring)
{
    if(docstring.empty())
    {
        m_Schema = boost::optional<Schema>();
        return;
    }
    
    m_Schema = Schema();
    
    vector<string> lines;
    boost::split(lines, docstring, boost::is_any_of("\n"));
    for(auto&& line: lines)
    {
        boost::trim(line);

        // tokenize each switch as space-seperated values
        // until non-prefixed token (or EOL)
        if(boost::starts_with(line, "-"))
        {
            vector<string> tokens;
            boost::split(tokens, line, boost::is_any_of(" \t"));
            for(auto&& tok: tokens)
            {
                if(boost::starts_with(tok, "--"))
                {
                    m_Schema->allowed.push_back(tok);
                }
                else if(boost::starts_with(tok, "-"))
                {
                    for(unsigned i=1;i<tok.size();++i)
                        m_Schema->allowed.push_back(string("-")+tok[i]);
                }
                else
                    break;
            }
        }
    }
}

void Args :: validate() const
{
    if(not m_Schema)
        return;
    
    for(auto&& arg: m_Args)
    {
        // compare against schema
        if(arg == "--") // sep
            break; // allow everything else
        else if(arg == "-") // ?
        {
            string fn = boost::filesystem::basename(m_Filename);
            K_ERRORf(GENERAL,
                "%s: unrecognized argument '%s'\nTry '%s --help for more information.'",
                fn % arg % fn
            );
        }
        else if(boost::starts_with(arg, "--"))
        {
            if(not kit::has(m_Schema->allowed, arg))
            {
                string fn = boost::filesystem::basename(m_Filename);
                K_ERRORf(GENERAL,
                    "%s: unrecognized argument '%s'\nTry '%s --help for more information.'",
                    fn % arg % fn
                );
            }
        }
        else if(boost::starts_with(arg, "-"))
        {
            for(size_t i=0; i<arg.size(); ++i)
            {
                if(0 == i) continue;
                char letter = arg[i];
                
                if(not kit::has(m_Schema->allowed, string("-")+letter))
                {
                    string fn = boost::filesystem::basename(m_Filename);
                    K_ERRORf(GENERAL,
                        "%s: unrecognized argument '-%s'\nTry '%s' --help for more information.",
                        fn % letter % fn
                    );
                }
            }
        }
    }
}

