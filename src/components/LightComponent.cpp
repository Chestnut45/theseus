#include "LightComponent.h"
#include <vector>
#include <map>
#include <GLShapesRenderer.h>
#include <ColliderManager.h>
#include <W_Camera2D.h>

//-----------------------------------------------------------------------------
// File:            LightComponent.h
// Original Author: Aurora Ryder
//
// A class representing a single colored light with a given radius
//-----------------------------------------------------------------------------

int LightComponent::s_iNextIDNum = 0;
int LightComponent::s_iRefCount = 0;

// To init the lighting geometry shader
float LightComponent::s_arfBaseVertexData[6] {
    // x,    y,    r,    g,    b,    a
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

// To draw the light/shadow texture to the screen
std::vector<TexturedVertex2D> LightComponent::s_vtvTexQuadVertices {
    {-1.0f,  1.0f,  0.0f, 1.0f}, // Top Left
    {-1.0f, -1.0f,  0.0f, 0.0f}, // Bot Left
    {1.0f, -1.0f,  1.0f, 0.0f}, // Bot Right

    {-1.0f,  1.0f,  0.0f, 1.0f}, // Top Left
    {1.0f, -1.0f,  1.0f, 0.0f}, // Bot Right
    {1.0f,  1.0f,  1.0f, 1.0f} // Top Right
};

// ------------------------------------------------------------------------------------------------------------
//                                  Resource Management & Init Methods
// ------------------------------------------------------------------------------------------------------------

LightComponent::LightComponent(const glm::vec4& p_v4Color, float p_fRadius, bool p_bIsOn) 
    : m_v4Color(p_v4Color), m_v2InitRadius(glm::vec2(p_fRadius, p_fRadius)), m_iIDNum(s_iNextIDNum), m_bIsOn(p_bIsOn)
{
    // If this is the first LightComponent instance in the scene...
    if (s_iRefCount == 0) {
        // ...we need to create the shader resources for the lights

        // Create the shader program
        s_pProgram = wolf::ProgramManager::CreateProgram("data/shaders/light2D.vs", "data/shaders/light2D.fs");

        // !---------------------- This code segment is courtesy of D'Anyil ------------------------------!
        // Create framebuffer
        glGenFramebuffers(1, &s_uiFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, s_uiFBO);
        glGenTextures(1, &s_uiTexture);
        glBindTexture(GL_TEXTURE_2D, s_uiTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s_v2DefaultFramebufferSize.x, s_v2DefaultFramebufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_uiTexture, 0);

        // Ensure completeness
        if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            wolf::Error("LightComponent framebuffer not complete!");
        }
        // !----------------------------- End of D'Anyil's code segment ---------------------------------!

        wolf::FrameBuffer::BindDefault();

        // Create the shader resources for lights
        s_pVBO = wolf::BufferManager::CreateVertexBuffer(s_arfBaseVertexData, sizeof(ColouredVertex2D));
        s_pVAO = new wolf::VertexDeclaration();
        s_pVAO->Begin();
        s_pVAO->AppendAttribute(wolf::AT_Position, 2, wolf::CT_Float);
        s_pVAO->AppendAttribute(wolf::AT_Color, 4, wolf::CT_Float);
        s_pVAO->SetVertexBuffer(s_pVBO);
        s_pVAO->End();

        // Create the shader resources for blending the light/shadow texture to the screen
        s_pQuadProgram = wolf::ProgramManager::CreateProgram("data/shaders/lightQuad2D.vs", "data/shaders/lightQuad2D.fs");

        s_pQuadVBO = wolf::BufferManager::CreateVertexBuffer(s_vtvTexQuadVertices.data(), sizeof(TexturedVertex2D) * s_vtvTexQuadVertices.size());
        s_pQuadVAO = new wolf::VertexDeclaration();
        s_pQuadVAO->Begin();
        s_pQuadVAO->AppendAttribute(wolf::Attribute::AT_Position, 2, wolf::ComponentType::CT_Float, 0);
        s_pQuadVAO->AppendAttribute(wolf::Attribute::AT_TexCoord1, 2, wolf::ComponentType::CT_Float, sizeof(float) * 2);
        s_pQuadVAO->SetVertexBuffer(s_pQuadVBO);
        s_pQuadVAO->End();
    }

    // Initialize the radius
    m_v2CurRadius = m_v2InitRadius;

    // Get the next ID number ready
    s_iNextIDNum++;

    // Increase the number of LightComponent references
    s_iRefCount++;

    // Register for lighting events
    wolf::EventManager::AddListener<LightToggleEvent, LightComponent, &LightComponent::HandleLightToggleEvent>(*this);
}

LightComponent::~LightComponent() {
    // Clear out the colliding points map
    m_vv2fCollidingPoints.clear();

    // Decrease the number of LightComponent references
    s_iRefCount -= 1;

    // If this was the last LightComponent instance in the scene
    if (s_iRefCount == 0) {
        // Delete the shader resources
        glDeleteFramebuffers(1, &s_uiFBO);
        glDeleteTextures(1, &s_uiTexture);

        delete s_pVAO;
        delete s_pQuadVAO;
        s_pVAO = nullptr;
        s_pQuadVAO = nullptr;

        wolf::BufferManager::DestroyBuffer(s_pVBO);
        wolf::BufferManager::DestroyBuffer(s_pQuadVBO);
        s_pVBO = nullptr;
        s_pQuadVBO = nullptr;

        wolf::ProgramManager::DestroyProgram(s_pProgram);
        wolf::ProgramManager::DestroyProgram(s_pQuadProgram);
        s_pProgram = nullptr;
        s_pQuadProgram = nullptr;
    }

    // Deregister for lighting events
    wolf::EventManager::RemoveListener<LightToggleEvent, LightComponent, &LightComponent::HandleLightToggleEvent>(*this);
}

