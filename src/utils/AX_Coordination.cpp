#include "axle/utils/AX_Coordination.hpp"

#include <glm/gtx/euler_angles.hpp>

namespace axle::utils
{

glm::vec3 Coordination::SafeNormalize(const glm::vec3& v, const glm::vec3& fallback) {
    float len2 = glm::dot(v, v);
    return (len2 > EPS * EPS) ? v / glm::sqrt(len2) : fallback;
}

glm::vec3 Coordination::DirectionOf(float yaw, float pitch) {
    return glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    );
}

Coordination::Coordination(const glm::mat4& transform) {
    SetFromTransform(transform);
}

Coordination::Coordination(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale) {
    SetTRS(position, rotation, scale);
}

Coordination::Coordination() {
    SetIdentity();
}

void Coordination::UpdateTransform() {
    m_M = m_T * m_R * m_S;
}

// Expect pure scale matrix (diagonal). We read the diagonal signs too.
void Coordination::SetScaleMatrix(const glm::mat4& S) {
    m_Scale = glm::vec3(S[0][0], S[1][1], S[2][2]);
    m_S = glm::scale(glm::mat4(1.0f), m_Scale);
    UpdateTransform();
}

void Coordination::SetRotationMatrix(const glm::mat4& R) {
    // Extract and orthonormalize upper-left 3x3 to ensure a proper rotation (no scale/shear)
    glm::vec3 r = glm::vec3(R[0]);
    glm::vec3 u = glm::vec3(R[1]);
    glm::vec3 f = glm::vec3(R[2]);

    // Gram-Schmidt to get a clean right-handed basis
    r = SafeNormalize(r, glm::vec3(1, 0, 0));
    u = SafeNormalize(u - r * glm::dot(u, r), glm::vec3(0, 1, 0));
    f = glm::normalize(glm::cross(r, u));   // ensure right-handed: r x u = f

    glm::mat4 pure(1.0f);
    pure[0] = glm::vec4(r, 0.0f);
    pure[1] = glm::vec4(u, 0.0f);
    pure[2] = glm::vec4(f, 0.0f);
    m_R = pure;

    m_Rotation = glm::normalize(glm::quat_cast(glm::mat3(m_R)));
    UpdateTransform();
}

void Coordination::SetTranslationMatrix(const glm::mat4& T) {
    m_Position = glm::vec3(T[3]);
    m_T = glm::translate(glm::mat4(1.0f), m_Position);
    UpdateTransform();
}

// Convenience setters (TRS values)
void Coordination::SetPosition(const glm::vec3& p) {
    m_Position = p;
    m_T = glm::translate(glm::mat4(1.0f), m_Position);
    UpdateTransform();
}

void Coordination::AddPosition(const glm::vec3& dp) {
    SetPosition(m_Position + dp);
}

void Coordination::SetScale(const glm::vec3& s) {
    m_Scale = s;
    m_S = glm::scale(glm::mat4(1.0f), m_Scale);
    UpdateTransform();
}

void Coordination::SetUniformScale(float s) {
    SetScale(glm::vec3(s));
}

void Coordination::SetRotation(const glm::quat& q) {
    m_Rotation = glm::normalize(q);
    m_R = glm::toMat4(m_Rotation);
    UpdateTransform();
}

//Euler angles in radians, applied X->Y->Z (pitch, yaw, roll)
void Coordination::SetRotationEulerXYZ(const glm::vec3& eulerRadians) {
    m_R = glm::eulerAngleXYZ(eulerRadians.x, eulerRadians.y, eulerRadians.z);
    m_Rotation = glm::normalize(glm::quat_cast(glm::mat3(m_R)));
    UpdateTransform();
}

