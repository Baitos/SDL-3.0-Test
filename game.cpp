#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <string>
#include <array>
#include <iostream>
#include <format>

#include "headers/gameobject.h"

using namespace std;

struct SDLState
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    int width, height, logW, logH;
    const bool *keys;
    SDLState() : keys(SDL_GetKeyboardState(nullptr)) {

    }
};

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;

const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;

struct GameState {
    std::array<std::vector<GameObject>, 2> layers;
    std::vector<GameObject> bgTiles;
    std::vector<GameObject> fgTiles;
    std::vector<GameObject> bullets;
    int playerIndex;
    SDL_FRect mapViewport;
    float bg2Scroll, bg3Scroll, bg4Scroll;
    bool debugMode;

    GameState(const SDLState &state) {
        playerIndex = -1; // will change when map is loaded
        mapViewport = SDL_FRect {
            .x = 0,
            .y = 0,
            .w = static_cast<float>(state.logW),
            .h = static_cast<float>(state.logH)
        };
        bg2Scroll = bg3Scroll = bg4Scroll = 0;
        debugMode = false;
    }
    GameObject &player() {
        return layers[LAYER_IDX_CHARACTERS][playerIndex];
    }
};

struct Resources {
    const int ANIM_PLAYER_IDLE = 0;
    const int ANIM_PLAYER_RUN = 1;
    const int ANIM_PLAYER_SLIDE = 2;
    const int ANIM_PLAYER_SHOOT = 3;
    const int ANIM_PLAYER_JUMP = 4;
    const int ANIM_PLAYER_DIE = 5;
    std::vector<Animation> playerAnims;
    const int ANIM_BULLET_MOVING = 0;
    const int ANIM_BULLET_HIT = 1;
    std::vector<Animation> bulletAnims;
    const int ANIM_ENEMY = 0;
    const int ANIM_ENEMY_DEAD = 1;
    std::vector<Animation> enemyAnims;

    std::vector<SDL_Texture *> textures;
    SDL_Texture *texIdle, *texRun, *texJump, *texSlide, *texShoot, *texDie, 
                *texGrass, *texStone, *texBrick, *texFence, *texBush, 
                *texBullet, *texBulletHit, *texSpiny, *texSpinyDead,
                *texBg1, *texBg2, *texBg3, *texBg4;

    SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filepath) { // load texture from filepath
        // load game assets
        SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST); // pixel perfect
        textures.push_back(tex);
        return tex;
    }

    void load(SDLState &state) {
        playerAnims.resize(6); // 
        playerAnims[ANIM_PLAYER_IDLE] = Animation(1, 1.6f); // 1 frames, 1.6 seconds
        playerAnims[ANIM_PLAYER_RUN] = Animation(3, 0.3f);
        playerAnims[ANIM_PLAYER_SLIDE] = Animation(1, 1.0f);
        playerAnims[ANIM_PLAYER_SHOOT] = Animation(1, 1.0f);
        playerAnims[ANIM_PLAYER_JUMP] = Animation(1, 1.0f); 
        playerAnims[ANIM_PLAYER_DIE] = Animation(1, 1.0f);
        bulletAnims.resize(2); // 
        bulletAnims[ANIM_BULLET_MOVING] = Animation(4, 0.5f);
        bulletAnims[ANIM_BULLET_HIT] = Animation(3, 0.5f);
        enemyAnims.resize(2);
        enemyAnims[ANIM_ENEMY] = Animation(2, 0.6f);
        enemyAnims[ANIM_ENEMY_DEAD] = Animation(1, 1.0f);
        
        texIdle = loadTexture(state.renderer, "data/IdleM.png");
        texRun = loadTexture(state.renderer, "data/WalkLRM.png");
        texJump = loadTexture(state.renderer, "data/JumpM.png");
        texSlide = loadTexture(state.renderer, "data/SlideM.png");
        texShoot = loadTexture(state.renderer, "data/ShootM.png");
        texDie = loadTexture(state.renderer, "data/DieM.png");
        texGrass = loadTexture(state.renderer, "data/grass.png");
        texBrick = loadTexture(state.renderer, "data/brick.png");
        texStone = loadTexture(state.renderer, "data/stone.png");
        texBush = loadTexture(state.renderer, "data/bush.png");
        texFence = loadTexture(state.renderer, "data/fence.png");
        texBg1 = loadTexture(state.renderer, "data/bg_layer1.png");
        texBg2 = loadTexture(state.renderer, "data/bg_layer2.png");
        texBg3 = loadTexture(state.renderer, "data/bg_layer3.png");
        texBg4 = loadTexture(state.renderer, "data/bg_layer4.png");
        texBullet = loadTexture(state.renderer, "data/fireball.png");
        texBulletHit = loadTexture(state.renderer, "data/fireballHit.png");
        texSpiny = loadTexture(state.renderer, "data/Spiny.png");
        texSpinyDead = loadTexture(state.renderer, "data/SpinyDead.png");
    }

    void unload() {
        for (SDL_Texture *tex : textures) {
            SDL_DestroyTexture(tex);
        }
    }
};