// !-- Init MUST be called before LightComponent::Update is called or a SegFault WILL MOST DEFINITELY occur --!
void LightComponent::Init() {
    // If this component has already been initialized
    if (m_pTransform) {
        // Then don't do it again!
        return;
    }

    // Retrieve the transform
    m_pTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();
    m_v2Origin = m_pTransform->GetGlobalPosition();

    // Retrieve the scene
    m_pScene = &this->GetGameObject()->GetScene();

    // Add the light collider
    m_pCollider = &this->GetGameObject()->AddComponent<ColliderComponent>(ColliderComponent::NONE, false, true);
    m_pCollider->AddColliderBox(m_v2CurRadius, glm::vec2(-m_v2CurRadius.x / 2.0f, m_v2CurRadius.y / 2.0f));

    // Retrieve the labyrinth manager
    for (auto&& [_, lbmg] : this->m_pScene->Each<LabyrinthManager>())
    {
        m_pLabyrinthManager = &lbmg;
        break;
    }

    // Get a pointer to the LightComponent's GameObject's parent (if one exists)
    m_pParentGO = this->GetGameObject()->GetParent();
}

// ------------------------------------------------------------------------------------------------------------
//                                         Begin Lighting Pass
// ------------------------------------------------------------------------------------------------------------

void LightComponent::Update(float p_fDelta) {
    // If the light's AOE collider isn't active or the light isn't on
    if (!m_pCollider->IsActive() || !m_bIsOn) {
        // Then we don't want to do anything at all!
        return;
    }

    // Update the origin point of the light's radius
    m_v2Origin = m_pTransform->GetGlobalPosition();

    // Scale the radius to match the GlobalScale
    // (this happens repeatedly in case the light's GameObject suddenly gains a parent and the scale changes)
    m_v2CurRadius = m_v2InitRadius * m_pTransform->GetGlobalScale(); 

    // Empty out the map of last frame's ray end points
    m_vv2fCollidingPoints.clear();

    // Create a vector to hold all of the colliders that are in the light's AOE
    std::vector<wolf::Rectangle> vpRectanglesInAOE;

    // ---------------------------------- Phase 1: Finding colliders in AOE ----------------------------------------------

    // Find the colliders that are inside the area of effect by iterating through the colliders in the scene
    for (auto&& [_, collider] : m_pScene->Each<ColliderComponent>()) {
        // If the collider is active and in this area of effect
        if (collider.IsActive() && ColliderManager::StaticMethodIsColliding(*m_pCollider, collider, p_fDelta)) {

            // If this light's GameObject has a parent and the collider we're looking at belongs to them
            if (m_pParentGO && m_pParentGO->GetID() == collider.GetGameObject()->GetID()) {
                // Then we want to ignore it
                continue;
            }

            // If the collider we're looking at is another light's AOE collider
            if (this->GetGameObject()->GetID() != collider.GetGameObject()->GetID() && collider.GetGameObject()->GetComponent<LightComponent>()) {
                // Then we want to ignore it
                continue;
            }

            // If this collider is part of a projectile
            if (collider.IsHurtboxDamageDealer()) {
                // Then we want to ignore it
                continue;
            }

            // If this collider doesn't have a type AND it doesn't have a LightComponent then it's a trap
            if (collider.GetColliderType() == ColliderComponent::ColliderType::NONE && collider.GetGameObject()->GetComponent<LightComponent>() == nullptr) {
                // So we want to ignore it
                continue;
            }

            // Go through the corner points of each rectangle in the collider
            std::vector<glm::vec2> vv2ColliderCorners = collider.GetWorldSpaceCorners();
            for (int k = 0; k < vv2ColliderCorners.size(); k += 4) {
                // Find the top left and bottom right points
                glm::vec2 v2TopLeft = vv2ColliderCorners.at(k);
                glm::vec2 v2BotRight = vv2ColliderCorners.at(k + 2);

                // Check if the light's origin is inside of the rectangle
                if (m_v2Origin.x > v2TopLeft.x && m_v2Origin.x < v2BotRight.x && m_v2Origin.y > v2BotRight.y && m_v2Origin.y < v2TopLeft.y) {
                    // If it is, and the rectangle is NOT the light's AOE collider
                    if (this->GetGameObject()->GetID() != collider.GetGameObject()->GetID()) {
                        // Then the light is inside of a wall/solid object and we don't want to draw ANY rays
                        return;
                    }
                }

                // Check if the rectangle is completely outside of the light's radius
                // (This can happen because wall colliders are grouped by chunk)
                if ((v2TopLeft.x > m_v2Origin.x + m_v2CurRadius.x * 0.5f) ||
                    (v2TopLeft.y < m_v2Origin.y - m_v2CurRadius.y * 0.5f) ||
                    (v2BotRight.x < m_v2Origin.x - m_v2CurRadius.x * 0.5f) ||
                    (v2BotRight.y > m_v2Origin.y + m_v2CurRadius.y * 0.5f))
                {
                    // If it is, we do not want to cast rays to it
                    continue;
                }

                // Check if the rectangle is part of a wall
                if (CheckForWallAtPos({(v2TopLeft.x + v2BotRight.x) * 0.5f, (v2TopLeft.y + v2BotRight.y) * 0.5f})) {

                    // If it is, we want to make a rectangle for each individual tile within it.
                    // To do that, we get the width and height of the rectangle...
                    int iWidth = v2BotRight.x - v2TopLeft.x;
                    int iHeight = v2TopLeft.y - v2BotRight.y;

                    // ...and use that to determine how many tiles are inside of it
                    int iNumTilesX = iWidth / 96;
                    int iNumTilesY = iHeight / 96;

                    // If we're subdividing along the X-axis
                    if (iWidth >= 96) {
                        // We iterate through each of the tiles
                        for (int i = 0; i <= iNumTilesX - 1; i++) {
                            // Figure out what we need to increment the initial X coordinate by for the next "step" in the subdivision
                            float fXInc = i * 96.0f;

                            // Use that incremental value to find the top-left and bottom-right of the next rectangle
                            glm::vec2 v2SubXTopLeft = {v2TopLeft.x + fXInc, v2TopLeft.y};
                            glm::vec2 v2SubXBotRight = {v2TopLeft.x + 96.0f + fXInc, v2BotRight.y};

                            // Check if the new rectangle is outside of the light's AOE
                            if ((v2SubXTopLeft.x > m_v2Origin.x + m_v2CurRadius.x * 0.5f) ||
                                (v2SubXTopLeft.y < m_v2Origin.y - m_v2CurRadius.y * 0.5f) ||
                                (v2SubXBotRight.x < m_v2Origin.x - m_v2CurRadius.x * 0.5f) ||
                                (v2SubXBotRight.y > m_v2Origin.y + m_v2CurRadius.y * 0.5f))
                            {
                                // If it is, we move onto the next subdivision
                                continue;
                            }

                            // Then add the new rectangle to the list of rects that will be checked later
                            vpRectanglesInAOE.push_back(wolf::Rectangle(v2SubXTopLeft.x, v2SubXTopLeft.y, v2SubXBotRight.x, v2SubXBotRight.y)); 
                        }
                    }
                    else if (iHeight > 96) { // If we're subdividing along the Y axis
                        // We follow the same process as the X-axis and iterate through the tiles
                        for (int j = 0; j <= iNumTilesY - 1; j++) {
                            // Find the value we need to decrement the initial Y coordinate by for the next "step" in the subdivision
                            float fYDec = j * 96.0f;

                            // Find the top-left and bottom-right points on the new rectangle
                            glm::vec2 v2SubYTopLeft = {v2TopLeft.x, v2TopLeft.y - fYDec};
                            glm::vec2 v2SubYBotRight = {v2TopLeft.x + 96.0f, v2TopLeft.y - 96.0f - fYDec};

                            // Check if the rectangle is outside of the light's AOE
                            if ((v2SubYTopLeft.x > m_v2Origin.x + m_v2CurRadius.x * 0.5f) ||
                                (v2SubYTopLeft.y < m_v2Origin.y - m_v2CurRadius.y * 0.5f) ||
                                (v2SubYBotRight.x < m_v2Origin.x - m_v2CurRadius.x * 0.5f) ||
                                (v2SubYBotRight.y > m_v2Origin.y + m_v2CurRadius.y * 0.5f))
                            {
                                // If it is, we move onto the next subdivision
                                continue;
                            }

                            // Then we add the new rectangle to the list of rects that will be checked later
                            vpRectanglesInAOE.push_back(wolf::Rectangle(v2SubYTopLeft.x, v2SubYTopLeft.y, v2SubYBotRight.x, v2SubYBotRight.y)); 
                        }
                    }
                }
                else {
                    // If we've made it this far, we send a copy of this rectangle to the next phase of the collision testing
                    vpRectanglesInAOE.push_back(wolf::Rectangle(v2TopLeft.x, v2TopLeft.y, v2BotRight.x, v2BotRight.y));  
                }
            }
        }
    }

    // ------------------------------- Phase 2: Finding potential collision points  --------------------------------------

    // Go through all of the rectangles in the AOE
    for (wolf::Rectangle rect : vpRectanglesInAOE) {
        // Get the corners of the rectangle
        // !-- GetCorners() returns: top-left, top-right, bottom-left, bottom-right --!
        std::array<glm::vec2, 4> arv2Corners = rect.GetCorners();

        // And store them in variables for easy readability
        glm::vec2 v2TopLeft = arv2Corners[0];
        glm::vec2 v2TopRight = arv2Corners[1];
        glm::vec2 v2BotLeft = arv2Corners[2];
        glm::vec2 v2BotRight = arv2Corners[3];

        // Figure out the approximate location of the rectangle in relation to the light source
        glm::vec2 v2RectPos = glm::vec2((v2TopLeft.x + v2TopRight.x) * 0.5f, (v2TopLeft.y + v2BotLeft.y) * 0.5f);
        RoughPosition enRoughPos = this->CalculateRoughObjPosition(v2RectPos);

        // Use said location to figure out which sides of the rectangle the light could
        // be hitting and which sides can be safely ignored.
        switch (enRoughPos) {
            case TOP_LEFT:
                // Shoot a line to the bottom-left, bottom-right, and top-right corners
                this->CheckForCollisionAndAdd(v2BotRight, {v2BotLeft, v2BotRight});

                /* --------------------------------------------------------------------------------------
                // The points that are furthest away from the light when the object is in one of the
                // diagonal positions (TOP_LEFT, TOP_RIGHT, BOT_LEFT, BOT_RIGHT) have the potential
                // to cause the light to shoot a ray through the object so we check those points against
                // the side directly across from them before they get added to the colliding points vector
                // --------------------------------------------------------------------------------------- */
                this->CheckForCollisionAndAdd(v2BotLeft, {v2TopRight, v2BotRight});
                this->CheckForCollisionAndAdd(v2TopRight, {v2BotLeft, v2BotRight});
        
            break;
            
            case TOP_CENTER:
                // Shoot a line to the bottom-left and bottom-right corners
                this->CheckForCollisionAndAdd(v2BotLeft, {v2BotLeft, v2BotRight});
                this->CheckForCollisionAndAdd(v2BotRight, {v2BotLeft, v2BotRight});

            break;
            
            case TOP_RIGHT:
                // Shoot a line to the top-left, bottom-left, and bottom-right corners
                this->CheckForCollisionAndAdd(v2BotLeft, {v2BotLeft, v2BotRight});

                // Make sure that the ray doesn't go through the rectangle
                this->CheckForCollisionAndAdd(v2BotRight, {v2TopLeft, v2BotLeft});
                this->CheckForCollisionAndAdd(v2TopLeft, {v2BotLeft, v2BotRight});

            break;
            
            case MID_LEFT:
                // Shoot a line to the top-right and bottom-right corners
                this->CheckForCollisionAndAdd(v2TopRight, {v2TopLeft, v2TopRight});
                this->CheckForCollisionAndAdd(v2BotRight, {v2BotLeft, v2BotRight});

            break;
            
            case MID_RIGHT:
                // Shoot a line to the top-left and bottom-left corners
                this->CheckForCollisionAndAdd(v2TopLeft, {v2TopLeft, v2TopRight});
                this->CheckForCollisionAndAdd(v2BotLeft, {v2BotLeft, v2BotRight});

            break;
            
            case BOT_LEFT:
                // Shoot a line to the top-left, top-right, and bottom-right corners
                this->CheckForCollisionAndAdd(v2TopRight, {v2TopLeft, v2TopRight});

                // Make sure that the ray doesn't go through the rectangle
                this->CheckForCollisionAndAdd(v2TopLeft, {v2TopRight, v2BotRight});
                this->CheckForCollisionAndAdd(v2BotRight, {v2TopLeft, v2TopRight});

            break;
            
            case BOT_CENTER:
                // Shoot a line to the top-left and top-right corners
                this->CheckForCollisionAndAdd(v2TopLeft, {v2TopLeft, v2TopRight});
                this->CheckForCollisionAndAdd(v2TopRight, {v2TopLeft, v2TopRight});

            break;
            
            case BOT_RIGHT:
                // Shoot a line to the top-right, top-left, and bottom-left corners
                this->CheckForCollisionAndAdd(v2TopLeft, {v2BotLeft, v2BotRight});

                // Make sure that the ray doesn't go through the rectangle
                this->CheckForCollisionAndAdd(v2BotLeft, {v2TopLeft, v2TopRight});
                this->CheckForCollisionAndAdd(v2TopRight, {v2TopLeft, v2BotLeft});

            break;

            case SELF:
                // Shoot a line to the four corners of the light's radius rectangle and in a + shape centered at the light's origin
                // (If these lines intersect something they'll be removed in the next pass)
                m_vv2fCollidingPoints.push_back({v2TopLeft, CalculateAngleOfIntersection(v2TopLeft)});
                m_vv2fCollidingPoints.push_back({v2TopRight, CalculateAngleOfIntersection(v2TopRight)});
                m_vv2fCollidingPoints.push_back({v2BotLeft, CalculateAngleOfIntersection(v2BotLeft)});
                m_vv2fCollidingPoints.push_back({v2BotRight, CalculateAngleOfIntersection(v2BotRight)});
            
            break;
        }

        // We also want to add any point where the rectangle's sides collide with the borders of the light's AOE
        this->CheckForAOECollisionAndAdd(v2TopLeft, v2BotLeft);     // Check and add left
        this->CheckForAOECollisionAndAdd(v2TopRight, v2BotRight);   // Check and add right
        this->CheckForAOECollisionAndAdd(v2TopLeft, v2TopRight);    // Check and add top
        this->CheckForAOECollisionAndAdd(v2BotLeft, v2BotRight);    // Check and addbottom
    }

// ------------------------ Phase 3: Removing obstructed or impossible collision points  -----------------------------

    // We don't want to draw light rays that go THROUGH the rectangles in the scene
    // so we are going to need to check if any of our lines intersect with other colliders

    // When we do that check, we're going to be adding and removing things from m_vv2fCollidingPoints
    // in-between iterations through the list of rectangles in the AOE, so we make two vectors to hold
    // the points that are being added or removed midway.
    std::vector<std::pair<glm::vec2, float>> vv2fPointsToRemove;
    std::vector<std::pair<glm::vec2, float>> vv2fPointsToAdd;

    // Then we go through all of the rectangles in the AOE again
    for (wolf::Rectangle rect : vpRectanglesInAOE) {
        // Clear the lists of points to add and remove
        vv2fPointsToRemove.clear();
        vv2fPointsToAdd.clear();

        // Get the corner points associated with this rectangle
        std::array<glm::vec2, 4> arv2RectCorners = rect.GetCorners();

        // And use them to onstruct the sides again
        glm::vec2 v2TopStart = arv2RectCorners[0]; // Top
        glm::vec2 v2TopEnd = arv2RectCorners[1];

        glm::vec2 v2BotStart = arv2RectCorners[2]; // Bottom
        glm::vec2 v2BotEnd = arv2RectCorners[3];

        glm::vec2 v2LeftStart = arv2RectCorners[0]; // Left
        glm::vec2 v2LeftEnd = arv2RectCorners[2];

        glm::vec2 v2RightStart = arv2RectCorners[1]; // Right
        glm::vec2 v2RightEnd = arv2RectCorners[3];

        // Determine if this rectangle is part of a wall tile or the light's AOE
        bool bRectIsWall = CheckForWallAtPos({(v2TopStart.x + v2BotEnd.x) * 0.5f, (v2TopStart.y + v2BotEnd.y) * 0.5f});
        bool bIsAOE = this->IsAOERect(rect);

        // Then go through all of the corner points that we THINK we'll be casting a light ray to
        for (std::pair<glm::vec2, float> v2fCorner : m_vv2fCollidingPoints) {
            // Skip corner points that belong to the rectangle we're currently looking at (provided it is not part of a wall)
            if (!bRectIsWall && std::find(arv2RectCorners.begin(), arv2RectCorners.end(), v2fCorner.first) != arv2RectCorners.end()) {
                continue;
            }

            // Check for a collision between the ray shot to this corner point and the rectangle we're currently interested in
            std::pair<bool, glm::vec2> v2fLeftResullt = this->LineLineCollisionTest(m_v2Origin, v2fCorner.first, v2LeftStart, v2LeftEnd);
            std::pair<bool, glm::vec2> v2fRightResullt = this->LineLineCollisionTest(m_v2Origin, v2fCorner.first, v2RightStart, v2RightEnd);
            std::pair<bool, glm::vec2> v2fTopResullt = this->LineLineCollisionTest(m_v2Origin, v2fCorner.first, v2TopStart, v2TopEnd);
            std::pair<bool, glm::vec2> v2fBotResullt = this->LineLineCollisionTest(m_v2Origin, v2fCorner.first, v2BotStart, v2BotEnd);

            // Check if the line between the origin and the corner point we're casting to intersects with another collider.
            if (v2fLeftResullt.first || v2fRightResullt.first || v2fTopResullt.first || v2fBotResullt.first)
            {
                // If it does, then we want to replace corner point we're looking at with the nearest point of intersection
                // between the light and the rectangle instead, so we get the distance of each potential intersect
                float fLeftDist = glm::distance(m_v2Origin, v2fLeftResullt.second);
                float fRightDist = glm::distance(m_v2Origin, v2fRightResullt.second);
                float fTopDist = glm::distance(m_v2Origin, v2fTopResullt.second);
                float fBotDist = glm::distance(m_v2Origin, v2fBotResullt.second);

                // Then we find the point with the shortest distance
                float fMinDist = std::min(fLeftDist, std::min(fRightDist, std::min(fTopDist, fBotDist)));

                // And add it to a vector of points that will be added to m_vv2fCollidingPoints in the next pass
                if (fMinDist == fLeftDist) {
                    // Left intersection point
                    vv2fPointsToAdd.push_back({v2fLeftResullt.second, CalculateAngleOfIntersection(v2fLeftResullt.second)});
                }
                else if (fMinDist == fRightDist) {
                    // Right intersection point
                    vv2fPointsToAdd.push_back({v2fRightResullt.second, CalculateAngleOfIntersection(v2fRightResullt.second)});
                }
                else if (fMinDist == fTopDist) {
                    // Top intersection point
                    vv2fPointsToAdd.push_back({v2fTopResullt.second, CalculateAngleOfIntersection(v2fTopResullt.second)});
                }
                else if (fMinDist == fBotDist) {
                    // Bottom intersection point
                    vv2fPointsToAdd.push_back({v2fBotResullt.second, CalculateAngleOfIntersection(v2fBotResullt.second)});
                }
                
                // Then mark the original point for removal
                vv2fPointsToRemove.push_back(v2fCorner);
            }
        }

        // Remove the points this rectangle collided with (if any)
        for (std::pair<glm::vec2, float> v2fBadCorner : vv2fPointsToRemove) {
            auto it = std::find(m_vv2fCollidingPoints.begin(), m_vv2fCollidingPoints.end(), v2fBadCorner);
            if (it != m_vv2fCollidingPoints.end()) {
                m_vv2fCollidingPoints.erase(it);
            }
        }

        // And add in the new intersection points (if any)
        for (std::pair<glm::vec2, float> v2fGoodPoint : vv2fPointsToAdd) {
            m_vv2fCollidingPoints.push_back(v2fGoodPoint);
        }
    }

    // If by some devilry we have removed every point in m_vv2fCollidingPoints, we want to
    // fail gracefully via error rather than SegFaulting so we return here
    if (m_vv2fCollidingPoints.empty()) {
        wolf::Error("Problem in LightingComponent: all points removed from m_vv2fCollidingPoints");
        return;
    }

    // --------------------------------------- Phase 4: Assembling geometry  ---------------------------------------------

    // Otherwise, we sort the collision points by the slopes of their intersection lines
    std::sort(m_vv2fCollidingPoints.begin(), m_vv2fCollidingPoints.end(), LightComponent::CompareVec2FloatPair);

    // Get the first collison point (we'll need it for the final triangle)
    glm::vec2 v2FirstPoint = m_vv2fCollidingPoints.back().first;

    // Form triangles using the two points that form each side and the origin
    while (m_vv2fCollidingPoints.size() != 1) {
        // Take the first point out of the vector
        glm::vec2 v2Point1 = m_vv2fCollidingPoints.back().first;
        m_vv2fCollidingPoints.pop_back();

        glm::vec2 v2Point2 = m_vv2fCollidingPoints.back().first;
        // We don't pop the second point because we want the triangles to connect to each other

        // Add the triangle to the vertex data
        m_vcvVertexData.push_back({m_v2Origin.x, m_v2Origin.y, m_v4Color.r, m_v4Color.g, m_v4Color.b, m_v4Color.a});
        m_vcvVertexData.push_back({v2Point1.x, v2Point1.y, m_v4Color.r, m_v4Color.g, m_v4Color.b, m_v4Color.a});
        m_vcvVertexData.push_back({v2Point2.x, v2Point2.y, m_v4Color.r, m_v4Color.g, m_v4Color.b, m_v4Color.a});
    }

    // Pop the last point and use it to form the final triangle
    glm::vec2 v2LastPoint = m_vv2fCollidingPoints.back().first;
    m_vv2fCollidingPoints.pop_back();

    // Form a final triangle from the first and last points in m_iv2CollidingPoints and the origin
    m_vcvVertexData.push_back({m_v2Origin.x, m_v2Origin.y, m_v4Color.r, m_v4Color.g, m_v4Color.b, m_v4Color.a});
    m_vcvVertexData.push_back({v2FirstPoint.x, v2FirstPoint.y, m_v4Color.r, m_v4Color.g, m_v4Color.b, m_v4Color.a});
    m_vcvVertexData.push_back({v2LastPoint.x, v2LastPoint.y, m_v4Color.r, m_v4Color.g, m_v4Color.b, m_v4Color.a});
}

