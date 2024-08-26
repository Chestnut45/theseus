//-----------------------------------------------------------------------------
// File:			W_Model.cpp
// Original Author:	Gordon Wood
//
// See header for notes
//-----------------------------------------------------------------------------
#include "W_Model.h"
#include "W_Common.h"
#include "W_BufferManager.h"
#include "W_MaterialManager.h"

namespace wolf
{

static ComponentType gs_aPODTypeMap[] = 
{
    wolf::CT_Invalid,   //EPODDataNone,
	wolf::CT_Float,		//EPODDataFloat,
	wolf::CT_Int,		//EPODDataInt,
	wolf::CT_UShort,	//EPODDataUnsignedShort,
	wolf::CT_UByte4,	//EPODDataRGBA,
	wolf::CT_UByte4,	//EPODDataARGB,
	wolf::CT_UByte4,	//EPODDataD3DCOLOR,
	wolf::CT_UByte4,	//EPODDataUBYTE4,
	wolf::CT_Invalid,	//EPODDataDEC3N,
	wolf::CT_Invalid,	//EPODDataFixed16_16,
	wolf::CT_UByte,		//EPODDataUnsignedByte,
	wolf::CT_Short,		//EPODDataShort,
	wolf::CT_ShortNorm,	//EPODDataShortNorm,
	wolf::CT_Byte,		//EPODDataByte,
	wolf::CT_ByteNorm,	//EPODDataByteNorm,
	wolf::CT_UByteNorm, //EPODDataUnsignedByteNorm,
	wolf::CT_UShortNorm, //EPODDataUnsignedShortNorm,
	wolf::CT_UInt,		//EPODDataUnsignedInt
};

//----------------------------------------------------------
// Constructor
//----------------------------------------------------------
Model::Model(const std::string& path, const MaterialProvider& matProvider) 
{
    _loadModel(path, matProvider);
}

Model::Model() 
{
}

void Model::_loadModel(const std::string& path, const MaterialProvider& matProvider)
{
    m_path = path;
    m_importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
    m_pImportedModel = m_importer.ReadFile(path.c_str(),
                                         aiProcess_Triangulate |
                                         aiProcess_OptimizeMeshes |
                                         aiProcess_TransformUVCoords |
                                         aiProcess_CalcTangentSpace |
                                         aiProcess_LimitBoneWeights |
                                         aiProcess_JoinIdenticalVertices);

    // Build all the meshes
	for(unsigned int i = 0; i < m_pImportedModel->mNumMeshes; i++)
	{
		Mesh mesh;
		const aiMesh* pSrcMesh = m_pImportedModel->mMeshes[i];

        _precacheBoneVertexData(pSrcMesh, &mesh);
		Model::InterleavedMesh* pInterleavedMesh = _interleaveMeshData(pSrcMesh);

		// Create the vertex declaration
		wolf::VertexDeclaration* pDecl = new wolf::VertexDeclaration;
		pDecl->Begin();

        // Create the vertex buffer
		wolf::VertexBuffer* pVB = wolf::BufferManager::CreateVertexBuffer(pInterleavedMesh->pInterleavedData, pInterleavedMesh->interleavedDataSize);
        
		// Create the index buffer
		wolf::IndexBuffer* pIB = wolf::BufferManager::CreateIndexBuffer(pInterleavedMesh->pIndexData, pInterleavedMesh->numIndices);

		// We'll always have a position
		pDecl->AppendAttribute(wolf::AT_Position, 3, wolf::CT_Float, pInterleavedMesh->posOffset);

		if( pInterleavedMesh->normalOffset >= 0 )
			pDecl->AppendAttribute(wolf::AT_Normal, 3, wolf::CT_Float, pInterleavedMesh->normalOffset);

		if( pInterleavedMesh->tangentOffset >= 0 )
			pDecl->AppendAttribute(wolf::AT_Tangent, 3, wolf::CT_Float, pInterleavedMesh->tangentOffset);

		if( pInterleavedMesh->bitangentOffset >= 0 )
			pDecl->AppendAttribute(wolf::AT_BiTangent, 3, wolf::CT_Float, pInterleavedMesh->bitangentOffset);

		if( pInterleavedMesh->numColorChannels > 0 )
			pDecl->AppendAttribute(wolf::AT_Color, 4, wolf::CT_UByte, pInterleavedMesh->pColorOffsets[0]);

		for( int x = 0; x < pInterleavedMesh->numUVChannels; x++ )
			pDecl->AppendAttribute((wolf::Attribute)(wolf::AT_TexCoord1 + x), pInterleavedMesh->pUVSizes[x], wolf::CT_Float, pInterleavedMesh->pUVOffsets[x]);

        if( pInterleavedMesh->boneIndicesOffset >= 0)
        {
            pDecl->AppendAttribute(wolf::AT_BoneIndices, 1, wolf::CT_UByte4, pInterleavedMesh->boneIndicesOffset);
            pDecl->AppendAttribute(wolf::AT_BoneWeights, 4, wolf::CT_Float, pInterleavedMesh->boneWeightsOffset);
        }

		pDecl->SetVertexBuffer(pVB);
		pDecl->SetIndexBuffer(pIB);
		pDecl->End(pInterleavedMesh->vertexStride);

		// Add them to our list of meshes
		mesh.m_pVB = pVB;
		mesh.m_pIB = pIB;
		mesh.m_pDecl = pDecl;
		mesh.m_numTriangles = pSrcMesh->mNumFaces;
        mesh.m_minPos = pInterleavedMesh->boundsMin;
        mesh.m_maxPos = pInterleavedMesh->boundsMax;
		m_meshes.push_back(mesh);

        if(m_pBoneVertMappings)
            delete[] m_pBoneVertMappings;
	}

    _addMeshesFromNode(m_pImportedModel, m_pImportedModel->mRootNode, aiMatrix4x4(), matProvider);
}

//----------------------------------------------------------
// Destructor
//----------------------------------------------------------
Model::~Model()
{
	for(unsigned int i = 0; i < m_meshes.size(); i++)
	{
		const Mesh& m = m_meshes[i];
		wolf::BufferManager::DestroyBuffer(m.m_pVB);
		wolf::BufferManager::DestroyBuffer(m.m_pIB);
		delete m.m_pDecl;
	}

	for(unsigned int i = 0; i < m_meshes.size(); i++)
    {
        const MeshNode& meshNode = m_meshNodes[i];
        if(meshNode.m_ownMaterial)
            wolf::MaterialManager::DestroyMaterial(meshNode.m_pMat);
    }
}

void Model::Update(float dt)
{
}

//----------------------------------------------------------
// Renders this model
//----------------------------------------------------------
void Model::Render(const glm::mat4& worldMatrix, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
{
	for(int i = 0; i < m_meshNodes.size(); i++)
	{
		const MeshNode& m = m_meshNodes[i];
        glm::mat4 world = worldMatrix * m.m_transform;

        wolf::Material* pMat = m.m_pMatOverride ? m.m_pMatOverride : m.m_pMat;
		pMat->SetUniform("projection", projMatrix);
		pMat->SetUniform("view", viewMatrix);
		pMat->SetUniform("world", world);
		pMat->SetUniform("worldIT", glm::transpose(glm::inverse(world)));

        if(m.m_pMesh->m_bones.size() > 0)
            pMat->SetUniform("u_bones", m.m_boneTransforms);
		pMat->Apply();

		m.m_pMesh->m_pDecl->Bind();
		glDrawElements(GL_TRIANGLES, m.m_pMesh->m_numTriangles * 3, GL_UNSIGNED_SHORT, 0);
	}
}

void Model::OverrideMaterial(wolf::Material *pMat)
{
	for(int i = 0; i < m_meshNodes.size(); i++)
	{
		MeshNode& m = m_meshNodes[i];
		m.m_pMatOverride = pMat;
	}
}

void Model::RemoveMaterialOverride()
{
	for(int i = 0; i < m_meshNodes.size(); i++)
	{
		MeshNode& m = m_meshNodes[i];
		m.m_pMatOverride = nullptr;
	}
}

int Model::_getInterleavedVertexSize(const aiMesh* pSrcMesh)
{
    int total = sizeof(aiVector3D); // assuming presence of positions
    // Only support one color channel right now (to keep wolf simple)
    total += (pSrcMesh->GetNumColorChannels() > 0 ? 1 : 0) * 4;
    total += pSrcMesh->HasNormals() ? sizeof(aiVector3D) : 0;
    total += pSrcMesh->HasTangentsAndBitangents() ? sizeof(aiVector3D) * 2 : 0;
    for(unsigned int i = 0; i < pSrcMesh->GetNumUVChannels(); ++i)
    {
        total += pSrcMesh->mNumUVComponents[i] * sizeof(float);
    }
    total += pSrcMesh->HasBones() ? 20 /* 4 bytes + 4 floats */: 0;

    return total;
}

void Model::_precacheBoneVertexData(const aiMesh* pSrcMesh, Mesh* pMesh)
{
    if(!pSrcMesh->HasBones())
        return;

    m_pBoneVertMappings = new BoneVertMapping[pSrcMesh->mNumVertices];

    for(int boneIdx = 0; boneIdx < pSrcMesh->mNumBones; boneIdx++) 
    {
        aiBone* pSrcBone = pSrcMesh->mBones[boneIdx];

        for(int boneWeightIdx = 0; boneWeightIdx < pSrcBone->mNumWeights; boneWeightIdx++)
        {
            int vertIdx = pSrcBone->mWeights[boneWeightIdx].mVertexId;
            float weight = pSrcBone->mWeights[boneWeightIdx].mWeight;

            if(m_pBoneVertMappings[vertIdx].numBones == 4)
            {
                printf("Error: Too many bones influencing a single vertex!\n");
                exit(1);
            }

            int numBones = m_pBoneVertMappings[vertIdx].numBones;
            m_pBoneVertMappings[vertIdx].boneIndices[numBones] = boneIdx;
            m_pBoneVertMappings[vertIdx].boneWeights[numBones] = weight;
            m_pBoneVertMappings[vertIdx].numBones++;
        }

        Bone bone;
        bone.name = pSrcBone->mName.data;
        bone.bindPoseMatrix = pSrcBone->mOffsetMatrix;
        pMesh->m_bones.push_back(bone);
    }
}

Model::InterleavedMesh* Model::_interleaveMeshData(const aiMesh* pSrcMesh)
{
    glm::vec3 minPos(1000000.0f, 1000000.0f, 1000000.0f);
    glm::vec3 maxPos(-1000000.0f, -1000000.0f, -1000000.0f);

    Model::InterleavedMesh* pOutMesh = new Model::InterleavedMesh();
    pOutMesh->vertexStride = _getInterleavedVertexSize(pSrcMesh);
    pOutMesh->interleavedDataSize = pOutMesh->vertexStride * pSrcMesh->mNumVertices;
    pOutMesh->pInterleavedData = new unsigned char[pOutMesh->interleavedDataSize];
    pOutMesh->numIndices = pSrcMesh->mNumFaces * 3;
    pOutMesh->pIndexData = new unsigned short[pOutMesh->numIndices];

    unsigned char* pCursor = (unsigned char*)pOutMesh->pInterleavedData;
    int vertOffset = 0;

    for(unsigned int vertIdx = 0; vertIdx < pSrcMesh->mNumVertices; ++vertIdx)
    {
        bool firstVert = vertIdx == 0;
        aiVector3D* pPos = (aiVector3D*)pCursor;
        *pPos = pSrcMesh->mVertices[vertIdx];
        pCursor += sizeof(aiVector3D);

        if(pPos->x > maxPos.x)
            maxPos.x = pPos->x;
        else if(pPos->x < minPos.x)
            minPos.x = pPos->x;

        if(pPos->y > maxPos.y)
            maxPos.y = pPos->y;
        else if(pPos->y < minPos.y)
            minPos.y = pPos->y;

        if(pPos->z > maxPos.z)
            maxPos.z = pPos->z;
        else if(pPos->z < minPos.z)
            minPos.z = pPos->z;

        if(firstVert)
        {
            pOutMesh->posOffset = vertOffset;
            vertOffset += sizeof(aiVector3D);
        }

        if(pSrcMesh->HasNormals())
        {
            aiVector3D* pNormal = (aiVector3D*)pCursor;
            *pNormal = pSrcMesh->mNormals[vertIdx];
            pCursor += sizeof(aiVector3D);

            if(firstVert)
            {
                pOutMesh->normalOffset = vertOffset;
                vertOffset += sizeof(aiVector3D);
            }
        }

        if(pSrcMesh->HasTangentsAndBitangents())
        {
            aiVector3D* pTangent = (aiVector3D*)pCursor;
            *pTangent = pSrcMesh->mTangents[vertIdx];
            pCursor += sizeof(aiVector3D);

            aiVector3D* pBiTangent = (aiVector3D*)pCursor;
            *pBiTangent = pSrcMesh->mBitangents[vertIdx];
            pCursor += sizeof(aiVector3D);

            if(firstVert)
            {
                pOutMesh->tangentOffset = vertOffset;
                vertOffset += sizeof(aiVector3D);
                pOutMesh->bitangentOffset = vertOffset;
                vertOffset += sizeof(aiVector3D);
            }
        }

        // only support one color channel right now (would require changing the "push" attr location mapping
        // to "pull" which complicates early learning)
        const int numColorChannels = pSrcMesh->GetNumColorChannels() > 0 ? 1 : 0;

        for(int c = 0; c < numColorChannels; ++c)
        {
            pCursor[0] = (unsigned char) (pSrcMesh->mColors[c][vertIdx].r * 255.0f);
            pCursor[1] = (unsigned char) (pSrcMesh->mColors[c][vertIdx].g * 255.0f);
            pCursor[2] = (unsigned char) (pSrcMesh->mColors[c][vertIdx].b * 255.0f);
            pCursor[3] = (unsigned char) (pSrcMesh->mColors[c][vertIdx].a * 255.0f);
            pCursor += 4;

            if(firstVert)
            {
                if( c == 0)
                {
                    pOutMesh->numColorChannels = pSrcMesh->GetNumColorChannels();
                    pOutMesh->pColorOffsets = new int[pOutMesh->numColorChannels];
                }

                pOutMesh->pColorOffsets[c] = vertOffset;
                vertOffset += 4;
            }
        }

        for(unsigned int uvSet = 0; uvSet < pSrcMesh->GetNumUVChannels(); ++uvSet)
        {
            if(firstVert)
            {
                if( uvSet == 0)
                {
                    pOutMesh->numUVChannels = pSrcMesh->GetNumUVChannels();
                    pOutMesh->pUVOffsets = new int[pOutMesh->numUVChannels];
                    pOutMesh->pUVSizes = new int[pOutMesh->numUVChannels];
                }
            }

            float* pUV = (float*)pCursor;
            switch(pSrcMesh->mNumUVComponents[uvSet])
            {
                case 1:
                    pUV[0] = pSrcMesh->mTextureCoords[uvSet][vertIdx].x;
                    pCursor += sizeof(float);
                    if(firstVert)
                    {
                        pOutMesh->pUVOffsets[uvSet] = vertOffset;
                        pOutMesh->pUVSizes[uvSet] = 1;
                        vertOffset += sizeof(float);
                    }
                    break;
                case 2:
                    pUV[0] = pSrcMesh->mTextureCoords[uvSet][vertIdx].x;
                    pUV[1] = pSrcMesh->mTextureCoords[uvSet][vertIdx].y;
                    pCursor += sizeof(float) * 2;
                    if(firstVert)
                    {
                        pOutMesh->pUVOffsets[uvSet] = vertOffset;
                        pOutMesh->pUVSizes[uvSet] = 2;
                        vertOffset += sizeof(float) * 2;
                    }
                    break;
                case 3:
                    pUV[0] = pSrcMesh->mTextureCoords[uvSet][vertIdx].x;
                    pUV[1] = pSrcMesh->mTextureCoords[uvSet][vertIdx].y;
                    pUV[2] = pSrcMesh->mTextureCoords[uvSet][vertIdx].z;
                    pCursor += sizeof(float) * 3;
                    if(firstVert)
                    {
                        pOutMesh->pUVOffsets[uvSet] = vertOffset;
                        pOutMesh->pUVSizes[uvSet] = 3;
                        vertOffset += sizeof(float) * 3;
                    }
                    break;
            }
        }

        if(pSrcMesh->HasBones())
        {
            unsigned char* pBoneIndices = (unsigned char*)pCursor;
            BoneVertMapping* pSrcVert = &m_pBoneVertMappings[vertIdx];

            pBoneIndices[0] = (unsigned char) pSrcVert->boneIndices[0];
            pBoneIndices[1] = (unsigned char) pSrcVert->boneIndices[1];
            pBoneIndices[2] = (unsigned char) pSrcVert->boneIndices[2];
            pBoneIndices[3] = (unsigned char) pSrcVert->boneIndices[3];
            pCursor += 4;

            float* pBoneWeights = (float*)pCursor;
            pBoneWeights[0] = pSrcVert->boneWeights[0];
            pBoneWeights[1] = pSrcVert->boneWeights[1];
            pBoneWeights[2] = pSrcVert->boneWeights[2];
            pBoneWeights[3] = pSrcVert->boneWeights[3];
            pCursor += 4 * sizeof(float);

            if(firstVert)
            {
                pOutMesh->boneIndicesOffset = vertOffset;
                vertOffset += 4;
                pOutMesh->boneWeightsOffset = vertOffset;
                vertOffset += 4 * sizeof(float);
            }
        }
    }

    for(unsigned int i = 0; i < pSrcMesh->mNumFaces; ++i)
    {
        aiFace* pFace = &pSrcMesh->mFaces[i];
        assert(pFace->mNumIndices == 3);
        pOutMesh->pIndexData[i*3+0] = pFace->mIndices[0];
        pOutMesh->pIndexData[i*3+1] = pFace->mIndices[1];
        pOutMesh->pIndexData[i*3+2] = pFace->mIndices[2];
    }

    pOutMesh->boundsMin = minPos;
    pOutMesh->boundsMax = maxPos;
    return pOutMesh;
}

glm::vec3 Model::getAABBMin() const
{
    glm::vec3 minPos(1000000.0f, 1000000.0f, 1000000.0f);
    for(int i = 0; i < m_meshes.size(); ++i)
    {
        const Mesh& mesh = m_meshes[i];

        if(mesh.m_minPos.x < minPos.x)
            minPos.x = mesh.m_minPos.x;

        if(mesh.m_minPos.y < minPos.y)
            minPos.y = mesh.m_minPos.y;

        if(mesh.m_minPos.z < minPos.z)
            minPos.z = mesh.m_minPos.z;
    }
    return minPos;
}

glm::vec3 Model::getAABBMax() const
{
    glm::vec3 maxPos(-1000000.0f, -1000000.0f, -1000000.0f);
    for(int i = 0; i < m_meshes.size(); ++i)
    {
        const Mesh& mesh = m_meshes[i];

        if(mesh.m_maxPos.x > maxPos.x)
            maxPos.x = mesh.m_maxPos.x;

        if(mesh.m_maxPos.y > maxPos.y)
            maxPos.y = mesh.m_maxPos.y;

        if(mesh.m_maxPos.z > maxPos.z)
            maxPos.z = mesh.m_maxPos.z;
    }
    return maxPos;
}

glm::vec3 Model::getAABBCenter() const
{
    glm::vec3 min = getAABBMin();
    glm::vec3 max = getAABBMax();
    return min + ((max - min) * 0.5f);
}

void Model::_addMeshesFromNode(const aiScene* pScene, const aiNode* pNode, const aiMatrix4x4& parentTransform, const MaterialProvider& matProvider)
{
    aiMatrix4x4 t = pNode->mTransformation * parentTransform;

    for(unsigned int i = 0; i < pNode->mNumMeshes; ++i)
    {
        int meshIdx = pNode->mMeshes[i];
        aiMesh* pSrcMesh = pScene->mMeshes[meshIdx];
        
        MeshNode meshNode;
        meshNode.m_name = pNode->mName.C_Str();
        meshNode.m_pMesh = &m_meshes[meshIdx];
        meshNode.m_transform = glm::mat4( t.a1,t.b1,t.c1,t.d1,
                                          t.a2,t.b2,t.c2,t.d2,
                                          t.a3,t.b3,t.c3,t.d3,
                                          t.a4,t.b4,t.c4,t.d4);

        aiString matName;
        aiMaterial* pMat = pScene->mMaterials[pSrcMesh->mMaterialIndex];
		pMat->Get(AI_MATKEY_NAME, matName);

        meshNode.m_pMat = matProvider.getMaterial(pNode->mName.C_Str(), i, matName.C_Str()); 
        meshNode.m_ownMaterial = false;

        if(!meshNode.m_pMat)
        {
            if(m_createdMaterials.find(pSrcMesh->mMaterialIndex) == m_createdMaterials.end())
                m_createdMaterials[pSrcMesh->mMaterialIndex] = _createMaterialFromAssimpMaterial(pMat);

            meshNode.m_pMat = m_createdMaterials[pSrcMesh->mMaterialIndex];
            meshNode.m_ownMaterial = true;
        }

        if(meshNode.m_pMesh->m_bones.size() > 0)
        {
            meshNode.m_boneTransforms.reserve(meshNode.m_pMesh->m_bones.size());
            for(int b = 0; b < meshNode.m_pMesh->m_bones.size(); ++b)
                meshNode.m_boneTransforms.push_back(glm::mat4(1.0f));
        }
        m_meshNodes.push_back(meshNode);
    }

    for(unsigned int i = 0; i < pNode->mNumChildren; ++i)
    {
        _addMeshesFromNode(pScene, pNode->mChildren[i], t, matProvider);
    }
}

wolf::Color4 _getColorRGBA(const aiMaterial* pMat, 
                           const char* key, 
                           int type, 
                           int idx, 
                           float defaultR = 1.0f, 
                           float defaultG = 1.0f, 
                           float defaultB = 1.0f, 
                           float defaultA = 1.0f)
{
    aiColor3D c(defaultR,defaultG,defaultB);
    pMat->Get(key, type, idx, c);
    return wolf::Color4(c.r, c.g, c.b, 1.0f);
}

float _getFloat(const aiMaterial* pMat, const char* key, int type, int idx, float defaultVal = 0.0f)
{
    float f = defaultVal;
    pMat->Get(key, type, idx, f);
    return f;
}

wolf::Texture* _getTexture(const aiMaterial* pMat, aiTextureType texType)
{
    aiString path; 
    aiReturn ret = pMat->GetTexture(texType, 0, &path);
    if (ret == AI_SUCCESS)
        return wolf::TextureManager::CreateTexture(std::string("data/") + path.C_Str());

    return nullptr;
}

wolf::Material* Model::_createMaterialFromAssimpMaterial(const aiMaterial* pAssimpMat)
{
    aiString name;
    pAssimpMat->Get(AI_MATKEY_NAME,name);
    wolf::Material *pMat = wolf::MaterialManager::CreateMaterial(m_path + "/" + name.C_Str());
    pMat->SetProgram("data/uber.vsh", "data/uber.fsh");
    pMat->SetDepthTest(true);
    pMat->SetDepthWrite(true);

    wolf::Texture* pAlbedo = _getTexture(pAssimpMat, aiTextureType_DIFFUSE);
    if(pAlbedo)
    {
        pMat->EnableKeyword("ALBEDO_TEX");
        pMat->SetTexture("u_albedoTex", pAlbedo);
    }
    else 
    {
        pMat->SetUniform("u_albedo", _getColorRGBA(pAssimpMat, AI_MATKEY_COLOR_DIFFUSE));
    }

    // TODO
    // aiColor3D specularColor = _getColorRGB(pAssimpMat, AI_MATKEY_COLOR_SPECULAR);
    // aiColor3D emissiveColor = _getColorRGB(pAssimpMat, AI_MATKEY_COLOR_EMISSIVE);
    // Specular strength:
    //   float specStrength = 2.0f * _getFloat(pAssimpMat,AI_MATKEY_SHININESS_STRENGTH);
    // Roughness:
    //   float roughness = 1.0f - (_getFloat(pAssimpMat,AI_MATKEY_REFLECTIVITY / 100.0f);
    // Opacity
    //   float alpha = _getFloat(pAssimpMat,AI_MATKEY_OPACITY);

    // Textures TODO
    // Using Blender's "Principled BSDF", we get the following mappings in Assimp:
    //   Specular  = aiTextureType_SPECULAR
    //   Roughness  = aiTextureType_SHININESS
    //   Emission  = aiTextureType_EMISSIVE
    //   Normal  = aiTextureType_NORMALS

    return pMat;
}


}