#include <NPCComponent.h>
#include <ColliderComponent.h>
#include <LabyrinthManager.h>
int NPCComponent::m_iNextID = 0;
wolf::RNG NPCComponent::s_RNG;

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
    // If the NPC is inactive, do not update
    if (!m_isActive)
    {    
        return;
    }

    // Code taken from D'Anyil in EnemyController
    for (auto&&[_, lm] : GetGameObject()->GetScene().Each<LabyrinthManager>())
    {
        // Get the ID of the chunk that the NPC is currently on
        glm::ivec2 newChunkID = lm.GetChunkID(m_pTransform->GetGlobalPosition());
        // If the current ID is different from the previous one
        if (m_chunkID != newChunkID)
        {
            // Get the chunk object
            wolf::GameObject* pChunk = lm.GetChunk(newChunkID);
            
            // if the chunk object exists
            if (pChunk)
            {   
                pChunk->AddChild(*GetGameObject()); // Add the NPC object as a child of the chunk
                m_chunkID = newChunkID; // Update the chunk ID
                m_isActive = lm.IsChunkActive(m_chunkID); // Update active flag

                // If the current chunk is not active, then change state to IDLE
                if(!m_isActive) 
                {
                    ChangeState(State::IDLE);
                }
            }
        }
        break;
    }

    // If we have no health left
    if (m_pHealthComp->GetHealth() <= 0 && m_state != State::DEAD) {
        // If we're dead we pretend we're playing dialogue so that the player can't talk to us (perhaps we're praying?)
        m_bPlayingDialogue = true;

        // Check if we have an open merchant inventory
        if (m_pMerchInvComp && m_pMerchInvComp->IsOpen()) {
            // And close it if so
            m_pMerchInvComp->Close();
        }
        // Then transition to the death state
        ChangeState(State::DEAD);
    }

    // Handle the current state of the NPC accordingly
    switch (m_state)
    {
        case State::IDLE:
        {
            HandleIdleState(p_fDelta);
            break;
        }
        case State::ROAM:
        {
            HandleRoamState(p_fDelta);
            break;
        }
        case State::STUNNED:
        {
            HandleStunnedState(p_fDelta);
            break;
        }
        case State::DEAD:
        {
            HandleDeadState(p_fDelta);
            break;
        }
        default:
        {
            break;
        }
    }
}

// Call this method once the Health, AnimatedSprite2D, Velocity, and optionally the MerchantInventory
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

    // Retrieve the Velocity Component
    m_pVeloComp = this->GetGameObject()->GetComponent<VelocityComponent>();

    m_state = State::IDLE;
    m_isActive = true;
}