// ------------------------------------------------------------------------------------------------------------
//                                         End Lighting Pass
// ------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------
//                                         Helper Functions
// ------------------------------------------------------------------------------------------------------------

// Determines if a ray can be shot to the given corner without intersecting the given side and adds the corner to the vector
// of colliding points if it can. If the ray does intersect the given side, then the point of intersection is added to the
// colliding points vector, instead
void LightComponent::CheckForCollisionAndAdd(const glm::vec2& p_v2Corner, std::pair<const glm::vec2&, const glm::vec2&> p_v2v2Side) {
    // Check that the corner point is not outside of the radius
    if (!(p_v2Corner.x > m_v2Origin.x + m_v2CurRadius.x * 0.5f) &&
        !(p_v2Corner.x < m_v2Origin.x - m_v2CurRadius.x * 0.5f) &&
        !(p_v2Corner.y > m_v2Origin.y + m_v2CurRadius.y * 0.5f) &&
        !(p_v2Corner.y < m_v2Origin.y - m_v2CurRadius.y * 0.5f))
    {
        // Then check if the ray we're casting goes through the rectangle this corner is a part of
        glm::vec2 v2FinalPoint = this->LineLineCollisionTest(m_v2Origin, p_v2Corner, p_v2v2Side.first, p_v2v2Side.second).second;
        float fAngle = this->CalculateAngleOfIntersection(v2FinalPoint);

        // Then either add the point of self-collision or the corner point to the vector or collision points
        m_vv2fCollidingPoints.push_back({v2FinalPoint, fAngle});

        // Then create two angles that are slightly offset from the ray we intend to shoot
        float fPosOffsetAngle = fAngle + glm::radians(1.0f);
        float fNegOffsetAngle = fAngle - glm::radians(1.0f);

        // And use them to shoot two more rays that will go beyond the object's corners and hit the wall behind them
        glm::vec2 v2PosOffset = {m_v2Origin.x + m_v2CurRadius.x * glm::sin(fPosOffsetAngle), m_v2Origin.y + m_v2CurRadius.y * glm::cos(fPosOffsetAngle)};
        this->CheckForAOECollisionAndAdd(m_v2Origin, v2PosOffset);

        glm::vec2 v2NegOffset = {m_v2Origin.x + m_v2CurRadius.x * glm::sin(fNegOffsetAngle), m_v2Origin.y + m_v2CurRadius.y * glm::cos(fNegOffsetAngle)};
        this->CheckForAOECollisionAndAdd(m_v2Origin, v2NegOffset);
    }
}