bool initialize(SDLState &state);
void cleanup(SDLState &state);
void drawObject(const SDLState &state, GameState &gs, GameObject &obj, float width, float height, float deltaTime);
void update(const SDLState &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime);
void createTiles(const SDLState &state, GameState &gs, const Resources &res);
void checkCollision(const SDLState &state, GameState &gs, const Resources &res, GameObject &a, GameObject &b, float deltaTime);
void collisionResponse(const SDLState &state, GameState &gs, const Resources &res, 
                       const SDL_FRect &rectA, const SDL_FRect &rectB, 
                       const SDL_FRect &rectC, GameObject &a, GameObject &b, float deltaTime);
void handleKeyInput(const SDLState &state, GameState &gs, GameObject &obj,
                    SDL_Scancode key, bool keyDown);
void drawParallaxBackground(SDL_Renderer *renderer, SDL_Texture *texture, float xVelocity, float &scrollPos, float scrollFactor, float deltaTime);

bool running = true;

int main(int argc, char** argv) { // SDL needs to hijack main to do stuff; include argv/argc
    
    SDLState state;
    state.width = 1600;
    state.height = 900;
    state.logW = 640;
    state.logH = 480;

    // go to result when you die, should probably change!!!!
    //main_loop: absolutely do not use this holy shit my computer almost crashed. fork bomb!
    if (!initialize(state)) {
        return 1;
    }
    // load game assets
    Resources res;
    res.load(state);

    // setup game data
    GameState gs(state);
    createTiles(state, gs, res);
    uint64_t prevTime = SDL_GetTicks();

    // start game loop
    while (running) {
        uint64_t nowTime = SDL_GetTicks(); // take time from previous frame to calculate delta
        float deltaTime = (nowTime - prevTime) / 1000.0f; // convert to seconds from ms
        SDL_Event event { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                {
                    running = false;
                    break;
                }
                case SDL_EVENT_WINDOW_RESIZED: 
                {
                    state.width = event.window.data1;
                    state.height = event.window.data2;
                    
                    //printf("Width = %d, Height = %d", state.width, state.height);
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                {
                    handleKeyInput(state, gs, gs.player(), event.key.scancode, true);
                    break;
                }
                case SDL_EVENT_KEY_UP:
                {
                    handleKeyInput(state, gs, gs.player(), event.key.scancode, false);
                    if (event.key.scancode == SDL_SCANCODE_F12) {
                        gs.debugMode = !gs.debugMode;
                    }
                    break;
                }
            }
        }

        // update objs
        for (auto &layer : gs.layers) {
            for (GameObject &obj : layer) {
                update(state, gs, res, obj, deltaTime);
            }
        }
        // update bullets
        for (GameObject &bullet : gs.bullets) {
            update(state, gs, res, bullet, deltaTime);
        }
        // used for camera system
        gs.mapViewport.x = (gs.player().pos.x + TILE_SIZE / 2) - (gs.mapViewport.w / 2); 
        //draw stuff
        SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
        SDL_RenderClear(state.renderer);

        // draw background
        SDL_RenderTexture(state.renderer, res.texBg1, nullptr, nullptr);
        drawParallaxBackground(state.renderer, res.texBg4, gs.player().vel.x, gs.bg4Scroll, 0.075f, deltaTime);
        drawParallaxBackground(state.renderer, res.texBg3, gs.player().vel.x, gs.bg3Scroll, 0.15f, deltaTime);
        drawParallaxBackground(state.renderer, res.texBg2, gs.player().vel.x, gs.bg2Scroll, 0.3f, deltaTime);

        // draw bg tiles
        for (GameObject &obj : gs.bgTiles) {
            SDL_FRect dst {
                .x = obj.pos.x - gs.mapViewport.x,
                .y = obj.pos.y,
                .w = static_cast<float>(obj.texture->w),
                .h = static_cast<float>(obj.texture->h)
            };
            SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
        }

        // draw objs
        for (auto &layer : gs.layers) {
            for (GameObject &obj : layer) {
                drawObject(state, gs, obj, TILE_SIZE, TILE_SIZE, deltaTime);
            }
        }

        // draw bullets
        for (GameObject &bullet : gs.bullets) {
            if (bullet.data.bullet.state != BulletState::inactive) {
                drawObject(state, gs, bullet, bullet.collider.w, bullet.collider.h, deltaTime);
            }
        }

        // draw fg tiles
        for (GameObject &obj : gs.fgTiles) {
            SDL_FRect dst {
                .x = obj.pos.x - gs.mapViewport.x,
                .y = obj.pos.y,
                .w = static_cast<float>(obj.texture->w),
                .h = static_cast<float>(obj.texture->h)
            };
            SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
        }

        if (gs.debugMode) {
        // debug info
            SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(state.renderer, 5, 5,
                            std::format("State: {}, Bullet: {}, Grounded: {}", 
                            static_cast<int>(gs.player().data.player.state), gs.bullets.size(), gs.player().grounded).c_str());
        }
        //swap buffers and present
        SDL_RenderPresent(state.renderer);
        prevTime = nowTime;
        /*if (dead) {
            goto main_loop;
            dead = false;
        }*/
    }

    res.unload();
    cleanup(state);
    return 0;
}

