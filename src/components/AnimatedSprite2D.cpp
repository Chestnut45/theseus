#include "AnimatedSprite2D.h"

//-----------------------------------------------------------------------------
// File:			AnimatedSprite2D.cpp
// Original Author:	Aurora Ryder
//
// A class representing a renderable, animated 2D sprite component.
// 
//-----------------------------------------------------------------------------

#include "W_GameObject.h"
#include "W_Logging.h"

#include <yaml-cpp/yaml.h>

// Initialize the counter
int AnimatedSprite2D::s_iAnimSprite2DCount = 0;

// This is the initial Vertex Data that all AnimatedSprite2Ds will have
const float AnimatedSprite2D::m_arBaseVertexData[] = {
    0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 0.0f
};

AnimatedSprite2D::AnimatedSprite2D(const std::string& p_strPathToInit)
{
    // Keep track of how many instances of AnimatedSprite2D exist for resource management
    IncreaseReferences();

    try {
        // Grab the file
        YAML::Node node = YAML::LoadFile(p_strPathToInit);

        // Get the frame size
        glm::vec2 v2Size;
        v2Size.x = node["frame_size"]["x"].as<float>();
        v2Size.y = node["frame_size"]["y"].as<float>();

        // Get the playback speed
        float fSpeed = node["playback_speed"].as<float>();

        // Figure out which animation we'll be starting with and grab it's texture path and name
        std::string strTexture = node["start_animation"]["texture"].as<std::string>();
        std::string strStartAnimName = node["start_animation"]["name"].as<std::string>();

        // Because AnimatedSprite2Ds will be frequently changing the UV coordinates they use
        // to render, we need to do a bit of processing when we create or change the texture.
        SetTexture(strTexture, v2Size);

        // We're going to want to save the playback speed for later
        m_fPlaybackSpeed = fSpeed;

        // Now we need to start processing the animation sets
        YAML::Node animSets = node["animation_sets"];
        for (int i = 0; i < animSets.size(); ++i) {
            std::string strSetFilePath = animSets[i].as<std::string>();
            AddAnimationSet(strSetFilePath);
        }

        // Once we've done that, we can set the start animation as active and setup the sprite origin
        SetAnimation(strStartAnimName);
        SetOriginToCenterOfFrame();
    }
    catch (YAML::Exception& e) {
        // Throw an error if something goes wrong
        wolf::Error("Error parsing file '", p_strPathToInit.c_str(), "': ", e.what());
    }
}

AnimatedSprite2D::AnimatedSprite2D(const std::string& p_strPathToAnimSheet, const glm::vec2& p_v2FrameSize, float p_fPlaybackSpeed)
{
    // Keep track of how many instances of AnimatedSprite2D exist for resource management
    IncreaseReferences();

    // Because AnimatedSprite2Ds will be frequently changing the UV coordinates they use
    // to render, we need to do a bit of processing when we create or change the texture.
    SetTexture(p_strPathToAnimSheet, p_v2FrameSize);

    // We're going to want to save the playback speed for later
    m_fPlaybackSpeed = p_fPlaybackSpeed;
}