// Determines if a line intersects with any of the AOE rectangle's sides and adds the point(s)
// of intersection to the list of colliding points if so
void LightComponent::CheckForAOECollisionAndAdd(const glm::vec2& p_v2LineStart, const glm::vec2& p_v2LineEnd) {
    // Find the AOE's corner points
    glm::vec2 v2TopLeft = {m_v2Origin.x - m_v2CurRadius.x * 0.5f, m_v2Origin.y + m_v2CurRadius.y * 0.5f};
    glm::vec2 v2TopRight = {m_v2Origin.x + m_v2CurRadius.x * 0.5f, m_v2Origin.y + m_v2CurRadius.y * 0.5f};
    glm::vec2 v2BotLeft = {m_v2Origin.x - m_v2CurRadius.x * 0.5f, m_v2Origin.y - m_v2CurRadius.y * 0.5f};
    glm::vec2 v2BotRight = {m_v2Origin.x + m_v2CurRadius.x * 0.5f, m_v2Origin.y - m_v2CurRadius.y * 0.5f};

    // Check for a collison with the left side
    std::pair<bool, glm::vec2> bv2LeftResult = this->LineLineCollisionTest(v2TopLeft, v2BotLeft, p_v2LineStart, p_v2LineEnd);
    if (bv2LeftResult.first) {
        // Add the point of intersection if it exists
        m_vv2fCollidingPoints.push_back({bv2LeftResult.second, this->CalculateAngleOfIntersection(bv2LeftResult.second)});
    }

    // Check for a collision with the right side
    std::pair<bool, glm::vec2> bv2RightResult = this->LineLineCollisionTest(v2TopRight, v2BotRight, p_v2LineStart, p_v2LineEnd);
    if (bv2RightResult.first) {
        // Add the point of intersection if it exists
        m_vv2fCollidingPoints.push_back({bv2RightResult.second, this->CalculateAngleOfIntersection(bv2RightResult.second)});
    }

    // Check for a collision with the top side
    std::pair<bool, glm::vec2> bv2TopResult = this->LineLineCollisionTest(v2TopLeft, v2TopRight, p_v2LineStart, p_v2LineEnd);
    if (bv2TopResult.first) {
        // Add the point of intersection if it exists
        m_vv2fCollidingPoints.push_back({bv2TopResult.second, this->CalculateAngleOfIntersection(bv2TopResult.second)});
    }

    // Check for a collision with the bottom side
    std::pair<bool, glm::vec2> bv2BotResult = this->LineLineCollisionTest(v2BotLeft, v2BotRight, p_v2LineStart, p_v2LineEnd);
    if (bv2BotResult.first) {
        // Can you guess what we're gonna do here?
        m_vv2fCollidingPoints.push_back({bv2BotResult.second, this->CalculateAngleOfIntersection(bv2BotResult.second)});
    }
    
}

