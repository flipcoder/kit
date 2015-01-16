#include "args.h"
#include "../log/log.h"
#include "../kit.h"
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
                after_sep = true;
            
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
                }
            }
        }
    }
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
                if(boost::starts_with(line, "-"))
                    m_Schema->allowed.push_back(tok);
                else
                    break;
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
        if(boost::starts_with(arg, "-") && not kit::has(m_Schema->allowed, arg))
            throw std::invalid_argument("");
    }
}

