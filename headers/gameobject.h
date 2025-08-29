#pragma once

#include <vector>
#include <SDL3/SDL.h>
#include "../ext/glm/glm.hpp"
#include "../headers/animation.h"

enum class PlayerState {
    idle, running, jumping, dead
};
enum class BulletState {
    moving, colliding, inactive
};
enum class EnemyState {
    idle, damaged, dead
};

struct PlayerData {
    PlayerState state;
    Timer weaponTimer;
    Timer deathTimer;
    int healthPoints;
    PlayerData() : weaponTimer(0.3f), deathTimer(3.0f)
    {
        state = PlayerState::idle;
        healthPoints = 1;
    }
};
struct LevelData {};
struct EnemyData {
    EnemyState state;
    Timer damagedTimer;
    int healthPoints;
    EnemyData() : state(EnemyState::idle), damagedTimer(0.5f) {
        healthPoints = 3;
    }
};
struct BulletData {
    BulletState state;
    BulletData() : state(BulletState::moving)
    {

    }
};

union ObjectData {
    PlayerData player;
    LevelData level;
    EnemyData enemy;
    BulletData bullet;
};

enum class ObjectType {
    player, level, enemy, bullet
};

struct GameObject {
    ObjectType type;
    ObjectData data;
    glm::vec2 pos, vel, acc;
    float dir;
    float maxSpeedX;
    std::vector<Animation> animations;
    int curAnimation;
    SDL_Texture *texture;
    bool dynamic;
    bool grounded;
    SDL_FRect collider; // rectangle for collision
    Timer flashTimer;
    bool shouldFlash;
    int spriteFrame;
    GameObject() : data{.level = LevelData()}, collider{ 0 }, flashTimer(0.05f)
    {
        type = ObjectType::level;
        dir = 1;
        maxSpeedX = 0;
        pos = vel = acc = glm::vec2(0);
        curAnimation = -1;
        texture = nullptr;
        dynamic = false;
        grounded = false;
        shouldFlash = false;   
        spriteFrame = 1;
    }
};