#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <fstream>
#include <sstream>
//#include <boost/circular_buffer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/utility.hpp>
#include <boost/current_function.hpp>
#include <unordered_map>
#include "errors.h"
#include "../kit.h"

class Log:
    public kit::singleton<Log>,
    public kit::mutexed<std::recursive_mutex>
{
public:

    class LogThread {
        public:
            unsigned m_Indents = 0;
            bool m_bSilence = false;
            bool m_bCapture = false;
            std::vector<std::string> m_Captured;
            bool empty() const {
                return !m_Indents &&
                    !m_bSilence &&
                    !m_bCapture &&
                    m_Captured.empty();
            }
    };
    
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
            //auto l = Log::get().lock();
            m_bOldValue = Log::get().capture();
            Log::get().capture(true);
        }
        ~Capturer(){
            //auto l = Log::get().lock();
            Log::get().capture(m_bOldValue);
            if(!m_bOldValue)
                Log::get().emit();
        }
        std::string emit() const {
            return Log::get().emit();
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
    class Indent:
        boost::noncopyable
    {
    private:
        std::string m_ExitMsg;
        bool m_bPushed = false;
    public:
        Indent(std::nullptr_t) {}
        Indent(
            const std::string& msg = std::string(),
            const std::string& exit_msg = std::string()
        ):
            m_ExitMsg(exit_msg)
        {
            push(msg);
        }
        Indent& operator=(Indent&& rhs) {
            m_ExitMsg = std::move(rhs.m_ExitMsg);
            m_bPushed = rhs.m_bPushed;
            rhs.m_bPushed = false;
            return *this;
        }
        Indent(Indent&& rhs):
            m_ExitMsg(std::move(rhs.m_ExitMsg)),
            m_bPushed(std::move(rhs.m_bPushed))
        {
            rhs.m_bPushed = false;
        }
        operator bool() const {
            return m_bPushed;
        }
        void push(std::string msg) {
            if(!m_bPushed) {
                m_bPushed = true;
                if(!msg.empty())
                    Log::get().write(std::move(msg));
                Log::get().push_indent();
            }
        }
        ~Indent(){
            try{
                pop();
            }catch(...) {}
        }
        void pop() {
            if(m_bPushed) {
                m_bPushed = false;
                Log::get().pop_indent();
                if(!m_ExitMsg.empty())
                    Log::get().write(move(m_ExitMsg));
            }
        }
        unsigned level() const {
            return Log::get().indents();
        }
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
        auto& captured = this_log().m_Captured;
        for(auto line: captured)
            oss << line << std::endl;
        captured.clear();
        optimize();
        return oss.str();
    }
    void clear_capture() {
        auto l = lock();
        this_log().m_Captured.clear();
        optimize();
    }
    bool capture() const {
        auto l = lock();
        return this_log().m_bCapture;
    }
    void capture(bool b) {
        auto l = lock();
        this_log().m_bCapture = b;
    }

    void push_indent() {
        auto l = lock();
        ++this_log().m_Indents;
    }
    void pop_indent() {
        auto l = lock();
        auto& indents = this_log().m_Indents;
        //auto indent = m_Indents.find(std::this_thread::get_id());
        assert(indents > 0);
        --(indents);
        if(!indents)
            optimize();
    }
    unsigned indents() const {
        #ifdef USE_THREAD_LOCAL
            assert(false);
            return 0;
        #else
            auto l = lock();
            auto itr = m_Threads.find(std::this_thread::get_id());
            if(itr != m_Threads.end())
                return itr->second.m_Indents;
            else
                return 0;
        #endif
    }

    void write(std::string s, Message::eLoggingLevel lev = Message::LL_INFO);
    void warn(std::string s) { write(s,Message::LL_WARNING); }
    void error(std::string s) {write(s,Message::LL_ERROR);}

    //unsigned int size() const { return m_cbLog.size(); }
    //bool empty() const { return (size()==0); }
    void silence(bool b) {
        auto l = lock();
        this_log().m_bSilence=b;
        if(!b)
            optimize();
    }
    bool silence() const {
        auto l = lock();
        return this_log().m_bSilence;
    }

    unsigned num_threads() const {
        #ifndef USE_THREAD_LOCAL
            auto l = lock();
            return m_Threads.size();
        #endif
        return 0;
    }

    //void clear_threads() {
    //    auto l = lock();
    //    m_Threads.clear();
    //}
    
    void clear_indents() {
        auto l = lock();
        this_log().m_Indents = 0;
        optimize();
    }

    //boost::circular_buffer<Message>* getBuffer() { return &m_cbLog; }
    //const boost::circular_buffer<Message>* getBuffer_c() const { return &m_cbLog; }
    //std::string message(unsigned int idx) {
    //    if(idx < m_cbLog.size())
    //        return m_cbLog.at(idx).sMessage;
    //    return "";
    //}
    
    LogThread& this_log() const {
        #ifdef USE_THREAD_LOCAL
            //thread_local LogThread th;
            //return th;
        #else
            auto l = lock();
            return m_Threads[std::this_thread::get_id()];
        #endif
    }

private:
    
    void optimize() {
        #ifndef USE_THREAD_LOCAL
            //auto l = lock(); // already locked
            for(auto itr = m_Threads.begin();
                itr != m_Threads.end();)
            {
                if(itr->second.empty())
                    itr = m_Threads.erase(itr);
                else
                    ++itr;
            }
        #endif
    }

    //boost::circular_buffer<Message> m_cbLog;
    std::ofstream m_LogFile;
    //bool m_bSilence = false;
    //bool m_bCapture = false;
    //std::vector<std::string> m_Captured;

    static const int DEFAULT_LENGTH = 256;
    //unsigned m_Indent = 0;

    //std::unordered_map<std::thread::id, unsigned> m_Indents;
    #ifndef USE_THREAD_LOCAL
        mutable std::unordered_map<std::thread::id, LogThread> m_Threads;
    #endif
};

