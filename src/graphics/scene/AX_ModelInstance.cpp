#include "axle/graphics/scene/AX_ModelInstance.hpp"

// TODO: This

namespace axle::scene
{

ModelInstance::ModelInstance(const ModelDesc& desc) {

}

void ModelInstance::PushLeafParents(assets::NodeId, std::vector<assets::NodeId> parentList) {

}

void ModelInstance::ClearCachedTransforms() {

}

void ModelInstance::ClearLeafParents() {

}

void ModelInstance::ClearDiscarded() {

}

glm::mat4 ModelInstance::GetWorldTransform(assets::NodeId leafInstance, bool cacheFinalTransform = true) {

}

glm::mat4 ModelInstance::GetCachedWorldTransform(assets::NodeId leafInstance) {

}

bool ModelInstance::IsTransformCached(assets::NodeId leafInstance) {

}

void ModelInstance::SetDiscard(assets::NodeId nodeInstance, bool discard) {

}

NodeInstance& ModelInstance::GetRoot() const {

}

ModelDesc& ModelInstance::GetModelDesc() const {

}

}