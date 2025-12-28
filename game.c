#include "raylib.h"
#include <math.h>

typedef struct Particle {
  Vector2 pos;
  Vector2 vel;
  float age;
  float life;
} Particle;

typedef struct Star {
  Vector2 pos;
  Vector2 vel;
  float life;
  float maxLife;
} Star;

typedef struct Obstacle {
  Rectangle rect;
  float speed;
  float vx;
  bool active;
} Obstacle;

typedef struct LaserItem {
  Rectangle rect;
  float speed;
  bool active;
} LaserItem;

typedef struct StarItem {
  Rectangle rect;
  float speed;
  bool active;
} StarItem;

typedef struct Bullet {
  Vector2 pos;
  Vector2 vel;
  bool active;
} Bullet;

typedef struct CircleObstacle {
  Vector2 pos;
  float radius;
  float speed;
  float shootTimer;
  bool active;
  bool isFiring;
  float fireDelay;
  int currentAngle;
} CircleObstacle;

typedef struct Turret {
  float x;
  float vx;
  float shootTimer;
  bool active;
} Turret;

typedef struct TurretLaser {
  float x;
  float width;
  float progress;
  float warningTimer;
  bool active;
  bool firing;
} TurretLaser;

static void AddParticles(Particle *particles, int maxParticles, int count,
                         Vector2 pos) {
  for (int i = 0; i < count; i++) {
    int idx = -1;
    for (int j = 0; j < maxParticles; j++) {
      if (particles[j].age < 0.0f) {
        idx = j;
        break;
      }
    }
    if (idx < 0)
      break;
    float ang = (float)GetRandomValue(0, 359) * DEG2RAD;
    float spd = (float)GetRandomValue(80, 220);
    particles[idx].pos = pos;
    particles[idx].vel = (Vector2){cosf(ang) * spd, sinf(ang) * spd};
    particles[idx].age = 0.0f;
    particles[idx].life = 0.35f + (float)GetRandomValue(0, 20) / 100.0f;
  }
}

