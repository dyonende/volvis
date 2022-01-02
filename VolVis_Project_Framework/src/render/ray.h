#pragma once
#include <glm/vec3.hpp>

namespace render {

// This is a ray data struct, used for raytracing (using rays)
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float tmin, tmax;
};

}