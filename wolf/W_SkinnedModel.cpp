//-----------------------------------------------------------------------------
// File:			W_SkinnedModel.cpp
// Original Author:	Gordon Wood
//
// See header for details
//-----------------------------------------------------------------------------
#include "W_SkinnedModel.h"
#include "W_Common.h"
#include "tinyjson.hpp"

static inline glm::mat4 mat4_cast(const aiMatrix4x4 &m) { return glm::transpose(glm::make_mat4(&m.a1)); }
static inline glm::mat4 mat4_cast(const aiMatrix3x3 &m) { return glm::transpose(glm::make_mat3(&m.a1)); }

namespace wolf
{

SkinnedModel::SkinnedModel(const std::string& jsonFilePath, const MaterialProvider& matProvider)
{
    char* jsonString = (char*) wolf::LoadWholeFile(jsonFilePath);
    tiny::TinyJson json;
    if(json.ReadJson(jsonString))
    {
        std::string fbxFile = json.Get<std::string>("fbx");
        tiny::xarray clips = json.Get<tiny::xarray>("clips");

        _loadModel(std::string("data/") + fbxFile, matProvider);

        for(int i = 0; i < clips.Count(); ++i)
        {
            clips.Enter(i);

            std::string name = clips.Get<std::string>("name");
            m_animClips[name] = AnimClip( name, (float)clips.Get<int>("start"), (float)clips.Get<int>("end"));
        }
    }

    m_animStates.reserve(m_meshNodes.size());
    for(int i = 0; i < m_meshNodes.size(); ++i)
    {
        m_animStates.push_back( AnimState() );
    }
    delete[] jsonString;

    PlayClip(m_animClips.begin()->second.name);
}

void SkinnedModel::Update(float dt)
{        
    Model::Update(dt);

    for(int meshNodeIdx = 0; meshNodeIdx < m_meshNodes.size(); ++meshNodeIdx)
    {
        MeshNode* pMeshNode = &m_meshNodes[meshNodeIdx];
        AnimState* pAnimState = &m_animStates[meshNodeIdx];

        if(pMeshNode->m_pMesh->m_bones.size() > 0)
        {
            pAnimState->frame += 1.0f;

            while(pAnimState->frame >= pAnimState->pClip->endFrame)
                pAnimState->frame -= (pAnimState->pClip->endFrame - pAnimState->pClip->startFrame);

            aiMatrix4x4 rootTransform;
            _readNodeHierarchy(m_pImportedModel, pMeshNode, pAnimState->frame, m_pImportedModel->mRootNode, rootTransform);
        }
    }
}

void SkinnedModel::_readNodeHierarchy(const aiScene* pScene, MeshNode* pMeshNode, float animationTime, const aiNode* pNode, const aiMatrix4x4& parentTransform)
{ 
    std::string NodeName(pNode->mName.data);

    const aiAnimation* pAnimation = pScene->mAnimations[0];

    aiMatrix4x4 NodeTransformation(pNode->mTransformation);

    const aiNodeAnim* pNodeAnim = _findNodeAnim(pAnimation, NodeName);

    int boneIdx = _findBone(NodeName, pMeshNode->m_pMesh);

    if (pNodeAnim) 
    {
        // Interpolate scaling and generate scaling transformation matrix
        aiVector3D Scaling;
        _calcInterpolatedScaling(Scaling, animationTime, pNodeAnim);
        aiMatrix4x4 ScalingM;
        aiMatrix4x4::Scaling(Scaling, ScalingM);

        // Interpolate rotation and generate rotation transformation matrix
        aiQuaternion RotationQ;
        _calcInterpolatedRotation(RotationQ, animationTime, pNodeAnim); 
        aiMatrix4x4 RotationM = aiMatrix4x4(RotationQ.GetMatrix());

        // Interpolate translation and generate translation transformation matrix
        aiVector3D Translation;
        _calcInterpolatedPosition(Translation, animationTime, pNodeAnim);
        aiMatrix4x4 TranslationM;
        aiMatrix4x4::Translation(Translation, TranslationM);

        // Combine the above transformations
        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    aiMatrix4x4 GlobalTransformation = parentTransform * NodeTransformation;

    if (boneIdx != -1)
    {
        aiMatrix4x4 m_GlobalInverseTransform = pScene->mRootNode->mTransformation;
        m_GlobalInverseTransform.Inverse();

        aiMatrix4x4 t = parentTransform * NodeTransformation * pMeshNode->m_pMesh->m_bones[boneIdx].bindPoseMatrix;
        pMeshNode->m_boneTransforms[boneIdx] = mat4_cast(t);
    }

    for (uint i = 0 ; i < pNode->mNumChildren ; i++) {
        _readNodeHierarchy(pScene, pMeshNode, animationTime, pNode->mChildren[i], GlobalTransformation);
    }
}

const aiNodeAnim* SkinnedModel::_findNodeAnim(const aiAnimation* pAnimation, const std::string& nodeName)
{
    for (uint i = 0 ; i < pAnimation->mNumChannels ; i++) 
    {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
        
        if (std::string(pNodeAnim->mNodeName.data) == nodeName) 
        {
            return pNodeAnim;
        }
    }
    
    return NULL;
}

int SkinnedModel::_findBone(const std::string& name, const Model::Mesh* pMesh)
{
    for(int i = 0; i < pMesh->m_bones.size(); i++)
    {
        if(pMesh->m_bones[i].name == name)
            return i;
    }
    return -1;
}

void SkinnedModel::_calcInterpolatedScaling(aiVector3D& out, float animationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumScalingKeys == 1) {
        out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    uint ScalingIndex = _findScaling(animationTime, pNodeAnim);
    uint NextScalingIndex = (ScalingIndex + 1);
    if(NextScalingIndex >= pNodeAnim->mNumScalingKeys)
        NextScalingIndex = pNodeAnim->mNumScalingKeys - 1;
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
    float Factor = DeltaTime > 0.0f ? (animationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime : 1.0f;
    if(Factor < 0.0f)
        Factor = 0.0f;
    if(Factor > 1.0f)
        Factor = 1.0f;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    out = Start + Factor * Delta;
}

void SkinnedModel::_calcInterpolatedPosition(aiVector3D& out, float animationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumPositionKeys == 1) {
        out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }
            
    uint PositionIndex = _findPosition(animationTime, pNodeAnim);
    uint NextPositionIndex = (PositionIndex + 1);
    if(NextPositionIndex >= pNodeAnim->mNumPositionKeys)
        NextPositionIndex = pNodeAnim->mNumPositionKeys - 1;
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
    float Factor = DeltaTime > 0.0f ? (animationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime : 1.0f;
    if(Factor < 0.0f)
        Factor = 0.0f;
    if(Factor > 1.0f)
        Factor = 1.0f;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    out = Start + Factor * Delta;
}

void SkinnedModel::_calcInterpolatedRotation(aiQuaternion& out, float animationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }
    
    uint RotationIndex = _findRotation(animationTime, pNodeAnim);
    uint NextRotationIndex = (RotationIndex + 1);
    if(NextRotationIndex >= pNodeAnim->mNumRotationKeys)
        NextRotationIndex = pNodeAnim->mNumRotationKeys - 1;
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
    float Factor = DeltaTime > 0.0f ? (animationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime : 1.0f;
    if(Factor < 0.0f)
        Factor = 0.0f;
    if(Factor > 1.0f)
        Factor = 1.0f;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& EndRotationQ   = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;    
    aiQuaternion::Interpolate(out, StartRotationQ, EndRotationQ, Factor);
    out = out.Normalize();
}

uint SkinnedModel::_findScaling(float animationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);
    
    for (uint i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
        if (animationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
            return i;
        }
    }
    
    return pNodeAnim->mNumScalingKeys - 1;
}

uint SkinnedModel::_findPosition(float animationTime, const aiNodeAnim* pNodeAnim)
{    
    for (uint i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
        if (animationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
            return i;
        }
    }
    
    return pNodeAnim->mNumPositionKeys - 1;
}

uint SkinnedModel::_findRotation(float animationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (uint i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
        if (animationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
            return i;
        }
    }
    
    return pNodeAnim->mNumRotationKeys - 1;
}

void SkinnedModel::PlayClip(const std::string& name)
{
    if(m_animClips.find(name) != m_animClips.end())
    {
        for(int i = 0; i < m_meshNodes.size(); ++i)
        {
            AnimState* pAnimState = &m_animStates[i];

            pAnimState->pClip = &m_animClips[name];
            pAnimState->frame = pAnimState->pClip->startFrame;
        }
    }
}

}