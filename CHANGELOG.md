# Theseus v1.2 Changelog

## Features

### Fog of War

![fog.png](https://github.com/Chestnut45/theseus/blob/main-config-polish/screenshots/fog.png)

![minimap.png](https://github.com/Chestnut45/theseus/blob/main-config-polish/screenshots/minimap.png)

A dense fog now covers the labyrinth and obscures the minimap until explored.

### New Enemy: Snake

![snake.png](https://github.com/Chestnut45/theseus/blob/main-config-polish/screenshots/snake.png)

![snake_angry.png](https://github.com/Chestnut45/theseus/blob/main-config-polish/screenshots/snake_angry.png)

Venomous snakes now slither around the labyrinth, beware!

### New Weapons

![weapons.png](https://github.com/Chestnut45/theseus/blob/main-config-polish/screenshots/weapons.png)

- Burning Blade
- Poison-Tipped Spear
- Triple-Shot Bow
- Medusa's Bow
- Zeus' Wrath

### Daedalus' Terminal v1.0

![terminal.png](https://github.com/Chestnut45/theseus/blob/main-config-polish/screenshots/terminal.png)

- Full styling pass to match with the rest of the UI
- Shadow color and opacity can be set per config
- Fog color and opacity can be set per config
- Player light color and radius can be set per config
- The starting dispensary can be disabled per config
- Labyrinth configs can now list starting items to give to the player

## Bugfixes
- Fix pathfinding manager not regenerating with labyrinth
- Fix textured particles rendering in the wrong coordinate system
- Fix debug drawing / indicators having shadows applied
- Fix bow attack charge gui causing other windows to drop inputs
- Fix SFX stacking up to insane levels
    - There is now a small cooldown when the same sound cannot be played again
    - There is also an optional volume falloff based on how many voices are playing the same sound
- Fix harpy hitboxes
- Fix petrification timer / special effects
- Fix duplicate lights on NPCs
- Fix spawn room not being queryable from the labyrinth manager
- Fix missing GUI styling in a few areas
- Fix animations not freezing on death for some enemies
- Fix throwables spawning on top of pillars in the Minotaur's Chamber
- Fix seed determinism (loot tables, npc choices, and even drops are now consistent)
- Fix harpies shooting without line of sight to player
- Fix gorgon line of sight cheese when hugging a wall
- Fix portals not telefragging the boss
- Fix portal particles disappearing
- Fix portals teleporting harpies (they should not)
- Fix enemies sticking on each other
- Fix arrow knockback not scaling with charge amount
- Fix being able to pause in the death state
- Fix generation issues with small and non-square labyrinth configs
- Fix initial chunk activation bug with smaller labyrinths
- Fix wall collider generation bugs
- Fix monster spawner component adding enemies to the wrong chunk
- Fix death screen GUI not being centered
- Fix credits GUI not being centered
- Fix color flash in health / stamina bars
- Fix enemy controllers not being deactivated when in inactive chunks
- Fix crash when labyrinth config has bad data (returns to main menu now)

## Other
- Full balance pass on loot tables, dispensaries, item stats, and rarities
- Minimap changes
    - Icon colors adjusted
    - Empty chests no longer render into the map
    - Zoom is now linear
    - Added starting room to map render
- Added debug controls to pause menu in debug mode
- Added seed display to pause menu and final credits
- Added config caches to most data loaders to reduce stuttering
- Added additive blending to particles
- Made double spike roll possible vertically and horizontally
- Increase chunk size to avoid pop-in
- Status effects no longer give you invulnerability
- Added gorgon attack animation
- Added shadow toggle to LightComponent
- Added sprite tint for status effects
- Added a new collider type for occluders (cast shadows, not solid)
- Throwables now use mouse direction instead of player facing direction
- Allow boss to have status effects applied
- Items dropped from the player's inventory now last forever
- Non-animated sprites can now ignore lighting
- Healing status effect is now much less broken
- Allow LightComponents to change radius after Init
- Added idle direction variation to all enemies
- New SFX
    - Minitaur oink
    - Minitaur attack
    - Harpy wing flap
    - Harpy screech
    - Gorgon slither
    - Gorgon screech
    - Snake rattle
    - Snake hiss
    - Armor equip / unequip
    - Healing item use


## Secrets

Secret challenges await...

- goodluck
- gazedandconfused
- minitaurmania
- portalcombat
- gottagofast