bool initialize(SDLState &state) {
    bool initSuccess = true;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error Initializing SDL3", nullptr);
        initSuccess = false;
    } 
    state.window = SDL_CreateWindow("SDL3 Demo", state.width, state.height, SDL_WINDOW_RESIZABLE);
    if (!state.window) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error Creating Window", nullptr);
        cleanup(state);
        initSuccess = false;
    }

    state.renderer = SDL_CreateRenderer(state.window, nullptr);
    if (!state.renderer) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error Creating Renderer", nullptr);
        cleanup(state);
        initSuccess = false;
    }

    SDL_SetRenderVSync(state.renderer, 1); // turn this SHIT off

    // configure presentation
    SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    return initSuccess;
}

void cleanup(SDLState &state) {
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}

void drawObject(const SDLState &state, GameState &gs, GameObject &obj, float width, float height, float deltaTime) {
        float srcX = obj.curAnimation != -1 
                     ? obj.animations[obj.curAnimation].currentFrame() * width 
                     : (obj.spriteFrame - 1) * width;
        SDL_FRect src {
            .x = srcX,
            .y = 0,
            .w = width,
            .h = height
        };

        SDL_FRect dst {
            .x = obj.pos.x - gs.mapViewport.x,
            .y = obj.pos.y,
            .w = width,
            .h = height
        };
        SDL_FlipMode flipMode = obj.dir == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        if (!obj.shouldFlash) {
            SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode); // src is for sprite stripping, dest is for where sprite should be drawn
        } else {
            // flash with white tint
            SDL_SetTextureColorModFloat(obj.texture, 2.5f, 2.5f, 2.5f);  
            SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
            SDL_SetTextureColorModFloat(obj.texture, 1.0f, 1.0f, 1.0f);
            if (obj.flashTimer.step(deltaTime)) {
                obj.shouldFlash = false;
            }
        }


        if (gs.debugMode) {
            SDL_FRect rectA {
                .x = obj.pos.x + obj.collider.x - gs.mapViewport.x, 
                .y = obj.pos.y + obj.collider.y,
                .w = obj.collider.w, 
                .h = obj.collider.h
            };
            SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);

            SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 150);
            SDL_RenderFillRect(state.renderer, &rectA);
            SDL_FRect sensor{
			    .x = obj.pos.x + obj.collider.x - gs.mapViewport.x,
			    .y = obj.pos.y + obj.collider.y + obj.collider.h,
			    .w = obj.collider.w, .h = 1
		    };
		    SDL_SetRenderDrawColor(state.renderer, 0, 0, 255, 150);
		    SDL_RenderFillRect(state.renderer, &sensor);

            SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);
        }
}