bool AnimatedSprite2D::SetTexture(const std::string& p_strPathToAnimSheet, const glm::vec2& p_v2FrameSize) {
    // If we're given the path to the texture file that we're already using then we don't need
    // to go through the process of setting it again, so we just return true
    if (strcmp(p_strPathToAnimSheet.c_str(), m_strCurrentTexturePath.c_str()) == 0) {
        return true;
    }

    // Create a new texture
    wolf::Texture* pNewTexture = wolf::TextureManager::CreateTexture(p_strPathToAnimSheet);

    // In order to cleanly divide the texture into animation frames, we need to be sure that the size of the texture
    // divided by the size of a single frame has a remainder of 0 (zero).
    if ((pNewTexture->GetWidth() * pNewTexture->GetHeight()) % (int)(p_v2FrameSize.x * p_v2FrameSize.y) != 0) {
        // If the new texture doesn't match the frame size then we delete it and leave the current texture unchanged
        wolf::TextureManager::DestroyTexture(pNewTexture);
        wolf::Error("Incorrectly sized texture file \"", p_strPathToAnimSheet, "\" passed to AnimatedSprite2D.");
        return false;
    }

    // Otherwise, we can start dividing the texture into animation frames
    
    // First we need to make sure that the previous UV coordinates (if there are any) are deleted
    if (!m_vpFrameUVCoords.empty()) {
        m_vpFrameUVCoords.clear();
    }

    // Then we need to know how many frames are in the texture
    int iNumFramesX = pNewTexture->GetWidth() / p_v2FrameSize.x;
    int iNumFramesY = pNewTexture->GetHeight() / p_v2FrameSize.y;

    int iWidth = iNumFramesX + 1;
    int iHeight = iNumFramesY + 1;


    // We can use those values to create UV coordinates by treating them
    // as points between 0 and 1 on the X and Y axes

    // So we make a place to store them
    glm::vec2 av2WorkingUVCoords[iWidth * iHeight];

    // Figure out how much we'll be incrementing each X and Y by
    float fIncX = 1.0f / iNumFramesX;
    float fIncY = 1.0f / iNumFramesY;

    // And start calculating them
    for (int i = 0; i <= iNumFramesY; i++) {
        // A V coordinate would be calculated as:
        float fVCord = i * fIncY;

        for (int j = 0; j <= iNumFramesX; j++) {
            // And a U coordinate would be calculated the same way, but with x
            float fUCord = j * fIncX;

            // Once we have a UV coordinate set we store it for later
            av2WorkingUVCoords[(i * iWidth) + j] = glm::vec2(fUCord, fVCord);

        }
    }

    // Because frames are numbered 1-n but vectors are index 0-n, we need an offset
    // frame coordinate set that occupies the first index.
    FrameUVCoordSet* pOffsetCoord = new FrameUVCoordSet();
    m_vpFrameUVCoords.push_back(pOffsetCoord);

    // Now that we have all our UV coordinates, we're going to assign them to frames
    for (int p = 0; p <= iNumFramesY - 1; p++) {
        for (int q = 0; q <= iNumFramesX - 1; q++) {
            int iOriginPoint = (p * iWidth) + q;
            // First we find the four UV coordinates that will be used to render this frame
            // and store them in a struct that holds four glm::vec2s
            FrameUVCoordSet* frameCoords = new FrameUVCoordSet();
            frameCoords->m_v2TopRight = av2WorkingUVCoords[iOriginPoint];
            frameCoords->m_v2BotRight = av2WorkingUVCoords[iOriginPoint + 1];
            frameCoords->m_v2TopLeft = av2WorkingUVCoords[iOriginPoint + iWidth];
            frameCoords->m_v2BotLeft = av2WorkingUVCoords[iOriginPoint + iWidth + 1];

            // Then we store 'em
            m_vpFrameUVCoords.push_back(frameCoords);
        }
    }

    // Now we check if this AnimatedSprite2D instance already had a texture
    if (m_pTexture) {
        wolf::TextureManager::DestroyTexture(m_pTexture);   // And if it did we destroy it
    }

    // All we need to do now is set the Filter Mode to FM_Nearest (best mode for pixel art)
    pNewTexture->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);

    // And then let the AnimatedSprite2D instance know that its texture is good to go!
    m_pTexture = pNewTexture;
    m_v2FrameSize = p_v2FrameSize;
    m_strCurrentTexturePath = p_strPathToAnimSheet;
    return true;
}

void AnimatedSprite2D::SetOriginToCenterOfFrame()
{
    // Don't bother if we don't have an animation loaded
    if (!m_pCurrentAnim)
    {
        wolf::Error("No animation loaded, can't center origin of AnimatedSprite2D");
        return;
    }

    const glm::vec2& frameSize = m_pCurrentAnim->m_v2FrameSize;
    m_origin.x = frameSize.x * 0.5f;
    m_origin.y = frameSize.y * 0.5f;
}

