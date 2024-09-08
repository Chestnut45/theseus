#pragma once

//-----------------------------------------------------------------------------
// File:			W_Sprite2D.h
// Original Author:	D'Anyil Landry
//
// A class representing a renderable 2D sprite component.
//-----------------------------------------------------------------------------

#include "W_Texture.h"
#include <string>

namespace wolf
{

class Sprite2D
{

// Public implementation
public:

    // Create a sprite component with the given texture
    Sprite2D(const std::string& texturePath);
    ~Sprite2D();

    // TODO: Render directly to screen

    // TODO: Render via Scene2D, Transform2D, and Camera2D

// Implementaton
private:

    wolf::Texture* m_pTexture = nullptr;

    // TODO: VAO, VBO, Shader program
};

}