void NPCComponent::HandleDeadState(float p_fDelta) {
    // *** This method is taken directly from Nhat's HandleDeathState() in the PlayerController ***
    // Fall over
    if(m_fFallDeadTimer <= m_fTimeToFallDead)
    {
        if(m_fFallDeadTimer == 0.0f)
        {
            m_pVeloComp->SetVelocity(glm::vec2(0.0f));

            ColliderComponent* collider = this->GetGameObject()->GetComponent<ColliderComponent>();
            if(collider != nullptr)
            {
                collider->SetIgnoreTag(this->GetGameObject()->GetScene().GetPlayerID());
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
    if (!m_bPlayingDialogue) {
        TurnTowardsPlayer();
        ChangeState(State::IDLE);
        
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

// Immediately play a given dialogue/cutscene sequence regardless of what is in the queue
void NPCComponent::TriggerDialogue(const std::string& p_strEntryID) {
    // If we are not already playing a dialogue or cutscene sequence
    if (!m_bPlayingDialogue) {
        // Look for the given sequence
        NPCDialogueEntry* dialogue = m_mDialogueEntries.at(p_strEntryID);
        if (dialogue) {
            // And play it if it exists
            wolf::EventManager::TriggerEvent(DialogueAndCutsceneEvent(dialogue->strDialogueID, m_strDialogueFilePath, m_iID));
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
                // If the sequence that just played WASN'T us saying goodbye
                if (p_event.sequenceID != "goodbye") {
                    // Open the store
                    m_pMerchInvComp->Open();
                }
            }
        }
    }
}

void NPCComponent::SayGoodbye() {
    this->TriggerDialogue("goodbye");
}

void NPCComponent::StunNPC()
{
    ChangeState(State::STUNNED);
}

void NPCComponent::SetActive(bool p_active)
{
    // std::cout << "Set Active: " << p_active << std::endl;
    m_isActive = p_active;
}

void NPCComponent::ChangeState(State p_state)
{   
    // We exit the old state
    switch (m_state)
    {
        case State::IDLE:
        {
            break;
        }
        case State::ROAM:
        {
            break;
        }
        case State::STUNNED:
        {
            ExitStunnedState();
            break;
        }
        default:
        {
            break;
        }
    }

    // And enter the new state
    switch (p_state)
    {
        case State::IDLE:
        {
            EnterIdleState();
            break;
        }
        case State::ROAM:
        {
            EnterRoamState();
            break;
        }
        case State::STUNNED:
        {
            EnterStunnedState();
            break;
        }
        case State::DEAD:
        {
            break;
        }
        default:
        {
            break;
        }
    }

    // Update the current state of the NPC
    m_state = p_state;
}

void NPCComponent::EnterIdleState()
{
    m_fIdleTimer = s_RNG.NextFloat(2.0f, 4.0f); // Reset the idle timer
    m_pVeloComp->SetVelocity(glm::vec2(0.0f)); // Set the NPC velocity to 0
    m_iRoamBlockedCounter = 0;  // Reset the roam-blocked counter
}

void NPCComponent::EnterRoamState()
{
    m_fRoamTimer = s_RNG.NextFloat(4.0f, 6.0f); // Reset the roam timer
    glm::vec2 newVector = glm::normalize(glm::vec2(s_RNG.NextFloat(-5.0f, 5.0f), s_RNG.NextFloat(-5.0f, 5.0f))) * m_fRoamSpeed; // Get a random roam direction
    TurnToDirection(newVector); // Set the NPC sprite to the new direction
    m_pVeloComp->SetVelocity(newVector); // Set the NPC velocity to the new velocity
    m_fRoamSpeedCheckTimer = 0.0f; // reset the speed-check timer
}
void NPCComponent::EnterStunnedState()
{
    m_fStunnedTimer = m_fStunnedTime; // Reset the stunned timer
    m_pAnimSpriteComp->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE); // Set NPC sprite to be completely white
    m_pVeloComp->SetVelocity(glm::vec2(0.0f, 0.0f)); // Set the NPC velocity to 0
}

void NPCComponent::HandleIdleState(float p_fDelta)
{
    // If the time for idling is over, change state to Rome
    if(m_fIdleTimer <= 0.0f)
    {
        ChangeState(State::ROAM);
    }
    // If not, update the timer
    else
    {
        m_fIdleTimer -= p_fDelta;
    }
}

void NPCComponent::HandleRoamState(float p_fDelta)
{
    // If the time for Romans is over, change state to idle
    if(m_fRoamTimer <= 0.0f)
    {
        ChangeState(State::IDLE);
    }
    // If not, update the timer
    else
    {
        m_fRoamTimer -= p_fDelta;
    
        // If the speed-check timer has expired, reset the timer & check the roam speed
        if(m_fRoamSpeedCheckTimer <= 0.0f)
        {
            m_fRoamSpeedCheckTimer = m_fRoamSpeedCheckTime;
            CheckRoamSpeed();
        }
        // If not, then update the timer
        else
        {
            m_fRoamSpeedCheckTimer -= p_fDelta;
        }
    }
}

void NPCComponent::HandleStunnedState(float p_fDelta)
{
    // If the stunned timer has expired, change to idle state
    if(m_fStunnedTimer <= 0.0f)
    {
        ChangeState(State::IDLE);
    }
    // If not, update the timer
    else
    {   
        m_fStunnedTimer -= p_fDelta;
    }
}

void NPCComponent::ExitStunnedState()
{
    // Disable the white sprite effect
    m_pAnimSpriteComp->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
}

void NPCComponent::CheckRoamSpeed()
{
    glm::vec2 currentVelocity = m_pVeloComp->GetVelocity();

    // If the NPC is under the speed limit - indicates that the NPC is sliding against a wall at a steep angle
    if(currentVelocity.length() <= m_RoamSpeedMin)
    {
        glm::vec2 newVelocity = currentVelocity;
        // If the NPC is sliding along the X axis
        if(currentVelocity.x != 0.0f && currentVelocity.y == 0.0f)
        {
            // Set the velocity along the X axis based on roaming direction, with a slight offset for the Y axis
            if(currentVelocity.x > 0.0f)
            {
                newVelocity = glm::normalize(glm::vec2(m_fRoamSpeed, s_RNG.NextFloat(-20.0f, 20.0f))) * m_fRoamSpeed;
            }
            else
            {
                newVelocity = glm::normalize(glm::vec2(-m_fRoamSpeed, s_RNG.NextFloat(-20.0f, 20.0f))) * m_fRoamSpeed;
            }
            
        }
        
        // If the NPC is sliding along the Y axis
        else if(currentVelocity.y != 0.0f && currentVelocity.x == 0.0f)
        {
            // Set the velocity along the Y axis based on roaming direction, with a slight offset for the X axis
            if(currentVelocity.y > 0.0f)
            {
                newVelocity = glm::normalize(glm::vec2(s_RNG.NextFloat(-20.0f, 20.0f), m_fRoamSpeed)) * m_fRoamSpeed;
            }
            else
            {
                newVelocity = glm::normalize(glm::vec2(s_RNG.NextFloat(-20.0f, 20.0f), -m_fRoamSpeed)) * m_fRoamSpeed;
            }
        }

        // // If the NPC is being blocked still by a wall or corner
        else if (currentVelocity.x == 0.0f && currentVelocity.y == 0.0f)
        {
            // If the NPC is blocked more times than the limit, reset counter & change to idle - prevents overly long roam chains
            if(m_iRoamBlockedCounter == m_iRoamBlockedLimit)
            {
                ChangeState(State::IDLE);
                return;
            }
            // If not, increase counter & restart roam
            else
            {
                m_iRoamBlockedCounter++;
                ChangeState(State::ROAM);
                return;
            }
        }

        // Apply calculated velocity to the component
        if(newVelocity != currentVelocity)
        {
            m_pVeloComp->SetVelocity(newVelocity);
            TurnToDirection(newVelocity);
        }
    }
}

void NPCComponent::TurnTowardsPlayer()
{
    // Figure out where the player is and rotate to face them
    wolf::Scene* pScene = &this->GetGameObject()->GetScene();
    glm::vec2 v2PlayerPos = pScene->GetObject(pScene->GetPlayerID())->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 v2MyPos = m_pTransform->GetGlobalPosition();

    // If the player is to our right...
    if (v2PlayerPos.x > v2MyPos.x) {
        if (v2PlayerPos.y > v2MyPos.y + 64.0f) { // ...and above us
            m_pAnimSpriteComp->SetAnimation("StandNorth");
        }
        else if (v2PlayerPos.y < v2MyPos.y - 64.0f) { // ...and below us
            m_pAnimSpriteComp->SetAnimation("StandSouth");
        }
        else { // ...and roughly in-line with us
            m_pAnimSpriteComp->SetAnimation("StandEast");
        }
    }
    else { // If the player is to our left...
        if (v2PlayerPos.y > v2MyPos.y + 64.0f) { // ...and above us
            m_pAnimSpriteComp->SetAnimation("StandNorth");
        }
        else if (v2PlayerPos.y < v2MyPos.y - 64.0f) { //...and below us
            m_pAnimSpriteComp->SetAnimation("StandSouth");
        }
        else { // ...and roughly in-line with us
            m_pAnimSpriteComp->SetAnimation("StandWest");
        }
    }
}

void NPCComponent::TurnToDirection(glm::vec2 p_vDirection)
{
    glm::vec2 v2MyPos = m_pTransform->GetGlobalPosition();
    glm::vec2 v2NewPos = v2MyPos + p_vDirection;
    // If the new position is to our right...
    if (v2NewPos.x > v2MyPos.x) {
        if (v2NewPos.y > v2MyPos.y + 64.0f) { // ...and above us
            m_pAnimSpriteComp->SetAnimation("StandNorth");
        }
        else if (v2NewPos.y < v2MyPos.y - 64.0f) { // ...and below us
            m_pAnimSpriteComp->SetAnimation("StandSouth");
        }
        else { // ...and roughly in-line with us
            m_pAnimSpriteComp->SetAnimation("StandEast");
        }
    }
    else { // If the new position is to our left...
        if (v2NewPos.y > v2MyPos.y + 64.0f) { // ...and above us
            m_pAnimSpriteComp->SetAnimation("StandNorth");
        }
        else if (v2NewPos.y < v2MyPos.y - 64.0f) { //...and below us
            m_pAnimSpriteComp->SetAnimation("StandSouth");
        }
        else { // ...and roughly in-line with us
            m_pAnimSpriteComp->SetAnimation("StandWest");
        }
    }
}