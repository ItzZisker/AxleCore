#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace axle::utils {

class Coordination {
private:
    static constexpr float EPS = 1e-7f;

    glm::mat4 m_S{1.0f};
    glm::mat4 m_R{1.0f};
    glm::mat4 m_T{1.0f};
    glm::mat4 m_M{1.0f}; // (M)odel = (T)ranslation * (R)otation * (S)cale

    glm::vec3 m_Position{0.0f};
    glm::quat m_Rotation{1,0,0,0};
    glm::vec3 m_Scale{1.0f};

    static glm::vec3 SafeNormalize(const glm::vec3& v, const glm::vec3& fallback);
public:
    static glm::vec3 DirectionOf(float yaw, float pitch);

    explicit Coordination(const glm::mat4& transform);
    Coordination(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
    Coordination();

    const glm::mat4& GetScaleMatrix()       const { return m_S; }
    const glm::mat4& GetRotationMatrix()    const { return m_R; }
    const glm::mat4& GetTranslationMatrix() const { return m_T; }
    const glm::mat4& GetTransform()         const { return m_M; }

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::quat& GetRotation() const { return m_Rotation; }
    const glm::vec3& GetScale()    const { return m_Scale; }

    glm::vec3 GetRight()   const { return glm::normalize(glm::vec3(m_R[0])); } // +X
    glm::vec3 GetUp()      const { return glm::normalize(glm::vec3(m_R[1])); } // +Y
    glm::vec3 GetForward() const { return glm::normalize(glm::vec3(m_R[2])); } // +Z (glTF default)

    virtual void UpdateTransform();

    // Expect pure scale matrix (diagonal). We read the diagonal signs too.
    void SetScaleMatrix(const glm::mat4& S);
    void SetRotationMatrix(const glm::mat4& R);
    void SetTranslationMatrix(const glm::mat4& T);

    // Convenience setters (TRS values)
    void SetPosition(const glm::vec3& p);
    void AddPosition(const glm::vec3& dp);

    void SetScale(const glm::vec3& s);
    void SetUniformScale(float s);

    void SetRotation(const glm::quat& q);
    // Euler angles in radians, applied X->Y->Z (pitch, yaw, roll)
    void SetRotationEulerXYZ(const glm::vec3& eulerRadians);
    // Look/aim the +Z axis towards a direction (glTF-style forward)
    void SetRotationTowardsDirection(const glm::vec3& direction, const glm::vec3& upHint = glm::vec3(0, 1, 0));
    void SetRotationTowardsDirection_Z(const glm::vec3& dir);
    void SetRotationTowardsDirection_Y(const glm::vec3& dir);
    void SetRotationTowardsDirection_Z_KeepUp(const glm::vec3& dir, const glm::vec3& worldUp = glm::vec3(0, 1, 0));

    void SetTRS(const glm::vec3& p, const glm::quat& q, const glm::vec3& s);
    void SetMatrices(const glm::mat4& T, const glm::mat4& R, const glm::mat4& S);

    // Decompose a combined transform (works for typical glTF TRS, incl. non-uniform and negative scale)
    void SetFromTransform(const glm::mat4& M);
    void Decompose(const glm::mat4& M);
    void SetIdentity();
};

}