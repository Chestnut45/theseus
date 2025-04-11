//-----------------------------------------------------------------------------
// File: DDACalculator.h
// Original Author: Nguyễn Minh Nhật
// Contains methods related to line-based grid traversal
//-----------------------------------------------------------------------------
#include <wolf.h>
#include <LabyrinthManager.h>
#include "VertexDeclarations.h"

class DDACalculator
{
public:
    static void CreateInstance(wolf::Scene* p_scene);
    static void DestroyInstance();

    static DDACalculator* GetInstance();

    glm::vec2 GetEndpoint(glm::vec2 p_src_pos, glm::vec2 p_dst_pos);
    std::vector<glm::ivec2> GetTraversedTiles(glm::vec2 p_src_pos, glm::vec2 p_dst_pos, bool p_is_blocked);
    LabyrinthManager* GetLabyrinthManager() const { return m_pLBMG; }


private:
    wolf::Scene* m_pScene = nullptr;
    LabyrinthManager* m_pLBMG = nullptr;

    static DDACalculator* s_pDDAC;

    DDACalculator(wolf::Scene* p_scene);
    ~DDACalculator();

    bool IsWallTile(int p_tile_id);
    glm::vec2 GetTileWorldPos(glm::ivec2 p_tile_pos);
};