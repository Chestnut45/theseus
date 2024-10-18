#pragma once

struct PercentHealthItemEvent {
    float fHealthChangeAmt;
};

struct PercentStaminaItemEvent {
    float fStaminaAmt;
};

struct FlatHealthItemEvent {
    float fHealthChangeAmt;
};

struct FlatStaminaItemEvent {
    float fStaminaAmt;
};

struct ApplyStatusEffectEvent {
    // Note that StatusEffectComponent and StatusEffectItems both use an enum to represent the type, but I did not think
    // it was a good idea to include the whole component header here so the event itself uses an integer.
    int iType;
    float fDuration;
};