#ifndef _ANIMATION_H
#define _ANIMATION_H

#include <memory>
#include <functional>
#include <vector>
#include <deque>
#include <cmath>
#include <boost/signals2.hpp>
#include "../kit.h"
#include "freq.h"
#include "../math/common.h"

template<class T> class Animation;

namespace Interpolation {
    template<class T>
    T linear(T a, T b, float t) {
        return a + (b-a)*t;
    }
    template<class T>
    T out_sine(T a, T b, float t) {
        const float qtau = K_TAU / 4.0f;
        const float nt = sin(t * qtau);
        return linear(a,b,nt);
    }
    template<class T>
    T in_sine(T a, T b, float t) {
        const float qtau = K_TAU / 4.0f;
        const float nt = 1.0f + sin((t - 1.0f) * qtau);
        return linear(a,b,nt);
    }
    //(1-sqrt(x))|cos(x*2*tau)|
    template<class T, int bounces=2, int depth=3>
    T bounce(T a, T b, float t) {
        const float nt = 1.0f - (std::pow(1.0f-t, depth)) *
            std::fabs(std::cos(t * bounces * K_PI));
        return linear(a,b,nt);
    }
    //f(x) = sin(2pi*(.75x-0.25))+1.0
    template<class T>
    T exaggerate(T a, T b, float t) {
        float nt = std::sin(K_TAU*(0.75*t + 0.25)) + 1.0f;
        return linear(a,b,nt);
    }
}

#define INTERPOLATE(FUNC)\
    std::bind(\
        &Interpolation::FUNC,\
        std::placeholders::_1,\
        std::placeholders::_2,\
        std::placeholders::_3\
    )\

/*
 * Template class should be a floating point value, a (math) vector,
 * matrix, or quaternion.  But it can also be a color, or anything
 * interpolatable.
 *
 * You define the easing function in the form of:
 *     T func(const T&, const T&, float t)
 * Where t is a value that ranges from 0 to 1.
 *
 * Time is in seconds, but exists on a "timeline", so the actual real time
 * value can be whatever you want, depending on how fast you advance the
 * timeline.
 *
 * Negative time values are not yet supported.
 * I don't have derivative approximation yet, but I think it could be useful
 * for nonlinear easing functions when you need to also know the speed.
 */
template<class T>
class Frame
    //public IRealtime
{
    private:
        
        // TODO: execution callback?
        // TODO: expiry callback?
        // TODO: interpolative callback?
        T m_Value;

        Freq::Time m_Time; // length of time
        Freq::Alarm m_Alarm;

        std::function<T (T, T, float)> m_Easing;

        //Animation<T>* m_pAnimation;
        Freq::Timeline* m_pTimeline;

        std::shared_ptr<boost::signals2::signal<void()>> m_pCallback;
        
    public:
        
        Frame(
            T value,
            Freq::Time time = Freq::Time(0),
            std::function<T (T, T, float)> easing =
                std::function<T (T, T, float)>(),
            std::function<void()> cb = std::function<void()>(),
            Freq::Timeline* timeline = nullptr
        ):
            m_Value(value),
            m_Time(time),
            m_Easing(easing),
            m_pTimeline(timeline),
            m_Alarm(timeline),
            m_pCallback(std::make_shared<boost::signals2::signal<void()>>())
            //m_pAnimation(nav)
        {
            if(cb)
                m_pCallback->connect(std::move(cb));
            //assert(m_pTimeline);
        }
        //void nav(Animation<T>* nav) {
        //    assert(nav);
        //    m_pAnimation = nav;
        //}

        virtual ~Frame() {}
        void reset() {
            m_Alarm.set(m_Time);
        }

        template<class Func>
        void connect(Func func) {
            m_pCallback->connect(func);
        }
        
        std::shared_ptr<boost::signals2::signal<void()>> callback() {
            return m_pCallback;
        }
        void callback(std::function<void()> cb) {
            m_pCallback->connect(std::move(cb));
        }

        //virtual void logic(Freq::Time t) {
        //    //m_pTimeline->logic(t);
        //    // TODO: trigger callbacks here?
        //}
        
        virtual bool elapsed() {
            return m_Alarm.elapsed();
        }

        unsigned long remaining() {
            return m_Alarm.ms();
        }

        Freq::Alarm& alarm() { return m_Alarm; }

        T& value() {
            return m_Value;
        }

        void easing(std::function<T (T, T, float)> func) {
            m_Easing = func;
        }

        const std::function<T (T, T, float)>& easing() const {
            return m_Easing;
        }
        
        void timeline(Freq::Timeline* tl) {
            m_pTimeline = tl;
            m_Alarm.timer(m_pTimeline);
        }
        Freq::Timeline* timeline() { return m_pTimeline; }
};


