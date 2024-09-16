#include "W_TileMap.h"

#include <fstream>

#include <W_BufferManager.h>
#include <W_Logging.h>
#include <W_ProgramManager.h>

#include <stb_image.h>

namespace wolf
{

TileMap::TileMap(int mapWidth, int mapHeight)
    :   m_width(mapWidth),
        m_height(mapHeight),
        m_tileData(mapWidth * mapHeight, EMPTY_TILE)
{
    if (s_refCount == 0)
    {
        // Initialize static resources

        // Shader program for all tilemaps
        s_pProgram = wolf::ProgramManager::CreateProgram("data/shaders/tilemap.vs", "data/shaders/tilemap.fs");
        
        // Generate quad vertex buffer data
        float quadVerts[16] =
        {
            0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f
        };

        // Create quad vertex buffer
        s_pQuadVertexBuffer = wolf::BufferManager::CreateVertexBuffer(quadVerts, sizeof(quadVerts));

        // Generate quad index buffer data
        unsigned short quadInds[6] =
        {
            0, 2, 1, 1, 2, 3
        };

        // Create index buffer
        s_pQuadIndexBuffer = wolf::BufferManager::CreateIndexBuffer(quadInds, 6);
    }
    s_refCount++;

    // Generate VAO and VBO
    _GenerateVAO();
}

TileMap::~TileMap()
{
    // Cleanup per-tilemap resources
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);

    s_refCount--;
    if (s_refCount == 0)
    {
        // Cleanup static resources
        wolf::ProgramManager::DestroyProgram(s_pProgram);
        wolf::BufferManager::DestroyBuffer(s_pQuadVertexBuffer);
        wolf::BufferManager::DestroyBuffer(s_pQuadIndexBuffer);
    }
}

bool TileMap::LoadTileSet(const std::string& filepath)
{
    // Unload our current tile set if it exists
    if (m_arrayTexture != 0)
    {
        // Find the entry
        const auto& it = s_tileSetIDMap.find(m_tileSetPath);
        if (it != s_tileSetIDMap.end())
        {
            // Reduce reference count and delete if necessary
            it->second.m_refCount--;
            if (it->second.m_refCount == 0)
            {
                // Delete the texture object and the entry from the map
                glDeleteTextures(1, &it->second.m_texID);
                s_tileSetIDMap.erase(m_tileSetPath);
            }
        }

        // Set our cached values to default
        m_arrayTexture = 0;
        m_tileWidth = 0;
        m_tileHeight = 0;
        m_tileSetPath.clear();
    }

    // Check if the requested tile set is already loaded by another tilemap component
    const auto& it = s_tileSetIDMap.find(filepath);
    if (it != s_tileSetIDMap.end())
    {
        // Tile set already loaded

        // Grab array texture ID and path
        m_arrayTexture = it->second.m_texID;
        m_tileSetPath = filepath;

        // Grab tile size
        m_tileWidth = it->second.m_tileSize.x;
        m_tileHeight = it->second.m_tileSize.y;

        // Update reference counter and return
        it->second.m_refCount++;
        return true;
    }
    else
    {
        // Tile set has NOT been loaded and must be created

        // Open the tile set file
        std::ifstream file(filepath);

        // Ensure file opened correctly
        if (!file.is_open())
        {
            wolf::Error("Couldn't open tile set file: \"", filepath, "\"");
            return false;
        }

        // Parse first line
        std::string line;
        std::getline(file, line);

        // Load first texture data
        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* pFirstImageData = stbi_load(line.c_str(), &width, &height, &channels, 4);

        // Calculate size of image data array
        size_t texelDataSize = width * height * channels;

        // Ensure first image loads correctly
        if (!pFirstImageData)
        {
            wolf::Error("Couldn't open tile image file: \"", line, "\"");
            stbi_image_free(pFirstImageData);
            return false;
        }

        // Create the buffer to hold all texel data for each tile in the set
        std::vector<unsigned char> arrayTexData;
        
        // Initialize the array texture buffer with the first image's texel data
        arrayTexData.insert(arrayTexData.end(), &pFirstImageData[0], &pFirstImageData[texelDataSize]);

        // Free first image data
        stbi_image_free(pFirstImageData);

        // Parse rest of lines in the tile set file
        int tileCount = 1;
        while (std::getline(file, line))
        {
            // Early out if we pass the 2048 tile limit
            if (++tileCount >= 2048)
            {
                wolf::Error("Tile set limit exceeded (Max of 2048 tiles per set)");
                return false;
            }

            // Load next image data
            int nextWidth = 0;
            int nextHeight = 0;
            int nextChannels = 0;
            unsigned char* pNextImageData = stbi_load(line.c_str(), &nextWidth, &nextHeight, &nextChannels, 4);

            // Make sure the next image loaded correctly
            if (!pNextImageData)
            {
                wolf::Error("Couldn't load tile image file: \"", line, "\"");
                stbi_image_free(pNextImageData);
                return false;
            }

            // Calculate size of next image data
            size_t nextTexelDataSize = nextWidth * nextHeight * nextChannels;

            // Early out if one of the tile images is a different size
            if (nextTexelDataSize != texelDataSize)
            {
                wolf::Error("Tile set size mismatch: \"", line, "\"");
                stbi_image_free(pNextImageData);
                return false;
            }

            // Append the next image's texel data to the array texture buffer
            arrayTexData.insert(arrayTexData.end(), &pNextImageData[0], &pNextImageData[nextTexelDataSize]);

            // Free the next image's texel data now that we've stored it
            stbi_image_free(pNextImageData);
        }

        // If we reach here every tile image has loaded correctly and our array texture buffer is full

        // Create the array texture object and bind
        glGenTextures(1, &m_arrayTexture);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_arrayTexture);

        // Allocate the storage for the texture data
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height, tileCount);

        // Upload the entire array texture at once
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, tileCount, GL_RGBA, GL_UNSIGNED_BYTE, arrayTexData.data());
        
        // Set default filter and wrap parameters
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Create the map entry so the array texture can be shared between multiple tilemaps
        // NOTE: Entry's reference counter initializes to one automatically
        TileSetEntry entry;
        entry.m_texID = m_arrayTexture;
        entry.m_tileSize = glm::ivec2(width, height);
        s_tileSetIDMap[filepath] = entry;

        // Cache the tile set file path
        m_tileSetPath = filepath;

        // Cache texture size from tile set
        m_tileWidth = width;
        m_tileHeight = height;

        // Return success
        return true;
    }
}

