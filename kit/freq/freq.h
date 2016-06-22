#ifndef _FREQ_H
#define _FREQ_H

#include <iostream>
#include <boost/signals2.hpp>
#include "../math/common.h"
#include "../kit.h"
#include "../log/log.h"

class Freq
{
public:

    class Time
    {
    public:
        unsigned int value;
        Time():
            value(0) {}
        explicit Time(unsigned int ms) {
            value = ms;
        }
        Time(const Time& t) = default;
        Time& operator=(const Time& t) = default;
        Time(Time&& t) = default;
        Time& operator=(Time&& t) = default;

        unsigned int internal() const { return value; }
        //static Time seconds(unsigned int s) { return Time(s * 1000);}
        static Time seconds(float s) { return Time((unsigned int)(s * 1000.0f)); }
        //static Time minutes(unsigned int m) { return Time(m * 60000);}
        static Time minutes(float m) { return Time((unsigned int)(m * 60000.0f));}
        static Time ms(unsigned int ms) { return Time(ms); }

        //operator bool() const { return value; }
        //operator float() const { return value / 1000.0f; }

        Time& operator+=(Time t) {
            value += t.internal();
            return *this;
        }
        bool operator>(Time t) const { return value > t.internal(); }
        bool operator<(Time t) const { return value < t.internal(); }
        bool operator>=(Time t) const { return value >= t.internal(); }
        bool operator<=(Time t) const { return value <= t.internal(); }
        operator bool() const {
            return value;
        }

        float s() const { return value / 1000.0f; }
        float seconds() const { return value / 1000.0f; }
        unsigned int ms() const { return value; }
    };

    class Timeline {
        private:
            //unsigned long m_ulPassedTime;
            unsigned long m_ulPassedTime;
            //unsigned int m_uiLastAdvance;
            float m_fSpeed;
        public:
            Timeline():
                m_ulPassedTime(0L),
                m_fSpeed(1.0f)
            {
                assert(m_ulPassedTime == 0L);
                assert(m_fSpeed == 1.0f);
            }
            virtual ~Timeline() {}

            Timeline(const Timeline&) = default;
            Timeline(Timeline&&) = default;
            Timeline& operator=(const Timeline&) = default;
            Timeline& operator=(Timeline&&) = default;
            
            virtual unsigned long ms() const {
                return m_ulPassedTime;
            }
            virtual float seconds() const {
                return m_ulPassedTime / 1000.0f;
            }
            
            /*virtual */Freq::Time logic(Freq::Time t) { // ms
                float advance = t.ms() * m_fSpeed;
                unsigned adv = kit::round_int(advance);
                //auto adv = std::rint(advance);
                //return Freq::Time::ms((unsigned long)(advance));
                //LOGf("passed time: %s", m_ulPassedTime);
                //LOGf("passed time += : %s", t.ms());
                m_ulPassedTime += adv;
                return Freq::Time::ms(adv);
            }
            //float logic(float a) { // seconds
            //    float advance = a * m_fSpeed;
            //    //m_uiLastAdvance = round_int(advance);
            //    m_ulPassedTime += std::rint(advance * 1000.0f);
            //    return advance;
            //}
            //unsigned int advance() const { return m_uiLastAdvance; }

            void speed(float s) {
                m_fSpeed = s;
            }
            float speed() const {
                return m_fSpeed;
            }
            void pause() {
                m_fSpeed = 0.0f;
            }
            void resume(float speed = 1.0f) {
                m_fSpeed = speed;
            }
            void reset() {
                m_ulPassedTime = 0L;
                m_fSpeed = 1.0f;
            }

            bool elapsed(Freq::Time value) {
                return m_ulPassedTime > value.ms();
            }
            Freq::Time age() const {
                return Freq::Time::ms(m_ulPassedTime);
            }
    };

    // eventually migrate to chrono
    class Alarm
    {
    protected:
    
        //Freq* m_pTimer;
        Timeline* m_pTimer;
        unsigned long m_ulAlarmTime;
        unsigned long m_ulStartTime;
        bool m_bStarted = false;

        std::shared_ptr<boost::signals2::signal<void()>> m_pCallback =
            std::make_shared<boost::signals2::signal<void()>>();
        
    public:
    
        //Alarm():
        //    m_ulAlarmTime(0L),
        //    m_ulStartTime(0L),
        //    m_pTimer(Freq::get().accumulator())
        //{
        //    assert(m_pTimer);
        //}

        Alarm() {}
        
        explicit Alarm(Timeline* timer):
            m_pTimer(timer),
            m_ulAlarmTime(0L),
            m_ulStartTime(0L)
        {
            //assert(m_pTimer);
        }

        Alarm(const Alarm&) = default;
        Alarm(Alarm&&) = default;
        Alarm& operator=(const Alarm&) = default;
        Alarm& operator=(Alarm&&) = default;

        explicit Alarm(Time t, Timeline* timer):
            m_pTimer(timer)
            //m_pTimer(timer ? timer : Freq::get().accumulator())
        {
            //assert(m_pTimer);
            set(t);
        }

        virtual ~Alarm() {}
        
        Alarm& operator+=(Freq::Time t) {
            m_ulAlarmTime += t.ms();
            return *this;
        }
        
        void reset() {
            m_ulAlarmTime = 0L;
            m_ulStartTime = 0L;
            m_bStarted = false;
        }

        bool has_timer() const { return (m_pTimer!=NULL); }
        
        void timer(Timeline* timerRef)
        {
            assert(timerRef);
            m_pTimer = timerRef;
        }
        const Timeline* timer() const { return m_pTimer; }
        Timeline* timer() { return m_pTimer; }
        
