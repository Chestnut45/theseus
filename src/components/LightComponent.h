#pragma once

//-----------------------------------------------------------------------------
// File:            LightComponent.h
// Original Author: Aurora Ryder
//
// A class representing a single colored light with a given radius
//-----------------------------------------------------------------------------

#include <LabyrinthManager.h>
#include <W_BaseComponent.h>
#include <W_Transform2D.h>
#include <W_Scene.h>
#include <string>

#include <VertexDeclarations.h>
#include "ColliderComponent.h"
#include <LabyrinthManager.h>

class LightComponent : public wolf::BaseComponent {
    public:
        // IMPORTANT: LightComponent::Init() MUST be called immediately after component creation
        LightComponent(const glm::vec4& p_v4Color, const glm::vec2& p_v2Radius, bool m_bCanMove);
        ~LightComponent();

        // Delete copy constructor/assignment
        LightComponent(const LightComponent&) = delete;
        LightComponent& operator=(const LightComponent&) = delete;

        // Delete move constructor/assignment
        LightComponent(LightComponent&& other) = delete;
        LightComponent& operator=(LightComponent&& other) = delete;

        void Init();
        void Update(float p_fDelta);

        inline int GetID() const {return m_iIDNum;};

        inline glm::vec4 GetColor() const {return m_v4Color;};
        inline void SetColor(const glm::vec4& p_v4Color) {m_v4Color = p_v4Color;};

        inline glm::vec2 GetRadius() const {return m_v2CurRadius;};

        inline bool CanMove() const {return m_bCanMove;};
        inline void SetCanMove(bool p_bCanMove) {m_bCanMove = p_bCanMove;};

        void RenderToFBO();
        static void BlendFBOAndScreen();
        static void ClearFBO();

    private:

        enum RoughPosition {
            TOP_LEFT,
            TOP_CENTER,
            TOP_RIGHT,
            MID_LEFT,
            SELF, // If the object's rough position is dead-center then we're comparing against ourselves
            MID_RIGHT,
            BOT_LEFT,
            BOT_CENTER,
            BOT_RIGHT,
        };

        static float s_arfBaseVertexData[6];
        static std::vector<TexturedVertex2D> s_vtvQuadVertices;

        static bool CompareVec2FloatPair(std::pair<glm::vec2, float> p_v2fA, std::pair<glm::vec2, float> p_v2fB);

        RoughPosition CalculateRoughObjPosition(const glm::vec2& p_v2ObjCenterPos);
        std::pair<bool, glm::vec2> LineLineCollisionTest(const glm::vec2& p_v2AStart, const glm::vec2& p_v2AEnd, const glm::vec2& p_v2BStart, const glm::vec2& p_v2BEnd);
        float CalculateAngleOfIntersection(const glm::vec2& p_v2Intersect);

        // Determines if a corner is colliding with the light's ray(s) and adds it to the list of colliding points if it is
        void CheckForCollisionAndAdd(const glm::vec2& p_v2Corner, std::pair<const glm::vec2&, const glm::vec2&> p_v2v2Side);

        // Determines if a line intersects with any of the AOE rectangle's sides and adds the point(s)
        // of intersection to the list of colliding points if so
        void CheckForAOECollisionAndAdd(const glm::vec2& p_v2LineStart, const glm::vec2& p_v2LineEnd);

        // Helper function to determine if a point falls on a wall tile
        bool CheckForWallAtPos(const glm::vec2& p_v2Pos);

        bool IsAOERect(const wolf::Rectangle& p_pRect);

        // ID number to discern between lights
        static int s_iNextIDNum;
        const int m_iIDNum;
        
        // Bool to determine whether or not this light can move
        bool m_bCanMove;

        // Color, radius, and origin point of the light
        glm::vec4 m_v4Color;
        glm::vec2 m_v2CurRadius;
        glm::vec2 m_v2InitRadius;
        glm::vec2 m_v2Origin;

        // Pointer to the scene this light is in
        wolf::Scene* m_pScene = nullptr;
        LabyrinthManager* m_pLabyrinthManager = nullptr;
        
        // This light's transform and the four corners at the edges of its radius
        wolf::Transform2D* m_pTransform = nullptr;
        ColliderComponent* m_pCollider = nullptr;

        // Vector to hold the points that collide with the light and the
        // angle of the line they intersect on
        std::vector<std::pair<glm::vec2, float>> m_vv2fCollidingPoints;

        // Vector to hold the points that will be written to the shader
        std::vector<ColouredVertex2D> m_vcvVertexData;

        // Light shader resources
        static inline wolf::Program* s_pProgram = nullptr;
        static inline wolf::VertexBuffer* s_pVBO = nullptr;
        static inline wolf::VertexDeclaration* s_pVAO = nullptr;
        static inline wolf::FrameBuffer* s_pFBO = nullptr;

        static int s_iRefCount;

        static bool s_bFBOIsClear;
        static int s_iLightsRendered;

        // Textured quad shader resources
        static inline wolf::Program* s_pQuadProgram = nullptr;
        static inline wolf::VertexBuffer* s_pQuadVBO = nullptr;
        static inline wolf::VertexDeclaration* s_pQuadVAO = nullptr;
};