int TileMap::GetTile(int x, int y) const
{
    // Calculate index and return empty if out of bounds
    int index = y * m_width + x;
    if (index < 0 || index >= m_tileData.size()) return EMPTY_TILE;

    // Return actual tile value if in bounds
    return m_tileData[index];
}

void TileMap::SetTile(int x, int y, int tileID)
{
    // Calculate index and do nothing if out of bounds
    int index = y * m_width + x;
    if (index < 0 || index >= m_tileData.size()) return;

    // Update tile id and set update flag
    m_tileData[index] = tileID;
    m_VBODirty = true;
}

void TileMap::Clear()
{
    // Set every tile to -1
    std::fill(m_tileData.begin(), m_tileData.end(), EMPTY_TILE);
    m_VBODirty = true;
}

void TileMap::Resize(int width, int height)
{
    // Resize the internal flattened tile ID array and clear
    m_tileData.resize(width * height);
    Clear();

    // Regenerate VAO and VBO
    _GenerateVAO();
}

void TileMap::Draw(const glm::vec2& position, float rotationRadians, const glm::vec2& scale, const glm::vec3& tint)
{
    // Early out if no tile set is loaded
    if (m_arrayTexture == 0) return;

    // Ensure VBO data is up-to-date
    if (m_VBODirty) _UpdateVBO();

    // Bind resources
    s_pProgram->Bind();
    glBindVertexArray(m_VAO);
    glBindTextureUnit(1, m_arrayTexture);

    // Calculate model matrix
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(position, 0.0f));
    model = glm::rotate(model, rotationRadians, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(scale, 1.0f));

    // Set program uniforms
    s_pProgram->SetUniform("model", model);
    s_pProgram->SetUniform("tileWidth", m_tileWidth);
    s_pProgram->SetUniform("tileHeight", m_tileHeight);
    s_pProgram->SetUniform("mapTint", tint);

    // Draw and unbind
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr, m_tilesToDraw);
    glBindVertexArray(0);
}

void TileMap::_UpdateVBO()
{
    // Buffer to hold new VBO data
    std::vector<float> data;

    // Iterate through all tiles in the map
    int n = m_tileData.size();
    int toDraw = 0;
    for (int i = 0; i < n; ++i)
    {
        int tile = m_tileData[i];
        if (tile != EMPTY_TILE)
        {
            // Add the tile's data to the local buffer

            // NOTE: This could be optimized by caching a LUT containing
            // the positions for each tile index in the flattened array.
            data.push_back(i % m_width); // X coordinate
            data.push_back(i / m_width); // Y coordinate
            data.push_back(tile); // Index into the tile set

            // Increase counter
            toDraw++;
        }
    }

    // Upload to GPU buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Set draw count
    m_tilesToDraw = toDraw;
    
    // Reset flag
    m_VBODirty = false;
}

void TileMap::_GenerateVAO()
{
    // Delete VAO and VBO if already initialized
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);

    // Create VAO
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    // Bind the static quad vertex buffers
    s_pQuadVertexBuffer->Bind();
    s_pQuadIndexBuffer->Bind();

    // Setup quad vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Create and bind tile VBO
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // Create storage for tile VBO
    glBufferData(GL_ARRAY_BUFFER, m_tileData.size() * sizeof(float) * 3, nullptr, GL_STATIC_DRAW);

    // Add instanced tile vertex attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Unbind VAO and buffers
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}