class IAnimation
{
    public:
        virtual ~IAnimation() {}
};

template<class T>
class Animation:
    public IAnimation
{
    private:

        mutable std::deque<Frame<T>> m_Frames;
        mutable T m_Current = T();

        //float m_fSpeed = 1.0f;
        //Freq::Timeline* m_pTimeline = nullptr;
        Freq::Timeline m_Timeline;

        void reset(Freq::Time excess = Freq::Time(0)) const {
            if(m_Frames.empty())
                return;

            m_Frames.front().reset();
            m_Frames.front().alarm() += excess;
        }

        void process() const {
            while(!m_Frames.empty() && m_Frames.front().elapsed())
            {
                auto& frame = m_Frames.front();
                Freq::Time excess = frame.alarm().excess();
                m_Current = frame.value();
                std::shared_ptr<boost::signals2::signal<void()>> cb =
                    frame.callback();
                m_Frames.pop_front();
                reset(excess);
                (*cb)();
            }
        }
        
    public:
    
        Animation() = default;
        //Animation(T&& current):
        //    m_Current(current)
        //{}
        Animation& operator=(T&& a){
            m_Current = a;
            return *this;
        }
        virtual ~Animation() {}

        Frame<T>* first_frame() {
            if(!m_Frames.empty())
                return &m_Frames.front();
            return nullptr;
        }
        Frame<T>* last_frame() {
            if(!m_Frames.empty())
                return &m_Frames.back();
            return nullptr;
        }

        //Animation(
        //    Frame<T> initial,
        //    T current=T()
        //):
        //    m_Current(current)
        //    //m_fSpeed(1.0f)
        //{
        //    //initial.nav(this);
        //    initial.timeline(&m_Timeline);
        //    m_Frames.push_back(initial);
        //    reset();
        //}

        size_t size() const {
            return m_Frames.size();
        }
        
        void from(
            T current=T()
        ) {
            m_Current=current;
        }

        operator T() const {
            return get();
        }

        virtual void logic(Freq::Time t) {
            m_Timeline.logic(t);
        }

        void pause() {
            m_Timeline.pause();
        }
        void resume() {
            m_Timeline.resume();
        }

        /*
         * Append a frame to the end of the cycle
         */
        void frame(Frame<T> frame) {
            frame.timeline(&m_Timeline);
            m_Frames.push_back(frame);
            reset();
        }

        bool elapsed() const {
            process();
            return m_Frames.empty();
        }

        void abort(){
            m_Current = get();
            m_Frames.clear();
        }
        
        void do_callbacks()
        {
            for(auto&& frame: m_Frames)
            {
                auto cb = frame.callback();
                if(cb) (*cb)();
            }
        }
        void finish(){
            if(!m_Frames.empty()) {
                auto current = m_Frames.front().value();
                process();
                do_callbacks();
                m_Frames.clear();
                m_Current = current;
            }
        }
        void stop(T position = T()) {
            m_Current = position;
            m_Frames.clear();
        }
        void stop(
            T position,
            Freq::Time t,
            std::function<T (T, T, float)> easing,
            std::function<void()> cb = std::function<void()>()
        ){
            m_Current = t.ms() ? get() : position;
            m_Frames.clear();

            if(t.ms())
            {
                auto f = Frame<T>(position, t, easing);
                f.timeline(&m_Timeline);
                if(cb)
                    f.connect(cb);
                m_Frames.push_back(std::move(f));
                reset();
            }
        }

        void ensure(
            T position,
            Freq::Time t,
            std::function<T (T, T, float)> easing,
            std::function<bool (T, T)> equality_cmp
        ){
            process();

            if(equality_cmp(m_Current, position))
            {
                // TODO: if t < current frame time left
                //       use t instead
                return;
            }

            if(m_Frames.empty()) {
                stop(position,t,easing);
                return;
            }
        }

        T get() const {
            process();
            if(m_Frames.empty())
                return m_Current;

            return ease(
                m_Frames.front().easing(),
                m_Current,
                m_Frames.front().value(),
                m_Frames.front().alarm().fraction()
            );
        }

        T ease(
            const std::function<T (T, T, float)>& easing,
            T a,
            T b,
            float t
        ) const {
            return easing(a,b,t);
        }

        bool empty() const {
            return m_Frames.empty();
        }

        //void speed(float s) {
        //    m_fSpeed = s;
        //}
        //float speed() const {
        //    return m_fSpeed;
        //}

        //void timeline(Freq::Timeline* tl) {
        //    m_pTimeline = tl;
        //}
        Freq::Timeline* timeline() { return &m_Timeline; }
};

#endif

