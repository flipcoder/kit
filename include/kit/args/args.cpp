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

