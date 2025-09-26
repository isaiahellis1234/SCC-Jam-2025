#include <raylib.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

enum GameState {
    STATE_MENU,
    STATE_OPTIONS,
    STATE_GAME
};

enum Difficulty {
    DIFF_CASUAL = 0,
    DIFF_NORMAL = 1,
    DIFF_HARD = 2
};

enum UnitType {
    UNIT_RIFLE,   
    UNIT_SHOTGUN,
    UNIT_SNIPER,
    UNIT_HEAVY,
    UNIT_ROCKET,
    UNIT_HEALER
};

enum EnemyType {
    ENEMY_GRUNT,
    ENEMY_FAST,
    ENEMY_TANK,
    ENEMY_SHOOTER,
    ENEMY_SIEGE
};

struct Unit {
    int x, y; 
    int width, height;   
    int speed;           
    Texture2D texture;   
    bool selected = false;
    bool moving = false;
    int targetX = 0, targetY = 0;
    float fx = 0.0f, fy = 0.0f;
    
    UnitType type;
    int hp = 100;
    int maxHp = 100;
    float fireRate = 1.0f;   
    float range = 100.0f;
    int damage = 10;
    float healRate = 0.0f;
    bool showHp = false;
};

struct EnemyNPC {
    int x, y;           
    int width, height; 
    int hp = 1;       
    int maxHp = 1;     
    bool showHp = false;
    bool alive = true;
    
    float fx = 0.0f, fy = 0.0f; 
    float moveSpeed = 120.0f;
    float detectionRange = 450.0f; 
    float attackRange = 100.0f;
    float shipDetectionRange = 12000.0f; 
    float attackDamage = 15.0f;
    float attackCooldown = 2.0f;
    float timeSinceLastAttack = 0.0f;
    int targetUnitIndex = -1;
    EnemyType type = ENEMY_GRUNT;
    bool prioritizeShip = false;     
    float avoidUnitsRange = 0.0f;   
};

struct Bullet {
    float x, y;
    float vx, vy;
    float speed;
    int damage = 10;
    int unitIndex = -1;
    bool active = true;
};

struct Particle {
    float x, y, vx, vy;
    float life, maxLife;
    Color color;
    bool active;
};

struct Rock {
    int x, y;
    int width, height;
    int hp = 200;
    int maxHp = 200;
    bool alive = true;
    bool showHp = false;
    int scrapMin = 6;
    int scrapMax = 14;
};

struct Ship {
    float x, y;
    int width, height;
    int hp = 100;
    int maxHp = 100;
    int hullIntegrity = 0;
    int maxHullIntegrity = 100;
    int shielding = 0;
    int maxShielding = 50;
    int engines = 0;
    int maxEngines = 25;
    int lifeSupportSystems = 0;
    int maxLifeSupportSystems = 25;
    bool isComplete = false;
};

struct UpgradeShop {
    int scrapMetal = 0;
    int hullUpgradeCost = 10;
    int shieldingUpgradeCost = 15;
    int engineUpgradeCost = 20;
    int lifeSupportUpgradeCost = 25;
};

