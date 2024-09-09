#include "W_Scene2D.h"

#include "W_GameObject.h"
#include "W_Transform2D.h"
#include "W_Sprite2D.h"

namespace wolf
{

Scene2D::Scene2D()
{
}

Scene2D::~Scene2D()
{
}

GameObject& Scene2D::CreateObject()
{
    GameObjectID id = m_registry.create();
    GameObject& object = m_registry.emplace<GameObject>(id, *this, id);
    object.AddComponent<Transform2D>();
    return object;
}

void Scene2D::SetActiveCamera(Camera2D& camera)
{
    m_pActiveCamera = &camera;
}

void Scene2D::RemoveActiveCamera()
{
    m_pActiveCamera = nullptr;
}

void Scene2D::Update(float delta)
{
    // TODO: Sync all camera components to their transforms (smooth following?)
}

void Scene2D::Render()
{
    if (!m_pActiveCamera) return;

    // Bind the active camera
    m_pActiveCamera->Bind();

    // Render all sprites with transform components
    for (auto&&[_, sprite, transform] : Each<Sprite2D, Transform2D>())
    {
        sprite.Draw(transform.GetGlobalPosition(), transform.GetGlobalRotation(), transform.GetGlobalScale());
    }
}

}