AnimatedSprite2D::~AnimatedSprite2D() {
    // Update the number of AnimatedSprite2D instances that currently exist
    s_iAnimSprite2DCount -= 1;

    // Because AnimatedSprite2D components rely on having all animations for one object/entity/etc.
    // stored in a single image, we can assume that destroying this instance of the AnimatedSprite2D
    // means that we no longer need access to its texture, so it can be deleted
    wolf::TextureManager::DestroyTexture(m_pTexture);

    // Iterate through the animation map and delete all of the SpriteAnimation2Ds within it
    for (std::map<std::string, SpriteAnimation2D*>::iterator it = m_mAnimationMap.begin(); it != m_mAnimationMap.end(); ++it) {
        delete(it->second);
    }

    // Then clear the map
    m_mAnimationMap.clear();

    // Delete all of the FrameUVCoords in the vector
    m_vpFrameUVCoords.clear();

    // If this was the last AnimatedSprite2D instance then we no longer need our shader resources
    if (s_iAnimSprite2DCount == 0) {
        // So we can delete them
        s_pCurrentProgram = nullptr;
        wolf::ProgramManager::DestroyProgram(s_pProgram);
        wolf::ProgramManager::DestroyProgram(s_pGrayscaleProgram);
        wolf::ProgramManager::DestroyProgram(s_pWhiteProgram);
        wolf::ProgramManager::DestroyProgram(s_pMultitexProgram);
        wolf::ProgramManager::DestroyProgram(s_pMultitexPetrifiedProgram);
        wolf::BufferManager::DestroyBuffer(s_pVertexBuffer);
        wolf::BufferManager::DestroyBuffer(s_pIndexBuffer);
        delete s_pVAO;
        for(auto texture : s_vMasks)
        {
            wolf::TextureManager::DestroyTexture(texture);
        }
        s_vMasks.clear();
    }
}

bool AnimatedSprite2D::AddAnimation(const std::string& p_strName, const std::string& p_strTexturePath, const glm::vec2& p_vec2FrameSize, int p_iStartFrame, int p_iEndFrame, const glm::vec2& p_vec2Origin, bool p_bLoop, const std::string& p_strNextAnimName) {
    // Check that the start and end frames are valid
    if (p_iStartFrame > p_iEndFrame || p_iStartFrame < 0 || p_iEndFrame < 0) {
        wolf::Error("Attempted to add animation to AnimatedSprite2D with invalid start and end frames.");
        return false;
    }

    // Creates a new SpriteAnimation2D out of the parameters and stores it in the map with p_strName as its key
    SpriteAnimation2D* p_anim = new SpriteAnimation2D(p_strName, p_strTexturePath, p_vec2FrameSize, p_iStartFrame, p_iEndFrame, p_vec2Origin, p_bLoop, p_strNextAnimName);
    m_mAnimationMap.insert(std::pair<std::string, SpriteAnimation2D*>(p_strName, p_anim));
    return true;
}

bool AnimatedSprite2D::RemoveAnimation(const std::string& p_strName) {
    // Attempt to erase (and destroy) the map element with key p_strName
    auto animIt = m_mAnimationMap.find(p_strName);
    if (animIt != m_mAnimationMap.end()) {
        delete(animIt->second);
        m_mAnimationMap.erase(p_strName);
        return true;
    }
    return false; // And false if we couldn't
}

void AnimatedSprite2D::SetAnimation(const std::string& p_strName) {
    auto animIt = m_mAnimationMap.find(p_strName);
    if (animIt != m_mAnimationMap.end()) {
        if (m_pCurrentAnim == animIt->second) {
            // If we're already playing this animation then we shouldn't restart it
            return;
        }
        m_pCurrentAnim = animIt->second;
        this->SetTexture(m_pCurrentAnim->m_strItemsTexturePath, m_pCurrentAnim->m_v2FrameSize);
        this->SetOrigin(m_pCurrentAnim->m_v2Origin);
        m_fCurrentFrame = m_pCurrentAnim->m_iStartFrame;
        m_pCurrentFrameUVs = m_vpFrameUVCoords[m_pCurrentAnim->m_iStartFrame];
        m_bFrameChanged = true;
        m_bIsAnimFinished = false;
        m_iAnimLoopCount = 0;
    }
}

