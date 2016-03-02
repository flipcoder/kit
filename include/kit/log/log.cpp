#include "log.h"
//#include <boost/circular_buffer.hpp>
#include <iostream>
#include <fstream>
using namespace std;

//const int LOG_LENGTH = 256;
//const std::string LOG_FILE = "log.txt";

Log::Log()
{
    //m_cbErrors.set_capacity(LOG_LENGTH);
    //m_LogFile.open(LOG_FILE, ios_base::trunc);
    //ios::sync_with_stdio(false);
}

void Log::write(std::string s, Log::Level lev)
{
    auto l = lock();
    std::function<void()> after_unlock;
    {
        auto& th = this_log();

        if(th.m_SilenceFlags == Log::Silencer::ALL)
            return;
        
        
        if(!th.m_bCapture)
        {
            ostringstream line;
            if(lev == Log::LL_ERROR)
            {
                for(unsigned i=0;i<th.m_Indents;++i)
                    line << "  ";
                line << s;
                if(!(th.m_SilenceFlags & Log::Silencer::ERRORS))
                    cerr << line.str() << endl;
                if(th.m_bCapture)
                    th.m_Captured.push_back(line.str());
                th.on_log(s, lev);
                auto _this = this;
                after_unlock = [_this,s,lev]{
                    _this->on_log(s,lev);
                };
            }
            else
            {
                // apparently no capturing of non-error messages yet
                if(lev == LL_WARNING &&
                    (th.m_SilenceFlags & Log::Silencer::WARNINGS))
                    return;
                
                if(lev == LL_INFO &&
                    (th.m_SilenceFlags & Log::Silencer::INFO))
                    return;

                for(unsigned i=0;i<th.m_Indents;++i)
                    line << "  ";
                if(lev == LL_WARNING)
                    line << "[WARNING] ";
                line << s;
                cout << line.str() << endl;
                th.on_log(s, lev);
                auto _this = this;
                after_unlock = [_this,s,lev]{
                    _this->on_log(s,lev);
                };
            }
        }
    }
    
    if(after_unlock)
        after_unlock();
    
    //else if(lev == LL_DEBUG)
    //    cout << "[DEBUG] ";

    //if(m_LogFile.is_open())
    //{
    //    for(unsigned i=0;i<m_Indent;++i)
    //        m_LogFile << "  ";
    //    if(lev == LL_WARNING)
    //        m_LogFile << "[WARNING] ";
    //    else if(lev == LL_ERROR)
    //        m_LogFile << "[ERROR] ";
    //    //else if(lev == LL_DEBUG)
    //    //    m_LogFile << "[DEBUG] ";
    //    m_LogFile << s << endl;
    //}
}

std::string Log :: consume_prefix(std::string s)
{
    vector<string> lines_in;
    vector<string> lines_out;
    boost::split(lines_in, s, boost::is_any_of("\n"));
    for(auto&& line: lines_in)
    {
        if(boost::starts_with(line, "("))
        {
            const std::string end_prefix = "): ";
            auto itr = line.find(end_prefix);
            if(itr != string::npos)
                line = line.substr(itr + end_prefix.length());
        }
        lines_out.push_back(line);
    }
    return boost::join(lines_out, "\n");
}

