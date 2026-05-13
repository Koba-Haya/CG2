#include "AnimationManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>

std::shared_ptr<Animation> AnimationManager::LoadAnimation(const std::string& directoryPath, const std::string& filename) {
  std::string filePath = directoryPath + "/" + filename;
  
  if (cache_.find(filePath) != cache_.end()) {
    return cache_[filePath];
  }

  Assimp::Importer importer;
  const unsigned flags = aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder;
  const aiScene* scene = importer.ReadFile(filePath.c_str(), flags);
  assert(scene);
  if (scene->mNumAnimations == 0) return nullptr;

  aiAnimation* animationAssimp = scene->mAnimations[0];
  auto animation = std::make_shared<Animation>();
  
  float ticksPerSecond = static_cast<float>(animationAssimp->mTicksPerSecond);
  if (ticksPerSecond == 0.0f) {
    ticksPerSecond = 1.0f; // GLTF defaults to seconds
  }
  
  animation->duration = static_cast<float>(animationAssimp->mDuration) / ticksPerSecond;

  for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
    aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
    NodeAnimation& nodeAnimation = animation->nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

    for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
      aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
      KeyframeVector3 keyframe;
      keyframe.time = static_cast<float>(keyAssimp.mTime) / ticksPerSecond;
      keyframe.value = {keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z};
      nodeAnimation.translate.keyframes.push_back(keyframe);
    }

    for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
      aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
      KeyframeQuaternion keyframe;
      keyframe.time = static_cast<float>(keyAssimp.mTime) / ticksPerSecond;
      keyframe.value = {keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z, keyAssimp.mValue.w};
      nodeAnimation.rotate.keyframes.push_back(keyframe);
    }

    for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
      aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
      KeyframeVector3 keyframe;
      keyframe.time = static_cast<float>(keyAssimp.mTime) / ticksPerSecond;
      keyframe.value = {keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z};
      nodeAnimation.scale.keyframes.push_back(keyframe);
    }
  }

  cache_[filePath] = animation;
  return animation;
}
