//-----------------------------------------------------------------------------
// File: TrappedChestComponent.h
// Original Author: Nguyễn Minh Nhật
// Trap component for chests
//-----------------------------------------------------------------------------

#include <wolf.h>

class TrappedChestComponent : public wolf::BaseComponent
{
public:
    enum TrapType
    {
        EXPLODE,
        TRANSFORM_GORGON,
        TRANSFORM_HARPY,
        TRANSFORM_MINITAUR,
        NONE            // Indicates end of enum; NOT for external use
    };

    TrappedChestComponent(TrapType p_trap_event);
    virtual ~TrappedChestComponent();
    
    void Update(float p_delta);

    bool IsOpen();
    void OpenTrappedChest(); // Triggers trap
private:
    wolf::RNG m_RNG;

    int m_iID = 0;

    TrapType m_trapEvent = TrapType::EXPLODE;
    bool m_bIsOpen = false;
    float m_fSelfDestructTimer = 1.0f;
    int m_iTauntIndex = 0;
    
    static std::vector<std::string> s_vTaunts;

    static std::vector<ImVec2> s_vTauntTextSizes;

    static const inline ImVec2 WINDOW_SIZE = ImVec2(256, 16);
    static const inline ImVec2 WINDOW_SIZE_HALF = ImVec2(128, 8);

    static inline int s_iComponentCount = 0;

    void DisplayTaunt();
    void Explode();
    void SpawnGorgon();
    void SpawnHarpy();
    void SpawnMinitaur();

    static void InitTaunts();
};