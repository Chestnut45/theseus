//-----------------------------------------------------------------------------
// File:			W_SkinnedModel.h
// Original Author:	Gordon Wood
//
// Class representing a skinned model that can be loaded from a JSON file, specifying
// the FBX and the animation clips it contains
//-----------------------------------------------------------------------------
#ifndef W_SKINNING_MODEL_H
#define W_SKINNING_MODEL_H

#include "W_Model.h"

namespace wolf
{

class SkinnedModel : public Model
{
	public:
        SkinnedModel(const std::string& jsonFilePath, const MaterialProvider& matProvider = MaterialProvider());

        virtual void Update(float dt) override;
        void PlayClip(const std::string& name);

    protected:

        struct AnimClip
        {
            std::string name;
            float startFrame;
            float endFrame;

            AnimClip() : startFrame(0.0f), endFrame(0.0f) {}
            AnimClip(const std::string& _name, float _startFrame, float _endFrame) 
                : name(_name), startFrame(_startFrame), endFrame(_endFrame) {}
        };

        struct AnimState
        {
            float frame = 0.0f;
            const AnimClip* pClip;
        };

		void _readNodeHierarchy(const aiScene* pScene, MeshNode* pMeshNode, float animationTime, const aiNode* pNode, const aiMatrix4x4& parentTransform);
		const aiNodeAnim* _findNodeAnim(const aiAnimation* pAnimation, const std::string& nodeName);
		int _findBone(const std::string& name, const Mesh* pMesh);
		void _calcInterpolatedScaling(aiVector3D& out, float animationTime, const aiNodeAnim* pNodeAnim);
		void _calcInterpolatedPosition(aiVector3D& out, float animationTime, const aiNodeAnim* pNodeAnim);
		void _calcInterpolatedRotation(aiQuaternion& out, float animationTime, const aiNodeAnim* pNodeAnim);
		uint _findScaling(float animationTime, const aiNodeAnim* pNodeAnim);
		uint _findPosition(float animationTime, const aiNodeAnim* pNodeAnim);
		uint _findRotation(float animationTime, const aiNodeAnim* pNodeAnim);

        std::map<std::string,AnimClip> m_animClips;
        std::vector<AnimState> m_animStates; // one per m_meshNodes (not all necessarily will be used, but 1:1 mapping exists for simplicity)
};

}

#endif