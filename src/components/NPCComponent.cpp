#include "NPCComponent.h"

int NPCComponent::m_iNextID = 0;

NPCComponent::NPCComponent(const std::string& p_strName, const std::string& p_strDialogueFilePath, const std::string& p_strDropTableFilePath, bool p_bIsMerchant, bool p_bCanBeMerchant)
    : m_strName(p_strName), m_strDialogueFilePath(p_strDialogueFilePath), m_strDropTableFilePath(p_strDropTableFilePath), m_bCanBeMerchant(p_bCanBeMerchant)
{
    // If the NPC has the ability to be a merchant
    if (p_bCanBeMerchant) {
        // Then we let the user choose whether or not they START as a merchant
        m_bIsMerchant = p_bIsMerchant;
    }
    else {
        // Otherwise, we want to make sure that the NPC doesn't show as being a merchant
        // when the rest of the code is not treating them as one.
        // (We want to avoid the case where CanBeMerchant == FALSE but IsMerchant == TRUE)
        m_bIsMerchant = false;
    }
}

NPCComponent::~NPCComponent() {

}