// Figure out the rough position of the object in relation to the light's origin point by treating the world
// as a 3x3 grid with the light in the center position
LightComponent::RoughPosition LightComponent::CalculateRoughObjPosition(const glm::vec2& p_v2ObjCenterPos) {
    // Test the diagonals first
    if (p_v2ObjCenterPos.x < m_v2Origin.x && p_v2ObjCenterPos.y > m_v2Origin.y) return LightComponent::TOP_LEFT;
    if (p_v2ObjCenterPos.x > m_v2Origin.x && p_v2ObjCenterPos.y > m_v2Origin.y) return LightComponent::TOP_RIGHT;
    if (p_v2ObjCenterPos.x < m_v2Origin.x && p_v2ObjCenterPos.y < m_v2Origin.y) return LightComponent::BOT_LEFT;
    if (p_v2ObjCenterPos.x > m_v2Origin.x && p_v2ObjCenterPos.y < m_v2Origin.y) return LightComponent::BOT_RIGHT;

    // Then the cardinal directions
    if (p_v2ObjCenterPos.y < m_v2Origin.y) return LightComponent::BOT_CENTER;
    if (p_v2ObjCenterPos.y > m_v2Origin.y) return LightComponent::TOP_CENTER;
    if (p_v2ObjCenterPos.x < m_v2Origin.x) return LightComponent::MID_LEFT;
    if (p_v2ObjCenterPos.x > m_v2Origin.x) return LightComponent::MID_RIGHT;

    // If none of those succeed then the the light is comparing against itself
    return SELF;
}

