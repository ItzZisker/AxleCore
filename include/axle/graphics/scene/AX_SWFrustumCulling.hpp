// #pragma once

// #include <glm/glm.hpp>

// namespace axle::scene
// {

// struct SWFrustumPlane {
//     glm::vec3 normal;
//     float distance;

//     SWFrustumPlane();
//     SWFrustumPlane(const glm::vec3& point, const glm::vec3& normalVec);

//     float GetSignedDistanceToPlane(const glm::vec3& point) const;
// };

// enum class SWFrustumFace {
//     Top, Bottom, Right, Left,
//     Far, Near
// };

// struct SWFrustum {
//     SWFrustumPlane faces[6];

//     inline SWFrustumPlane Face(const SWFrustumFace& face) {
//         return faces[static_cast<uint32_t>(face)];
//     }
// };

// class AABB {
// public:
//     glm::vec3 center {0.0f, 0.0f, 0.0f};
//     glm::vec3 extents {0.0f, 0.0f, 0.0f};

//     AABB(std::vector<glm::vec3> positions);
//     AABB(const glm::vec3& min, const glm::vec3& max);
//     AABB(const glm::vec3& inCenter, float iI, float iJ, float iK);

//     bool IsOnOrForwardPlane(const SWFrustumPlane& plane) const;
// };

// class SWDiscardable {
// public:
//     virtual bool DiscardOrNot(Scene_T snapshot, const glm::mat4& transform) = 0;
// };

// class SWFrustumDiscardable : public SWDiscardable {
// protected:
//     AABB bounding;
// public:
//     SWFrustumDiscardable(AABB bounding);
//     SWFrustumDiscardable();
//     ~SWFrustumDiscardable();

//     SWFrustum CreateFrustum(Scene* scene);
//     SWFrustum CreateFrustum(Scene_T snapshot);

//     bool IsInFrustum(const SWFrustum& frustum, const glm::mat4& transform);
//     bool IsInView(Scene_T snapshot, const glm::mat4& transform);
//     bool IsInView(Scene* scene, const glm::mat4& transform);

//     bool DiscardOrNot(Scene_T snapshot, const glm::mat4& transform) override;
//     bool DiscardOrNot(Scene* scene, const glm::mat4& transform);

//     AABB GetBounding();
// };

// }