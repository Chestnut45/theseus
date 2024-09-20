#pragma once

#include <wolf.h>
#include <map>
#include <utility>

//-----------------------------------------------------------------------------
// File:			AnimatedSprite2D.h
// Original Author:	Aurora Ryder
//
// A class representing a renderable, animated 2D sprite component.
// 
//-----------------------------------------------------------------------------

// TODO:
//  > Comments (as needed)
//  > Clean Up (as needed)

// The thought with this is that you have one file (and by extension one texture) that contains all of the
// animation frames for a particular object or character. You then use the SpriteAnimation2D struct to 'break'
// that file into unique animations so that you don't have to swap textures when you switch between animations

struct SpriteAnimation2D {
    SpriteAnimation2D(const std::string& p_strName, int p_iStartFrame, int p_iEndFrame, bool p_bLoop) :
    m_strName(p_strName), m_iStartFrame(p_iStartFrame), m_iEndFrame(p_iEndFrame), m_bLoop(p_bLoop){};
    
    std::string m_strName;  // What is this an animation of?

    int m_iStartFrame;      // Which frame does this animation start on?
    int m_iEndFrame;        // Which frame does this animation end on?

    bool m_bLoop;           // Does this animation loop?
};

// To render one frame of animation you're going to need the corresponding UV coordinates from
// the texture, so rather than having to calculate those coordinates each time we change
// animation frames, I've chosen to store them in structs
struct FrameUVCoordSet {
    glm::vec2 m_v2TopLeft;
    glm::vec2 m_v2BotLeft;
    glm::vec2 m_v2TopRight;
    glm::vec2 m_v2BotRight;
};

class AnimatedSprite2D : public wolf::BaseComponent {
    public:
        AnimatedSprite2D(const std::string& p_strPathToAnimSheet, const glm::vec2& p_v2FrameSize, float p_fPlaybackSpeed);
        ~AnimatedSprite2D();

        void Update(float p_fDelta);

        void AddAnimation(const std::string& p_strName, int p_iStartFrame, int p_iEndFrame, bool p_bLoop);
        bool RemoveAnimation(const std::string& p_strName);

        void SetAnimation(const std::string& p_strName);
        void SetAnimation(const std::string& p_strName, int p_iTargetAnimFrame);
        SpriteAnimation2D* GetCurrentAnimation() const {return m_pCurrentAnim;};

        void SetPlaybackSpeed(float p_fSpeed) {m_fPlaybackSpeed = p_fSpeed;};
        float GetPlaybackSpeed() const {return m_fPlaybackSpeed;};

        // It is possible to change the texture of AnimatedSprite2D but ONLY if that
        // texture uses the same frame size as the old one i.e. you can't use a
        // texture that has 128x128 pixel frames for a 32x32 pixel sprite ->
        bool SetTexture(const std::string& p_strPathToAnimSheet, const glm::vec2& p_v2FrameSize);
        wolf::Texture* GetTexture() const {return m_pTexture;};

        // -> As such, have a getter for the frame size but not a setter
        const glm::vec2& GetFrameSize() const {return m_v2FrameSize;};

        void Draw(const glm::vec2& position, float rotationRadians, const glm::vec2& scale);

    private:
        // Map of animations
        std::map<std::string, SpriteAnimation2D*> m_mAnimationMap; // Name = Key, Animation Details = Value

        // Vector of UV coordinates per frame
        std::vector<FrameUVCoordSet*> m_vpFrameUVCoords; // Frame Number = Key, Set of Coordinates = Value

        SpriteAnimation2D* m_pCurrentAnim = nullptr;
        FrameUVCoordSet* m_pCurrentFrameUVs = nullptr;
        wolf::Texture* m_pTexture = nullptr;

        float m_fPlaybackSpeed;
        float m_fCurrentFrame;
        float m_fLastFrame = 0.0f;

        // We want to limit the amount of changes we make to the Vertex Buffer contents so we use
        // a dirty flag to keep track of when the UV coordinates have changed
        bool m_bFrameChanged = true;

        glm::vec2 m_v2FrameSize; // This vector represents the size of a single animation frame in a spritesheet

        static const float m_arBaseVertexData[];
        float m_arTempVertexData[16];

        glm::vec2 m_v2Origin{0.0f}; // !-- Origin needs to be per frame -- !
        
        static int s_iAnimSprite2DCount; // Static variable to keep track of how many AnimatedSprite2D instances exist

        static inline wolf::Program* s_pProgram = nullptr;
        static inline wolf::VertexBuffer* s_pVertexBuffer = nullptr;
        static inline wolf::IndexBuffer* s_pIndexBuffer = nullptr;
        static inline wolf::VertexDeclaration* s_pVAO = nullptr;
};