#ifndef _ANGLE_H
#define _ANGLE_H

#include "common.h"
#include <glm/gtx/vector_angle.hpp>

#ifndef K_ANGLE_UNIT
#define K_ANGLE_UNIT TURNS
#endif

class Angle
{

    float m_fTurns;

    public:
        
        enum Type{
            TURNS = 0,
            DEGREES,
            RADIANS
        };

        Angle(float a = 0.0f, Type t = K_ANGLE_UNIT)
        {
            switch(t){
                case DEGREES:
                    m_fTurns = a;
                    break;
                case RADIANS:
                    m_fTurns = RAD2DEGf(a);
                    break;
                case TURNS:
                    m_fTurns = RAD2DEGf(a*K_TAU);
                    break;
                default:
                    assert(false);
            }
            wrap();
        }

        //Angle(const glm::vec2& v) {
        //    m_fTurns = glm::angle(Axis::X, glm::normalize(v));
        //}
        
        glm::vec2 vector() const {
            return glm::vec2(Angle::cos(), Angle::sin());
        }

        //static Angle between(const Angle& a, const Angle& b) {
        //    return b-a;
        //}
        
        static Angle degrees(float deg) {
            return Angle(deg, DEGREES);
        }
        static Angle radians(float rad) {
            return Angle(RAD2DEGf(rad), RADIANS);
        }
        static Angle turns(float t) {
            return Angle(t * 360.0f, DEGREES);
        }

        void wrap()
        {
            while(m_fTurns >= 0.5f)
                m_fTurns -= 1.0f;
            while(m_fTurns < -0.5f)
                m_fTurns += 1.0f;
        }

        glm::vec2 vector(float mag = 1.0f) {
            float rad = m_fTurns*K_TAU;
            return glm::vec2(
                mag * std::cos(rad),
                mag * std::sin(rad)
            );
        }

        float cos() const {
            return std::cos(DEG2RADf(m_fTurns));
        }
        float sin() const {
            return std::sin(DEG2RADf(m_fTurns));
        }

        Angle flip() const {
            return *this+Angle::turns(0.5f);
        }

        float degrees() const { return m_fTurns*360.0f; }
        float radians() const { return m_fTurns*K_TAU; }
        float turns() const { return DEG2RADf(m_fTurns*K_TAU); }
        //void degrees(float deg) {
        //    m_fTurns = deg;
        //    wrap();
        //}
        //void radians(float rad) {
        //    m_fTurns = RAD2DEGf(rad);
        //    wrap();
        //}

        Angle operator +(const Angle& a) const{
            return Angle(m_fTurns + a.turns());
        }
        const Angle& operator +=(const Angle& a) {
            return (*this = *this+a);
        }
        Angle operator -(const Angle& a) const{
            return Angle(m_fTurns - a.turns());
        }
        Angle operator -() const {
            return Angle(-m_fTurns);
        }
        Angle operator *(float f) const{
            return Angle(m_fTurns * f);
        }
        const Angle& operator*=(float f) {
            return (*this = *this*f);
        }
        bool operator==(const Angle& a) const {
            return floatcmp(m_fTurns, a.turns());
        }

        bool operator==(float f) const {
            return floatcmp(m_fTurns, Angle::turns(f).turns());
        }
        bool operator!=(float f) const {
            return !floatcmp(m_fTurns, Angle::turns(f).turns());
        }
        const Angle& operator=(float f) {
            m_fTurns = f;
            wrap();
            return *this;
        }

        bool operator>(float f) const {
            return turns() > f;
        }
        bool operator>=(float f) const {
            return turns() >= f;
        }
        bool operator<(float f) const {
            return turns() < f;
        }
        bool operator<=(float f) const {
            return turns() <= f;
        }

        bool operator>(const Angle& a) const {
            return turns() > a.turns();
        }
        bool operator>=(const Angle& a) const {
            return turns() >= a.turns();
        }
        bool operator<(const Angle& a) const {
            return turns() < a.turns();
        }
        bool operator<=(const Angle& a) const {
            return turns() <= a.turns();
        }
};

#endif

