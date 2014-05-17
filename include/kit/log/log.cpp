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

void Log::write(std::string s, Log::Message::eLoggingLevel lev)
{
    auto l = lock();
    auto& th = this_log();

    if(th.m_SilenceFlags == Log::Silencer::ALL)
        return;
    
    if(!th.m_bCapture)
    {
        ostringstream line;
        if(lev == Message::LL_ERROR)
        {
            for(unsigned i=0;i<th.m_Indents;++i)
                line << "  ";
            line << "[ERROR] " << s;
            if(!(th.m_SilenceFlags & Log::Silencer::ERRORS))
                cerr << line.str() << endl;
            if(th.m_bCapture)
                th.m_Captured.push_back(line.str());
            th.on_log(s, lev);
        }
        else
        {
            // apparently no capturing of non-error messages yet
            if(lev == Message::LL_WARNING &&
                (th.m_SilenceFlags & Log::Silencer::WARNINGS))
                return;
            
            if(lev == Message::LL_INFO &&
                (th.m_SilenceFlags & Log::Silencer::INFO))
                return;

            for(unsigned i=0;i<th.m_Indents;++i)
                line << "  ";
            if(lev == Message::LL_WARNING)
                line << "[WARNING] ";
            line << s;
            cout << line.str() << endl;
            th.on_log(s, lev);
        }
    }
    //else if(lev == Message::LL_DEBUG)
    //    cout << "[DEBUG] ";

    //if(m_LogFile.is_open())
    //{
    //    for(unsigned i=0;i<m_Indent;++i)
    //        m_LogFile << "  ";
    //    if(lev == Message::LL_WARNING)
    //        m_LogFile << "[WARNING] ";
    //    else if(lev == Message::LL_ERROR)
    //        m_LogFile << "[ERROR] ";
    //    //else if(lev == Message::LL_DEBUG)
    //    //    m_LogFile << "[DEBUG] ";
    //    m_LogFile << s << endl;
    //}
}

