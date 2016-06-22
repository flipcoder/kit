#ifndef VECTOROPS_H
#define VECTOROPS_H

////#include "common.h"
//#include <glm/glm.hpp>
//#include <glm/gtc/swizzle.hpp>
#include <string>

namespace Vector
{
//    inline bool isZero2(const glm::vec2& v) {
//        if( floatcmp(v.x, 0.0f) &&
//            floatcmp(v.y, 0.0f))
//            return true;
//        return false;
//    }
//    inline bool isZero(const glm::vec3& v) {
//        if( floatcmp(v.x, 0.0f) &&
//            floatcmp(v.y, 0.0f) &&
//            floatcmp(v.z, 0.0f))
//            return true;
//        return false;
//    }
    inline std::string to_string(const glm::vec2& v) {
        return std::string("(") +
            std::to_string(v.x) + ", " +
            std::to_string(v.y) + ")";
    }
    inline std::string to_string(const glm::vec3& v) {
        return std::string("(") +
            std::to_string(v.x) + ", " +
            std::to_string(v.y) + ", " +
            std::to_string(v.z) + ")";
    }
    inline std::string to_string(const glm::vec4& v) {
        return std::string("(") +
            std::to_string(v.x) + ", " +
            std::to_string(v.y) + ", " +
            std::to_string(v.z) + ", " +
            std::to_string(v.w) + ")";
    }
    
}

#endif