// This override lets you change to a specific frame of the animation you are setting
void AnimatedSprite2D::SetAnimation(const std::string& p_strName, int p_iTargetAnimFrame) {
    auto animIt = m_mAnimationMap.find(p_strName);
    if (animIt != m_mAnimationMap.end()) {
        if (m_pCurrentAnim == animIt->second) {
            // If we're already playing this animation then we shouldn't restart it
            return;
        }
        // Quick check to make sure that changing to the given frame won't send us out of bounds
        int iTargetFrame = m_pCurrentAnim->m_iStartFrame + p_iTargetAnimFrame;
        if (iTargetFrame >= m_vpFrameUVCoords.size() || iTargetFrame < 0) {
            return;
        }

        m_pCurrentAnim = animIt->second;
        this->SetTexture(m_pCurrentAnim->m_strItemsTexturePath, m_pCurrentAnim->m_v2FrameSize);
        this->SetOrigin(m_pCurrentAnim->m_v2Origin);
        m_fCurrentFrame = m_pCurrentAnim->m_iStartFrame + p_iTargetAnimFrame;
        m_pCurrentFrameUVs = m_vpFrameUVCoords[iTargetFrame];
        m_bFrameChanged = true;
        m_bIsAnimFinished = false;
        m_iAnimLoopCount = 0;
    }
}

// *** This method was heavily informed by Jason Gregory's "Game Engine Architecture" 3rd Ed.
// and Carol Boers' UPEI CS-4650 Animation Controller Component ***
void AnimatedSprite2D::Update(float p_fDelta) {
    // Check if the animation is paused
    if(!m_bIsAnimPaused)
    {
        // Quick check to make sure that we have an animation
        if (m_pCurrentAnim) {
            // Advance the current frame
            m_fCurrentFrame += p_fDelta * m_fPlaybackSpeed;

        // If that advancement causes us to reach the end of the last frame of this animation
        if (m_fCurrentFrame >= m_pCurrentAnim->m_iEndFrame + 1) {
            // Check if this is a looping animation
            if (m_pCurrentAnim->m_bLoop) {
                // And if it is, restart the animation
                m_fCurrentFrame = (float)m_pCurrentAnim->m_iStartFrame;
                m_iAnimLoopCount++;
            }
            else {
                // Check if this animation triggers another one
                if (m_pCurrentAnim->m_strNextAnimName != "") {
                    // If it does, play that animation
                    this->SetAnimation(m_pCurrentAnim->m_strNextAnimName);
                }
                else {
                    // Otherwise, don't advance beyond the last frame of the animation
                    m_bIsAnimFinished = true;
                    m_fCurrentFrame = (float)m_pCurrentAnim->m_iEndFrame;
                }
            }
        }

            // Similarly, if that advancement caused us to change animation frames
            if ((int)m_fCurrentFrame != (int)m_fLastFrame) { 
                // Look for the next frame in the animation
                m_bFrameChanged = true;
                m_pCurrentFrameUVs = m_vpFrameUVCoords[(int)m_fCurrentFrame];
            }

            // Keep track of which animation frame we played last so that
            // we are only changing the UV coordinates when necessary
            m_fLastFrame = m_fCurrentFrame;
        }        
    }


    // If fade-in timer not expired 
    if(m_fGradualFadeInTimer < m_fGradualFadeInTime)
    {
        // Update
        m_fGradualFadeInTimer += p_fDelta;
    }
    // Else
    else
    {
        // Force-set fade-in timer to be equal to set time
        if(m_fGradualFadeInTimer > m_fGradualFadeInTime)
        {
            m_fGradualFadeInTimer = m_fGradualFadeInTime;
        }

        // If duration timer not expired
        if(m_fSEDurationTimer < m_fSEDurationTime)
        {
            // Update
            m_fSEDurationTimer += p_fDelta;
        }
        else
        {
            // If fade-out timer not expired
            if(m_fGradualFadeOutTimer > 0.0f)
            {
                // Update
                m_fGradualFadeOutTimer -= p_fDelta;
            }
            // Else
            else
            {
                // Force-set fade-out timer to be equal to 0
                if(m_fGradualFadeOutTimer > 0.0f)
                {
                    m_fGradualFadeOutTimer = 0.0f;
                }
            }
        }


    }
}