void update(const SDLState &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime) {
    // update animation
    if (obj.curAnimation != -1) {
        obj.animations[obj.curAnimation].step(deltaTime);
    }
    if (obj.dynamic && !obj.grounded) {
        obj.vel += glm::vec2(0, 700) * deltaTime; // gravity
        //printf("x=%d, y=%d\n", obj.pos.x, obj.pos.y);
    }
    float currentDirection = 0;
    if (obj.type == ObjectType::player) {
        if (obj.data.player.state != PlayerState::dead) {
            if (state.keys[SDL_SCANCODE_A]) {
                currentDirection += -1;
            }
            if (state.keys[SDL_SCANCODE_D]) {
                currentDirection += 1;
            }
            Timer &weaponTimer = obj.data.player.weaponTimer;
            weaponTimer.step(deltaTime);
            const auto handleShooting = [&state, &gs, &res, &obj, &weaponTimer]() {
                if (state.keys[SDL_SCANCODE_J]) {
                    // bullets!
                    //obj.texture = res.texShoot; // in 2.5 hour video, go to 1:54:19 if you want to sync up shooting sprites with animations for running
                    if (weaponTimer.isTimeOut()) {
                        weaponTimer.reset();
                        GameObject bullet;
                        bullet.data.bullet = BulletData();
                        bullet.type = ObjectType::bullet;
                        bullet.dir = gs.player().dir;
                        bullet.texture = res.texBullet;
                        bullet.curAnimation = res.ANIM_BULLET_MOVING;
                        bullet.collider = SDL_FRect {
                            .x = 0,
                            .y = 0,
                            .w = static_cast<float>(res.texBullet->h),
                            .h = static_cast<float>(res.texBullet->h)
                        };
                        const float left = 0;
                        const float right = 24;
                        const float t = (obj.dir + 1) / 2.0f; // results in 0 to 1
                        const float xOffset = left + right * t; // LERP between left and right
                        const float yVariation = 40;
                        const float yVelocity = SDL_rand(yVariation) - yVariation / 2.0f;
                        bullet.vel = glm::vec2(
                        obj.vel.x + 300.0f * obj.dir, yVelocity);
                        //printf("bullet.vel.x = %f\n", bullet.vel.x);
                        bullet.maxSpeedX = 1000.0f;
                        bullet.animations = res.bulletAnims;
                        bullet.pos = glm::vec2( 
                            obj.pos.x + xOffset,
                            obj.pos.y + TILE_SIZE / 2 + 1
                        );
                        // try to reuse old inactive bullets
                        bool foundInactive = false;
                        for (int i = 0; i < gs.bullets.size() && !foundInactive; i++) {
                            if (gs.bullets[i].data.bullet.state == BulletState::inactive) {
                                foundInactive = true;
                                gs.bullets[i] = bullet;
                            }
                        }
                        // otherwise push new bullet
                        if (!foundInactive) {
                            gs.bullets.push_back(bullet);
                        }
                    }
                }
            };
            switch (obj.data.player.state) {
                case PlayerState::idle:
                {
                    if(currentDirection) { // if moving change to running
                        obj.data.player.state = PlayerState::running;
                    }
                    else {
                        if (obj.vel.x) { // slow player down when idle
                            const float factor = obj.vel.x > 0 ? -1.5f : 1.5f;
                            float amount = factor * obj.acc.x * deltaTime;
                            if (std::abs(obj.vel.x) < std::abs(amount)) {
                                obj.vel.x = 0;
                            }
                            else {
                                obj.vel.x += amount;
                            }
                        }
                    }
                    handleShooting();
                    obj.texture = res.texIdle;
                    obj.curAnimation = res.ANIM_PLAYER_IDLE;
                    break;
                }
                case PlayerState::running:
                {
                    if (!currentDirection) { // if not moving return to idle
                        obj.data.player.state = PlayerState::idle;
                    }
                    handleShooting();

                    if (obj.vel.x * obj.dir < 0 && obj.grounded) { // moving in different direction of vel, sliding
                        obj.texture = res.texSlide;
                        obj.curAnimation = res.ANIM_PLAYER_SLIDE;
                    } else {
                        obj.texture = res.texRun;
                        obj.curAnimation = res.ANIM_PLAYER_RUN;
                    }
                    break;
                }
                case PlayerState::jumping:
                {
                    handleShooting();
                    obj.texture = res.texJump;
                    obj.curAnimation = res.ANIM_PLAYER_JUMP;
                    break;
                }
            }
            if (obj.pos.y - gs.mapViewport.y > state.logH) {
                obj.data.player.state = PlayerState::dead; // die if you fall off
                obj.vel.x = 0;
            }
            //printf("Player x = %f, Player y = %f\n", obj.pos.x, obj.pos.y);
        } else { // player is dead, reset map
            Timer &deathTimer = obj.data.player.deathTimer;
            deathTimer.step(deltaTime);
            if (deathTimer.isTimeOut()) {
                running = false; // exit program
            }
        }
        
    } else if (obj.type == ObjectType::bullet) {
        switch (obj.data.bullet.state) {
            case BulletState::moving: {
                if (obj.pos.x - gs.mapViewport.x < 0 || // left side
                    obj.pos.x - gs.mapViewport.x > state.logW || // right side
                    obj.pos.y - gs.mapViewport.y < 0 || // up
                    obj.pos.y - gs.mapViewport.y > state.logH) // down
                { 
                    obj.data.bullet.state = BulletState::inactive;
                }
                break;
            }
            case BulletState::colliding: {
                if (obj.animations[obj.curAnimation].isDone()) {
                    obj.data.bullet.state = BulletState::inactive;
                }
            }
        }
    } else if (obj.type == ObjectType::enemy) {
        EnemyData &d = obj.data.enemy;
        switch (d.state) {
            /*case EnemyState::idle: {
                glm::vec2 playerDir = gs.player().pos - obj.pos;
                if (glm::length(playerDir) < 100) {
                    currentDirection = playerDir.x < 0 ? -1 : 1;
                } else {
                    obj.acc = glm::vec2(0);
                    obj.vel.x = 0;
                }
                break;
            }*/ // this is for proximity based movement, ignore
            case EnemyState::damaged:
            {
                if (d.damagedTimer.step(deltaTime)) {
                    // do nothing
                }
                break;
            }
            case EnemyState::dead: {
                obj.vel.x = 0;
                if (obj.curAnimation != -1 && obj.animations[obj.curAnimation].isDone()) {
                    obj.curAnimation = -1;
                    obj.spriteFrame = 1;
                }
                break;
            }
        }
    }
    if (currentDirection) {
        obj.dir = currentDirection;
    }
    obj.vel += currentDirection * obj.acc * deltaTime;
    if (std::abs(obj.vel.x) > obj.maxSpeedX) {
        obj.vel.x = currentDirection * obj.maxSpeedX;
    }
    // add vel to pos
    obj.pos += obj.vel * deltaTime;
    // collision
    bool foundGround = false;
    for (auto &layer : gs.layers) {
        for (GameObject &objB : layer) {
            if (&obj != &objB) {
                checkCollision(state, gs, res, obj, objB, deltaTime);
                if (objB.type == ObjectType::level) {
                    // grounded sensor
                    SDL_FRect sensor {
                        .x = obj.pos.x + obj.collider.x,
                        .y = obj.pos.y + obj.collider.y + obj.collider.h,
                        .w = obj.collider.w,
                        .h = 1
                    };
                    SDL_FRect rectB {
                        .x = objB.pos.x + objB.collider.x,
                        .y = objB.pos.y + objB.collider.y,
                        .w = objB.collider.w,
                        .h = objB.collider.h
                    };
                    SDL_FRect rectC { 0 };
                    if (SDL_GetRectIntersectionFloat(&sensor, &rectB, &rectC)) {
                        foundGround = true;
                    }
                }    
            }
        }
    }
    if (obj.grounded != foundGround) { // changing state
        obj.grounded = foundGround;
        if (foundGround && obj.type == ObjectType::player && obj.data.player.state != PlayerState::dead) {
            obj.data.player.state = PlayerState::running;
        }
    }
}

