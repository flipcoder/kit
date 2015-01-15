#ifndef _MATHCOMMON_H_INCLUDED
#define _MATHCOMMON_H_INCLUDED

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/transform2.hpp>

#define K_EPSILON 0.00001

#define K_PI 3.1415926535897932385
#define K_TAU 6.2831853071795864770

#define DEG2RAD(X) ((X)*K_TAU/360.0)
#define RAD2DEG(X) ((X)*360.0/K_TAU)
#define DEG2RADf(X) ((X)*((float)K_TAU)/360.0f)
#define RAD2DEGf(X) ((X)*360.0f/((float)K_TAU))

inline float sin_deg(float theta){
    return (sinf(theta*((float)K_TAU/360.0f)));
}
inline float cos_deg(float theta){
    return (cosf(theta*((float)K_TAU/360.0f)));
}
inline float tan_deg(float theta){
    return (tanf(theta*((float)K_TAU/360.0f)));
}

inline double sin_deg(double theta){
    return (sin(theta*(K_TAU/360.0)));
}
inline double cos_deg(double theta){
    return (cos(theta*(K_TAU/360.0)));
}
inline double tan_deg(double theta){
    return (tan(theta*(K_TAU/360.0)));
}

inline bool floatcmp(float a, float b){
    return(fabs(a-b) < K_EPSILON);
}

template <class T>
inline T abs(T x)
{
    return x>0?x:-x;
}

template<class T>
T sgn(T x) {
    return floatcmp(x,0.0)?0.0:(x>0.0?1.0:-1.0);
}

template <class T>
inline T max_val(T a, T b)
{
    return a>b?a:b;
}

// checks for absolute value, returns actual number
template<class T>
inline T max_abs(T a, T b)
{
    if(abs(a) > abs(b))
        return a;
    return b;
}

template<class T>
inline T min_abs(T a, T b)
{
    if(abs(a) < abs(b))
        return a;
    return b;
}


template<class T>
inline T clamp(T val, T low, T high)
{
    return val < low ? low : (val > high ? high : val);
}

#define sq(a) ((a)*(a))

//#ifdef _MATRIX_ROW_MAJOR
//    #define _MATRIX_INDEX(m,i,j) (m)[i][j]
//    #define _MATRIX_INDEX_1D(m,i,j) (m)[j*4+i]
//#else
//    #define _MATRIX_INDEX(m,i,j) (m)[j][i]
//    #define _MATRIX_INDEX_1D(m,i,j) (m)[i*4+j]
//#endif

inline bool IS_NAN(double f){ return f!=f; }
inline bool IS_NAN(float f){ return f!=f; }

namespace Axis{
    const glm::vec3 ZERO(0.0f, 0.0f, 0.0f);
    const glm::vec3 ALL(1.0f, 1.0f, 1.0f);
    const glm::vec3 X(1.0f, 0.0f, 0.0f);
    const glm::vec3 Y(0.0f, 1.0f, 0.0f);
    const glm::vec3 Z(0.0f, 0.0f, 1.0f);
    const glm::vec3 XY(1.0f, 1.0f, 0.0f);
    const glm::vec3 YZ(0.0f, 1.0f, 1.0f);
    const glm::vec3 XZ(1.0f, 0.0f, 1.0f);
    const glm::vec3 nX(1.0f, 0.0f, 0.0f);
    const glm::vec3 nY(0.0f, 1.0f, 0.0f);
    const glm::vec3 nZ(0.0f, 0.0f, 1.0f);
}

#include "matrixops.h"
#include "vectorops.h"

#endif