void AnimatedSprite2D::Draw(const glm::vec2& position, float rotationRadians, const glm::vec2& scale, const glm::vec3& tint) {
    // If we don't have a texture (or the coordinates that go with one) then we shouldn't be trying to draw anything
    if (!m_visible || !m_pTexture || m_vpFrameUVCoords.empty()) {
        return;
    }

    // If the animation frame changes we need to update the UV coordinates in the Vertex Buffer
    if (m_bFrameChanged) {
        // To do that, we're going to need to bind the VBO
        s_pVertexBuffer->Bind();

        // Then we need to update a temporary array with all of the data that we'd like to write to the buffer.

        // Vertex 1
        // We're not changing any of the actual geometry so we retrieve that from the BaseVertexData.
        m_arTempVertexData[0] = m_arBaseVertexData[0]; // X
        m_arTempVertexData[1] = m_arBaseVertexData[1]; // Y

        // The UV coordinates, however, come from the current frame's UV coordinate set
        m_arTempVertexData[2] = m_pCurrentFrameUVs->m_v2TopLeft.x;  // U
        m_arTempVertexData[3] = m_pCurrentFrameUVs->m_v2TopLeft.y;  // V

        // Vertex 2
        m_arTempVertexData[4] = m_arBaseVertexData[4]; // X
        m_arTempVertexData[5] = m_arBaseVertexData[5]; // Y

        m_arTempVertexData[6] = m_pCurrentFrameUVs->m_v2BotLeft.x; // U
        m_arTempVertexData[7] = m_pCurrentFrameUVs->m_v2BotLeft.y; // V

        // Vertex 3
        m_arTempVertexData[8] = m_arBaseVertexData[8]; // X
        m_arTempVertexData[9] = m_arBaseVertexData[9]; // Y

        m_arTempVertexData[10] = m_pCurrentFrameUVs->m_v2TopRight.x; // U
        m_arTempVertexData[11] = m_pCurrentFrameUVs->m_v2TopRight.y; // V

        // Vertex 4
        m_arTempVertexData[12] = m_arBaseVertexData[12]; // X
        m_arTempVertexData[13] = m_arBaseVertexData[13]; // Y

        m_arTempVertexData[14] = m_pCurrentFrameUVs->m_v2BotRight.x; // U
        m_arTempVertexData[15] = m_pCurrentFrameUVs->m_v2BotRight.y; // V
        
        // Once we've got our temporary array set up, we can copy the data to the buffer using glBufferSubData
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(m_arTempVertexData), m_arTempVertexData);

        // Then we reset the flag
        
    }

    // And perform the rest of the draw call

    // Grab the texture size
    const glm::vec2 texSize = glm::vec2(m_pTexture->GetWidth(), m_pTexture->GetHeight());

    //-----------------//
    //                 //
    //  Added by Nhật  //
    //                 //
    //-----------------//
    // Update shaders based on current special effects
    UpdateShaders();
    BindUniformsAndTextures(position, rotationRadians, scale, tint);

    // Draw!
    s_pVAO->Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

    // Unbind
    glBindVertexArray(0);
}