int main(void) {
  const int screenWidth = 1200;
  const int screenHeight = 900;

  InitWindow(screenWidth, screenHeight, "Ray Puzzle");
  InitAudioDevice();
  SetTargetFPS(60);

  const char *title = "ray puzzle";
  const int titleFontSize = 80;

  const int buttonWidth = 200;
  const int buttonHeight = 60;
  Rectangle startButton = {(float)(screenWidth - buttonWidth) / 2,
                           (float)screenHeight - buttonHeight - 60,
                           (float)buttonWidth, (float)buttonHeight};
  const int playerSize = 18;
  float playerX = screenWidth * 0.5f;
  float playerY = screenHeight - 80.0f;

  const int maxObstacles = 40;
  Obstacle obstacles[40];
  float obstacleSpawnTimer = 0.0f;

  const int maxItems = 8;
  LaserItem items[8];
  float itemSpawnTimer = 0.0f;
  int laserAmmo = 0;
  const int maxLaserAmmo = 3;

  const int maxStarItems = 3;
  StarItem starItems[3];
  float starSpawnTimer = 0.0f;
  bool invincible = false;
  float invincibilityTimer = 0.0f;
  const float invincibilityDuration = 5.0f;

  const int maxCircleObstacles = 10;
  CircleObstacle circleObstacles[10];
  float circleSpawnTimer = 0.0f;
  const int maxBullets = 200;
  Bullet bullets[200];

  const int maxTurrets = 2;
  Turret turrets[2];
  const int maxTurretLasers = 2;
  TurretLaser turretLasers[2];
  const float turretLaserWidth = 40.0f;
  const float turretWarningTime = 1.0f;
  const float turretFireTime = 0.5f;

  bool laserActive = false;
  float laserTimer = 0.0f;
  float laserProgress = 0.0f;
  const float laserDuration = 0.35f;
  const float laserSpeed = 1400.0f;
  const float laserWidth = 6.0f;
  const int maxParticles = 64;
  Particle particles[64];
  const int maxStars = 24;
  Star stars[24];

  Sound clickSound = LoadSound("決定ボタンを押す2.mp3");
  Sound wallHitSound = LoadSound("カーソル移動12.mp3");
  const int maxLaserSounds = 5;
  Sound laserSounds[5];
  for (int i = 0; i < maxLaserSounds; i++) {
    laserSounds[i] = LoadSound("気弾2.mp3");
  }
  Sound bulletFireSound = LoadSound("決定ボタンを押す34.mp3");
  Sound deathSound = LoadSound("チーン1.mp3");
  Sound itemGetSound = LoadSound("se_itemget_004.wav");
  Music bgm = LoadMusicStream("maou_bgm_8bit27.mp3");
  Texture2D laserItemTexture = LoadTexture("laser.gif");
  Texture2D starItemTexture = LoadTexture("star.gif");
  const float hueSpeed = 100.0f;         // degrees per second for hue shift
  const float transitionDuration = 0.6f; // seconds
  bool inGame = false;
  bool transitioning = false;
  bool fadeOut = true;
  float transitionAlpha = 0.0f;
  bool dead = false;
  float runTime = 0.0f;
  int dodgedCount = 0;

  for (int i = 0; i < maxParticles; i++) {
    particles[i].age = -1.0f;
  }
  for (int i = 0; i < maxStars; i++) {
    stars[i].life = -1.0f;
  }
  for (int i = 0; i < maxObstacles; i++) {
    obstacles[i].active = false;
  }
  for (int i = 0; i < maxItems; i++) {
    items[i].active = false;
  }
  for (int i = 0; i < maxStarItems; i++) {
    starItems[i].active = false;
  }
  for (int i = 0; i < maxCircleObstacles; i++) {
    circleObstacles[i].active = false;
  }
  for (int i = 0; i < maxBullets; i++) {
    bullets[i].active = false;
  }
  for (int i = 0; i < maxTurrets; i++) {
    // Distribute turrets evenly across the screen
    turrets[i].x = (float)(i + 1) * screenWidth / (maxTurrets + 1);
    // Alternate direction for each turret
    turrets[i].vx = (i % 2 == 0) ? 150.0f : -150.0f;
    // Stagger shoot timers so they don't all fire at once
    turrets[i].shootTimer = 5.0f + (float)i * 1.0f;
    turrets[i].active = true;
  }
  for (int i = 0; i < maxTurretLasers; i++) {
    turretLasers[i].active = false;
  }


  while (!WindowShouldClose()) {
    Vector2 mouse = GetMousePosition();
    bool hovered =
        !inGame && !transitioning && CheckCollisionPointRec(mouse, startButton);
    bool pressed = hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    if (!inGame && !transitioning && hovered &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      PlaySound(clickSound);
      transitioning = true;
      fadeOut = true;
      transitionAlpha = 0.0f;
      dead = false;
      runTime = 0.0f;
      dodgedCount = 0;
      laserAmmo = 0;
      laserActive = false;
      laserTimer = 0.0f;
      laserProgress = 0.0f;
      obstacleSpawnTimer = 0.0f;
      itemSpawnTimer = 0.0f;
      circleSpawnTimer = 0.0f;
      starSpawnTimer = 0.0f;
      invincible = false;
      invincibilityTimer = 0.0f;
      playerX = screenWidth * 0.5f;
      playerY = screenHeight - 80.0f;
      PlayMusicStream(bgm);
      for (int i = 0; i < maxObstacles; i++) {
        obstacles[i].active = false;
      }
      for (int i = 0; i < maxItems; i++) {
        items[i].active = false;
      }
      for (int i = 0; i < maxStarItems; i++) {
        starItems[i].active = false;
      }
      for (int i = 0; i < maxCircleObstacles; i++) {
        circleObstacles[i].active = false;
      }
      for (int i = 0; i < maxBullets; i++) {
        bullets[i].active = false;
      }
    }

    float dt = GetFrameTime();
    if (transitioning) {
      float delta = dt / transitionDuration;
      if (fadeOut) {
        transitionAlpha += delta;
        if (transitionAlpha >= 1.0f) {
          transitionAlpha = 1.0f;
          fadeOut = false;
          inGame = true;
        }
      } else {
        transitionAlpha -= delta;
        if (transitionAlpha <= 0.0f) {
          transitionAlpha = 0.0f;
          transitioning = false;
        }
      }
    }

    float t = (float)GetTime();
    float hueTop = fmodf(t * hueSpeed, 360.0f);
    float hueBottom = fmodf(t * hueSpeed + 60.0f, 360.0f);
    Color topLeft = ColorFromHSV(hueTop, 0.45f, 0.35f);
    Color topRight = ColorFromHSV(hueTop + 10.0f, 0.5f, 0.4f);
    Color bottomLeft = ColorFromHSV(hueBottom, 0.5f, 0.55f);
    Color bottomRight = ColorFromHSV(hueBottom + 15.0f, 0.55f, 0.6f);

    BeginDrawing();
    ClearBackground((Color){18, 18, 28, 255});

    UpdateMusicStream(bgm);

    if (!inGame) {
      starSpawnTimer -= dt;
      if (starSpawnTimer <= 0.0f) {
        for (int i = 0; i < maxStars; i++) {
          if (stars[i].life < 0.0f) {
            float startX = (float)GetRandomValue(0, screenWidth);
            float startY = (float)GetRandomValue(0, screenHeight / 2);
            float speed = (float)GetRandomValue(300, 520);
            float angle = (float)GetRandomValue(225, 255) * DEG2RAD;
            stars[i].pos = (Vector2){startX, startY};
            stars[i].vel = (Vector2){cosf(angle) * speed, sinf(angle) * speed};
            stars[i].life = 0.0f;
            stars[i].maxLife = 1.0f + (float)GetRandomValue(0, 60) / 100.0f;
            break;
          }
        }
        starSpawnTimer = 0.35f + (float)GetRandomValue(0, 40) / 100.0f;
      }

      DrawRectangleGradientEx(
          (Rectangle){0, 0, (float)screenWidth, (float)screenHeight}, topLeft,
          topRight, bottomRight, bottomLeft);

      for (int i = 0; i < maxStars; i++) {
        if (stars[i].life < 0.0f)
          continue;
        stars[i].life += dt;
        if (stars[i].life >= stars[i].maxLife) {
          stars[i].life = -1.0f;
          continue;
        }
        Vector2 prev = stars[i].pos;
        stars[i].pos.x += stars[i].vel.x * dt;
        stars[i].pos.y += stars[i].vel.y * dt;
        float t = stars[i].life / stars[i].maxLife;
        unsigned char alpha = (unsigned char)(200 * (1.0f - t));
        DrawLineEx(prev, stars[i].pos, 2.0f, (Color){255, 240, 200, alpha});
      }

      int titleWidth = MeasureText(title, titleFontSize);
      DrawText(title, (screenWidth - titleWidth) / 2, 50, titleFontSize, WHITE);

      Color buttonColor =
          hovered ? (Color){70, 160, 255, 255} : (Color){50, 130, 220, 255};
      if (pressed)
        buttonColor = (Color){30, 100, 200, 255};

      DrawRectangleRounded(startButton, 0.2f, 8, buttonColor);
      DrawRectangleRoundedLines(startButton, 0.2f, 8, 2,
                                (Color){10, 20, 30, 255});

      const char *startText = "START";
      int startTextSize = 28;
      int startTextWidth = MeasureText(startText, startTextSize);
      DrawText(startText,
               (int)(startButton.x + (buttonWidth - startTextWidth) / 2),
               (int)(startButton.y + (buttonHeight - startTextSize) / 2),
               startTextSize, WHITE);
    } else {
      ClearBackground(WHITE);

      if (!dead) {
        runTime += dt;
        playerX = mouse.x;
        playerY = mouse.y;
        if (playerX < playerSize / 2.0f)
          playerX = playerSize / 2.0f;
        if (playerX > screenWidth - playerSize / 2.0f)
          playerX = screenWidth - playerSize / 2.0f;
        if (playerY < playerSize / 2.0f)
          playerY = playerSize / 2.0f;
        if (playerY > screenHeight - playerSize / 2.0f)
          playerY = screenHeight - playerSize / 2.0f;
      }
      Vector2 playerPos = {playerX, playerY};
      Rectangle playerRect = {playerPos.x - playerSize / 2.0f,
                              playerPos.y - playerSize / 2.0f,
                              (float)playerSize, (float)playerSize};

      if (!dead && !laserActive && laserAmmo > 0 &&
          IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        laserActive = true;
        laserTimer = laserDuration;
        laserProgress = 0.0f;
        laserAmmo--;
      }

      if (!dead) {
        obstacleSpawnTimer -= dt;
        if (obstacleSpawnTimer <= 0.0f) {
          float speedScale = 1.0f + runTime * 0.015f;
          for (int i = 0; i < maxObstacles; i++) {
            if (!obstacles[i].active) {
              float w = (float)GetRandomValue(30, 80);
              float h = (float)GetRandomValue(20, 60);
              float speed = (float)GetRandomValue(160, 360) * speedScale;
              int mode = GetRandomValue(0, 2);
              float x = 0.0f;
              float vx = 0.0f;
              if (mode == 0) {
                x = (float)GetRandomValue(0, screenWidth - (int)w);
                vx = 0.0f;
              } else if (mode == 1) {
                x = -w - (float)GetRandomValue(0, 80);
                vx = (float)GetRandomValue(80, 200) * speedScale;
              } else {
                x = screenWidth + (float)GetRandomValue(0, 80);
                vx = -(float)GetRandomValue(80, 200) * speedScale;
              }
              obstacles[i].rect = (Rectangle){x, -h, w, h};
              obstacles[i].speed = speed;
              obstacles[i].vx = vx;
              obstacles[i].active = true;
              break;
            }
          }
          obstacleSpawnTimer = 0.16f + (float)GetRandomValue(0, 25) / 100.0f;
        }

        itemSpawnTimer -= dt;
        if (itemSpawnTimer <= 0.0f) {
          for (int i = 0; i < maxItems; i++) {
            if (!items[i].active) {
              float size = (float)laserItemTexture.width;
              float x = (float)GetRandomValue(0, screenWidth - (int)size);
              items[i].rect = (Rectangle){x, -size, size, (float)laserItemTexture.height};
              items[i].speed = (float)GetRandomValue(140, 240);
              items[i].active = true;
              break;
            }
          }
          itemSpawnTimer = 3.0f + (float)GetRandomValue(0, 200) / 100.0f;
        }

        starSpawnTimer -= dt;
        if (starSpawnTimer <= 0.0f) {
          for (int i = 0; i < maxStarItems; i++) {
            if (!starItems[i].active) {
              float size = (float)starItemTexture.width;
              float x = (float)GetRandomValue(0, screenWidth - (int)size);
              starItems[i].rect = (Rectangle){x, -size, size, (float)starItemTexture.height};
              starItems[i].speed = (float)GetRandomValue(100, 180);
              starItems[i].active = true;
              break;
            }
          }
          starSpawnTimer = 15.0f + (float)GetRandomValue(0, 1000) / 100.0f;
        }

        circleSpawnTimer -= dt;
        if (circleSpawnTimer <= 0.0f) {
          for (int i = 0; i < maxCircleObstacles; i++) {
            if (!circleObstacles[i].active) {
              float radius = (float)GetRandomValue(20, 35);
              float x = (float)GetRandomValue((int)radius, screenWidth - (int)radius);
              circleObstacles[i].pos = (Vector2){x, -radius};
              circleObstacles[i].radius = radius;
              circleObstacles[i].speed = (float)GetRandomValue(80, 150);
              circleObstacles[i].shootTimer = 1.0f + (float)GetRandomValue(0, 100) / 100.0f;
              circleObstacles[i].active = true;
              circleObstacles[i].isFiring = false;
              circleObstacles[i].fireDelay = 0.0f;
              circleObstacles[i].currentAngle = 0;
              break;
            }
          }
          circleSpawnTimer = 5.0f + (float)GetRandomValue(0, 150) / 100.0f;
        }

        // Update turrets
        for (int i = 0; i < maxTurrets; i++) {
          if (!turrets[i].active)
            continue;
          
          // Move turret horizontally
          turrets[i].x += turrets[i].vx * dt;
          
          // Bounce off screen edges
          if (turrets[i].x < 30) {
            turrets[i].x = 30;
            turrets[i].vx = -turrets[i].vx;
          } else if (turrets[i].x > screenWidth - 30) {
            turrets[i].x = screenWidth - 30;
            turrets[i].vx = -turrets[i].vx;
          }
          
          turrets[i].shootTimer -= dt;
          if (turrets[i].shootTimer <= 0.0f) {
            // Find an available laser slot
            for (int j = 0; j < maxTurretLasers; j++) {
              if (!turretLasers[j].active) {
                turretLasers[j].x = turrets[i].x;
                turretLasers[j].width = turretLaserWidth;
                turretLasers[j].progress = 0.0f;
                turretLasers[j].warningTimer = turretWarningTime;
                turretLasers[j].active = true;
                turretLasers[j].firing = false;
                // Find an available sound instance to play
                for (int k = 0; k < maxLaserSounds; k++) {
                  if (!IsSoundPlaying(laserSounds[k])) {
                    PlaySound(laserSounds[k]);
                    break;
                  }
                }
                break;
              }
            }
            turrets[i].shootTimer = 5.0f;
          }
        }
      }

      for (int i = 0; i < maxObstacles; i++) {
        if (!obstacles[i].active)
          continue;
        if (!dead) {
          obstacles[i].rect.y += obstacles[i].speed * dt;
          obstacles[i].rect.x += obstacles[i].vx * dt;
        }
        if (obstacles[i].rect.y > screenHeight + obstacles[i].rect.height ||
            obstacles[i].rect.x < -obstacles[i].rect.width * 2 ||
            obstacles[i].rect.x > screenWidth + obstacles[i].rect.width * 2) {
          obstacles[i].active = false;
          if (!dead)
            dodgedCount++;
          continue;
        }
        if (!dead && CheckCollisionRecs(playerRect, obstacles[i].rect)) {
          if (invincible) {
            obstacles[i].active = false;
            Vector2 hitPos = {obstacles[i].rect.x + obstacles[i].rect.width / 2,
                              obstacles[i].rect.y + obstacles[i].rect.height / 2};
            AddParticles(particles, maxParticles, 10, hitPos);
            PlaySound(wallHitSound);
          } else {
            dead = true;
            laserActive = false;
            StopMusicStream(bgm);
            for (int k = 0; k < maxLaserSounds; k++) {
              StopSound(laserSounds[k]);
            }
            PlaySound(deathSound);
          }
        }
      }

      for (int i = 0; i < maxItems; i++) {
        if (!items[i].active)
          continue;
        if (!dead)
          items[i].rect.y += items[i].speed * dt;
        if (items[i].rect.y > screenHeight + items[i].rect.height) {
          items[i].active = false;
          continue;
        }
        if (!dead && CheckCollisionRecs(playerRect, items[i].rect)) {
          items[i].active = false;
          if (laserAmmo < maxLaserAmmo)
            laserAmmo++;
          PlaySound(itemGetSound);
        }
      }

      for (int i = 0; i < maxStarItems; i++) {
        if (!starItems[i].active)
          continue;
        if (!dead)
          starItems[i].rect.y += starItems[i].speed * dt;
        if (starItems[i].rect.y > screenHeight + starItems[i].rect.height) {
          starItems[i].active = false;
          continue;
        }
        if (!dead && CheckCollisionRecs(playerRect, starItems[i].rect)) {
          starItems[i].active = false;
          invincible = true;
          invincibilityTimer = invincibilityDuration;
          PlaySound(itemGetSound);
        }
      }

      // Update invincibility timer
      if (invincible) {
        invincibilityTimer -= dt;
        if (invincibilityTimer <= 0.0f) {
          invincible = false;
          invincibilityTimer = 0.0f;
        }
      }

      for (int i = 0; i < maxCircleObstacles; i++) {
        if (!circleObstacles[i].active)
          continue;
        if (!dead) {
          circleObstacles[i].pos.y += circleObstacles[i].speed * dt;
          
          if (!circleObstacles[i].isFiring) {
            // Not currently firing, check if it's time to start
            circleObstacles[i].shootTimer -= dt;
            if (circleObstacles[i].shootTimer <= 0.0f) {
              circleObstacles[i].isFiring = true;
              circleObstacles[i].currentAngle = 0;
              circleObstacles[i].fireDelay = 0.0f;
              circleObstacles[i].shootTimer = 1.5f + (float)GetRandomValue(0, 100) / 100.0f;
            }
          } else {
            // Currently firing bullets sequentially
            circleObstacles[i].fireDelay -= dt;
            if (circleObstacles[i].fireDelay <= 0.0f) {
              // Fire one bullet at current angle
              for (int j = 0; j < maxBullets; j++) {
                if (!bullets[j].active) {
                  float rad = circleObstacles[i].currentAngle * DEG2RAD;
                  float bulletSpeed = 200.0f;
                  bullets[j].pos = circleObstacles[i].pos;
                  bullets[j].vel = (Vector2){cosf(rad) * bulletSpeed, sinf(rad) * bulletSpeed};
                  bullets[j].active = true;
                  PlaySound(bulletFireSound);
                  break;
                }
              }
              
              // Move to next angle
              circleObstacles[i].currentAngle += 20;
              if (circleObstacles[i].currentAngle >= 360) {
                // Finished firing all bullets
                circleObstacles[i].isFiring = false;
              } else {
                // Set delay for next bullet
                circleObstacles[i].fireDelay = 0.1f;
              }
            }
          }
        }
        if (circleObstacles[i].pos.y > screenHeight + circleObstacles[i].radius * 2) {
          circleObstacles[i].active = false;
          continue;
        }
        if (!dead && CheckCollisionCircleRec(circleObstacles[i].pos, circleObstacles[i].radius, playerRect)) {
          if (invincible) {
            circleObstacles[i].active = false;
            AddParticles(particles, maxParticles, 15, circleObstacles[i].pos);
            PlaySound(wallHitSound);
          } else {
            dead = true;
            laserActive = false;
            StopMusicStream(bgm);
            for (int k = 0; k < maxLaserSounds; k++) {
              StopSound(laserSounds[k]);
            }
            PlaySound(deathSound);
          }
        }
      }

      for (int i = 0; i < maxBullets; i++) {
        if (!bullets[i].active)
          continue;
        if (!dead) {
          bullets[i].pos.x += bullets[i].vel.x * dt;
          bullets[i].pos.y += bullets[i].vel.y * dt;
        }
        if (bullets[i].pos.x < -10 || bullets[i].pos.x > screenWidth + 10 ||
            bullets[i].pos.y < -10 || bullets[i].pos.y > screenHeight + 10) {
          bullets[i].active = false;
          continue;
        }
        if (!dead && CheckCollisionCircleRec(bullets[i].pos, 4.0f, playerRect)) {
          if (invincible) {
            bullets[i].active = false;
          } else {
            dead = true;
            laserActive = false;
            StopMusicStream(bgm);
            for (int k = 0; k < maxLaserSounds; k++) {
              StopSound(laserSounds[k]);
            }
            PlaySound(deathSound);
          }
        }
      }

      // Update turret lasers
      for (int i = 0; i < maxTurretLasers; i++) {
        if (!turretLasers[i].active)
          continue;
        
        if (!dead) {
          if (!turretLasers[i].firing) {
            // Warning phase
            turretLasers[i].warningTimer -= dt;
            if (turretLasers[i].warningTimer <= 0.0f) {
              turretLasers[i].firing = true;
              turretLasers[i].progress = 0.0f;
            }
          } else {
            // Firing phase
            turretLasers[i].progress += dt;
            if (turretLasers[i].progress >= turretFireTime) {
              turretLasers[i].active = false;
              continue;
            }
            
            // Check collision with player during firing
            Rectangle laserRect = {
              turretLasers[i].x - turretLasers[i].width / 2.0f,
              0,
              turretLasers[i].width,
              (float)screenHeight
            };
            if (CheckCollisionRecs(laserRect, playerRect)) {
              if (!invincible) {
                dead = true;
                laserActive = false;
                StopMusicStream(bgm);
                for (int k = 0; k < maxLaserSounds; k++) {
                  StopSound(laserSounds[k]);
                }
                PlaySound(deathSound);
              }
            }
          }
        }
      }

      if (laserActive) {
        laserTimer -= dt;
        laserProgress += laserSpeed * dt;
        if (laserProgress > playerPos.y)
          laserProgress = playerPos.y;
        if (laserTimer <= 0.0f)
          laserActive = false;

        float beamHue =
            fmodf((float)GetTime() * 180.0f + laserProgress * 0.2f, 360.0f);
        Color beamColor = ColorFromHSV(beamHue, 0.75f, 1.0f);
        beamColor.a = 200;
        Vector2 beamEnd = {playerPos.x, playerPos.y - laserProgress};
        DrawLineEx(playerPos, beamEnd, laserWidth, beamColor);

        Rectangle beamRect = {playerPos.x - laserWidth / 2.0f,
                              playerPos.y - laserProgress, laserWidth,
                              laserProgress};
        for (int i = 0; i < maxObstacles; i++) {
          if (!obstacles[i].active)
            continue;
          if (CheckCollisionRecs(beamRect, obstacles[i].rect)) {
            Vector2 hitPos = {obstacles[i].rect.x + obstacles[i].rect.width / 2,
                              obstacles[i].rect.y +
                                  obstacles[i].rect.height / 2};
            obstacles[i].active = false;
            AddParticles(particles, maxParticles, 10, hitPos);
            PlaySound(wallHitSound);
          }
        }
        for (int i = 0; i < maxCircleObstacles; i++) {
          if (!circleObstacles[i].active)
            continue;
          Rectangle circleRect = {circleObstacles[i].pos.x - circleObstacles[i].radius,
                                  circleObstacles[i].pos.y - circleObstacles[i].radius,
                                  circleObstacles[i].radius * 2,
                                  circleObstacles[i].radius * 2};
          if (CheckCollisionRecs(beamRect, circleRect)) {
            circleObstacles[i].active = false;
            AddParticles(particles, maxParticles, 15, circleObstacles[i].pos);
            PlaySound(wallHitSound);
          }
        }
        for (int i = 0; i < maxBullets; i++) {
          if (!bullets[i].active)
            continue;
          if (bullets[i].pos.x >= beamRect.x && bullets[i].pos.x <= beamRect.x + beamRect.width &&
              bullets[i].pos.y >= beamRect.y && bullets[i].pos.y <= beamRect.y + beamRect.height) {
            bullets[i].active = false;
          }
        }
      }

      Color obstacleColor = (Color){50, 60, 80, 255};
      for (int i = 0; i < maxObstacles; i++) {
        if (obstacles[i].active)
          DrawRectangleRec(obstacles[i].rect, obstacleColor);
      }

      for (int i = 0; i < maxItems; i++) {
        if (items[i].active) {
          DrawTexture(laserItemTexture, (int)items[i].rect.x, (int)items[i].rect.y, WHITE);
        }
      }

      for (int i = 0; i < maxStarItems; i++) {
        if (starItems[i].active) {
          DrawTexture(starItemTexture, (int)starItems[i].rect.x, (int)starItems[i].rect.y, WHITE);
        }
      }

      Color circleColor = (Color){180, 60, 100, 255};
      for (int i = 0; i < maxCircleObstacles; i++) {
        if (circleObstacles[i].active)
          DrawCircleV(circleObstacles[i].pos, circleObstacles[i].radius, circleColor);
      }

      Color bulletColor = (Color){220, 80, 80, 255};
      for (int i = 0; i < maxBullets; i++) {
        if (bullets[i].active)
          DrawCircleV(bullets[i].pos, 4.0f, bulletColor);
      }

      // Draw turrets
      Color turretColor = (Color){60, 60, 70, 255};
      for (int i = 0; i < maxTurrets; i++) {
        if (turrets[i].active) {
          DrawRectangle((int)(turrets[i].x - 15), 0, 30, 20, turretColor);
          DrawCircle((int)turrets[i].x, 20, 8, (Color){80, 80, 90, 255});
        }
      }

      // Draw turret lasers
      for (int i = 0; i < maxTurretLasers; i++) {
        if (!turretLasers[i].active)
          continue;
        
        if (!turretLasers[i].firing) {
          // Warning phase - red translucent indicator
          float alpha = 100 + 155 * (1.0f - turretLasers[i].warningTimer / turretWarningTime);
          Color warningColor = (Color){255, 50, 50, (unsigned char)alpha};
          DrawRectangle(
            (int)(turretLasers[i].x - turretLasers[i].width / 2.0f),
            0,
            (int)turretLasers[i].width,
            screenHeight,
            warningColor
          );
        } else {
          // Firing phase - bright laser beam
          float intensity = 1.0f - (turretLasers[i].progress / turretFireTime);
          unsigned char alpha = (unsigned char)(255 * intensity);
          Color laserColor = (Color){255, 255, 100, alpha};
          DrawRectangle(
            (int)(turretLasers[i].x - turretLasers[i].width / 2.0f),
            0,
            (int)turretLasers[i].width,
            screenHeight,
            laserColor
          );
        }
      }

      // Draw player with invincibility visual feedback
      Color playerColor = invincible ? (Color){255, 215, 0, 255} : (Color){40, 50, 80, 255};
      DrawRectangleRec(playerRect, playerColor);

      for (int i = 0; i < maxParticles; i++) {
        if (particles[i].age < 0.0f)
          continue;
        particles[i].age += dt;
        if (particles[i].age >= particles[i].life) {
          particles[i].age = -1.0f;
          continue;
        }
        particles[i].vel.x *= 0.96f;
        particles[i].vel.y *= 0.96f;
        particles[i].pos.x += particles[i].vel.x * dt;
        particles[i].pos.y += particles[i].vel.y * dt;
        float t = particles[i].age / particles[i].life;
        unsigned char alpha = (unsigned char)(200 * (1.0f - t));
        DrawCircleV(particles[i].pos, 2.5f, (Color){255, 170, 90, alpha});
      }

      DrawText(TextFormat("LASER: %d", laserAmmo), 20, 20, 22, BLACK);
      if (invincible) {
        DrawText(TextFormat("INVINCIBLE: %.1fs", invincibilityTimer), 20, 48, 22, (Color){255, 215, 0, 255});
        DrawText("MOUSE: MOVE", 20, 76, 20, BLACK);
        DrawText("LEFT CLICK: LASER", 20, 104, 20, BLACK);
      } else {
        DrawText("MOUSE: MOVE", 20, 48, 20, BLACK);
        DrawText("LEFT CLICK: LASER", 20, 76, 20, BLACK);
      }
      const char *dodgedLabel = TextFormat("DODGED: %d", dodgedCount);
      int dodgedWidth = MeasureText(dodgedLabel, 22);
      DrawText(dodgedLabel, screenWidth - dodgedWidth - 20, 20, 22, BLACK);

      if (dead) {
        DrawText("GAME OVER", screenWidth / 2 - 140, screenHeight / 2 - 40,
                 40, BLACK);
        DrawText("Press R to Restart", screenWidth / 2 - 150,
                 screenHeight / 2 + 10, 22, BLACK);
        if (IsKeyPressed(KEY_R)) {
          dead = false;
          runTime = 0.0f;
          dodgedCount = 0;
          laserAmmo = 0;
          laserActive = false;
          laserTimer = 0.0f;
          laserProgress = 0.0f;
          obstacleSpawnTimer = 0.0f;
          itemSpawnTimer = 0.0f;
          circleSpawnTimer = 0.0f;
          starSpawnTimer = 0.0f;
          invincible = false;
          invincibilityTimer = 0.0f;
          playerX = screenWidth * 0.5f;
          playerY = screenHeight - 80.0f;
          PlayMusicStream(bgm);
          for (int i = 0; i < maxObstacles; i++) {
            obstacles[i].active = false;
          }
          for (int i = 0; i < maxItems; i++) {
            items[i].active = false;
          }
          for (int i = 0; i < maxStarItems; i++) {
            starItems[i].active = false;
          }
          for (int i = 0; i < maxCircleObstacles; i++) {
            circleObstacles[i].active = false;
          }
          for (int i = 0; i < maxBullets; i++) {
            bullets[i].active = false;
          }
          for (int i = 0; i < maxTurretLasers; i++) {
            turretLasers[i].active = false;
          }
        }
      }
    }

    if (transitioning || fadeOut) {
      unsigned char alpha =
          (unsigned char)(255 * (transitionAlpha < 0 ? 0 : transitionAlpha));
      DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, alpha});
    }

    EndDrawing();
  }

  for (int i = 0; i < maxLaserSounds; i++) {
    UnloadSound(laserSounds[i]);
  }
  UnloadSound(wallHitSound);
  UnloadSound(clickSound);
  UnloadSound(bulletFireSound);
  UnloadSound(deathSound);
  UnloadSound(itemGetSound);
  UnloadMusicStream(bgm);
  UnloadTexture(laserItemTexture);
  UnloadTexture(starItemTexture);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