void collisionResponse(const SDLState &state, GameState &gs, const Resources &res, 
                       const SDL_FRect &rectA, const SDL_FRect &rectB, 
                       const SDL_FRect &rectC, GameObject &a, GameObject &b, float deltaTime) 
{
    const auto genericResponse = [&]() {
        if (rectC.w < rectC.h) { // horizontal col
            //printf("Horizontal Collision, %f = rectC.w, %f = rectC.h\n", rectC.w, rectC.h);
            if (a.vel.x > 0) { // going right
                a.pos.x -= rectC.w;
            }
            else if (a.vel.x < 0) { // going left
                a.pos.x += rectC.w;
            }
            if (a.type == ObjectType::enemy) {
                a.vel.x = -a.vel.x; // turn enemy around when it hits a wall
                a.dir = -a.dir;
            } else {
                a.vel.x = 0;
            }
            
        } 
        else { // vert col
            //printf("Vertical Collision, %f = rectC.w, %f = rectC.h\n", rectC.w, rectC.h);
            if (a.vel.y > 0) { // going down
                a.pos.y -= rectC.h;
            }
            else if (a.vel.y < 0)  { // going up
                a.pos.y += rectC.h;
            }
            a.vel.y = 0;
        }
    };
    // obj we are checking
    if (a.type == ObjectType::player) {
        if (a.data.player.state != PlayerState::dead) {
            // obj a is colliding with
            switch (b.type) {
                case ObjectType::level: {
                    genericResponse();
                    break;
                }
                case ObjectType::enemy: {
                    PlayerData &d = a.data.player;
                    d.healthPoints -= 1;
                    if (d.healthPoints <= 0) {
                        const float JUMP_DEAD = -350.0f;
                        d.state = PlayerState::dead;
                        a.texture = res.texDie;
                        a.curAnimation = res.ANIM_PLAYER_DIE;
                        a.vel.x = 0;
                        a.vel.y = JUMP_DEAD;
                    }
                    /*if (b.data.enemy.state != EnemyState::dead) {
                        a.vel = glm::vec2(100, 0) * -a.dir;
                    }
                    break;*/ // this would push the player away when touching an enemy. its buggy
                }
            }
        }
        
    } else if (a.type == ObjectType::bullet) {
        bool passthrough = false;
        switch (a.data.bullet.state) {
            case BulletState::moving:
            {
                switch (b.type) {
                    case ObjectType::level: {
                        break;
                    }
                    case ObjectType::enemy: {
                        EnemyData &d = b.data.enemy;
                        if (d.state != EnemyState::dead) {
                            if (b.dir == a.dir) {
                                b.dir = -a.dir; // turn enemy around
                                b.vel.x = -b.vel.x;
                            }
                            b.shouldFlash = true;
                            b.flashTimer.reset();
                            // could change enemy sprite here if needed
                            d.state = EnemyState::damaged;
                            // damage enemy and flag dead if needed
                            d.healthPoints -= 1;
                            if (d.healthPoints <= 0) {
                                const float JUMP_DEAD = -10.0f;
                                d.state = EnemyState::dead;
                                b.texture = res.texSpinyDead;
                                b.curAnimation = res.ANIM_ENEMY_DEAD;
                                b.pos.y += JUMP_DEAD; // make the enemy jump up a bit when they die then pass thru the floor
                            }
                            b.vel.x += 25.0f * b.dir;
                        } else {
                            passthrough = true;
                        }
                        break;
                    }
                }
                if (b.type != ObjectType::player && !passthrough) {
                    genericResponse();
                    a.vel *= 0;
                    a.data.bullet.state = BulletState::colliding;
                    a.texture = res.texBulletHit;
                    a.curAnimation = res.ANIM_BULLET_HIT;
                    a.collider.x = a.collider.y = 0;
                    a.collider.w = a.collider.h = static_cast<float>(res.texBulletHit->h); // exploding sprite has new size
                }
                break;
            }
        }
    } else if (a.type == ObjectType::enemy) {
        // obj a is colliding with
        switch (b.type) {
            case ObjectType::level: {
                if (a.data.enemy.state != EnemyState::dead) {
                    genericResponse();
                    break;
                }
            }
            case ObjectType::enemy: {
                if (a.data.enemy.state != EnemyState::dead && b.data.enemy.state != EnemyState::dead) {
                    genericResponse();
                    break;
                }
            }
        } // if we need to do something different for player uncomment
    }
}