// Note that all animations in an animation set are assumed to have the SAME texture file and frame size,
// If this is NOT the case, restructure your animation sets until it is.
bool AnimatedSprite2D::AddAnimationSet(const std::string& p_strPathToSetFile) {
    try {
        // Grab the file
        YAML::Node node = YAML::LoadFile(p_strPathToSetFile);

        // Get the texture path
        std::string strTexture = node["texture"].as<std::string>();

        // Get the frame size
        glm::vec2 v2Size;
        v2Size.x = node["frame_size"]["x"].as<float>();
        v2Size.y = node["frame_size"]["y"].as<float>();

        // Then go through each of the animations in the set
        YAML::Node animations = node["animations"];
        for (int i = 0; i < animations.size(); ++i) {
            // Get the node that represents each individual animation
            YAML::Node anim = animations[i];

            // And retrieve their unique information
            std::string strName = anim["name"].as<std::string>();  // Name
            int iStart = anim["start_frame"].as<int>(); // What frame it starts on (inclusive)
            int iEnd = anim["end_frame"].as<int>(); // What frame it ends on (inclusive)
            glm::vec2 v2Origin;
            v2Origin.x = anim["origin"]["x"] ? anim["origin"]["x"].as<float>() : v2Origin.x;
            v2Origin.y = anim["origin"]["y"] ? anim["origin"]["y"].as<float>() : v2Origin.y;
            bool bLoops = anim["loops"].as<bool>(); // And whether the animation loops
            std::string strNextAnim = anim["next_anim"] ? anim["next_anim"].as<std::string>() : ""; // Whether or not this animation triggers another one when it finishes

            // Then add the animation to the AnimatedSprite2D component
            this->AddAnimation(strName, strTexture, v2Size, iStart, iEnd, v2Origin, bLoops, strNextAnim);
        }
        return true;
    }
    catch (YAML::Exception& e) { // If something went wrong, we need to throw an error
        wolf::Error("Error parsing file '", p_strPathToSetFile.c_str(), "': ", e.what());
        return false;
    }
}

void AnimatedSprite2D::IncreaseReferences()
{
    // If there are currently no AnimatedSprite2D instances then we need to create shader resources
    if (s_iAnimSprite2DCount == 0) {
        // *** The following code segment is taken directly from D'Anyil Landry's W_Sprite2D.cpp _IncreaseRefCount() ***
        // *** some modifications have been made, but the majority of the code is the same                           ***

        // Load shader programs
        s_pProgram = wolf::ProgramManager::CreateProgram("data/shaders/animatedsprite2d.vs", "data/shaders/animatedsprite2d.fs");

            //-----------------//
            //                 //
            //  Added by Nhật  //
            //                 //
            //-----------------//
            s_pGrayscaleProgram = wolf::ProgramManager::CreateProgram("data/shaders/animatedsprite2d.vs", "data/shaders/animatedsprite2d_grayscale.fs");
            s_pWhiteProgram = wolf::ProgramManager::CreateProgram("data/shaders/animatedsprite2d.vs", "data/shaders/animatedsprite2d_white.fs");
            s_pMultitexProgram = wolf::ProgramManager::CreateProgram("data/shaders/animatedsprite2d.vs", "data/shaders/animatedsprite2d_multitex.fs");
            s_pMultitexPetrifiedProgram = wolf::ProgramManager::CreateProgram("data/shaders/animatedsprite2d.vs", "data/shaders/animatedsprite2d_multitex_petrified.fs");
            s_pCurrentProgram = s_pProgram;

        // Create vertex buffer
        s_pVertexBuffer = wolf::BufferManager::CreateVertexBuffer(m_arBaseVertexData, sizeof(m_arBaseVertexData));

        // Generate index buffer data
        unsigned short indexData[6] =
        {
            0, 2, 1, 1, 2, 3
        };

        // Create index buffer
        s_pIndexBuffer = wolf::BufferManager::CreateIndexBuffer(indexData, 6);

        // Create vertex declaration (VAO)
        s_pVAO = new wolf::VertexDeclaration();

        s_pVAO->Begin();
        s_pVAO->SetVertexBuffer(s_pVertexBuffer);
        s_pVAO->SetIndexBuffer(s_pIndexBuffer);
        s_pVAO->AppendAttribute(wolf::Attribute::AT_Position, 2, wolf::ComponentType::CT_Float, 0);
        s_pVAO->AppendAttribute(wolf::Attribute::AT_TexCoord1, 2, wolf::ComponentType::CT_Float, sizeof(float) * 2);
        s_pVAO->End();

        //-----------------//
        //                 //
        //  Added by Nhat  //
        //                 //
        //-----------------//
        s_vMasks.push_back(wolf::TextureManager::CreateTexture("data/textures/stone.png"));

        for(auto mask : s_vMasks)
        {
            mask->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
        }

        // *** End of borrowed code segment ***
    }

    s_iAnimSprite2DCount++;
}

