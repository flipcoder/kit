#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <fstream>
#include <sstream>
//#include <boost/circular_buffer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <unordered_map>
#include "errors.h"
#include "../kit.h"

class Log:
    public kit::singleton<Log>,
    public kit::mutexed<std::recursive_mutex>
{
public:

    class Message
    {
    public:
        std::string sMessage;
        enum eLoggingLevel { LL_BLANK, LL_INFO, LL_WARNING, LL_ERROR } eLevel;

        Message(std::string message, eLoggingLevel level):
            sMessage(message),
            eLevel(level) {}
    };

    class Capturer
    {
    public:
        Capturer(){
            auto l = Log::get().lock();
            m_bOldValue = Log::get().capture();
            Log::get().capture(true);
        }
        ~Capturer(){
            auto l = Log::get().lock();
            Log::get().capture(m_bOldValue);
            if(!m_bOldValue)
                Log::get().emit();
        }
    private:
        bool m_bOldValue;
    };

    class Silencer {
    public:
        Silencer() {
            auto l = Log::get().lock();
            m_bOldValue = Log::get().silence();
            Log::get().silence(true);
        }
        ~Silencer(){
            if(!m_bOldValue)
                Log::get().silence(false);
        }
    private:
        bool m_bOldValue;
    };

    // Scoped indentation
    class Indent
    {
    public:
        Indent(){
            auto l = Log::get().lock();
            Log::get().push_indent();
        }
        Indent(const std::string& msg){
            auto l = Log::get().lock();
            Log::get().write(msg);
            Log::get().push_indent();
        }
        ~Indent(){
            auto l = Log::get().lock();
            Log::get().pop_indent();
        }
        unsigned level() const {
            return Log::get().indents();
        }
    };

    class LogThread
    {
        public:
        private:
    };

    //static Log& get()
    //{
    //    static Log log;
    //    return log;
    //}
    
    Log();
    virtual ~Log() {}

    std::string emit() {
        auto l = lock();
        std::ostringstream oss;
        for(auto line: m_Captured)
            oss << line << std::endl;
        m_Captured.clear();
        return oss.str();
    }
    void clear_capture() {
        auto l = lock();
        m_Captured.clear();
    }
    bool capture() const {
        auto l = lock();
        return m_bCapture;
    }
    void capture(bool b) {
        auto l = lock();
        m_bCapture = b;
    }

    void push_indent() {
        auto l = lock();
        auto indent = m_Indents.find(std::this_thread::get_id());
        if(indent == m_Indents.end())
            m_Indents[std::this_thread::get_id()] = 1;
        else
            ++(indent->second);
    }
    void pop_indent() {
        auto l = lock();
        auto indent = m_Indents.find(std::this_thread::get_id());
        assert(indent->second > 0);
        --(indent->second);
        if(!indent->second)
            m_Indents.erase(indent);
    }
    unsigned indents() const {
        auto l = lock();
        auto indent = m_Indents.find(std::this_thread::get_id());
        if(indent == m_Indents.end())
            return 0;
        return indent->second;
    }

    void write(const std::string& s, Message::eLoggingLevel lev = Message::LL_INFO);
    void warn(const std::string&s) { write(s,Message::LL_WARNING); }
    void error(const std::string&s) {write(s,Message::LL_ERROR);}

    //unsigned int size() const { return m_cbLog.size(); }
    //bool empty() const { return (size()==0); }
    void silence(bool b) {
        auto l = lock();
        m_bSilence=b;
    }
    bool silence() const {
        auto l = lock();
        return m_bSilence;
    }

    unsigned num_indented_threads() const {
        auto l = lock();
        return m_Indents.size();
    }

    void clear_indents() {
        auto l = lock();
        m_Indents.clear();
    }

    //boost::circular_buffer<Message>* getBuffer() { return &m_cbLog; }
    //const boost::circular_buffer<Message>* getBuffer_c() const { return &m_cbLog; }
    //std::string message(unsigned int idx) {
    //    if(idx < m_cbLog.size())
    //        return m_cbLog.at(idx).sMessage;
    //    return "";
    //}
private:

    //boost::circular_buffer<Message> m_cbLog;
    std::ofstream m_LogFile;
    bool m_bSilence = false;
    bool m_bCapture = false;
    std::vector<std::string> m_Captured;

    static const int DEFAULT_LENGTH = 256;
    //unsigned m_Indent = 0;

    std::unordered_map<std::thread::id, unsigned> m_Indents;
};

#define LOG(X) Log::get().write(X)
#define LOGf(X,Y) Log::get().write((boost::format(X) % Y).str())
#define WARNING(X) Log::get().warn(X)
#define WARNINGf(X,Y) Log::get().warn((boost::format(X) % Y).str())
#define ERROR(CODE,X) {\
    if(ErrorCode::CODE != ErrorCode::UNKNOWN)\
        Log::get().error(g_ErrorString[(unsigned)ErrorCode::CODE]\
             + ": " + std::string(X));\
    else\
        Log::get().error(X);\
    \
    throw Error(ErrorCode::CODE, X);\
}
#define ERRORf(CODE,X,Y) {\
    const std::string _err = (boost::format(X) % Y).str();\
    if(ErrorCode::CODE != ErrorCode::UNKNOWN)\
        Log::get().error(g_ErrorString[(unsigned)ErrorCode::CODE] + ": " + _err);\
    else\
        Log::get().error(_err);\
    throw Error(ErrorCode::CODE, _err);\
}

#endif

