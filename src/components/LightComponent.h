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

struct AABBCorners {
    AABBCorners(const glm::vec2& p_v2TopLeft, const glm::vec2& p_v2BottomLeft, const glm::vec2& p_v2BottomRight, const glm::vec2& p_v2TopRight)
        : v2TopLeft(p_v2TopLeft), v2BottomLeft(p_v2BottomLeft), v2BottomRight(p_v2BottomRight), v2TopRight(p_v2TopRight) {}

    glm::vec2 v2TopLeft;
    glm::vec2 v2BottomLeft;
    glm::vec2 v2BottomRight;
    glm::vec2 v2TopRight;
};

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
            m_pCornerPoints->v2TopLeft = {m_v2Origin.x - m_v2Radius.x / 2.0f, m_v2Origin.y + m_v2Radius.y / 2.0f};
            m_pCornerPoints->v2BottomLeft = {m_v2Origin.x - m_v2Radius.x / 2.0f, m_v2Origin.y - m_v2Radius.y / 2.0f};
            m_pCornerPoints->v2BottomRight = {m_v2Origin.x + m_v2Radius.x / 2.0f, m_v2Origin.y - m_v2Radius.y / 2.0f};
            m_pCornerPoints->v2TopRight = {m_v2Origin.x + m_v2Radius.x / 2.0f, m_v2Origin.y + m_v2Radius.y / 2.0f};
        };

        inline bool CanMove() const {return m_bCanMove;};
        inline void SetCanMove(bool p_bCanMove) {m_bCanMove = p_bCanMove;};

        // Corner points are adjusted based on the radius so they don't need a setter
        inline AABBCorners& GetCornerPoints() const {return *m_pCornerPoints;};

    private:
        static int m_iNextIDNum;
        const int m_iIDNum;
        
        bool m_bCanMove;

        glm::vec4 m_v4Color;
        glm::vec2 m_v2Radius;
        glm::vec2 m_v2Origin;

        const wolf::Scene* m_pScene;
        
        wolf::Transform2D* m_pTransform;
        AABBCorners* m_pCornerPoints;
};