#pragma once

#include "axle/core/concurrency/AX_ThreadCycler.hpp"

#include "axle/utils/AX_Coordination.hpp"

namespace axle::scene
{

class Camera : AX_THR_RENDER_OWNED {
private:
    utils::Coordination m_Coords;
    glm::mat4 m_ViewMatrix;
public:
    Camera(ThreadGfxScope gfxThread, glm::vec3 position, glm::vec3 target, glm::vec3 up);
    Camera(ThreadGfxScope gfxThread, glm::vec3 position, float yaw, float pitch, glm::vec3 up);
    Camera(ThreadGfxScope gfxThread, glm::vec3 position, glm::vec3 target);
    Camera(ThreadGfxScope gfxThread, glm::vec3 position, float yaw, float pitch);

    AX_NON_COPYABLE_NON_MOVABLE(Camera);

    ThreadInvocationVoid SetPosition(const glm::vec3& position);
    ThreadInvocationVoid SetDirection(const glm::vec3& direction, const glm::vec3 upHint = glm::vec3(0, 1, 0));
    
    ThreadInvocation<utils::Coordination> GetCoords();
    ThreadInvocation<glm::mat4> GetViewMatrix();

    ThreadInvocationVoid UpdateViewMatrix();
};

}