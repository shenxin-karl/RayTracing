#pragma once
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <fmt/format.h>

namespace glm {

inline quat Direction2LookAtQuaternion(vec3 direction, glm::vec3 up = glm::vec3(0.f, 1.f, 0.f)) {
    direction = normalize(direction);
    up = normalize(up);
	mat4x4 view = lookAt(glm::vec3(0.f), direction, up);
    return quat_cast(view);
}

inline vec3 LookAtQuaternion2Direction(quat rotation) {
	rotation = normalize(rotation);
    mat3 matrix = mat3_cast(rotation);
    return vec3(matrix[0][2], matrix[1][2], matrix[2][2]);
}


inline mat4x4 MakeAffineMatrix(const vec3 &translation, const quat &rotation, const vec3 &scale) {
	glm::mat4x4 matrix = glm::scale(glm::mat4_cast(rotation), scale);
    matrix = glm::translate(glm::identity<glm::mat4>(), translation) * matrix;
    return matrix;
}

inline void Quaternion2BasisAxis(quat q, vec3 &x, vec3 &y, vec3 &z) {
	q = normalize(q);
    mat3 matrix = mat3_cast(q);
    x = vec3(matrix[0][0], matrix[1][0], matrix[2][0]);
    y = vec3(matrix[0][1], matrix[1][1], matrix[2][1]);
    z = vec3(matrix[0][2], matrix[1][2], matrix[2][2]);
}

struct BasisAxis {
	vec3 x;
    vec3 y;
    vec3 z;
};

inline BasisAxis Quaternion2BasisAxis(quat q) {
	BasisAxis ret;
    Quaternion2BasisAxis(q, ret.x, ret.y, ret.z);
    return ret;
}

// pitch, yaw, roll is degrees
inline quat MakeQuaternionByEuler(float pitch, float yaw, float roll) {
    return quat(glm::degrees(vec3(pitch, yaw, roll)));
}

inline mat4x4 WorldMatrixToNormalMatrix(const mat4x4 &world) {
	return mat4x4(transpose(inverse(mat3x3(world))));
}

}

template<>
struct fmt::formatter<glm::vec2> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }
    template<typename FormatContext>
    auto format(const glm::vec2 &v, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};

template<>
struct fmt::formatter<glm::vec3> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }
    template<typename FormatContext>
    auto format(const glm::vec3 &v, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};

template<>
struct fmt::formatter<glm::vec4> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }
    template<typename FormatContext>
    auto format(const glm::vec4 &v, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "({}, {}, {}, {})", v.x, v.y, v.z, v.w);
    }
};