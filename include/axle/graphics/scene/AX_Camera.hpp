#pragma once

#include "axle/utils/AX_Coordination.hpp"

namespace axle::scene
{

class Camera {
public:
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up);
    Camera(glm::vec3 position, float yaw, float pitch, glm::vec3 up);
    Camera(glm::vec3 position, glm::vec3 target);
    Camera(glm::vec3 position, float yaw, float pitch);

    void SetPosition(const glm::vec3& position);
    void SetDirection(const glm::vec3& direction, const glm::vec3 upHint = glm::vec3(0, 1, 0));

    glm::vec3 GetPosition();
    glm::vec3 GetDirection();
    glm::vec3 GetUp();

    glm::mat4 GetTransform();
    glm::mat4 GetViewMatrix();

    void UpdateViewMatrix();
private:
    utils::Coordination m_Coords;
    glm::mat4 m_ViewMatrix;
};

}