// Look/aim the +Z axis towards a direction (glTF-style forward)
void Coordination::SetRotationTowardsDirection(const glm::vec3& direction, const glm::vec3& upHint) {
    glm::vec3 f = SafeNormalize(direction, glm::vec3(0, 0, 1));
    glm::vec3 u = SafeNormalize(upHint, glm::vec3(0, 1, 0));

    // If direction and up are nearly parallel, choose a fallback up
    if (std::abs(glm::dot(f, u)) > 0.999f) {
        u = (std::abs(f.y) > 0.9f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    }

    glm::vec3 r = glm::normalize(glm::cross(u, f)); // right
    glm::vec3 cu = glm::cross(f, r); // corrected up

    glm::mat4 R(1.0f);
    R[0] = glm::vec4(r, 0.0f);
    R[1] = glm::vec4(cu, 0.0f);
    R[2] = glm::vec4(f, 0.0f);

    SetRotationMatrix(R); // will update quaternion and M
}

void Coordination::SetRotationTowardsDirection_Z(const glm::vec3& dir) {
    if (glm::length2(dir) < 1e-10f) return;
    glm::vec3 f = glm::normalize(dir);
    glm::quat q = glm::rotation(glm::vec3(0, 0, 1), f); // rotate +Z to f
    SetRotation(q);
}

void Coordination::SetRotationTowardsDirection_Y(const glm::vec3& dir) {
    if (glm::length2(dir) < 1e-10f) return;
    glm::vec3 f = glm::normalize(dir);
    glm::quat q = glm::rotation(glm::vec3(0, 1, 0), f); // rotate +Y to f
    SetRotation(q);
}

void Coordination::SetRotationTowardsDirection_Z_KeepUp(const glm::vec3& dir, const glm::vec3& worldUp) {
    glm::vec3 f = glm::normalize(dir);
    if (glm::length2(f) < 1e-12f) return;

    glm::vec3 u = worldUp;
    if (std::abs(glm::dot(f, u)) > 0.999f) {
        u = (std::abs(f.y) > 0.9f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    }
    glm::vec3 r  = glm::normalize(glm::cross(u, f));
    glm::vec3 cu = glm::cross(f, r);

    glm::mat4 R(1.0f);
    R[0] = glm::vec4(r, 0.0f);
    R[1] = glm::vec4(cu, 0.0f);
    R[2] = glm::vec4(f, 0.0f);
    SetRotationMatrix(R); // Coordination setter
}

// Combined setters
void Coordination::SetTRS(const glm::vec3& p, const glm::quat& q, const glm::vec3& s) {
    m_Position = p;
    m_Rotation = glm::normalize(q);
    m_Scale = s;

    m_T = glm::translate(glm::mat4(1.0f), m_Position);
    m_R = glm::toMat4(m_Rotation);
    m_S = glm::scale(glm::mat4(1.0f), m_Scale);

    UpdateTransform();
}

void Coordination::SetMatrices(const glm::mat4& T, const glm::mat4& R, const glm::mat4& S) {
    SetTranslationMatrix(T);
    SetRotationMatrix(R);
    SetScaleMatrix(S);
}

// Decompose a combined transform (works for typical glTF TRS, incl. non-uniform and negative scale)
void Coordination::SetFromTransform(const glm::mat4& M) {
    Decompose(M);
}

void Coordination::Decompose(const glm::mat4& M) {
    // Translation
    m_Position = glm::vec3(M[3]);
    m_T = glm::translate(glm::mat4(1.0f), m_Position);

    // Extract columns (basis with scale)
    glm::vec3 col0 = glm::vec3(M[0]);
    glm::vec3 col1 = glm::vec3(M[1]);
    glm::vec3 col2 = glm::vec3(M[2]);

    float sx = glm::length(col0);
    float sy = glm::length(col1);
    float sz = glm::length(col2);

    // Avoid divide-by-zero; if scale is zero on an axis, pick a fallback basis axis
    if (sx > EPS) col0 /= sx; else { col0 = glm::vec3(1,0,0); sx = 0.0f; }
    if (sy > EPS) col1 /= sy; else { col1 = glm::vec3(0,1,0); sy = 0.0f; }
    if (sz > EPS) col2 /= sz; else { col2 = glm::vec3(0,0,1); sz = 0.0f; }

    // Ensure right-handed rotation (move any reflection into scale)
    glm::mat3 R3(col0, col1, col2);
    if (glm::determinant(R3) < 0.0f) {
        // Flip one axis. we flip Z.
        sz = -sz;
        col2 = -col2;
        R3 = glm::mat3(col0, col1, col2);
    }

    m_Rotation = glm::normalize(glm::quat_cast(R3));
    m_R = glm::toMat4(m_Rotation);

    m_Scale = glm::vec3(sx, sy, sz);
    m_S = glm::scale(glm::mat4(1.0f), m_Scale);

    UpdateTransform();
}

void Coordination::SetIdentity() {
    m_Position = glm::vec3(0.0f);
    m_Rotation = glm::quat(1, 0, 0, 0);
    m_Scale    = glm::vec3(1.0f);

    m_T = glm::mat4(1.0f);
    m_R = glm::mat4(1.0f);
    m_S = glm::mat4(1.0f);
    UpdateTransform();
}

}