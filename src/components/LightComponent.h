#pragma once

//-----------------------------------------------------------------------------
// File:            LightComponent.h
// Original Author: Aurora Ryder
//
// A class representing a single colored light with a given radius
//-----------------------------------------------------------------------------

#include <W_BaseComponent.h>
#include <W_Transform2D.h>
#include <W_Scene.h>
#include <string>

#include "ColliderComponent.h"

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

        inline glm::vec2 GetRadius() const {return m_v2Radius;};

        inline bool CanMove() const {return m_bCanMove;};
        inline void SetCanMove(bool p_bCanMove) {m_bCanMove = p_bCanMove;};

    private:
        std::pair<bool, glm::vec2> LineToCornerRectSideCollisionTest(const glm::vec2& p_v2LineEnd, const glm::vec2& p_v2SideStart, const glm::vec2& p_v2SideEnd);
        static bool CompareVec2FloatPair(std::pair<glm::vec2, float> p_v2fA, std::pair<glm::vec2, float> p_v2fB);
        float CalculateCosAngleOfIntersection(const glm::vec2& p_v2Intersect);

        // ID number to discern between lights
        static int m_iNextIDNum;
        const int m_iIDNum;
        
        // Bool to determine whether or not this light can move
        bool m_bCanMove;

        // Color, radius, and origin point of the light
        glm::vec4 m_v4Color;
        glm::vec2 m_v2Radius;
        glm::vec2 m_v2Origin;

        // Pointer to the scene this light is in
        wolf::Scene* m_pScene = nullptr;
        
        // This light's transform and the four corners at the edges of its radius
        wolf::Transform2D* m_pTransform = nullptr;
        ColliderComponent* m_pCollider = nullptr;

        // Vector to hold the points that collide with the light and the
        // slope of the line they intersected with the light on
        std::vector<std::pair<glm::vec2, float>> m_vfv2CollidingPoints;
};