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

    TrappedChestComponent(TrapType p_trap_type, bool p_is_taunting = true, bool p_is_tinting = true);
    virtual ~TrappedChestComponent();
    
    void Init();
    void Update(float p_delta);

    bool IsOpen();
    void OpenTrappedChest(); // Triggers trap
private:
    int m_iID = 0;

    // General member variables
    TrapType m_trapType = TrapType::EXPLODE;
    bool m_bIsOpen = false;
    float m_fSelfDestructTimer = 1.6f;

    //Taunting-related member variables
    bool m_bIsTaunting = true;
    int m_iTauntIndex = 0;

    // Tinting-related member variables
    bool m_bIsTinting = true;
    float m_fTintingTimer = 0.0f;
    float m_fTintingSpeed = 1.0f;
    float m_fTintingAccelerator = 1.75f;
    const float FULL_RAD = 2.0f * MATH_PI;

    // Explode-related member variables
    float m_fBlastRadius = 100.0f;
    glm::vec4 m_vBlastRadiusColour = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    float m_fRotateSpeed = 64.0f;
    float m_fRotateAngle = 0.0f;

    // Taunt-related static variables
    static wolf::RNG s_RNG;
    static std::vector<std::string> s_vTaunts;
    static std::vector<ImVec2> s_vTauntTextSizes;
    static const inline ImVec2 WINDOW_SIZE = ImVec2(256, 16);
    static const inline ImVec2 WINDOW_SIZE_HALF = ImVec2(128, 8);

    // static class object counter
    static inline int s_iComponentCount = 0;

    void DisplayTaunt();
    void DisplayTinting(float p_delta);
    void DisplayBlastRadius(float p_delta);
    void Explode();
    void SpawnGorgon();
    void SpawnHarpy();
    void SpawnMinitaur();

    static void InitTaunts();
};