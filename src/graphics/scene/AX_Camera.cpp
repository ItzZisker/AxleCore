#include "axle/graphics/scene/AX_Camera.hpp"

namespace axle::scene
{

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    SetPosition(position);
    SetDirection(glm::normalize(target - position), up);
    UpdateViewMatrix();
}

Camera::Camera(glm::vec3 position, float yaw, float pitch, glm::vec3 up)
    : Camera(position, glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
)) {}

Camera::Camera(glm::vec3 position, glm::vec3 target)
    : Camera(position, target, glm::vec3(0, 1, 0)) {}

Camera::Camera(glm::vec3 position, float yaw, float pitch)
    : Camera(position, yaw, pitch, glm::vec3(0, 1, 0)) {}

void Camera::UpdateViewMatrix() {
    m_ViewMatrix = glm::mat4_cast(glm::conjugate(m_Coords.GetRotation()));
    m_ViewMatrix = glm::translate(m_ViewMatrix, -m_Coords.GetPosition());
}

void Camera::SetPosition(const glm::vec3& position) {
    m_Coords.SetPosition(position);
    UpdateViewMatrix();
}

void Camera::SetDirection(const glm::vec3& direction, const glm::vec3 upHint) {
    m_Coords.SetRotationTowardsDirection(direction);
    UpdateViewMatrix();
}

glm::vec3 Camera::GetPosition() {
    return m_Coords.GetPosition();
}

glm::vec3 Camera::GetDirection() {
    return m_Coords.GetForward();
}

glm::vec3 Camera::GetUp() {
    return m_Coords.GetUp();
}

glm::mat4 Camera::GetTransform() {
    return m_Coords.GetTransform();
}

glm::mat4 Camera::GetViewMatrix() {
    return m_ViewMatrix;
}

}