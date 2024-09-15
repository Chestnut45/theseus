#include "TileMap.h"

#include <cstdint>
#include <fstream>
#include <W_Logging.h>
#include <stb_image.h>

TileMap::TileMap(int width, int height)
    :   m_width(width),
        m_height(height),
        m_tileData(width * height, 0)
{
}

TileMap::~TileMap()
{
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
        m_tileSetPath.clear();
    }

    // Check if the requested tile set is already loaded by another tilemap component
    const auto& it = s_tileSetIDMap.find(filepath);
    if (it != s_tileSetIDMap.end())
    {
        // Tile set already loaded, cache ID / file path, update reference count, and return
        m_arrayTexture = it->second.m_texID;
        m_tileSetPath = filepath;
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
        if (texelDataSize <= 0)
        {
            wolf::Error("Couldn't open image file: \"", line, "\"");
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
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width, height, tileCount);

        // Upload the entire array texture at once
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, tileCount, GL_RGBA, GL_UNSIGNED_BYTE, arrayTexData.data());
        
        // Set default filter and wrap parameters
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Create the map entry so the array texture can be shared between multiple tilemaps
        TileSetEntry entry;
        entry.m_texID = m_arrayTexture;
        s_tileSetIDMap[filepath] = entry;

        // Cache the tile set file path
        m_tileSetPath = filepath;

        // Return success
        return true;
    }
}

int TileMap::GetTile(int x, int y) const
{
    return 0;
}

void TileMap::SetTile(int x, int y, int tileID)
{

}

void TileMap::Clear()
{

}

void TileMap::Resize(int width, int height)
{
    
}