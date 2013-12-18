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

void Log::write(const std::string& s, Log::Message::eLoggingLevel lev)
{
    //m_cbLog.push_back(Message(s,lev));

    auto l = lock();


    if(!m_bSilence || m_bCapture)
    {
        ostringstream line;
        if(lev == Message::LL_ERROR)
        {
            unsigned tabs = indents();
            for(unsigned i=0;i<tabs;++i)
                line << "  ";
            line << "[ERROR] " << s;
            if(!m_bSilence)
                cerr << line.str() << endl;
            if(m_bCapture)
                m_Captured.push_back(line.str());
        }
        else
        {
            unsigned tabs = indents();
            for(unsigned i=0;i<tabs;++i)
                line << "  ";
            if(lev == Message::LL_WARNING)
                line << "[WARNING] ";
            line << s;
            cout << line.str() << endl;
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

