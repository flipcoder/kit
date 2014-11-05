#ifndef _ANGLE_H
#define _ANGLE_H

#include "common.h"
#include <glm/gtx/vector_angle.hpp>

class Angle
{

    float m_fDeg;

    public:
        
        enum Type{
            DEGREES,
            RADIANS
            //TURNS
        };

        Angle(float a = 0.0f, Type t = DEGREES)
        {
            m_fDeg = t==DEGREES ? a : RAD2DEGf(a);
            wrap();
        }

        //Angle(const glm::vec2& v) {
        //    m_fDeg = glm::angle(Axis::X, glm::normalize(v));
        //}
        
        glm::vec2 vector() const {
            return glm::vec2(cos(), sin());
        }

        //static Angle between(const Angle& a, const Angle& b) {
        //    return b-a;
        //}
        
        static Angle degrees(float deg) {
            return Angle(deg, DEGREES);
        }
        static Angle radians(float rad) {
            return Angle(rad, RADIANS);
        }
        static Angle turns(float tau) {
            return Angle(tau/360.0f, DEGREES);
        }

        void wrap()
        {
            while(m_fDeg >= 180.0f)
                m_fDeg -= 360.0f;
            while(m_fDeg < -180.0f)
                m_fDeg += 360.0f;
        }

        glm::vec2 vector(float mag = 1.0f) {
            float rad = DEG2RADf(m_fDeg);
            return glm::vec2(
                mag * std::cos(rad),
                mag * std::sin(rad)
            );
        }

        float cos() const {
            return std::cos(DEG2RADf(m_fDeg));
        }
        float sin() const {
            return std::sin(DEG2RADf(m_fDeg));
        }

        Angle flip() {
            return *this+Angle::degrees(180.0f);
        }

        float degrees() const { return m_fDeg; }
        float radians() const { return DEG2RADf(m_fDeg); }
        //void degrees(float deg) {
        //    m_fDeg = deg; 
        //    wrap();
        //}
        //void radians(float rad) {
        //    m_fDeg = RAD2DEGf(rad);
        //    wrap();
        //}

        Angle operator +(const Angle& a) const{
            return Angle(m_fDeg + a.degrees());
        }
        const Angle& operator +=(const Angle& a) {
            return (*this = *this+a);
        }
        Angle operator -(const Angle& a) const{
            return Angle(m_fDeg - a.degrees());
        }
        Angle operator -() const {
            return Angle(-m_fDeg);
        }
        Angle operator *(float f) const{
            return Angle(m_fDeg * f);
        }
        const Angle& operator*=(float f) {
            return (*this = *this*f);
        }
        bool operator==(const Angle& a) const {
            return floatcmp(m_fDeg, a.degrees());
        }

        bool operator==(float f) const {
            return floatcmp(m_fDeg, Angle::degrees(f).degrees());
        }
        bool operator!=(float f) const {
            return !floatcmp(m_fDeg, Angle::degrees(f).degrees());
        }
        const Angle& operator=(float f) {
            m_fDeg = f;
            wrap();
            return *this;
        }

        bool operator>(float f) const {
            return degrees() > f;
        }
        bool operator>=(float f) const {
            return degrees() >= f;
        }
        bool operator<(float f) const {
            return degrees() < f;
        }
        bool operator<=(float f) const {
            return degrees() <= f;
        }

        bool operator>(const Angle& a) const {
            return degrees() > a.degrees();
        }
        bool operator>=(const Angle& a) const {
            return degrees() >= a.degrees();
        }
        bool operator<(const Angle& a) const {
            return degrees() < a.degrees();
        }
        bool operator<=(const Angle& a) const {
            return degrees() <= a.degrees();
        }
};

#endif

