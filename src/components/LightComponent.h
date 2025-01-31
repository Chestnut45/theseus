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
        LightComponent(const glm::vec4& p_v4Color, const glm::vec2& p_v2Radius, bool m_bCanMove);
        ~LightComponent();

        // Delete copy constructor/assignment
        LightComponent(const LightComponent&) = delete;
        LightComponent& operator=(const LightComponent&) = delete;

        // Delete move constructor/assignment
        LightComponent(LightComponent&& other) = delete;
        LightComponent& operator=(LightComponent&& other) = delete;

        void Update(float p_fDelta);

        inline int GetID() const {return m_iIDNum;};

        inline glm::vec4 GetColor() const {return m_v4Color;};
        inline void SetColor(const glm::vec4& p_v4Color) {m_v4Color = p_v4Color;};

        inline glm::vec2 GetRadius() const {return m_v2Radius;};
        inline void SetRadius(const glm::vec2& p_v2Radius) {
            m_v2Radius = p_v2Radius;

            // If the radius has changed then so have the corner points
            m_pRadiusRectangle->m_left = m_v2Origin.x - m_v2Radius.x / 2.0f;
            m_pRadiusRectangle->m_top = m_v2Origin.y + m_v2Radius.y / 2.0f;
            m_pRadiusRectangle->m_right = m_v2Origin.x + m_v2Radius.x / 2.0f;
            m_pRadiusRectangle->m_bottom = m_v2Origin.y - m_v2Radius.y / 2.0f;
        };

        inline bool CanMove() const {return m_bCanMove;};
        inline void SetCanMove(bool p_bCanMove) {m_bCanMove = p_bCanMove;};

        // Corner points are adjusted based on the radius so they don't need a setter
        inline wolf::Rectangle& GetRadiusRectangle() const {return *m_pRadiusRectangle;};

        bool SweepLinePointCollisionTest(const glm::vec2& p_v2LineEnd, const glm::vec2& p_v2Point);
        std::pair<bool, glm::vec2> SweepLineRectCollisionTest(const glm::vec2& p_v2LineEnd, const wolf::Rectangle& p_pRect);

    private:
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
        wolf::Scene* m_pScene;
        
        // This light's transform and the four corners at the edges of its radius
        wolf::Transform2D* m_pTransform;
        wolf::Rectangle* m_pRadiusRectangle;

        // Map to hold all of the points this light's rays are colliding with
        std::map<int, glm::vec2> m_iv2CollidingPoints;
        int m_iEndOfCollidingPointsMap = 0;
};