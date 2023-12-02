#pragma once
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace glm {

inline quat Direction2Quaternion(vec3 direction) {
    direction = normalize(direction);
    float angle = acos(dot(direction, vec3(0, 0, 1)));
    return angleAxis(angle, cross(vec3(0, 0, 1), direction));
}

inline mat4x4 MakeAffineMatrix(const vec3 &translation, const quat &rotation, const vec3 &scale) {
	glm::mat4x4 matrix = glm::scale(glm::mat4_cast(rotation), scale);
    matrix = glm::translate(glm::identity<glm::mat4>(), translation) * matrix;
    return matrix;
}

inline void Quaternion2BasisAxis(quat q, vec3 &x, vec3 &y, vec3 &z) {
	q = normalize(q);
    mat3 matrix = mat3_cast(q);
    x = matrix[0];
    y = matrix[1];
    z = matrix[2];
}

// pitch, yaw, roll is degrees
inline quat MakeQuaternionByEuler(float pitch, float yaw, float roll) {
    return quat(glm::degrees(vec3(pitch, yaw, roll)));
}

}