#ifdef DEBUG
    #define LOG_FORMAT boost::format("(%s:%s): %s")
    #define ERR_FORMAT boost::format("%s (%s:%s): %s")
    #define DEBUG_OPTIONAL(X) X
#else
    #define LOG_FORMAT boost::format("%s")
    #define ERR_FORMAT boost::format("%s: %s")
    #define DEBUG_OPTIONAL(X)
#endif

#define LOG(X) {\
    auto msg = (LOG_FORMAT %\
        DEBUG_OPTIONAL(__FILE__ % __LINE__ %)\
        std::string(X)\
    ).str();\
    Log::get().write(msg);\
}
#define LOGf(X,Y) {\
    auto msg = (LOG_FORMAT  %\
        DEBUG_OPTIONAL(__FILE__ % __LINE__ %)\
        (boost::format(X) % Y).str()\
    ).str();\
    Log::get().write(msg);\
}
#define WARNING(X) {\
    auto msg = (LOG_FORMAT %\
        DEBUG_OPTIONAL(__FILE__ % __LINE__ %)\
        std::string(X)\
    ).str();\
    Log::get().warn(msg);\
}
#define WARNINGf(X,Y) {\
    auto msg = (LOG_FORMAT %\
        DEBUG_OPTIONAL(__FILE__ % __LINE__ %)\
        (boost::format(X) % Y).str()\
    ).str();\
    Log::get().warn(msg);\
}
#define ERROR(CODE,X) {\
    auto msg = (ERR_FORMAT %\
        g_ErrorString[(unsigned)ErrorCode::CODE] %\
        DEBUG_OPTIONAL(__FILE__ % __LINE__ %)\
        std::string(X)\
    ).str();\
    Log::get().error(msg);\
    throw Error(ErrorCode::CODE, msg);\
}
#define ERRORf(CODE,X,Y) {\
    auto msg = (ERR_FORMAT %\
        g_ErrorString[(unsigned)ErrorCode::CODE] %\
        DEBUG_OPTIONAL(__FILE__ % __LINE__ %)\
        (boost::format(X) % Y).str()\
    ).str();\
    Log::get().error(msg);\
    throw Error(ErrorCode::CODE, msg);\
}

#ifdef DEBUG
    #define _()\
        Log::Indent _li(\
            std::string(BOOST_CURRENT_FUNCTION)+" {",\
            "}"\
        );\
        Log::Indent _li2;
#else
    #define _()
#endif

#endif

