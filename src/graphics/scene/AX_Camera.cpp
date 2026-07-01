#include "axle/graphics/scene/AX_Camera.hpp"

namespace axle::scene
{

Camera::Camera(ThreadGfxScope gfxThread, glm::vec3 position, glm::vec3 target, glm::vec3 up)
    : ThreadOwned(gfxThread) {
    core::InstaFutureOrQueue(*gfxThread, [&](){
        SetPosition(position);
        SetDirection(glm::normalize(target - position), up);
        UpdateViewMatrix();
        return 0;
    }).get();
}

Camera::Camera(ThreadGfxScope gfxThread, glm::vec3 position, float yaw, float pitch, glm::vec3 up)
    : Camera(gfxThread, position, glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
)) {}

Camera::Camera(ThreadGfxScope gfxThread, glm::vec3 position, glm::vec3 target)
    : Camera(gfxThread, position, target, glm::vec3(0, 1, 0)) {}

Camera::Camera(ThreadGfxScope gfxThread, glm::vec3 position, float yaw, float pitch)
    : Camera(gfxThread, position, yaw, pitch, glm::vec3(0, 1, 0)) {}

ThreadInvocationVoid Camera::SetPosition(const glm::vec3& position) {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_Coords.SetPosition(position);
        return VoidInvoke{};
    });
}

ThreadInvocationVoid Camera::SetDirection(const glm::vec3& direction, const glm::vec3 upHint) {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_Coords.SetRotationTowardsDirection(direction, upHint);
        return VoidInvoke{};
    });
}

ThreadInvocationVoid Camera::UpdateViewMatrix() {
    return ThreadInvocationVoid(m_Thread, [&](){
        m_ViewMatrix = glm::mat4_cast(glm::conjugate(m_Coords.GetRotation()));
        m_ViewMatrix = glm::translate(m_ViewMatrix, -m_Coords.GetPosition());
        return VoidInvoke{};
    });
}

ThreadInvocation<glm::mat4> Camera::GetViewMatrix() {
    return ThreadInvocation<glm::mat4>(m_Thread, [&](){
        return m_ViewMatrix;
    });
}

ThreadInvocation<utils::Coordination> Camera::GetCoords() {
    return ThreadInvocation<utils::Coordination>(m_Thread, [&](){
        return m_Coords;
    });
}

}