        void set(Time t, Timeline* timer = NULL)
        {
            if(timer)
                m_pTimer = timer;
            assert(m_pTimer);
            m_ulStartTime = m_pTimer->ms();
            m_ulAlarmTime = m_ulStartTime + t.ms();
            m_bStarted = true;
        }

        void delay(Time t) {
            assert(m_pTimer);
            m_ulAlarmTime += ((unsigned long)t.ms());
        }

        Freq::Time pause() {
            return Freq::Time(m_ulAlarmTime - m_ulStartTime);
        }

        void minutes(unsigned int m)
        {
           set(Time::minutes(m));
        }
        
        void seconds(unsigned int s)
        {
           set(Time::seconds(s));
        }

        void ms(unsigned int ms)
        {
           set(Time(ms));
        }
        
        unsigned long ms() const
        {
            assert(m_pTimer);
            if(!elapsed())
            {
                unsigned long t = (m_ulAlarmTime - m_pTimer->ms());
                return t;
            }
            return 0L;
        }
        
        float seconds() const
        {
            assert(m_pTimer);
            float t = (m_ulAlarmTime - m_pTimer->ms()) / 1000.0f;
            return t;
        }
        
        bool elapsed() const
        {
            if(not m_bStarted)
                return false;
            assert(m_pTimer);
            return (m_pTimer->ms() >= m_ulAlarmTime);
        }
        
        float fraction() const
        {
            return 1.0f - fraction_left();
        }

        Freq::Time excess() const {
            if(!elapsed())
                return Freq::Time(0);
            return Freq::Time(m_pTimer->ms() - m_ulAlarmTime);
        }

        void connect(std::function<void()> cb) {
            m_pCallback->connect(std::move(cb));
        }
        void poll() {
            if(elapsed()) {
                (*m_pCallback)();
                m_pCallback->disconnect_all_slots();
            }
        }
        
        float fraction_left() const
        {
            if(elapsed())
                return 0.0f;

            unsigned long remaining = ms();
            unsigned long range = m_ulAlarmTime - m_ulStartTime;
            if(floatcmp(range, 0.0f))
                return 0.0f;
            return (float)remaining / (float)range;
        }

        bool started() const {
            return m_bStarted;
        }

        //unsigned long endTickTime() { return m_ulAlarmTime; }
    };

    template<class T>
    class Timed : public Freq::Alarm
    {
    protected:
        Time m_Length;
        T m_Start;
        T m_End;
    public:
        //Timed():
        //    Alarm()
        //{
        //    m_Length = Time(0);
        //}
        //Timed(Time t, T start, T end):
        //    Alarm()
        //{
        //    m_Length = Time(t);
        //    set(t, start, end);
        //}
        explicit Timed(Timeline* timer):
            Alarm(timer),
            m_Length(Time(0))
        {}

        Timed(const Timed&) = default;
        Timed(Timed&&) = default;
        Timed& operator=(const Timed&) = default;
        Timed& operator=(Timed&&) = default;

        //Timed(const Timed<T>& t) {
        //    m_Start = t.start();
        //    m_End = t.end();
        //    m_Length = t.length();
        //}
        virtual ~Timed() {}
        T get() const{
            return m_Start + (m_End - m_Start) * fraction();
        }
        T inverse() const {
            return m_End - (m_End - m_Start) * fraction();
        }
        T start() const{
            return m_Start;
        }
        T end() const {
            return m_End;
        }
        T diff() const {
            return m_End - m_Start;
        }
        void restart() {
            static_cast<Alarm*>(this)->set(m_Length);
        }
        void set(Time t, T start, T end) {
            m_Start = start;
            m_End = end;
            m_Length = Time(t);
            static_cast<Alarm*>(this)->set(t);
        }
        void clear(T val) {
            m_Start = m_End = val;
            m_Length = Time(0);
            static_cast<Alarm*>(this)->set(Time(m_Length));
        }
        void shift(T val){
            //m_Start = m_End = (m_End + val);
            //m_Length = Time(0);
            //static_cast<Alarm*>(this)->set(Time(m_Length));

            m_Start += val;
            m_End += val;
        }
        void finish(){
            m_Start = m_End;
            static_cast<Alarm*>(this)->set(Time(0));
        }
        void reverse(){
            std::swap(m_Start, m_End);
            static_cast<Alarm*>(this)->set(m_Length);
        }
    };

private:

    Timeline m_globalTimeline;
    //unsigned long m_uiLastMark;
    unsigned long m_ulStartTime;
    unsigned int m_uiMinTick;
    
public:

    Freq():
        m_ulStartTime(get_ticks()),
        m_uiMinTick(1)
    {}
    
    unsigned long get_ticks() const {
        return SDL_GetTicks();
    }
    float get_seconds() const {
        return SDL_GetTicks() * 0.001f;
    }
    
    // TODO: not yet implemented
    //void pause();
    //void unpause();
    
    //bool tick();

    // accumulates time passed, until enough has passed to advance forward
    // may advance more than 1 frame's worth of time
    // returns # of ms to advance
    Freq::Time tick() {
        unsigned long ticks = get_ticks() - m_ulStartTime;
        //LOGf("start time: %s", m_ulStartTime);
        //LOGf("get_ticks(): %s", get_ticks());
        //LOGf("ticks: %s", ticks);
        //LOGf("global timeline: %s", m_globalTimeline.ms());
        unsigned int advance = (unsigned int)(ticks - m_globalTimeline.ms());
        //LOGf("advance: %s", advance);
        if(advance >= m_uiMinTick) {
            m_globalTimeline.logic(Freq::Time::ms(advance));
            //LOGf("advance post-logic: %s", advance);
            return Freq::Time::ms(advance);
        }
        return Freq::Time::ms(0);
    }

    Timeline* timeline() { return &m_globalTimeline; }
};

#endif

