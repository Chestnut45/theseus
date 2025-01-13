#include <NPCComponent.h>
#include <ColliderComponent.h>

int NPCComponent::m_iNextID = 0;

NPCComponent::NPCComponent(const std::string& p_strName, const std::string& p_strDialogueFilePath, std::unordered_map<std::string, NPCDialogueEntry*>& p_mDialogueEntries, const std::string& p_strDropTableFilePath, bool p_bIsMerchant, bool p_bCanBeMerchant)
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

    // Grab a reference to the dialogue entries map
    m_mDialogueEntries = p_mDialogueEntries;

    // Then queue up all of the entries that don't require a trigger
    for (const auto& entry : m_mDialogueEntries) {
        if (!entry.second->bHasTrigger) {
            m_pqDialogueQueue.push(entry.second);
        }
    }

    // Assign a unique ID number to this NPC and update the NextID counter
    m_iID = m_iNextID;
    m_iNextID += 1;

    wolf::EventManager::AddListener<DialogueOrCutsceneEndEvent, NPCComponent, &NPCComponent::HandleDialogueOrCutsceneEndEvent>(*this);
}

NPCComponent::~NPCComponent() {
    // Empty the dialogue queue
    this->EmptyDialogueQueue();

    // And delete all of the dialogue entries
    m_mDialogueEntries.clear();

    wolf::EventManager::RemoveListener<DialogueOrCutsceneEndEvent, NPCComponent, &NPCComponent::HandleDialogueOrCutsceneEndEvent>(*this);
}

void NPCComponent::Update(float p_fDelta) {
    // If we have no health left
    if (m_pHealthComp->GetHealth() <= 0) {
        // If we're dead we pretend we're playing dialogue so that the player can't talk to us (perhaps we're praying?)
        m_bPlayingDialogue = true;

        // Check if we have an open merchant inventory
        if (m_pMerchInvComp && m_pMerchInvComp->IsOpen()) {
            // And close it if so
            m_pMerchInvComp->Close();
        }

        // Then handle the death state
        this->HandleDeadState(p_fDelta);
    }
}

// Call this method once the Health, AnimatedSprite2D, and optionally the MerchantInventory
// components have been added to the NPC GameObject
void NPCComponent::Init() {
    // Retrieve the Transform2D Component
    m_pTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();

    // Retrieve the HealthComponent
    m_pHealthComp = this->GetGameObject()->GetComponent<HealthComponent>();

    // If this NPC can be a merchant, retrieve the MerchantInventoryComponent
    if (m_bCanBeMerchant) {
        m_pMerchInvComp = this->GetGameObject()->GetComponent<MerchantInventoryComponent>();
    }

    // Retrieve the AnimatedSprite2D Component
    m_pAnimSpriteComp = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
}

void NPCComponent::HandleDeadState(float p_fDelta) {
    // *** This method is taken directly from Nhat's HandleDeathState() in the PlayerController ***
    // Fall over
    if(m_fFallDeadTimer <= m_fTimeToFallDead)
    {
        if(m_fFallDeadTimer == 0.0f)
        {
            ColliderComponent* collider = this->GetGameObject()->GetComponent<ColliderComponent>();
            if(collider != nullptr)
            {
                collider->SetColliderType(ColliderComponent::ColliderType::NONE);
            }
            
            m_pAnimSpriteComp->SetTint(glm::vec3(1,0,0));
        }

        float angle = (90.0f / m_fTimeToFallDead) * p_fDelta;
        m_pTransform->RotateDegrees(angle);
        
        m_fFallDeadTimer += p_fDelta;
    }

    // Lie dead
    else
    {
        if(m_fLieDeadTimer >= m_fTimeToLieDead)
        {
            ItemDropCreator::Instance()->CreateItemDropFromLootTable(m_strDropTableFilePath, m_pTransform->GetGlobalPosition(), -1.0f);
            GetGameObject()->Delete();
        }
        m_fLieDeadTimer += p_fDelta;
    } 
}

// Play the dialogue entry with the lowest priority value and remove it from the queue
// (Note that replayable entries are re-added to the queue with a higher priority value
//  so that they will play AFTER new or unique dialogue)
void NPCComponent::PlayNextDialogue() {
    printf("Called it!\n");
    if (!m_bPlayingDialogue) {
        // Take the top element off of the queue
        NPCDialogueEntry* dialogue = m_pqDialogueQueue.top();
        m_pqDialogueQueue.pop();

        // Play the dialogue
        wolf::EventManager::TriggerEvent(DialogueAndCutsceneEvent(dialogue->strDialogueID, m_strDialogueFilePath, m_iID));

        // If the dialogue can be replayed
        if (dialogue->bCanRepeat) {
            // Add it back into the queue with a higher priority so that it will be at the back of the queue
            m_iCurHighPriorityVal += 1;
            dialogue->iPriority = m_iCurHighPriorityVal;
            m_pqDialogueQueue.push(dialogue);
        }
    }
}

// Add a dialogue entry to the priority queue
void NPCComponent::QueueDialogue(const std::string& p_strEntryID) {
    NPCDialogueEntry* dialogue = m_mDialogueEntries.at(p_strEntryID);
    if (dialogue) {
        if (!dialogue->bHasPlayed || dialogue->bHasPlayed && dialogue->bCanRepeat) {
            m_pqDialogueQueue.push(dialogue);

            // If the priority value of that item was higher than the current highest
            if (dialogue->iPriority > m_iCurHighPriorityVal) {
                // Set it as the current highest
                m_iCurHighPriorityVal = dialogue->iPriority;
            }
        }
    }
}

// Check whether or not a given dialogue entry has been played yet
bool NPCComponent::HasDialoguePlayed(const std::string& p_strEntryID) {
    NPCDialogueEntry* dialogue = m_mDialogueEntries.at(p_strEntryID);
    if (dialogue) {
        return dialogue->bHasPlayed;
    }
    return false;
}

// Change the priority level of a given dialogue entry
// ?-- Need to figure out how this will work with the priority queue --?
bool NPCComponent::ChangeDialoguePriority(const std::string& p_strEntryID, int p_iNewPriority) {
    NPCDialogueEntry* dialogue = m_mDialogueEntries.at(p_strEntryID);
    if (dialogue) {
        dialogue->iPriority = p_iNewPriority;
        return true;
    }
    return false;
}

// Wrapper method to empty the dialogue queue
void NPCComponent::EmptyDialogueQueue() {
    while (!m_pqDialogueQueue.empty()) {
        m_pqDialogueQueue.pop();
    }
}

void NPCComponent::HandleDialogueOrCutsceneEndEvent(const DialogueOrCutsceneEndEvent& p_event) {
    // If we were the NPC who triggered the dialogue (if the dialogue was triggered by an npc)
    if (p_event.triggerNPCID == m_iID) {
        // Find the dialogue in our map
        std::unordered_map<std::string, NPCDialogueEntry*>::const_iterator search = m_mDialogueEntries.find(p_event.sequenceID);
        if (search != m_mDialogueEntries.end()) {
            // And set it to played
            search->second->bHasPlayed = true;

            // Take ourselves out of the dialogue state
            m_bPlayingDialogue = false;

            // If we're a merchant right now
            if (m_bIsMerchant) {
                // Open the store
                m_pMerchInvComp->Open();
            }
        }
    }
}