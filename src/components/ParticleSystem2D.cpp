//-----------------------------------------------------------------------------
// File:			ParticleSystem2D.cpp
// Original Author:	Youssef Ashraf
// ver 1.1
// class responsible for managing all instances of particles
//-----------------------------------------------------------------------------
#include "ParticleSystem2D.h"
#include <iostream>

// Constructor
ParticleSystem2D::ParticleSystem2D()
{
    if (s_refCount == 0)
    {
        s_pShader = wolf::ProgramManager::CreateProgram("data/shaders/particle2D.vs", "data/shaders/particle2D.fs");
    }
    s_refCount++;

    InitGLResources();
}

// Destructor
ParticleSystem2D::~ParticleSystem2D()
{
    s_refCount--;
    if (s_refCount == 0)
    {
        wolf::ProgramManager::DestroyProgram(s_pShader);
        s_pShader = nullptr;
    }

    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_instanceVBO);
    glDeleteVertexArrays(1, &m_vao);
}

// Register a ParticleComponent to be managed
void ParticleSystem2D::RegisterComponent(ParticleComponent* component)
{
    m_components.insert(component);
}

// Unregister a ParticleComponent
void ParticleSystem2D::UnregisterComponent(ParticleComponent* component)
{
    m_components.erase(component);
}

void ParticleSystem2D::TrackNewComponents()
{
    if (m_components.empty()) return;

    // Get a valid game object from any registered component
    wolf::GameObject* sampleGameObject = (*m_components.begin())->GetGameObject();
    if (!sampleGameObject) return;

    // Get the scene from that game object
    wolf::Scene& scene = sampleGameObject->GetScene();
    // ✅ **Efficiently iterating over all `ParticleComponent` instances**
    for (auto&& [entity, particleComponent] : scene.Each<ParticleComponent>())
    {
        if (m_components.find(&particleComponent) == m_components.end())
        {
            RegisterComponent(&particleComponent); // Auto-register if not already added
        }
    }
}

// Update all registered particle components
void ParticleSystem2D::Update(float delta)
{
    TrackNewComponents();  // Detect new particle components dynamically

    for (auto* component : m_components)
    {
        component->Update(delta);
    }
}

// Renders all active particles from registered components
void ParticleSystem2D::Render()
{
    if (!s_pShader) return;

    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    s_pShader->Bind();
    glBindVertexArray(m_vao);

    std::vector<glm::vec2> positions;
    std::vector<glm::vec4> colors;
    std::vector<float> sizes;

    int renderCount = 0;

    for (auto* component : m_components)
    {
        for (const auto& particle : component->GetParticles())
        {
            if (particle.m_active)
            {
                positions.push_back(particle.m_pos);
                colors.push_back(particle.m_color);
                sizes.push_back(particle.m_size);
                renderCount++;
            }
        }
    }


    if (positions.empty()) return;

    // Upload updated particle data
    glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2), positions.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_sizeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizes.size() * sizeof(float), sizes.data(), GL_STREAM_DRAW);

    glPointSize(3.0f); // ✅ Adjust particle size  

    glDrawArrays(GL_POINTS, 0, positions.size());

    glBindVertexArray(0);
    glDisable(GL_BLEND);
}







// Initializes OpenGL resources for rendering
void ParticleSystem2D::InitGLResources()
{
    // Generate quad VBO
    float quadVertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };
    glGenBuffers(1, &m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_posVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &m_colorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &m_sizeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_sizeVBO);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
}

// Shows ImGui editor for controlling particles
void ParticleSystem2D::ShowEditor()
{
    if (!m_showEditor) return; // Only show if toggled ON
    ImGui::Begin("Particle System Editor");

    ImGui::Text("Registered Particle Components: %d", static_cast<int>(m_components.size()));

    // Iterate over all registered components
    for (auto* component : m_components)
    {
        wolf::GameObject* owner = component->GetGameObject();
        std::string label = owner ? "Object ID: " + std::to_string(owner->GetID()) : "Unknown Object";

        if (ImGui::CollapsingHeader(label.c_str()))
        {
            // Adjustable properties
            glm::vec4 color = m_editorColor;
            ImGui::ColorEdit4("Particle Color", &color.r);

            float size = m_editorSize;
            ImGui::SliderFloat("Size", &size, 1.0f, 20.0f, "%.1f");

            float lifetime = m_editorLifetime;
            ImGui::SliderFloat("Lifetime", &lifetime, 0.1f, 5.0f, "%.1f");

            glm::vec2 velocity = m_editorVelocity;
            ImGui::SliderFloat2("Velocity", &velocity.x, -20.0f, 20.0f, "%.1f");

            // Apply changes to editor variables
            m_editorColor = color;
            m_editorSize = size;
            m_editorLifetime = lifetime;
            m_editorVelocity = velocity;

            // Spawn particle manually
            if (ImGui::Button("Emit Particle"))
            {
                std::cout << "Emit button pressed!" << std::endl;
                glm::vec2 position = owner->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                component->Emit(position, velocity, color, size, lifetime);
            }

            ImGui::SameLine();

            // Clear all particles in this component
            if (ImGui::Button("Clear Particles"))
            {
                for (auto& particle : component->GetParticles())
                {
                    particle.m_active = false;
                }
            }
        }
    }

    ImGui::End();
}