void checkCollision(const SDLState &state, GameState &gs, const Resources &res, GameObject &a, GameObject &b, float deltaTime) {
    SDL_FRect rectA { // create rectangle c by intersecting a and b; if c exists, its height is y coordinates overlapping and width is x coordinates overlapping
        .x = a.pos.x + a.collider.x, 
        .y = a.pos.y + a.collider.y,
        .w = a.collider.w, 
        .h = a.collider.h
    };
    SDL_FRect rectB {
        .x = b.pos.x + b.collider.x, 
        .y = b.pos.y + b.collider.y,
        .w = b.collider.w, 
        .h = b.collider.h
    };
    SDL_FRect rectC{ 0 };
    if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC)) {
        // found intersection, respond accordingly
        collisionResponse(state, gs, res, rectA, rectB, rectC, a, b, deltaTime);
    }
}

void createTiles(const SDLState &state, GameState &gs, const Resources &res) { // 50 x 5
    /*
        1 - Stone
        2 - Brick
        3 - Enemy
        4 - Player
        5 - Grass
        6 - Fence
        7 - Bush
    */
    short map[MAP_ROWS][MAP_COLS] = {
        4,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,3,0,3,0,5,0,0,0,0,3,0,3,0,5,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        2,2,0,0,2,0,0,2,0,0,0,0,0,0,5,3,0,0,0,0,0,0,0,5,3,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,2,2,2,1,0,5,5,5,5,5,5,5,5,5,5,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    short foreground[MAP_ROWS][MAP_COLS] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0,0,6,6,0,6,6,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    short background[MAP_ROWS][MAP_COLS] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    const auto loadMap = [&state, &gs, &res](short layer[MAP_ROWS][MAP_COLS])
    {
        const auto createObject = [&state](int r, int c, SDL_Texture *tex, ObjectType type) {
            GameObject o;
            o.type = type; 
            o.pos = glm::vec2(c * TILE_SIZE, state.logH - (MAP_ROWS - r) * TILE_SIZE); // subtract r from map rows to not be backwards. drawn top to bottom and flush with resolution
            o.texture = tex;
            o.collider = {
                .x = 0,
                .y = 0,
                .w = TILE_SIZE,
                .h = TILE_SIZE
            };
            return o;
        };
        for (int r = 0; r < MAP_ROWS; r++) {
            for (int c = 0; c < MAP_COLS; c++) {
                switch (layer[r][c]) {
                    case 1: // stone
                    {
                        GameObject o = createObject(r, c, res.texStone, ObjectType::level);
                        gs.layers[LAYER_IDX_LEVEL].push_back(o);
                        break;
                    }
                    case 2: // brick
                    {
                        GameObject o = createObject(r, c, res.texBrick, ObjectType::level);
                        gs.layers[LAYER_IDX_LEVEL].push_back(o);
                        break;
                    }
                    case 3: // enemy
                    {
                        GameObject o = createObject(r, c, res.texSpiny, ObjectType::enemy);
                        o.data.enemy = EnemyData();
                        o.curAnimation = res.ANIM_ENEMY;
                        o.animations = res.enemyAnims;
                        o.collider = SDL_FRect {
                            .x = 2,
                            .y = 2,
                            .w = 28,
                            .h = 30
                        };
                        o.dynamic = true;
                        o.maxSpeedX = 100;
                        o.vel.x = 50.0f;
                        o.acc = glm::vec2(300, 0);
                        gs.layers[LAYER_IDX_CHARACTERS].push_back(o);
                        break;
                    }
                    case 4: // player
                    {
                        GameObject player = createObject(r, c, res.texIdle, ObjectType::player);
                        player.data.player = PlayerData(); // initialize player data to idle
                        player.animations = res.playerAnims; // load anims
                        player.curAnimation = res.ANIM_PLAYER_IDLE; // set player anim to idle
                        player.acc = glm::vec2(300, 0);
                        player.maxSpeedX = 150;
                        player.dynamic = true;
                        player.collider = { 
                            .x = 2,
                            .y = 1,
                            .w = 28,
                            .h = 31 
                        };
                        gs.layers[LAYER_IDX_CHARACTERS].push_back(player); // put into array
                        gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;
                        break;
                    }
                    case 5: // grass
                    {
                        GameObject o = createObject(r, c, res.texGrass, ObjectType::level);
                        gs.layers[LAYER_IDX_LEVEL].push_back(o);
                        break;
                    }
                    case 6: // bush
                    {
                        GameObject o = createObject(r, c, res.texBush, ObjectType::level);
                        gs.fgTiles.push_back(o);
                        break;
                    }
                    case 7: // fence
                    {
                        GameObject o = createObject(r, c, res.texFence, ObjectType::level);
                        gs.bgTiles.push_back(o);
                        break;
                    }
                }
            }
        }
    };
    loadMap(map);
    loadMap(background);
    loadMap(foreground);
    assert(gs.playerIndex != -1);
}

void handleKeyInput(const SDLState &state, GameState &gs, GameObject &obj,
                    SDL_Scancode key, bool keyDown) {
    const float JUMP_FORCE = -350.f;
    if (obj.type == ObjectType::player) {
        switch (obj.data.player.state) {
            case PlayerState::idle:
            {
                if (key == SDL_SCANCODE_K && keyDown) {
                    obj.data.player.state = PlayerState::jumping;
                    obj.vel.y += JUMP_FORCE;
                }
                break;
            }
            case PlayerState::running:
            {
                if (key == SDL_SCANCODE_K && keyDown) {
                    obj.data.player.state = PlayerState::jumping;
                    obj.vel.y += JUMP_FORCE;
                }
                break;
            }
        }
    }
}

void drawParallaxBackground(SDL_Renderer *renderer, SDL_Texture *texture, float xVelocity, float &scrollPos, float scrollFactor, float deltaTime) {
    scrollPos -= xVelocity * scrollFactor * deltaTime; // moving background to the left at rate dependent on playerX
    if (scrollPos <= -texture->w) {
        scrollPos = 0;
    }
    SDL_FRect dst {
        .x = scrollPos,
        .y = 200,
        .w = texture->w * 2.0f,
        .h = static_cast<float>(texture->h)
    };
    SDL_RenderTextureTiled(renderer, texture, nullptr, 1, &dst);
}