// Check if the line between the light and a given corner point intersects with a given side of a rectangle,
// and return the point of intersection if it does, and the original point if it does not.
// Uses the line-line intersection test from this address:
// https://www.jeffreythompson.org/collision-detection/line-line.php
std::pair<bool, glm::vec2> LightComponent::LineLineCollisionTest(const glm::vec2& p_v2AStart, const glm::vec2& p_v2AEnd, const glm::vec2& p_v2BStart, const glm::vec2& p_v2BEnd) {
    // Variables that will be overriden if the lines intersect
    bool bIntersected = false;
    glm::vec2 v2Intersect = p_v2AEnd;

    // Calculate distance to intersection point
    float fDistX = ((p_v2BEnd.x - p_v2BStart.x) * (p_v2AStart.y - p_v2BStart.y) - (p_v2BEnd.y - p_v2BStart.y) * (p_v2AStart.x - p_v2BStart.x)) 
                / ((p_v2BEnd.y - p_v2BStart.y) * (p_v2AEnd.x - p_v2AStart.x) - (p_v2BEnd.x - p_v2BStart.x) * (p_v2AEnd.y - p_v2AStart.y));

    float fDistY = ((p_v2AEnd.x - p_v2AStart.x) * (p_v2AStart.y - p_v2BStart.y) - (p_v2AEnd.y - p_v2AStart.y) * (p_v2AStart.x - p_v2BStart.x))
                / ((p_v2BEnd.y - p_v2BStart.y) * (p_v2AEnd.x - p_v2AStart.x) - (p_v2BEnd.x - p_v2BStart.x) * (p_v2AEnd.y - p_v2AStart.y));

    // Check for collision
    if (fDistX >= 0 && fDistX <= 1 && fDistY >= 0 && fDistY <= 1) {
        bIntersected = true;
        v2Intersect.x = p_v2AStart.x + (fDistX * (p_v2AEnd.x - p_v2AStart.x));
        v2Intersect.y = p_v2AStart.y + (fDistX * (p_v2AEnd.y - p_v2AStart.y));
    }

    // Return the results
    return {bIntersected, v2Intersect};
}