//-----------------//
//                 //
//  Added by Nhật  //
//                 //
//-----------------//
void AnimatedSprite2D::SetSpecialEffects(SpecialEffectsType p_spe_type, float p_gradual_in, float p_se_duration, float p_gradual_out) 
{
    m_specialEffectsType = p_spe_type;
    
    // Fade-in
    m_fGradualFadeInTime = p_gradual_in;
    m_fGradualFadeInTimer = 0.0f;
    
    // Fade-out
    m_fGradualFadeOutTime = p_gradual_out;
    m_fGradualFadeOutTimer = p_gradual_out;
    
    // Duration
    m_fSEDurationTime = p_se_duration < 0.0f ? std::numeric_limits<float>::infinity() : p_se_duration;  
    m_fSEDurationTimer = 0.0f;
}

void AnimatedSprite2D::UpdateShaders()
{
    switch (m_specialEffectsType)
    {
        case SpecialEffectsType::GRAYSCALE:
        {
            s_pCurrentProgram = s_pGrayscaleProgram;
            break;
        }
        case SpecialEffectsType::WHITE:
        {
            s_pCurrentProgram = s_pWhiteProgram;
            break;
        }
        case SpecialEffectsType::MULTITEX_PETRIFIED:
        {
            s_pCurrentProgram = s_pMultitexPetrifiedProgram;
            break;
        }
        case SpecialEffectsType::NONE:
        {
            s_pCurrentProgram = s_pProgram;
            break;
        }
        default:
        {
            s_pCurrentProgram = s_pProgram;
            break;
        }
    }
}

void AnimatedSprite2D::BindUniformsAndTextures(const glm::vec2& position, float rotationRadians, const glm::vec2& scale, const glm::vec3& tint)
{
    // Determine tint to use
    const glm::vec3& chosenTint = tint == glm::vec3(-1.0f) ? m_tint : tint;

    // Build model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position - m_origin * scale, 0.0f));
    model = glm::rotate(model, rotationRadians, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(scale * m_v2FrameSize, 1.0f));

    // Set model uniform
    s_pCurrentProgram->SetUniform("model", model);
    s_pCurrentProgram->SetUniform("tint", chosenTint);

    // State 1 - If fade-in timer has expired
    if(m_fGradualFadeInTimer >= m_fGradualFadeInTime)
    {
        // Hard-set uniform to 1
        s_pCurrentProgram->SetUniform("fadingTime", 1.0f);
        
        // State 2 - If special effects duration has expired
        if(m_fSEDurationTimer >= m_fSEDurationTime)
        {
            // State 3 - If fade-out timer has expired
            if(m_fGradualFadeOutTimer <= 0.0f)
            {
                // Hard-set uniform to 0
                s_pCurrentProgram->SetUniform("fadingTime", 0.0f);
            }
            // State 3 - Fade-out timer still active
            else
            {
                // Set uniform to normalised time
                s_pCurrentProgram->SetUniform("fadingTime", (float)(m_fGradualFadeOutTimer / m_fGradualFadeOutTime));
            }
        }
        // State 2 - Duration timer still active
        else
        {
            // Hard-set uniform to 1
            s_pCurrentProgram->SetUniform("fadingTime", 1.0f);
        }
        
    }
    // State 1 - Fade-in timer still active
    else
    {
        // Set uniform to normalised time
        s_pCurrentProgram->SetUniform("fadingTime", (float)(m_fGradualFadeInTimer / m_fGradualFadeInTime));
        
    }

    s_pCurrentProgram->Bind();
    m_pTexture->Bind(0);
    // If multitexturing
    if(
        m_specialEffectsType != SpecialEffectsType::NONE            && 
        m_specialEffectsType >= SpecialEffectsType::MULTITEX_PETRIFIED
        )
    {   
        s_vMasks.at(m_specialEffectsType - SpecialEffectsType::MULTITEX_PETRIFIED)->Bind(1);
    }
}