template <typename T>
static inline T ClampVal(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

int main() {
    int SCREEN_WIDTH = 1280;
    int SCREEN_HEIGHT = 720;
    
    const float MAP_WIDTH = 3500.0f;
    const float MAP_HEIGHT = 3500.0f;
    const float MIN_ZOOM = 0.25f;
    const float MAX_ZOOM = 8.0f;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "SCC Game Jam 2025");
    SetWindowMinSize(800, 600);
    SetTargetFPS(60);

    GameState gameState = STATE_MENU;
    bool quitRequested = false;

    Camera2D camera{};
    camera.target = { MAP_WIDTH/2.0f, MAP_HEIGHT/2.0f };
    camera.offset = { (float)SCREEN_WIDTH/2.0f, (float)SCREEN_HEIGHT/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    int currentWave = 1;
    Difficulty difficulty = DIFF_NORMAL; // default
    int enemiesAlive = 0;

    Texture2D unitTex = LoadTexture("unit.png");
    SetTextureFilter(unitTex, TEXTURE_FILTER_POINT);
    Texture2D alienTex = LoadTexture("alien.png");
    SetTextureFilter(alienTex, TEXTURE_FILTER_POINT);

    const int count = 6;
    std::vector<Unit> units; units.reserve(count);
    const float TAU = 6.28318530718f; 
    const float ringRadius = 80.0f;  
    Vector2 center = camera.target;  
    for (int i = 0; i < count; ++i) {
        float t = (i / (float)count) * TAU;
        float cx = center.x + cosf(t) * ringRadius;
        float cy = center.y + sinf(t) * ringRadius;
        Unit u{};
        u.texture = unitTex;
        u.width = 90 / 2; u.height = 150 / 2;
        u.speed = 200;
        u.x = (int)lroundf(cx - u.width/2.0f);
        u.y = (int)lroundf(cy - u.height/2.0f);
        u.fx = (float)u.x; u.fy = (float)u.y;
        
        u.type = (UnitType)i;
        switch (u.type) {
            case UNIT_RIFLE:  
                u.hp = u.maxHp = 220;
                u.fireRate = 2.5f;
                u.range = 140.0f;
                u.damage = 18;
                break;
            case UNIT_SHOTGUN:
                u.hp = u.maxHp = 260;
                u.fireRate = 1.8f;
                u.range = 90.0f;
                u.damage = 30;
                u.speed = 260;
                break;
            case UNIT_SNIPER:  
                u.hp = u.maxHp = 180;
                u.fireRate = 1.0f;
                u.range = 240.0f;
                u.damage = 50;
                break;
            case UNIT_HEAVY:    
                u.hp = u.maxHp = 340;
                u.fireRate = 4.5f;
                u.range = 160.0f;
                u.damage = 10;
                u.speed = 160;
                break;
            case UNIT_ROCKET:  
                u.hp = u.maxHp = 200;
                u.fireRate = 0.7f;
                u.range = 190.0f;
                u.damage = 75; 
                break;
            case UNIT_HEALER:   
                u.hp = u.maxHp = 240;
                u.fireRate = 1.0f;
                u.range = 150.0f;
                u.damage = 5;
                u.healRate = 40.0f;
                break;
        }

        u.showHp = true;
        units.push_back(u);
    }

    std::vector<EnemyNPC> enemies; enemies.reserve(2000);
    auto spawnWave = [&](int wave){
        float enemyCountScale = 1.0f;
        float enemyStatScale = 1.0f;
        switch (difficulty) {
            case DIFF_CASUAL: enemyCountScale = 0.75f; enemyStatScale = 0.85f; break;
            case DIFF_NORMAL: enemyCountScale = 1.0f; enemyStatScale = 1.0f; break;
            case DIFF_HARD: enemyCountScale = 1.35f; enemyStatScale = 1.25f; break;
        }
        int baseCount = 8 + wave * 2;
        int spawnCount = (int)std::round(baseCount * enemyCountScale);
        if (spawnCount < 1) spawnCount = 1;
        for (int i = 0; i < spawnCount; ++i) {
            EnemyNPC e{};
            e.width = 32; e.height = 32;
            int margin = std::max(e.width, e.height) / 2 + 2;
            int side = GetRandomValue(0,3);
            if (side == 0) { e.x = GetRandomValue(margin, (int)MAP_WIDTH - margin); e.y = margin; }
            else if (side == 1) { e.x = GetRandomValue(margin, (int)MAP_WIDTH - margin); e.y = (int)MAP_HEIGHT - margin; }
            else if (side == 2) { e.x = margin; e.y = GetRandomValue(margin, (int)MAP_HEIGHT - margin); }
            else { e.x = (int)MAP_WIDTH - margin; e.y = GetRandomValue(margin, (int)MAP_HEIGHT - margin); }
            e.fx = (float)e.x; e.fy = (float)e.y;

            int roll = GetRandomValue(0, 99);
            if (roll < 50) { // grunt
                e.type = ENEMY_GRUNT; e.moveSpeed = 110.0f; e.attackRange = 65.0f; e.attackDamage = 10.0f; e.attackCooldown = 1.8f; e.hp = e.maxHp = (int)((70 + wave*4) * enemyStatScale);
            } else if (roll < 78) { // fast
                e.type = ENEMY_FAST; e.moveSpeed = 180.0f; e.attackRange = 45.0f; e.attackDamage = 7.0f; e.attackCooldown = 1.4f; e.hp = e.maxHp = (int)((50 + wave*3) * enemyStatScale);
            } else if (roll < 92) { // tank
                e.type = ENEMY_TANK; e.moveSpeed = 75.0f; e.attackRange = 80.0f; e.attackDamage = 18.0f; e.attackCooldown = 2.4f; e.hp = e.maxHp = (int)((160 + wave*10) * enemyStatScale);
            } else if (roll < 98) { // shooter
                e.type = ENEMY_SHOOTER; e.moveSpeed = 100.0f; e.attackRange = 260.0f; e.attackDamage = 8.0f; e.attackCooldown = 1.9f; e.hp = e.maxHp = (int)((60 + wave*5) * enemyStatScale);
            } else { // siege
                e.type = ENEMY_SIEGE; e.moveSpeed = 65.0f; e.attackRange = 380.0f; e.attackDamage = 10.0f; e.attackCooldown = 3.0f; e.hp = e.maxHp = (int)((55 + wave*5) * enemyStatScale);
                e.prioritizeShip = true; e.avoidUnitsRange = 200.0f;
            }
            e.detectionRange = 380.0f + wave * 10.0f;
            e.showHp = false; e.alive = true;
            enemies.push_back(e);
        }
    };

    spawnWave(currentWave);
    enemiesAlive = (int)enemies.size();

    std::vector<Bullet> bullets;
    std::vector<Particle> particles;
    std::vector<Rock> rocks;

    Ship playerShip{};
    playerShip.x = MAP_WIDTH/2.0f;
    playerShip.y = MAP_HEIGHT/2.0f;
    playerShip.width = 120;
    playerShip.height = 180;
    playerShip.maxHp = 200;
    playerShip.hp = playerShip.maxHp;
    
    UpgradeShop shop{};

    {
        int numRocks = 16;
        rocks.reserve(numRocks);
        for (int i = 0; i < numRocks; ++i) {
            Rock r{};
            r.width = 48; r.height = 48;
            int margin = 200;
            r.x = GetRandomValue(margin, (int)MAP_WIDTH - margin);
            r.y = GetRandomValue(margin, (int)MAP_HEIGHT - margin);
            float dx = r.x - playerShip.x; float dy = r.y - playerShip.y;
            if (dx*dx + dy*dy < 400.0f * 400.0f) {
                r.x += 400; r.y += 0;
            }
            r.hp = r.maxHp = GetRandomValue(160, 260);
            r.scrapMin = 10; r.scrapMax = 20;
            r.alive = true; r.showHp = false;
            rocks.push_back(r);
        }
    }

    std::vector<bool> unitAttacking; unitAttacking.resize(count, false);
    std::vector<int> unitTargetEnemy; unitTargetEnemy.resize(count, -1);
    std::vector<float> unitFireTimer; unitFireTimer.resize(count, 0.0f);

    std::vector<bool> unitAreaAttack; unitAreaAttack.resize(count, false);
    std::vector<Vector2> unitAreaCenter; unitAreaCenter.resize(count, Vector2{0,0});
    std::vector<float> unitAreaRadius; unitAreaRadius.resize(count, 0.0f);
    std::vector<Rectangle> unitAreaRect; unitAreaRect.resize(count, Rectangle{0,0,0,0});
    std::vector<std::vector<int>> unitAreaTargets; unitAreaTargets.resize(count);
    std::vector<float> unitHealFraction; unitHealFraction.resize(count, 0.0f);
    std::vector<int> unitAssignedRock; unitAssignedRock.resize(count, -1);
    bool rockAssignmentDirty = true;
    auto recomputeRockAssignments = [&]() {
        std::vector<int> aliveRocks; aliveRocks.reserve(rocks.size());
        for (int ri = 0; ri < (int)rocks.size(); ++ri) if (rocks[ri].alive) aliveRocks.push_back(ri);
        std::vector<char> used; used.assign(rocks.size(), 0);
        for (int ui = 0; ui < (int)units.size(); ++ui) {
            if (units[ui].type == UNIT_HEALER) { unitAssignedRock[ui] = -1; continue; }
            int bestR = -1; float bestD = 1e9f;
            float ucx = units[ui].fx + units[ui].width/2.0f;
            float ucy = units[ui].fy + units[ui].height/2.0f;
            for (int ri : aliveRocks) {
                if (used[ri]) continue;
                const Rock &r = rocks[ri];
                float dx = (float)r.x - ucx, dy = (float)r.y - ucy; float d = sqrtf(dx*dx + dy*dy);
                if (d < bestD) { bestD = d; bestR = ri; }
            }
            if (bestR != -1) { unitAssignedRock[ui] = bestR; used[bestR] = 1; }
            else unitAssignedRock[ui] = -1;
        }
        for (int ui = 0; ui < (int)units.size(); ++ui) {
            if (units[ui].type == UNIT_HEALER) continue;
            if (unitAssignedRock[ui] != -1) continue;
            int bestR = -1; float bestD = 1e9f;
            float ucx = units[ui].fx + units[ui].width/2.0f;
            float ucy = units[ui].fy + units[ui].height/2.0f;
            for (int ri : aliveRocks) {
                const Rock &r = rocks[ri];
                float dx = (float)r.x - ucx, dy = (float)r.y - ucy; float d = sqrtf(dx*dx + dy*dy);
                if (d < bestD) { bestD = d; bestR = ri; }
            }
            unitAssignedRock[ui] = bestR;
        }
        rockAssignmentDirty = false;
    };

    const float ATTACK_RANGE_HYST = 12.0f; 
    const float BULLET_SPEED = 500.0f;   

    bool isDragging = false, didDrag = false;
    Vector2 dragStart{0,0}, dragEnd{0,0};
    bool isRightDragging = false, rightDidDrag = false;
    Vector2 rightDragStart{0,0}, rightDragEnd{0,0};

    float timeScale = 1.0f;  
    bool isPaused = false;
    
    bool inIntermission = false;
    float intermissionTime = 0.0f;
    const float INTERMISSION_DURATION = 20.0f;

    auto startNewGame = [&]() {
        playerShip.x = MAP_WIDTH/2.0f;
        playerShip.y = MAP_HEIGHT/2.0f;
        playerShip.width = 120;
        playerShip.height = 180;
        playerShip.maxHp = 200;
        playerShip.hp = playerShip.maxHp;
        playerShip.hullIntegrity = 0;
        playerShip.shielding = 0;
        playerShip.engines = 0;
        playerShip.lifeSupportSystems = 0;
        playerShip.isComplete = false;

        shop.scrapMetal = 0;
        shop.hullUpgradeCost = 10;
        shop.shieldingUpgradeCost = 15;
        shop.engineUpgradeCost = 20;
        shop.lifeSupportUpgradeCost = 25;

        units.clear();
        Vector2 centerPos = { playerShip.x, playerShip.y };
        for (int i = 0; i < count; ++i) {
            float t = (i / (float)count) * TAU;
            float cx = centerPos.x + cosf(t) * ringRadius;
            float cy = centerPos.y + sinf(t) * ringRadius;
            Unit u{};
            u.texture = unitTex;
            u.width = 90 / 2; u.height = 150 / 2;
            u.speed = 200;
            u.x = (int)lroundf(cx - u.width/2.0f);
            u.y = (int)lroundf(cy - u.height/2.0f);
            u.fx = (float)u.x; u.fy = (float)u.y;
            u.type = (UnitType)i;
            switch (u.type) {
                case UNIT_RIFLE:  u.hp = u.maxHp = 200; u.fireRate = 2.0f; u.range = 120.0f; u.damage = 15; break;
                case UNIT_SHOTGUN: u.hp = u.maxHp = 240; u.fireRate = 1.5f; u.range = 80.0f;  u.damage = 25; u.speed = 250; break;
                case UNIT_SNIPER:  u.hp = u.maxHp = 160; u.fireRate = 0.8f; u.range = 200.0f; u.damage = 40; break;
                case UNIT_HEAVY:   u.hp = u.maxHp = 300; u.fireRate = 4.0f; u.range = 140.0f; u.damage = 8;  u.speed = 150; break;
                case UNIT_ROCKET:  u.hp = u.maxHp = 180; u.fireRate = 0.5f; u.range = 160.0f; u.damage = 60; break;
                case UNIT_HEALER:  u.hp = u.maxHp = 220; u.fireRate = 1.0f; u.range = 100.0f; u.damage = 5;  u.healRate = 20.0f; break;
            }
            u.selected = false; u.moving = false; u.showHp = true;
            units.push_back(u);
        }

        unitAttacking.assign(count, false);
        unitTargetEnemy.assign(count, -1);
        unitFireTimer.assign(count, 0.0f);
        unitAreaAttack.assign(count, false);
        unitAreaCenter.assign(count, Vector2{0,0});
        unitAreaRadius.assign(count, 0.0f);
        unitAreaRect.assign(count, Rectangle{0,0,0,0});
        for (auto &v : unitAreaTargets) v.clear();
    unitHealFraction.assign(count, 0.0f);

        enemies.clear();
        bullets.clear();
        particles.clear();
        rocks.clear();

        {
            int numRocks = 10;
            rocks.reserve(numRocks);
            for (int i = 0; i < numRocks; ++i) {
                Rock r{};
                r.width = 48; r.height = 48;
                int margin = 200;
                r.x = GetRandomValue(margin, (int)MAP_WIDTH - margin);
                r.y = GetRandomValue(margin, (int)MAP_HEIGHT - margin);
                float dx = r.x - playerShip.x; float dy = r.y - playerShip.y;
                if (dx*dx + dy*dy < 400.0f * 400.0f) { r.x += 400; }
                r.hp = r.maxHp = GetRandomValue(160, 260);
                r.scrapMin = 6; r.scrapMax = 14;
                r.alive = true; r.showHp = false;
                rocks.push_back(r);
            }
        }

        currentWave = 1;
        spawnWave(currentWave);
        enemiesAlive = (int)enemies.size();

        camera.target = { playerShip.x, playerShip.y };
        isPaused = false;
        timeScale = 1.0f;
        inIntermission = false;
        intermissionTime = 0.0f;
    };

    auto startNextWave = [&]() {
        inIntermission = false;
        intermissionTime = 0.0f;
        currentWave++;
        spawnWave(currentWave);
        enemiesAlive = (int)enemies.size();
    };

    while (!WindowShouldClose()) {
        camera.offset = { (float)GetScreenWidth()/2.0f, (float)GetScreenHeight()/2.0f };

        if (gameState != STATE_GAME) {
            Vector2 m = GetMousePosition();
            bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

            BeginDrawing();
            ClearBackground((Color){10, 10, 40, 255});

            const char* title = "Starfall Survivors";
            int tw = MeasureText(title, 48);
            DrawText(title, GetScreenWidth()/2 - tw/2, 80, 48, RAYWHITE);

            if (gameState == STATE_MENU) {
                int bw = 280, bh = 48, gap = 16;
                int bx = GetScreenWidth()/2 - bw/2;
                int by = 200;
                Rectangle btnStart{ (float)bx, (float)by, (float)bw, (float)bh };
                Rectangle btnOptions{ (float)bx, (float)(by + (bh+gap)), (float)bw, (float)bh };
                Rectangle btnQuit{ (float)bx, (float)(by + 2*(bh+gap)), (float)bw, (float)bh };
                auto drawBtn = [&](Rectangle r, const char* label){
                    bool hov = CheckCollisionPointRec(m, r);
                    DrawRectangleRec(r, hov ? Fade(BLUE, 0.4f) : Fade(BLACK, 0.5f));
                    DrawRectangleLinesEx(r, 2, hov ? SKYBLUE : GRAY);
                    int lw = MeasureText(label, 24);
                    DrawText(label, (int)(r.x + r.width/2 - lw/2), (int)(r.y + r.height/2 - 12), 24, RAYWHITE);
                };
                // Difficulty selection buttons (radio style)
                int diffBW = 140, diffBH = 34; int diffGap = 10;
                Rectangle btnCasual{ (float)(bx - diffBW - 20), (float)(by), (float)diffBW, (float)diffBH };
                Rectangle btnNormal{ (float)(bx - diffBW - 20), (float)(by + diffBH + diffGap), (float)diffBW, (float)diffBH };
                Rectangle btnHard{ (float)(bx - diffBW - 20), (float)(by + 2*(diffBH + diffGap)), (float)diffBW, (float)diffBH };
                auto drawDiff = [&](Rectangle r, const char* label, Difficulty d){
                    bool hov = CheckCollisionPointRec(m, r);
                    bool active = (difficulty == d);
                    DrawRectangleRec(r, active ? Fade(DARKBLUE, 0.7f) : (hov ? Fade(DARKBLUE, 0.4f) : Fade(BLACK, 0.4f)));
                    DrawRectangleLinesEx(r, 2, active ? SKYBLUE : (hov ? BLUE : GRAY));
                    int lw = MeasureText(label, 18);
                    DrawText(label, (int)(r.x + r.width/2 - lw/2), (int)(r.y + r.height/2 - 9), 18, RAYWHITE);
                };
                drawDiff(btnCasual, "Casual", DIFF_CASUAL);
                drawDiff(btnNormal, "Normal", DIFF_NORMAL);
                drawDiff(btnHard, "Hard", DIFF_HARD);

                drawBtn(btnStart, "Start Game");
                drawBtn(btnOptions, "Options");
                drawBtn(btnQuit, "Quit");

                DrawText("Tip: Right-drag to assign area targets", bx, by + 3*(bh+gap) + 30, 18, LIGHTGRAY);

                if (click) {
                    if (CheckCollisionPointRec(m, btnCasual)) {
                        difficulty = DIFF_CASUAL;
                    } else if (CheckCollisionPointRec(m, btnNormal)) {
                        difficulty = DIFF_NORMAL;
                    } else if (CheckCollisionPointRec(m, btnHard)) {
                        difficulty = DIFF_HARD;
                    } else if (CheckCollisionPointRec(m, btnStart)) {
                        startNewGame();
                        gameState = STATE_GAME;
                    } else if (CheckCollisionPointRec(m, btnOptions)) {
                        gameState = STATE_OPTIONS;
                    } else if (CheckCollisionPointRec(m, btnQuit)) {
                        quitRequested = true;
                    }
                }
            } else if (gameState == STATE_OPTIONS) {
                int bw = 320, bh = 44, gap = 14;
                int bx = GetScreenWidth()/2 - bw/2;
                int by = 200;
                Rectangle btn1080{ (float)bx, (float)by, (float)bw, (float)bh };
                Rectangle btn720{ (float)bx, (float)(by + (bh+gap)), (float)bw, (float)bh };
                Rectangle btnMax{ (float)bx, (float)(by + 2*(bh+gap)), (float)bw, (float)bh };
                Rectangle btnBack{ (float)bx, (float)(by + 3*(bh+gap)), (float)bw, (float)bh };
                auto drawBtn = [&](Rectangle r, const char* label){
                    bool hov = CheckCollisionPointRec(m, r);
                    DrawRectangleRec(r, hov ? Fade(DARKGREEN, 0.4f) : Fade(BLACK, 0.5f));
                    DrawRectangleLinesEx(r, 2, hov ? LIME : GRAY);
                    int lw = MeasureText(label, 22);
                    DrawText(label, (int)(r.x + r.width/2 - lw/2), (int)(r.y + r.height/2 - 11), 22, RAYWHITE);
                };
                drawBtn(btn1080, "Set 1920 x 1080");
                drawBtn(btn720,  "Set 1280 x 720");
                drawBtn(btnMax,  "Maximize Window");
                drawBtn(btnBack,  "Back");

                if (click) {
                    if (CheckCollisionPointRec(m, btn1080)) {
                        SCREEN_WIDTH = 1920; SCREEN_HEIGHT = 1080; SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
                    } else if (CheckCollisionPointRec(m, btn720)) {
                        SCREEN_WIDTH = 1280; SCREEN_HEIGHT = 720; SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
                    } else if (CheckCollisionPointRec(m, btnMax)) {
                        int monitor = GetCurrentMonitor();
                        int mw = GetMonitorWidth(monitor);
                        int mh = GetMonitorHeight(monitor);
                        SCREEN_WIDTH = mw; SCREEN_HEIGHT = mh;
                        SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
                    } else if (CheckCollisionPointRec(m, btnBack)) {
                        gameState = STATE_MENU;
                    }
                }

                DrawText("Options", bx, by - 50, 28, SKYBLUE);
            }

            EndDrawing();
            if (quitRequested) break;
            continue;
        }
    float camSpeed = 200.0f * GetFrameTime() / camera.zoom;
        if (IsKeyDown(KEY_W)) camera.target.y -= camSpeed;
        if (IsKeyDown(KEY_S)) camera.target.y += camSpeed;
        if (IsKeyDown(KEY_A) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) camera.target.x -= camSpeed;
        if (IsKeyDown(KEY_D)) camera.target.x += camSpeed;

        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            Vector2 mouse = GetMousePosition();
            Vector2 before = GetScreenToWorld2D(mouse, camera);
            float zoomFactor = 1.0f + (wheel * 0.125f);
            camera.zoom *= zoomFactor;
            if (camera.zoom < MIN_ZOOM) camera.zoom = MIN_ZOOM;
            if (camera.zoom > MAX_ZOOM) camera.zoom = MAX_ZOOM;
            Vector2 after = GetScreenToWorld2D(mouse, camera);
            camera.target.x += (before.x - after.x);
            camera.target.y += (before.y - after.y);
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector2 d = GetMouseDelta();
            camera.target.x -= d.x / camera.zoom;
            camera.target.y -= d.y / camera.zoom;
        }

        for (int i = 0; i < 9; ++i) {
            int key = KEY_ONE + i;
            if (IsKeyPressed((KeyboardKey)key)) {
                if (i < (int)units.size()) {
                    if (units[i].type != UNIT_HEALER) {
                        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                            units[i].selected = !units[i].selected;
                        } else {
                            for (auto &u : units) u.selected = false;
                            units[i].selected = true;
                        }
                    }
                }
            }
        }

        if (IsKeyPressed(KEY_SPACE)) {
            isPaused = !isPaused;
        }
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            timeScale += 0.25f;
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) { 
            timeScale -= 0.25f;
        }
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            float quantized = roundf(timeScale * 4.0f) / 4.0f;
            if (quantized < 0.25f) quantized = 0.25f;
            if (quantized > 3.0f) quantized = 3.0f;
            timeScale = quantized;
        }
        if (IsKeyPressed(KEY_R)) {  
            timeScale = 1.0f;
            isPaused = false;
        }

        if (IsKeyPressed(KEY_A) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
            for (auto &u : units) {
                if (u.type != UNIT_HEALER) u.selected = true; else u.selected = false;
            }
        }

        if (IsKeyPressed(KEY_H) && shop.scrapMetal >= shop.hullUpgradeCost && playerShip.hullIntegrity < playerShip.maxHullIntegrity) {
            shop.scrapMetal -= shop.hullUpgradeCost;
            playerShip.hullIntegrity += 8;
        }
        if (IsKeyPressed(KEY_U) && shop.scrapMetal >= shop.shieldingUpgradeCost && playerShip.shielding < playerShip.maxShielding) {
            shop.scrapMetal -= shop.shieldingUpgradeCost;
            playerShip.shielding += 5;
        }
        if (IsKeyPressed(KEY_E) && shop.scrapMetal >= shop.engineUpgradeCost && playerShip.engines < playerShip.maxEngines) {
            shop.scrapMetal -= shop.engineUpgradeCost;
            playerShip.engines += 3;
        }
        if (IsKeyPressed(KEY_L) && shop.scrapMetal >= shop.lifeSupportUpgradeCost && playerShip.lifeSupportSystems < playerShip.maxLifeSupportSystems) {
            shop.scrapMetal -= shop.lifeSupportUpgradeCost;
            playerShip.lifeSupportSystems += 3;
        }

        playerShip.isComplete = (playerShip.hullIntegrity >= playerShip.maxHullIntegrity &&
                                playerShip.shielding >= playerShip.maxShielding &&
                                playerShip.engines >= playerShip.maxEngines &&
                                playerShip.lifeSupportSystems >= playerShip.maxLifeSupportSystems);

    float halfViewW = ((float)GetScreenWidth() / camera.zoom) * 0.5f;
    float halfViewH = ((float)GetScreenHeight() / camera.zoom) * 0.5f;
        float minX = halfViewW, maxX = MAP_WIDTH - halfViewW;
        float minY = halfViewH, maxY = MAP_HEIGHT - halfViewH;
        camera.target.x = (MAP_WIDTH <= 2*halfViewW) ? MAP_WIDTH*0.5f : ClampVal(camera.target.x, minX, maxX);
        camera.target.y = (MAP_HEIGHT <= 2*halfViewH) ? MAP_HEIGHT*0.5f : ClampVal(camera.target.y, minY, maxY);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            dragStart = GetMousePosition(); dragEnd = dragStart; isDragging = true; didDrag = false;
        }
        if (isDragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            dragEnd = GetMousePosition();
            if (!didDrag && (fabsf(dragEnd.x - dragStart.x) > 4 || fabsf(dragEnd.y - dragStart.y) > 4)) didDrag = true;
        }
        if (isDragging && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            if (didDrag) {
                Vector2 ws = GetScreenToWorld2D(dragStart, camera), we = GetScreenToWorld2D(dragEnd, camera);
                float l = std::min(ws.x, we.x), r = std::max(ws.x, we.x);
                float t = std::min(ws.y, we.y), b = std::max(ws.y, we.y);
                Rectangle sel{l, t, r-l, b-t};
                if (!shiftHeld) for (auto &u : units) u.selected = false;
                for (auto &u : units) {
                    Rectangle rect{ (float)u.x, (float)u.y, (float)u.width, (float)u.height };
                    if (u.type != UNIT_HEALER && CheckCollisionRecs(sel, rect)) u.selected = true;
                }
            } else {
                Vector2 wMouse = GetScreenToWorld2D(GetMousePosition(), camera);
                int hit = -1;
                for (int i = (int)units.size() - 1; i >= 0; --i) {
                    if (units[i].type == UNIT_HEALER) continue; 
                    Rectangle rect{ (float)units[i].x, (float)units[i].y, (float)units[i].width, (float)units[i].height };
                    if (CheckCollisionPointRec(wMouse, rect)) { hit = i; break; }
                }
                if (hit != -1) {
                    if (shiftHeld) units[hit].selected = !units[hit].selected;
                    else for (int i = 0; i < (int)units.size(); ++i) units[i].selected = (i == hit);
                } else if (!shiftHeld) {
                    for (auto &u : units) u.selected = false;
                }
            }
            isDragging = false; didDrag = false;
        }
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            rightDragStart = GetMousePosition(); rightDragEnd = rightDragStart; isRightDragging = true; rightDidDrag = false;
        }
        if (isRightDragging && IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            rightDragEnd = GetMousePosition();
            if (!rightDidDrag && (fabsf(rightDragEnd.x - rightDragStart.x) > 4 || fabsf(rightDragEnd.y - rightDragStart.y) > 4)) rightDidDrag = true;
        }
        if (isRightDragging && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            if (rightDidDrag) {
                Vector2 ws = GetScreenToWorld2D(rightDragStart, camera), we = GetScreenToWorld2D(rightDragEnd, camera);
                float l = std::min(ws.x, we.x), r = std::max(ws.x, we.x);
                float t = std::min(ws.y, we.y), b = std::max(ws.y, we.y);
                Rectangle rect{l, t, r-l, b-t};
                std::vector<int> selIdx; Vector2 selCenter{0,0};
                for (int i=0;i<(int)units.size();++i) if (units[i].selected) { selIdx.push_back(i); selCenter.x += units[i].x + units[i].width/2.0f; selCenter.y += units[i].y + units[i].height/2.0f; }
                if (!selIdx.empty()) {
                    std::vector<int> captured;
                    for (int i = 0; i < (int)enemies.size(); ++i) {
                        if (!enemies[i].alive) continue;
                        Rectangle er{ (float)(enemies[i].x - enemies[i].width/2), (float)(enemies[i].y - enemies[i].height/2), (float)enemies[i].width, (float)enemies[i].height };
                        if (CheckCollisionRecs(rect, er)) captured.push_back(i);
                    }
                    selCenter.x /= (float)selIdx.size(); selCenter.y /= (float)selIdx.size();
                    for (int idx : selIdx) {
                        unitAreaAttack[idx] = true;
                        unitAreaCenter[idx] = { l + (r-l)*0.5f, t + (b-t)*0.5f };
                        unitAreaRadius[idx] = 0.0f;
                        unitAreaRect[idx] = rect;
                        unitAreaTargets[idx] = captured;
                        unitAttacking[idx] = false; unitTargetEnemy[idx] = -1;
                    }
                    if (!captured.empty()) {
                        std::vector<int> assigned; assigned.reserve(selIdx.size());
                        for (int idx : selIdx) {
                            if (units[idx].type == UNIT_HEALER) continue;
                            float ucx = units[idx].fx + units[idx].width/2.0f;
                            float ucy = units[idx].fy + units[idx].height/2.0f;
                            float bestD = 1e9f; int bestI = -1;
                            for (int tIdx : captured) {
                                if (std::find(assigned.begin(), assigned.end(), tIdx) != assigned.end()) continue;
                                if (tIdx < 0 || tIdx >= (int)enemies.size() || !enemies[tIdx].alive) continue;
                                float dx = (float)enemies[tIdx].x - ucx; float dy = (float)enemies[tIdx].y - ucy; float d = sqrtf(dx*dx + dy*dy);
                                if (d < bestD) { bestD = d; bestI = tIdx; }
                            }
                            if (bestI != -1) { unitAttacking[idx] = true; unitTargetEnemy[idx] = bestI; assigned.push_back(bestI); }
                        }
                        for (int idx : selIdx) {
                            if (units[idx].type == UNIT_HEALER) continue;
                            if (unitAttacking[idx]) continue;
                            float ucx = units[idx].fx + units[idx].width/2.0f;
                            float ucy = units[idx].fy + units[idx].height/2.0f;
                            float bestD = 1e9f; int bestI = -1;
                            for (int tIdx : captured) {
                                if (tIdx < 0 || tIdx >= (int)enemies.size() || !enemies[tIdx].alive) continue;
                                float dx = (float)enemies[tIdx].x - ucx; float dy = (float)enemies[tIdx].y - ucy; float d = sqrtf(dx*dx + dy*dy);
                                if (d < bestD) { bestD = d; bestI = tIdx; }
                            }
                            if (bestI != -1) { unitAttacking[idx] = true; unitTargetEnemy[idx] = bestI; }
                        }
                    }
                    Vector2 wCenter{ l + (r-l)*0.5f, t + (b-t)*0.5f };
                    for (int idx : selIdx) {
                        Unit &u = units[idx];
                        float offX = (u.x + u.width/2.0f) - selCenter.x;
                        float offY = (u.y + u.height/2.0f) - selCenter.y;
                        float tcx = wCenter.x + offX, tcy = wCenter.y + offY;
                        u.targetX = (int)lroundf(tcx - u.width/2.0f);
                        u.targetY = (int)lroundf(tcy - u.height/2.0f);
                        u.moving = true;
                    }
                }
            } else {
                Vector2 wMouse = GetScreenToWorld2D(GetMousePosition(), camera);
                int hitUnit = -1;
                for (int i = (int)units.size() - 1; i >= 0; --i) {
                    if (units[i].type == UNIT_HEALER) continue; 
                    Rectangle ur{ (float)units[i].x, (float)units[i].y, (float)units[i].width, (float)units[i].height };
                    if (CheckCollisionPointRec(wMouse, ur)) { hitUnit = i; break; }
                }
                if (hitUnit != -1) {
                    for (int i = 0; i < (int)units.size(); ++i) units[i].selected = (i == hitUnit);
                } else {
                    int clickedEnemy = -1;
                    for (int i = (int)enemies.size() - 1; i >= 0; --i) {
                        if (!enemies[i].alive) continue;
                        Rectangle er{ (float)(enemies[i].x - enemies[i].width/2), (float)(enemies[i].y - enemies[i].height/2), (float)enemies[i].width, (float)enemies[i].height };
                        if (CheckCollisionPointRec(wMouse, er)) { clickedEnemy = i; break; }
                    }
                    std::vector<int> selIdx; Vector2 selCenter{0,0};
                    for (int i=0;i<(int)units.size();++i) if (units[i].selected && units[i].type != UNIT_HEALER) { selIdx.push_back(i); selCenter.x += units[i].x + units[i].width/2.0f; selCenter.y += units[i].y + units[i].height/2.0f; }
                    if (!selIdx.empty()) {
                        if (clickedEnemy != -1) {
                            for (int idx : selIdx) { unitAreaAttack[idx] = false; unitAreaTargets[idx].clear(); unitAttacking[idx] = true; unitTargetEnemy[idx] = clickedEnemy; }
                        } else {
                            selCenter.x /= (float)selIdx.size(); selCenter.y /= (float)selIdx.size();
                            for (int idx : selIdx) { unitAreaAttack[idx] = false; unitAreaTargets[idx].clear(); unitAttacking[idx] = false; unitTargetEnemy[idx] = -1; }
                            for (int idx : selIdx) {
                                Unit &u = units[idx];
                                float offX = (u.x + u.width/2.0f) - selCenter.x;
                                float offY = (u.y + u.height/2.0f) - selCenter.y;
                                float tcx = wMouse.x + offX, tcy = wMouse.y + offY;
                                u.targetX = (int)lroundf(tcx - u.width/2.0f);
                                u.targetY = (int)lroundf(tcy - u.height/2.0f);
                                u.moving = true;
                            }
                        }
                    }
                }
            }
            isRightDragging = false; rightDidDrag = false;
        }

        float dt = isPaused ? 0.0f : GetFrameTime() * timeScale;
        if (playerShip.hp <= 0 || playerShip.isComplete) {
            dt = 0.0f;
        }

    if (!isPaused && playerShip.hp > 0 && !playerShip.isComplete) {
            int aliveCount = 0;
            for (const auto &e : enemies) if (e.alive) aliveCount++;
            enemiesAlive = aliveCount;
            if (aliveCount == 0 && !inIntermission) {
                enemies.clear();
                for (auto &u : units) {
                    int heal = (int)(u.maxHp * 0.4f);
                    u.hp = ClampVal(u.hp + heal, 0, u.maxHp);
                }
                playerShip.hp = ClampVal(playerShip.hp + (int)(playerShip.maxHp * 0.25f), 0, playerShip.maxHp);
                int rewardBase = 12 + currentWave * 2;
                float rewardScale = 1.0f;
                switch (difficulty) {
                    case DIFF_CASUAL: rewardScale = 1.15f; break; // a little more scrap to help
                    case DIFF_NORMAL: rewardScale = 1.0f; break;
                    case DIFF_HARD: rewardScale = 0.85f; break; // less scrap, harder economy
                }
                shop.scrapMetal += (int)std::round(rewardBase * rewardScale);
                for (int i = 0; i < 4; ++i) {
                    Rock r{}; r.width = 48; r.height = 48; int margin = 200;
                    r.x = GetRandomValue(margin, (int)MAP_WIDTH - margin);
                    r.y = GetRandomValue(margin, (int)MAP_HEIGHT - margin);
                    float dx = r.x - playerShip.x; float dy = r.y - playerShip.y;
                    if (dx*dx + dy*dy < 400.0f * 400.0f) { r.x += 400; }
                    r.hp = r.maxHp = GetRandomValue(160, 260);
                    r.scrapMin = 10; r.scrapMax = 20; r.alive = true; r.showHp = false;
                    rocks.push_back(r);
                }
                inIntermission = true;
                intermissionTime = INTERMISSION_DURATION;
                for (int ui = 0; ui < (int)units.size(); ++ui) {
                    unitAttacking[ui] = false;
                    unitTargetEnemy[ui] = -1;
                    unitAreaAttack[ui] = false;
                    unitAreaTargets[ui].clear();
                }
            }
        }

        if (inIntermission && playerShip.hp > 0 && !playerShip.isComplete) {
            if (IsKeyPressed(KEY_ENTER)) {
                startNextWave();
            } else if (!isPaused) {
                intermissionTime -= GetFrameTime() * timeScale;
                if (intermissionTime <= 0.0f) {
                    startNextWave();
                }
            }
        }
        
        for (int i = 0; i < (int)units.size(); ++i) {
            Unit &u = units[i];
            if (unitFireTimer[i] > 0.0f) { unitFireTimer[i] -= dt; if (unitFireTimer[i] < 0.0f) unitFireTimer[i] = 0.0f; }

            if (!unitAttacking[i] && u.type != UNIT_HEALER) {
                float ucx = u.fx + u.width/2.0f;
                float ucy = u.fy + u.height/2.0f;
                int nearestEnemy = -1;
                float nearestDist = 1e9f;

                if (unitAreaAttack[i]) {
                    auto &list = unitAreaTargets[i];
                    list.erase(std::remove_if(list.begin(), list.end(), [&](int idx){ return idx < 0 || idx >= (int)enemies.size() || !enemies[idx].alive; }), list.end());
                    if (!list.empty()) {
                        std::vector<char> used(enemies.size(), 0);
                        for (int j = 0; j < (int)units.size(); ++j) {
                            if (j == i) continue;
                            if (unitAreaAttack[j] && unitAttacking[j]) {
                                int tj = unitTargetEnemy[j];
                                if (tj >= 0 && tj < (int)enemies.size() && enemies[tj].alive) used[tj] = 1;
                            }
                        }
                        int bestI = -1; float bestD = 1e9f;
                        for (int t : list) {
                            if (t >= 0 && t < (int)enemies.size() && !used[t]) {
                                float dx = (float)enemies[t].x - ucx; float dy = (float)enemies[t].y - ucy; float d = sqrtf(dx*dx + dy*dy);
                                if (d < bestD) { bestD = d; bestI = t; }
                            }
                        }
                        if (bestI == -1) { 
                            bestD = 1e9f;
                            for (int t : list) {
                                float dx = (float)enemies[t].x - ucx; float dy = (float)enemies[t].y - ucy; float d = sqrtf(dx*dx + dy*dy);
                                if (d < bestD) { bestD = d; bestI = t; }
                            }
                        }
                        if (bestI != -1) { unitAttacking[i] = true; unitTargetEnemy[i] = bestI; }
                    } else {
                        unitAreaAttack[i] = false;
                    }
                }
                if (!unitAreaAttack[i] && !unitAttacking[i]) {
                    for (int ei = 0; ei < (int)enemies.size(); ++ei) {
                        const EnemyNPC &e = enemies[ei];
                        if (!e.alive) continue;
                        float dx = (float)e.x - ucx;
                        float dy = (float)e.y - ucy;
                        float d = sqrtf(dx*dx + dy*dy);
                        if (d < nearestDist) { nearestDist = d; nearestEnemy = ei; }
                    }
                    if (nearestEnemy >= 0 && nearestDist <= u.range) {
                        unitAttacking[i] = true;
                        unitTargetEnemy[i] = nearestEnemy;
                        u.moving = false;
                    } else {
                        if (rockAssignmentDirty) recomputeRockAssignments();
                        int assigned = (i < (int)unitAssignedRock.size()) ? unitAssignedRock[i] : -1;
                        if (assigned < 0 || assigned >= (int)rocks.size() || !rocks[assigned].alive) {
                            rockAssignmentDirty = true;
                            recomputeRockAssignments();
                            assigned = (i < (int)unitAssignedRock.size()) ? unitAssignedRock[i] : -1;
                        }
                        if (assigned != -1) {
                            float dxr = (float)rocks[assigned].x - ucx;
                            float dyr = (float)rocks[assigned].y - ucy;
                            float distR = sqrtf(dxr*dxr + dyr*dyr);
                            if (distR > u.range + ATTACK_RANGE_HYST) {
                                float inv = (distR > 0.0001f) ? (1.0f / distR) : 0.0f;
                                float dirx = dxr * inv;
                                float diry = dyr * inv;
                                float desiredCX = (float)rocks[assigned].x - dirx * u.range;
                                float desiredCY = (float)rocks[assigned].y - diry * u.range;
                                u.targetX = (int)lroundf(desiredCX - u.width/2.0f);
                                u.targetY = (int)lroundf(desiredCY - u.height/2.0f);
                                u.moving = true;
                            } else {
                                if (unitFireTimer[i] <= 0.0f) {
                                    float inv = (distR > 0.0001f) ? (1.0f / distR) : 0.0f;
                                    float dirx = dxr * inv;
                                    float diry = dyr * inv;
                                    Bullet b{}; b.x = ucx; b.y = ucy; b.speed = BULLET_SPEED; b.vx = dirx * b.speed; b.vy = diry * b.speed; b.active = true;
                                    b.damage = u.damage; b.unitIndex = i;
                                    bullets.push_back(b);
                                    unitFireTimer[i] = 1.0f / u.fireRate;
                                }
                                u.moving = false;
                            }
                        }
                    }
                }
            }
            if (unitAttacking[i]) {
                int ti = unitTargetEnemy[i];
                if (ti < 0 || ti >= (int)enemies.size() || !enemies[ti].alive) {
                    unitAttacking[i] = false; unitTargetEnemy[i] = -1; u.moving = false;
                    if (unitAreaAttack[i]) {
                        auto &list = unitAreaTargets[i];
                        list.erase(std::remove_if(list.begin(), list.end(), [&](int idx){ return idx < 0 || idx >= (int)enemies.size() || !enemies[idx].alive; }), list.end());
                        if (!list.empty()) {
                            float ucx2 = u.fx + u.width/2.0f; float ucy2 = u.fy + u.height/2.0f;
                            std::vector<char> used(enemies.size(), 0);
                            for (int j = 0; j < (int)units.size(); ++j) {
                                if (j == i) continue;
                                if (unitAreaAttack[j] && unitAttacking[j]) {
                                    int tj = unitTargetEnemy[j];
                                    if (tj >= 0 && tj < (int)enemies.size() && enemies[tj].alive) used[tj] = 1;
                                }
                            }
                            int bestI = -1; float bestD = 1e9f;
                            for (int t : list) {
                                if (!used[t]) {
                                    float dx2 = (float)enemies[t].x - ucx2; float dy2 = (float)enemies[t].y - ucy2; float d2 = sqrtf(dx2*dx2 + dy2*dy2);
                                    if (d2 < bestD) { bestD = d2; bestI = t; }
                                }
                            }
                            if (bestI == -1) {
                                bestD = 1e9f;
                                for (int t : list) {
                                    float dx2 = (float)enemies[t].x - ucx2; float dy2 = (float)enemies[t].y - ucy2; float d2 = sqrtf(dx2*dx2 + dy2*dy2);
                                    if (d2 < bestD) { bestD = d2; bestI = t; }
                                }
                            }
                            if (bestI != -1) { unitAttacking[i] = true; unitTargetEnemy[i] = bestI; }
                            else unitAreaAttack[i] = false;
                        } else unitAreaAttack[i] = false;
                    }
                } else {
                    float ucx = u.fx + u.width/2.0f;
                    float ucy = u.fy + u.height/2.0f;
                    float ecx = (float)enemies[ti].x;
                    float ecy = (float)enemies[ti].y;
                    float dx = ecx - ucx, dy = ecy - ucy;
                    float distToEnemy = sqrtf(dx*dx + dy*dy);
                    if (distToEnemy > u.range + ATTACK_RANGE_HYST) {
                        float inv = (distToEnemy > 0.0001f) ? (1.0f / distToEnemy) : 0.0f;
                        float dirx = dx * inv, diry = dy * inv;
                        float desiredCX = ecx - dirx * u.range;
                        float desiredCY = ecy - diry * u.range;
                        u.targetX = (int)lroundf(desiredCX - u.width/2.0f);
                        u.targetY = (int)lroundf(desiredCY - u.height/2.0f);
                        u.moving = true;
                    } else {
                        u.moving = false;
                        if (unitFireTimer[i] <= 0.0f) {
                            float inv = (distToEnemy > 0.0001f) ? (1.0f / distToEnemy) : 0.0f;
                            float dirx = dx * inv, diry = dy * inv;
                            Bullet b{}; b.x = ucx; b.y = ucy; b.speed = BULLET_SPEED; b.vx = dirx * b.speed; b.vy = diry * b.speed; b.active = true;
                            b.damage = u.damage; b.unitIndex = i;
                            bullets.push_back(b);
                            unitFireTimer[i] = 1.0f / u.fireRate;
                        }
                    }
                }
            }

            if (u.type == UNIT_HEALER) {
                int bestIdx = -1; float bestDist = 1e9f;
                float ucx = u.fx + u.width/2.0f; float ucy = u.fy + u.height/2.0f;
                for (int j = 0; j < (int)units.size(); ++j) {
                    if (j == i) continue;
                    const Unit &ally = units[j];
                    if (ally.type == UNIT_HEALER) continue;
                    if (ally.hp >= ally.maxHp) continue;
                    float acx = ally.fx + ally.width/2.0f; float acy = ally.fy + ally.height/2.0f;
                    float dx = acx - ucx, dy = acy - ucy; float d = sqrtf(dx*dx + dy*dy);
                    if (d < bestDist) { bestDist = d; bestIdx = j; }
                }
                const float MEDIC_SEARCH = 1000.0f;
                if (bestIdx != -1 && bestDist <= MEDIC_SEARCH) {
                    float acx = units[bestIdx].fx + units[bestIdx].width/2.0f; float acy = units[bestIdx].fy + units[bestIdx].height/2.0f;
                    float dx = acx - ucx, dy = acy - ucy; float len = sqrtf(dx*dx + dy*dy);
                    float stopDist = u.range * 0.85f;
                    if (len > stopDist) {
                        float inv = (len > 0.0001f) ? (1.0f/len) : 0.0f;
                        float tx = acx - dx*inv*stopDist;
                        float ty = acy - dy*inv*stopDist;
                        u.targetX = (int)lroundf(tx - u.width/2.0f);
                        u.targetY = (int)lroundf(ty - u.height/2.0f);
                        u.moving = true;
                    } else {
                        u.moving = false;
                    }
                }
                unitAttacking[i] = false; unitTargetEnemy[i] = -1; 
            }

            if (u.moving) {
                float dx = (float)u.targetX - u.fx, dy = (float)u.targetY - u.fy;
                float dist = sqrtf(dx*dx + dy*dy);
                float step = u.speed * dt;
                if (dist <= step || dist < 0.5f) { u.fx = (float)u.targetX; u.fy = (float)u.targetY; u.moving = false; }
                else if (dist > 0.0f) { u.fx += (dx/dist)*step; u.fy += (dy/dist)*step; }
            }
            u.x = (int)lroundf(u.fx); u.y = (int)lroundf(u.fy);
        }

        for (int i = 0; i < (int)units.size(); ++i) {
            Unit &healer = units[i];
            if (healer.type != UNIT_HEALER || healer.healRate <= 0.0f) continue;
            float healerCX = healer.fx + healer.width * 0.5f;
            float healerCY = healer.fy + healer.height * 0.5f;
            float maxRange = healer.range;
            for (int j = 0; j < (int)units.size(); ++j) {
                if (i == j) continue;
                Unit &ally = units[j];
                if (ally.hp >= ally.maxHp) continue;
                float allyCX = ally.fx + ally.width * 0.5f;
                float allyCY = ally.fy + ally.height * 0.5f;
                float dx = allyCX - healerCX;
                float dy = allyCY - healerCY;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist > maxRange) continue;
                float factor = (maxRange - dist) / maxRange;
                if (factor < 0.0f) factor = 0.0f;
                float frameHeal = healer.healRate * factor * dt; 
                if (frameHeal <= 0.0f) continue;
                unitHealFraction[j] += frameHeal;
                int whole = (int)unitHealFraction[j];
                if (whole > 0) {
                    ally.hp += whole;
                    unitHealFraction[j] -= (float)whole;
                    if (ally.hp > ally.maxHp) {
                        ally.hp = ally.maxHp;
                        unitHealFraction[j] = 0.0f; 
                    }
                    if (ally.hp < ally.maxHp) ally.showHp = true;
                }
            }
        }

        for (int i = 0; i < (int)enemies.size(); ++i) {
            EnemyNPC &enemy = enemies[i];
            if (!enemy.alive) continue;

            enemy.timeSinceLastAttack += dt;

            float closestDist = 1e9f;
            int closestUnit = -1;
            float enemyCX = enemy.fx;
            float enemyCY = enemy.fy;

            for (int j = 0; j < (int)units.size(); ++j) {
                const Unit &unit = units[j];
                float unitCX = unit.fx + unit.width/2.0f;
                float unitCY = unit.fy + unit.height/2.0f;
                float dx = unitCX - enemyCX;
                float dy = unitCY - enemyCY;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist < closestDist) { closestDist = dist; closestUnit = j; }
            }

            float distToShip = 1e9f;
            float shipCX = playerShip.x; float shipCY = playerShip.y;
            {
                float dxs = shipCX - enemyCX; float dys = shipCY - enemyCY;
                distToShip = sqrtf(dxs*dxs + dys*dys);
            }

            float shipPriorityRadius = enemy.attackRange + 10.0f; 
            bool shipClose = (distToShip <= shipPriorityRadius);
            bool engagingUnit = !shipClose && !enemy.prioritizeShip && (closestUnit >= 0 && closestDist <= enemy.detectionRange);
            bool engagingShip = shipClose || enemy.prioritizeShip || (!engagingUnit && distToShip <= enemy.shipDetectionRange);

            if (engagingUnit || engagingShip) {
                float tx = engagingUnit ? (units[closestUnit].fx + units[closestUnit].width/2.0f) : shipCX;
                float ty = engagingUnit ? (units[closestUnit].fy + units[closestUnit].height/2.0f) : shipCY;
                float distToTarget = engagingUnit ? closestDist : distToShip;
                bool shouldApproach = (distToTarget > enemy.attackRange);
                if (enemy.avoidUnitsRange > 0.0f && closestUnit >= 0 && closestDist < enemy.avoidUnitsRange) {
                    float ux = units[closestUnit].fx + units[closestUnit].width/2.0f;
                    float uy = units[closestUnit].fy + units[closestUnit].height/2.0f;
                    float dx = enemyCX - ux; float dy = enemyCY - uy; float len = sqrtf(dx*dx + dy*dy);
                    if (len > 0.001f) {
                        float step = enemy.moveSpeed * dt;
                        float nx = dx / len, ny = dy / len;
                        enemy.fx += nx * step;
                        enemy.fy += ny * step;
                        enemy.x = (int)lroundf(enemy.fx);
                        enemy.y = (int)lroundf(enemy.fy);
                    }
                } else if (shouldApproach) {
                    float dx = tx - enemyCX;
                    float dy = ty - enemyCY;
                    float len = sqrtf(dx*dx + dy*dy);
                    if (len > 0.001f) {
                        float step = enemy.moveSpeed * dt;
                        float nx = dx / len, ny = dy / len;
                        enemy.fx += nx * step;
                        enemy.fy += ny * step;
                        enemy.x = (int)lroundf(enemy.fx);
                        enemy.y = (int)lroundf(enemy.fy);
                    }
                }

                if (distToTarget <= enemy.attackRange && enemy.timeSinceLastAttack >= enemy.attackCooldown) {
                    if (engagingUnit) {
                        units[closestUnit].hp -= (int)enemy.attackDamage;
                        if (units[closestUnit].hp < 0) units[closestUnit].hp = 0;
                        if (units[closestUnit].hp < units[closestUnit].maxHp) units[closestUnit].showHp = true;
                    } else if (engagingShip) {
                        playerShip.hp -= (int)enemy.attackDamage;
                        if (playerShip.hp < 0) playerShip.hp = 0;
                    }
                    enemy.timeSinceLastAttack = 0.0f;
                }
            }
        }

        {
            Vector2 avg{0,0}; int cnt = 0;
            for (const auto &u : units) if (u.type != UNIT_HEALER) { avg.x += u.fx + u.width/2.0f; avg.y += u.fy + u.height/2.0f; cnt++; }
            if (cnt > 0) { avg.x /= cnt; avg.y /= cnt; }
            int inRadius = 0; const float clusterRadius = 140.0f;
            for (auto &u : units) if (u.type != UNIT_HEALER) {
                float cx = u.fx + u.width/2.0f, cy = u.fy + u.height/2.0f;
                float dx = cx - avg.x, dy = cy - avg.y; float d2 = dx*dx + dy*dy;
                if (d2 <= clusterRadius*clusterRadius) inRadius++;
            }
            static float blobTimer = 0.0f; blobTimer += dt;
            (void)inRadius; (void)clusterRadius; (void)avg; (void)cnt; (void)blobTimer;
        }
        
        for (int i = 0; i < (int)units.size(); ++i) {
            Unit &medic = units[i];
            if (medic.type != UNIT_HEALER || medic.healRate <= 0.0f) continue;
            
            Rectangle medicRect = {medic.fx, medic.fy, (float)medic.width, (float)medic.height};
            
            for (int j = 0; j < (int)units.size(); ++j) {
                if (i == j) continue;
                Unit &patient = units[j];
                if (patient.hp >= patient.maxHp) continue;
                
                Rectangle patientRect = {patient.fx, patient.fy, (float)patient.width, (float)patient.height};
                
                if (CheckCollisionRecs(medicRect, patientRect)) {
                    patient.hp += (int)(medic.healRate * dt * 2.0f);
                    if (patient.hp > patient.maxHp) patient.hp = patient.maxHp;
                }
            }
        }

        for (auto &b : bullets) {
            if (!b.active) continue;
            b.x += b.vx * dt;
            b.y += b.vy * dt;
            if (b.x < -50 || b.y < -50 || b.x > MAP_WIDTH + 50 || b.y > MAP_HEIGHT + 50) b.active = false;
            if (b.active) {
                for (auto &e : enemies) {
                    if (!e.alive) continue;
                    Rectangle er{ (float)(e.x - e.width/2), (float)(e.y - e.height/2), (float)e.width, (float)e.height };
                    if (CheckCollisionPointRec(Vector2{ b.x, b.y }, er)) {
                        e.showHp = true;
                        e.hp -= b.damage; 
                        if (e.hp <= 0) {
                            e.alive = false;
                            
                            shop.scrapMetal += GetRandomValue(2, 5);
                            
                            for (int p = 0; p < 12; ++p) {
                                Particle particle;
                                particle.x = e.x + e.width/2.0f;
                                particle.y = e.y + e.height/2.0f;
                                
                                float angle = (float)p / 12.0f * 2.0f * PI + ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
                                float speed = 80.0f + (float)rand() / RAND_MAX * 120.0f;
                                particle.vx = cosf(angle) * speed;
                                particle.vy = sinf(angle) * speed;
                                
                                particle.maxLife = 0.8f + (float)rand() / RAND_MAX * 0.4f;
                                particle.life = particle.maxLife;
                                
                                int colorVariant = rand() % 3;
                                if (colorVariant == 0) particle.color = (Color){180, 20, 20, 255};
                                else if (colorVariant == 1) particle.color = (Color){220, 40, 40, 255};
                                else particle.color = (Color){160, 10, 10, 255};                        
                                
                                particle.active = true;
                                particles.push_back(particle);
                            }
                        }
                        b.active = false; 
                        break; 
                    }
                }
                if (!b.active) continue;
                for (auto &r : rocks) {
                    if (!r.alive) continue;
                    Rectangle rr{ (float)(r.x - r.width/2), (float)(r.y - r.height/2), (float)r.width, (float)r.height };
                    if (CheckCollisionPointRec(Vector2{ b.x, b.y }, rr)) {
                        r.showHp = true;
                        r.hp -= b.damage;
                        if (r.hp <= 0) {
                            r.alive = false;
                            int gain = GetRandomValue(r.scrapMin, r.scrapMax);
                            shop.scrapMetal += gain;
                            rockAssignmentDirty = true;
                            for (int p = 0; p < 10; ++p) {
                                Particle particle;
                                particle.x = r.x;
                                particle.y = r.y;
                                float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
                                float speed = 60.0f + (float)rand() / RAND_MAX * 100.0f;
                                particle.vx = cosf(angle) * speed;
                                particle.vy = sinf(angle) * speed;
                                particle.maxLife = 0.6f + (float)rand() / RAND_MAX * 0.5f;
                                particle.life = particle.maxLife;
                                particle.color = (Color){140, 120, 80, 255}; 
                                particle.active = true;
                                particles.push_back(particle);
                            }
                        }
                        b.active = false;
                        break;
                    }
                }
            }
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b){ return !b.active; }), bullets.end());

        for (auto &p : particles) {
            if (p.active) {
                p.x += p.vx * dt;
                p.y += p.vy * dt;
                p.vy += 300.0f * dt; 
                p.vx *= 0.98f; 
                p.life -= dt;
                if (p.life <= 0.0f) {
                    p.active = false;
                }
            }
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p){ return !p.active; }), particles.end());

        BeginDrawing();
        ClearBackground((Color){10, 10, 40, 255});
        
        BeginMode2D(camera);
        
        DrawRectangleGradientV(-2000, -2000, (int)MAP_WIDTH + 4000, (int)MAP_HEIGHT + 4000, (Color){10, 10, 40, 255}, (Color){40, 20, 80, 255});
        
        for (int i = 0; i < 300; i++) {
            int x = (i * 137 + 200) % (int)(MAP_WIDTH + 1000) - 500;
            int y = (i * 181 + 300) % (int)(MAP_HEIGHT + 1000) - 500;
            Color starColor = (Color){255, 255, 255, (unsigned char)(80 + (i * 13) % 120)};
            DrawCircle(x, y, ((i % 3) + 1) * 0.7f, starColor);
        }
        
                for (int i = 0; i < 4; ++i) {
            int x = (i * 89 + 100) % (int)(MAP_WIDTH + 800) - 400;
            int y = (i * 73 + 150) % (int)(MAP_HEIGHT + 800) - 400;
            Color cloudColor;
            if (i % 3 == 0) cloudColor = (Color){80, 30, 120, 25};
            else if (i % 3 == 1) cloudColor = (Color){30, 80, 120, 20};
            else cloudColor = (Color){120, 30, 80, 18};
            DrawCircle(x, y, 80 + (i % 60), cloudColor);
        }
        
                rockAssignmentDirty = true;
        DrawRectangleLines(0, 0, (int)MAP_WIDTH, (int)MAP_HEIGHT, DARKGRAY);
        
        DrawRectangle((int)playerShip.x - playerShip.width/2, (int)playerShip.y - playerShip.height/2, playerShip.width, playerShip.height, DARKBLUE);
        DrawRectangleLines((int)playerShip.x - playerShip.width/2, (int)playerShip.y - playerShip.height/2, playerShip.width, playerShip.height, BLUE);
        {
            float pct = (float)playerShip.hp / (float)playerShip.maxHp;
            int barW = playerShip.width;
            int barH = 6;
            int bx = (int)playerShip.x - barW/2;
            int by = (int)playerShip.y + playerShip.height/2 + 6;
            DrawRectangle(bx, by, barW, barH, BLACK);
            Color col = (pct < 0.3f) ? RED : (pct < 0.6f ? YELLOW : GREEN);
            DrawRectangle(bx, by, (int)lroundf(barW * ClampVal(pct, 0.0f, 1.0f)), barH, col);
            DrawRectangleLines(bx, by, barW, barH, WHITE);
        }
        DrawText("SHIP", (int)playerShip.x - 20, (int)playerShip.y - 10, 16, SKYBLUE);
        
    for (const auto &u : units) {
            Rectangle src{0,0,(float)u.texture.width,(float)u.texture.height};
            Rectangle dst{(float)u.x,(float)u.y,(float)u.width,(float)u.height};
            DrawTexturePro(u.texture, src, dst, Vector2{0,0}, 0.0f, WHITE);
            
            if (u.showHp) {
                float pct = (u.maxHp > 0) ? (float)u.hp / (float)u.maxHp : 0.0f;
                const int barW = u.width;
                const int barH = 4;
                int barX = u.x;
                int barY = u.y - barH - 2;
                DrawRectangle(barX, barY, barW, barH, BLACK);
                
                Color healthColor = GREEN;
                if (pct < 0.3f) healthColor = RED;
                else if (pct < 0.6f) healthColor = YELLOW;
                
                DrawRectangle(barX, barY, (int)(barW * pct), barH, healthColor);
                DrawRectangleLines(barX, barY, barW, barH, WHITE);
            }
            
            const char* roleText = "";
            Color roleColor = WHITE;
            switch (u.type) {
                case UNIT_RIFLE: roleText = "RIFLE"; roleColor = LIGHTGRAY; break;
                case UNIT_SHOTGUN: roleText = "SHOTGUN"; roleColor = ORANGE; break;
                case UNIT_SNIPER: roleText = "SNIPER"; roleColor = BLUE; break;
                case UNIT_HEAVY: roleText = "HEAVY"; roleColor = RED; break;
                case UNIT_ROCKET: roleText = "ROCKET"; roleColor = YELLOW; break;
                case UNIT_HEALER: roleText = "MEDIC"; roleColor = GREEN; break;
            }
            int textWidth = MeasureText(roleText, 8);
            DrawText(roleText, u.x + (u.width - textWidth) / 2, u.y + u.height + 2, 8, roleColor);
        }
        for (const auto &b : bullets) if (b.active) {
            Color bulletColor = YELLOW; 
            if (b.unitIndex >= 0 && b.unitIndex < 6) {
                switch (b.unitIndex) {
                    case 0: bulletColor = WHITE; break;  
                    case 1: bulletColor = YELLOW; break;    
                    case 2: bulletColor = BLUE; break;     
                    case 3: bulletColor = RED; break;      
                    case 4: bulletColor = ORANGE; break;   
                    case 5: bulletColor = GREEN; break;    
                }
            }
            DrawCircle((int)b.x, (int)b.y, 5.5f, bulletColor);
        }
        
        for (const auto &p : particles) {
            if (p.active) {
                float alpha = p.life / p.maxLife;
                Color fadeColor = p.color;
                fadeColor.a = (unsigned char)(alpha * 255);
                float size = 2.0f + (1.0f - alpha) * 1.0f; 
                DrawCircle((int)p.x, (int)p.y, size, fadeColor);
            }
        }
        
        for (const auto &r : rocks) {
            if (!r.alive) continue;
            float pct = (r.maxHp > 0) ? (float)r.hp / (float)r.maxHp : 0.0f;
            Color baseCol = (Color){90, 80, 60, 255};
            float darkFactor = ClampVal(pct, 0.0f, 1.0f); 
            Color dynCol;
            dynCol.r = (unsigned char)(baseCol.r * (0.25f + 0.75f * darkFactor));
            dynCol.g = (unsigned char)(baseCol.g * (0.25f + 0.75f * darkFactor));
            dynCol.b = (unsigned char)(baseCol.b * (0.25f + 0.75f * darkFactor));
            dynCol.a = 255;
            DrawRectangle(r.x - r.width/2, r.y - r.height/2, r.width, r.height, dynCol);
            Color outlineCol = (Color){(unsigned char)ClampVal<int>(140 * darkFactor + 20*(1-darkFactor),0,255), (unsigned char)ClampVal<int>(120 * darkFactor + 20*(1-darkFactor),0,255), (unsigned char)ClampVal<int>(90 * darkFactor + 15*(1-darkFactor),0,255), 255};
            DrawRectangleLines(r.x - r.width/2, r.y - r.height/2, r.width, r.height, outlineCol);
            if (r.showHp) {
                const int barW = r.width;
                const int barH = 3;
                int bx = r.x - barW/2;
                int by = r.y - r.height/2 - 5;
                DrawRectangle(bx, by, barW, barH, DARKGRAY);
                DrawRectangle(bx, by, (int)lroundf(barW * ClampVal(pct, 0.0f, 1.0f)), barH, BROWN);
                DrawRectangleLines(bx, by, barW, barH, BLACK);
            }
        }

        for (const auto &e : enemies) {
            if (!e.alive) continue;
            Rectangle src{ 0, 0, (float)alienTex.width, (float)alienTex.height };
            Rectangle dst{ (float)(e.x - e.width/2), (float)(e.y - e.height/2), (float)e.width, (float)e.height };
            DrawTexturePro(alienTex, src, dst, Vector2{0,0}, 0.0f, WHITE);
            if (e.type == ENEMY_SIEGE) {
                DrawRectangleLines((int)dst.x, (int)dst.y, (int)dst.width, (int)dst.height, ORANGE);
            }
            if (e.showHp) {
                float pct = (e.maxHp > 0) ? (float)e.hp / (float)e.maxHp : 0.0f;
                const int barW = e.width;
                const int barH = 4;
                int bx = e.x - barW/2;
                int by = e.y - e.height/2 - 6;
                DrawRectangle(bx, by, barW, barH, DARKGRAY);
                DrawRectangle(bx, by, (int)lroundf(barW * pct), barH, LIME);
                DrawRectangleLines(bx, by, barW, barH, BLACK);
            }
        }
        EndMode2D();
    const char* diffLabel = (difficulty == DIFF_CASUAL) ? "CASUAL" : (difficulty == DIFF_NORMAL ? "NORMAL" : "HARD");
    DrawText(TextFormat("Wave: %d (%s)", currentWave, diffLabel), 12, 10, 20, RAYWHITE);
    DrawText(TextFormat("Enemies remaining: %d", enemiesAlive), 12, 34, 18, LIGHTGRAY);
    DrawText(TextFormat("Scrap: %d", shop.scrapMetal), 12, 56, 18, YELLOW);
    if (inIntermission && playerShip.hp > 0 && !playerShip.isComplete) {
        DrawText(TextFormat("Intermission: %.1fs", intermissionTime), 12, 78, 18, SKYBLUE);
    }
        BeginMode2D(camera);
        {
            bool anySelected = false;
            for (const auto &u : units) { if (u.selected) { anySelected = true; break; } }
            if (anySelected) {
                Vector2 wMouse = GetScreenToWorld2D(GetMousePosition(), camera);
                int hoverIdx = -1;
                int hoverRock = -1;
                for (int i = (int)enemies.size()-1; i >= 0; --i) {
                    const auto &e = enemies[i];
                    if (!e.alive) continue;
                    Rectangle er{ (float)(e.x - e.width/2), (float)(e.y - e.height/2), (float)e.width, (float)e.height };
                    if (CheckCollisionPointRec(wMouse, er)) { hoverIdx = i; break; }
                }
                if (hoverIdx == -1) {
                    for (int i = (int)rocks.size()-1; i >= 0; --i) {
                        const auto &r = rocks[i];
                        if (!r.alive) continue;
                        Rectangle rr{ (float)(r.x - r.width/2), (float)(r.y - r.height/2), (float)r.width, (float)r.height };
                        if (CheckCollisionPointRec(wMouse, rr)) { hoverRock = i; break; }
                    }
                }
                for (int i = 0; i < (int)enemies.size(); ++i) {
                    const auto &e = enemies[i];
                    if (!e.alive) continue;
                    float r = (float)(std::max(e.width, e.height)/2 + 6);
                    Color c = (i == hoverIdx) ? ORANGE : SKYBLUE;
                    DrawCircleLines(e.x, e.y, r, c);
                }
                for (int i = 0; i < (int)rocks.size(); ++i) {
                    const auto &r = rocks[i];
                    if (!r.alive) continue;
                    float rr = (float)(std::max(r.width, r.height)/2 + 6);
                    Color c = (i == hoverRock) ? GOLD : BROWN;
                    DrawCircleLines(r.x, r.y, rr, c);
                }
            }
        }
        for (const auto &u : units) if (u.selected) {
            int cx = u.x + u.width/2, cy = u.y + u.height/2; int r = (std::max(u.width,u.height)/2)+6;
            DrawCircleLines(cx, cy, (float)r, SKYBLUE);
        }
        {
            Color ring = Fade(RED, 0.55f);
            for (int i = 0; i < (int)units.size(); ++i) {
                if (units[i].selected && unitAreaAttack[i]) {
                    if (unitAreaRadius[i] > 0.0f) {
                        DrawCircleLines((int)unitAreaCenter[i].x, (int)unitAreaCenter[i].y, unitAreaRadius[i], ring);
                        DrawCircle((int)unitAreaCenter[i].x, (int)unitAreaCenter[i].y, 2.5f, ring);
                    } else if (unitAreaRect[i].width > 0 && unitAreaRect[i].height > 0) {
                        DrawRectangleLinesEx(unitAreaRect[i], 1.5f, ring);
                    }
                }
            }
        }
        EndMode2D();

        if (inIntermission && playerShip.hp > 0 && !playerShip.isComplete) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.35f));
            int panelW = 520, panelH = 180;
            int px = GetScreenWidth()/2 - panelW/2;
            int py = GetScreenHeight()/2 - panelH/2;
            DrawRectangle(px, py, panelW, panelH, Fade(DARKBLUE, 0.8f));
            DrawRectangleLines(px, py, panelW, panelH, SKYBLUE);
            const char* title = "Intermission";
            int tw = MeasureText(title, 28);
            DrawText(title, GetScreenWidth()/2 - tw/2, py + 12, 28, RAYWHITE);
            const char* sub = "Mine rocks and buy upgrades before the next wave.";
            int sw = MeasureText(sub, 16);
            DrawText(sub, GetScreenWidth()/2 - sw/2, py + 48, 16, LIGHTGRAY);
            const char* timer = TextFormat("Next wave in: %.1fs", intermissionTime);
            int timw = MeasureText(timer, 22);
            DrawText(timer, GetScreenWidth()/2 - timw/2, py + 72, 22, SKYBLUE);
            int bw = 260, bh = 44;
            Rectangle btn{ (float)(GetScreenWidth()/2 - bw/2), (float)(py + panelH - bh - 16), (float)bw, (float)bh };
            Vector2 m = GetMousePosition();
            bool hov = CheckCollisionPointRec(m, btn);
            DrawRectangleRec(btn, hov ? Fade(BLUE, 0.7f) : Fade(BLUE, 0.5f));
            DrawRectangleLinesEx(btn, 2, hov ? SKYBLUE : DARKBLUE);
            const char* label = "Start Next Wave (Enter)";
            int lw = MeasureText(label, 20);
            DrawText(label, (int)(btn.x + btn.width/2 - lw/2), (int)(btn.y + btn.height/2 - 10), 20, RAYWHITE);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hov) {
                startNextWave();
            }
        }

        if (isDragging && didDrag) {
            float l = std::min(dragStart.x, dragEnd.x), r = std::max(dragStart.x, dragEnd.x);
            float t = std::min(dragStart.y, dragEnd.y), b = std::max(dragStart.y, dragEnd.y);
            Rectangle sel{l,t,r-l,b-t};
            DrawRectangleLinesEx(sel, 1.0f, SKYBLUE);
        }
        if (isRightDragging && rightDidDrag) {
            float l = std::min(rightDragStart.x, rightDragEnd.x), r = std::max(rightDragStart.x, rightDragEnd.x);
            float t = std::min(rightDragStart.y, rightDragEnd.y), b = std::max(rightDragStart.y, rightDragEnd.y);
            Rectangle sel{l,t,r-l,b-t};
            DrawRectangleLinesEx(sel, 1.0f, RED);
        }

        {
            const int slot = 40;
            const int pad = 8;
            int baseX = pad;
            int baseY = SCREEN_HEIGHT - pad - slot;
            for (int i = 0; i < (int)units.size(); ++i) {
                int x = baseX + i * (slot + pad);
                int y = baseY;
                Rectangle src{0,0,(float)units[i].texture.width,(float)units[i].texture.height};
                Rectangle dst{(float)x,(float)y,(float)slot,(float)slot};
                DrawRectangleLines(x-1, y-1, slot+2, slot+2, units[i].selected?YELLOW:LIGHTGRAY);
                DrawTexturePro(units[i].texture, src, dst, Vector2{0,0}, 0.0f, WHITE);
                const char* label = TextFormat("%d", i+1);
                DrawRectangle(x, y, 14, 14, Fade(BLACK, 0.5f));
                DrawText(label, x+3, y+1, 12, RAYWHITE);
                
                Color unitColor = WHITE; 
                switch (i) {
                    case 0: unitColor = WHITE; break;   
                    case 1: unitColor = YELLOW; break;   
                    case 2: unitColor = BLUE; break;     
                    case 3: unitColor = RED; break;       
                    case 4: unitColor = ORANGE; break;    
                    case 5: unitColor = GREEN; break;     
                }
                
                int circleX = x + slot - 8; 
                int circleY = y + slot - 8;
                DrawCircle(circleX, circleY, 6, Fade(BLACK, 0.7f)); 
                DrawCircle(circleX, circleY, 5, unitColor);
            }
        }
        
        {
            int uiX = SCREEN_WIDTH - 200;
            int uiY = 10;
            
            DrawRectangle(uiX - 5, uiY - 5, 195, 80, Fade(BLACK, 0.7f));
            DrawRectangleLines(uiX - 5, uiY - 5, 195, 80, DARKGRAY);
            
            const char* statusText;
            Color statusColor;
            if (isPaused) {
                statusText = "PAUSED";
                statusColor = RED;
            } else if (timeScale > 1.0f) {
                statusText = TextFormat("FAST x%.1f", timeScale);
                statusColor = GREEN;
            } else if (timeScale < 1.0f) {
                statusText = TextFormat("SLOW x%.2f", timeScale);
                statusColor = ORANGE;
            } else {
                statusText = "NORMAL";
                statusColor = WHITE;
            }
            
            DrawText("TIME CONTROL", uiX, uiY, 12, LIGHTGRAY);
            DrawText(statusText, uiX, uiY + 15, 16, statusColor);
            DrawText("SPACE: Pause", uiX, uiY + 35, 10, LIGHTGRAY);
            DrawText("+/-: Speed  R: Reset", uiX, uiY + 47, 10, LIGHTGRAY);
        }
        
        {
            const int panelW = 250;
            const int panelH = 160;
            const int screenPad = 10;
            const int hotbarPad = 8;
            const int hotbarSlot = 40;
            const int bottomMargin = hotbarPad + hotbarSlot + screenPad;
            int shipUIX = GetScreenWidth() - panelW - screenPad;
            int shipUIY = GetScreenHeight() - panelH - bottomMargin;
            
            DrawRectangle(shipUIX - 5, shipUIY - 5, panelW, panelH, Fade(BLACK, 0.8f));
            DrawRectangleLines(shipUIX - 5, shipUIY - 5, panelW, panelH, BLUE);
            
            DrawText("SHIP RECONSTRUCTION", shipUIX, shipUIY, 14, SKYBLUE);
            DrawText(TextFormat("Scrap Metal: %d", shop.scrapMetal), shipUIX, shipUIY + 20, 12, YELLOW);
            
            Color hullColor = (playerShip.hullIntegrity >= playerShip.maxHullIntegrity) ? GREEN : WHITE;
            DrawText(TextFormat("Hull: %d/%d [H - %d scrap]", playerShip.hullIntegrity, playerShip.maxHullIntegrity, shop.hullUpgradeCost), 
                     shipUIX, shipUIY + 40, 10, hullColor);
            
            Color shieldColor = (playerShip.shielding >= playerShip.maxShielding) ? GREEN : WHITE;
            DrawText(TextFormat("Shielding: %d/%d [U - %d scrap]", playerShip.shielding, playerShip.maxShielding, shop.shieldingUpgradeCost), 
                     shipUIX, shipUIY + 55, 10, shieldColor);
            
            Color engineColor = (playerShip.engines >= playerShip.maxEngines) ? GREEN : WHITE;
            DrawText(TextFormat("Engines: %d/%d [E - %d scrap]", playerShip.engines, playerShip.maxEngines, shop.engineUpgradeCost), 
                     shipUIX, shipUIY + 70, 10, engineColor);
            
            Color lifeColor = (playerShip.lifeSupportSystems >= playerShip.maxLifeSupportSystems) ? GREEN : WHITE;
            DrawText(TextFormat("Life Support: %d/%d [L - %d scrap]", playerShip.lifeSupportSystems, playerShip.maxLifeSupportSystems, shop.lifeSupportUpgradeCost), 
                     shipUIX, shipUIY + 85, 10, lifeColor);
            
            if (playerShip.isComplete) {
                DrawText("SHIP COMPLETE! READY FOR TAKEOFF!", shipUIX, shipUIY + 110, 12, GREEN);
                DrawText("You can now escape this planet!", shipUIX, shipUIY + 125, 10, LIME);
            } else {
                int totalProgress = playerShip.hullIntegrity + playerShip.shielding + playerShip.engines + playerShip.lifeSupportSystems;
                int maxProgress = playerShip.maxHullIntegrity + playerShip.maxShielding + playerShip.maxEngines + playerShip.maxLifeSupportSystems;
                float progressPercent = (float)totalProgress / (float)maxProgress * 100.0f;
                DrawText(TextFormat("Completion: %.1f%%", progressPercent), shipUIX, shipUIY + 110, 12, ORANGE);
                DrawText("Kill enemies or mine rocks for scrap!", shipUIX, shipUIY + 125, 10, LIGHTGRAY);
            }
        }
    DrawFPS(10, GetScreenHeight() - 20);
        
        if (playerShip.hp <= 0) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));
            const char* over = "GAME OVER";
            int ow = MeasureText(over, 44);
            DrawText(over, GetScreenWidth()/2 - ow/2, GetScreenHeight()/2 - 120, 44, RED);
            const char* sub = "Ship destroyed";
            int sw = MeasureText(sub, 22);
            DrawText(sub, GetScreenWidth()/2 - sw/2, GetScreenHeight()/2 - 80, 22, LIGHTGRAY);

            int bw = 260, bh = 48;
            Rectangle btnBack{ (float)(GetScreenWidth()/2 - bw/2), (float)(GetScreenHeight()/2 - bh/2), (float)bw, (float)bh };
            Vector2 m = GetMousePosition();
            bool hov = CheckCollisionPointRec(m, btnBack);
            DrawRectangleRec(btnBack, hov ? Fade(DARKBLUE, 0.6f) : Fade(DARKBLUE, 0.4f));
            DrawRectangleLinesEx(btnBack, 2, hov ? SKYBLUE : BLUE);
            const char* label = "Back to Main Menu";
            int lw = MeasureText(label, 22);
            DrawText(label, (int)(btnBack.x + btnBack.width/2 - lw/2), (int)(btnBack.y + btnBack.height/2 - 11), 22, RAYWHITE);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hov) {
                gameState = STATE_MENU;
            }
        }

        if (playerShip.isComplete) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));
            const char* over = "YOU WIN!";
            int ow = MeasureText(over, 44);
            DrawText(over, GetScreenWidth()/2 - ow/2, GetScreenHeight()/2 - 120, 44, GREEN);
            const char* sub = "Ship ready for takeoff";
            int sw = MeasureText(sub, 22);
            DrawText(sub, GetScreenWidth()/2 - sw/2, GetScreenHeight()/2 - 80, 22, LIGHTGRAY);

            int bw = 300, bh = 48;
            Rectangle btnBack{ (float)(GetScreenWidth()/2 - bw/2), (float)(GetScreenHeight()/2 - bh/2), (float)bw, (float)bh };
            Vector2 m = GetMousePosition();
            bool hov = CheckCollisionPointRec(m, btnBack);
            DrawRectangleRec(btnBack, hov ? Fade(DARKGREEN, 0.6f) : Fade(DARKGREEN, 0.4f));
            DrawRectangleLinesEx(btnBack, 2, hov ? LIME : GREEN);
            const char* label = "Back to Main Menu";
            int lw = MeasureText(label, 22);
            DrawText(label, (int)(btnBack.x + btnBack.width/2 - lw/2), (int)(btnBack.y + btnBack.height/2 - 11), 22, RAYWHITE);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hov) {
                gameState = STATE_MENU;
            }
        }

        EndDrawing();
    }

    UnloadTexture(unitTex);
    UnloadTexture(alienTex);
    CloseWindow();
    return 0;
}