float LightComponent::CalculateAngleOfIntersection(const glm::vec2& p_v2Intersect) {
    // Calculate the slope of the line and use that as the hypotenuse
    glm::vec2 v2Direction = p_v2Intersect - m_v2Origin;

    // Calculate the cosine angle
    float fAngle = glm::atan(v2Direction.x, v2Direction.y);

    // Return the angle
    return fAngle;
}

bool LightComponent::CheckForWallAtPos(const glm::vec2& p_v2Pos) {
    // Convert the given world-space position to a tile position
    glm::vec2 v2TilePos = m_pLabyrinthManager->GetTilePosition(p_v2Pos);

    // Get the ID of the tile at that position
    int iTileID = m_pLabyrinthManager->GetTile(v2TilePos.x, v2TilePos.y);

    // And return true if it is a wall tile
    return (iTileID >= Tile::WallBottomLeft && iTileID <= Tile::WallTop);
}

// Helper function to check whether a given rectangle is the light's AOE rectangle
bool LightComponent::IsAOERect(const wolf::Rectangle& p_pRect) {
    // Get the corners of the rectangle
    std::array<glm::vec2, 4> arv2Corners = p_pRect.GetCorners();

    // Check if each of the points correspond to the AOE's corners
    return (arv2Corners[0] == glm::vec2(m_v2Origin.x - m_v2CurRadius.x * 0.5f, m_v2Origin.y + m_v2CurRadius.y * 0.5f) && // Top left
            arv2Corners[1] == glm::vec2(m_v2Origin.x + m_v2CurRadius.x * 0.5f, m_v2Origin.y + m_v2CurRadius.y * 0.5f) && // Top Right
            arv2Corners[2] == glm::vec2(m_v2Origin.x - m_v2CurRadius.x * 0.5f, m_v2Origin.y - m_v2CurRadius.y * 0.5f) && // Bottom Left
            arv2Corners[3] == glm::vec2(m_v2Origin.x + m_v2CurRadius.x * 0.5f, m_v2Origin.y - m_v2CurRadius.y * 0.5f));  // Bottom Right
}

// Compares two glm::vec2-float pairs and returns the one with the largest float value (break ties using the y coordinate)
bool LightComponent::CompareVec2FloatPair(std::pair<glm::vec2, float> p_v2fA, std::pair<glm::vec2, float> p_v2fB) {
    if (p_v2fA.second > p_v2fB.second) {
        return true;
    }

    return false;
}

// ------------------------------------------------------------------------------------------------------------
//                                              Rendering
// ------------------------------------------------------------------------------------------------------------

// CALL THIS METHOD BEFORE MAKING ANY LightComponent::RenderLightToFBO CALLS
void LightComponent::BindFBOAndBlendFunc() {
    // Get the current FBO and save it so we can bind it again later
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &s_iPrevBoundBuffer);

    // Bind the lighting FBO
    glBindFramebuffer(GL_FRAMEBUFFER, s_uiFBO);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// CALL THIS METHOD AFTER ALL LightComponent::RenderLightTOFBO CALLS HAVE COMPLETED
void LightComponent::UnbindFBOAndBlendFunc() {
    // Bind whatever buffer was bound before the lighting FBO
    glBindFramebuffer(GL_FRAMEBUFFER, s_iPrevBoundBuffer);

    // Disable blending
    glDisable(GL_BLEND);
}

// CALL THIS AFTER CALLING LightComponent::BindFBOAndBlendFunc
void LightComponent::RenderLightToFBO() {
    // If there is nothing colliding with the light or the light is not active
    if (m_vcvVertexData.empty() || !m_pCollider->IsActive()) {
        // We don't need to render anything
        return;
    }

    // Otherwise, render this light's geometry to the FBO
    glm::mat4 model = glm::mat4(1.0f);
    s_pProgram->SetUniform("model", model);
    s_pProgram->SetUniform("colour", m_v4Color);
    s_pProgram->SetUniform("radius", m_v2CurRadius.x / 2.0f);
    s_pProgram->SetUniform("lightPos", glm::vec3(m_v2Origin, 0.0f));
    s_pProgram->Bind();

    s_pVAO->Bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(ColouredVertex2D) * m_vcvVertexData.size(), m_vcvVertexData.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, m_vcvVertexData.size());
    m_vcvVertexData.clear();

    // Unbind the VAO
    glBindVertexArray(0);
}

// ONLY CALL THIS ONCE ALL LIGHTS HAVE FINISHED RENDERING TO THE FBO
// (LightComponent::RenderLightToFBO() for all lights, then LightComponent::UnbindFBOAndBlendFunc() once)
void LightComponent::BlendFBOAndScreen() {
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Bind the FBO texture
    glBindTextureUnit(11, s_uiTexture);

    // Bind the shader program
    s_pQuadProgram->Bind();

    // Bind the VAO and draw a screen-size quad
    s_pQuadVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, s_vtvTexQuadVertices.size());

    // Clean-up
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

// Clear the FBO DO THIS AT THE START OR END OF EACH UPDATE
void LightComponent::ClearFBO() {
    // Get the current FBO
    GLint iCurrentFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &iCurrentFBO);

    // Bind the lighting FBO and clear it to black
    glBindFramebuffer(GL_FRAMEBUFFER, s_uiFBO);
    glClearColor(0.0f, 0.0f, 0.0f, 0.75f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Then rebind the original FBO
    glBindFramebuffer(GL_FRAMEBUFFER, iCurrentFBO);
}

// !---------------------------- This code segment is courtesy of D'Anyil ---------------------------------!
void LightComponent::ResizeFBO(int p_iWidth, int p_iHeight) {
    // Ensure valid input
    assert(p_iWidth > 0 && p_iHeight > 0);

    // Don't bother if no fluid system components exist
    if (s_iRefCount < 1) return;

    // Create new texture
    glBindFramebuffer(GL_FRAMEBUFFER, s_uiFBO);
    GLuint newTexture;
    glGenTextures(1, &newTexture);
    glBindTexture(GL_TEXTURE_2D, newTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p_iWidth, p_iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Attach to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, newTexture, 0);

    // Ensure completeness
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        wolf::Error("Resize lighting framebuffer not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Delete old texture
    glDeleteTextures(1, &s_uiTexture);

    // Update our handle
    s_uiTexture = newTexture;
}
// !----------------------------------- End of D'Anyil's code segment ---------------------------------!

// ------------------------------------------------------------------------------------------------------------
//                                                Events
// ------------------------------------------------------------------------------------------------------------

void LightComponent::HandleLightToggleEvent(const LightToggleEvent& p_event) {
    // If this light has a parent GameObject
    if (m_pParentGO) {
        // And the parent is telling us to toggle
        if (m_pParentGO->GetID() == p_event.m_CallerID) {
            // Then do so!
            this->SetOn(p_event.m_bOn);
        }
    }
}