#include "raylib.h"
#include "cJSON.h"
#include <math.h>

#define MAX_STAGE_RECTS 32
#define MAX_STAGE_CIRCLES 32

typedef struct StageData {
  int rectCount;
  Rectangle rects[MAX_STAGE_RECTS];
  int circleCount;
  Vector2 circlePos[MAX_STAGE_CIRCLES];
  float circleRadius[MAX_STAGE_CIRCLES];
  Vector2 goalPos;
  float goalRadius;
  bool hasGoal;
} StageData;

typedef struct Ripple {
  Vector2 pos;
  float age;
} Ripple;

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

static void AddRipple(Ripple *ripples, int maxRipples, int *nextIndex,
                      Vector2 pos) {
  ripples[*nextIndex].pos = pos;
  ripples[*nextIndex].age = 0.0f;
  *nextIndex = (*nextIndex + 1) % maxRipples;
}

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

static bool RayIntersectCircle(Vector2 pos, Vector2 dir, Vector2 center,
                               float radius, float *tHit, Vector2 *normal) {
  Vector2 m = {pos.x - center.x, pos.y - center.y};
  float b = m.x * dir.x + m.y * dir.y;
  float c = m.x * m.x + m.y * m.y - radius * radius;
  if (c > 0.0f && b > 0.0f)
    return false;
  float discr = b * b - c;
  if (discr < 0.0f)
    return false;
  float t = -b - sqrtf(discr);
  if (t < 0.0f)
    t = 0.0f;
  if (tHit)
    *tHit = t;
  if (normal) {
    Vector2 hit = {pos.x + dir.x * t, pos.y + dir.y * t};
    Vector2 n = {hit.x - center.x, hit.y - center.y};
    float len = sqrtf(n.x * n.x + n.y * n.y);
    if (len > 0.0001f) {
      n.x /= len;
      n.y /= len;
    }
    *normal = n;
  }
  return true;
}

static bool RayIntersectRect(Vector2 pos, Vector2 dir, Rectangle rect,
                             float *tHit, Vector2 *normal) {
  if (pos.x > rect.x && pos.x < rect.x + rect.width && pos.y > rect.y &&
      pos.y < rect.y + rect.height)
    return false;

  float tmin = -INFINITY;
  float tmax = INFINITY;
  Vector2 n = {0.0f, 0.0f};

  if (fabsf(dir.x) < 0.0001f) {
    if (pos.x < rect.x || pos.x > rect.x + rect.width)
      return false;
  } else {
    float tx1 = (rect.x - pos.x) / dir.x;
    float tx2 = (rect.x + rect.width - pos.x) / dir.x;
    float tEntry = tx1 < tx2 ? tx1 : tx2;
    float tExit = tx1 < tx2 ? tx2 : tx1;
    Vector2 nEntry = tx1 < tx2 ? (Vector2){-1.0f, 0.0f}
                               : (Vector2){1.0f, 0.0f};
    if (tEntry > tmin) {
      tmin = tEntry;
      n = nEntry;
    }
    if (tExit < tmax)
      tmax = tExit;
  }

  if (fabsf(dir.y) < 0.0001f) {
    if (pos.y < rect.y || pos.y > rect.y + rect.height)
      return false;
  } else {
    float ty1 = (rect.y - pos.y) / dir.y;
    float ty2 = (rect.y + rect.height - pos.y) / dir.y;
    float tEntry = ty1 < ty2 ? ty1 : ty2;
    float tExit = ty1 < ty2 ? ty2 : ty1;
    Vector2 nEntry = ty1 < ty2 ? (Vector2){0.0f, -1.0f}
                               : (Vector2){0.0f, 1.0f};
    if (tEntry > tmin) {
      tmin = tEntry;
      n = nEntry;
    }
    if (tExit < tmax)
      tmax = tExit;
  }

  if (tmax < tmin || tmax < 0.0f)
    return false;
  if (tmin < 0.0001f)
    return false;

  if (tHit)
    *tHit = tmin;
  if (normal)
    *normal = n;
  return true;
}

static bool RayIntersectWalls(Vector2 pos, Vector2 dir, float xMin, float xMax,
                              float yMin, float yMax, float *tHit,
                              Vector2 *normal) {
  float bestT = INFINITY;
  Vector2 bestN = {0.0f, 0.0f};
  bool hit = false;

  if (dir.x > 0.0001f) {
    float t = (xMax - pos.x) / dir.x;
    float y = pos.y + dir.y * t;
    if (t > 0.0001f && y >= yMin && y <= yMax && t < bestT) {
      bestT = t;
      bestN = (Vector2){-1.0f, 0.0f};
      hit = true;
    }
  } else if (dir.x < -0.0001f) {
    float t = (xMin - pos.x) / dir.x;
    float y = pos.y + dir.y * t;
    if (t > 0.0001f && y >= yMin && y <= yMax && t < bestT) {
      bestT = t;
      bestN = (Vector2){1.0f, 0.0f};
      hit = true;
    }
  }

  if (dir.y > 0.0001f) {
    float t = (yMax - pos.y) / dir.y;
    float x = pos.x + dir.x * t;
    if (t > 0.0001f && x >= xMin && x <= xMax && t < bestT) {
      bestT = t;
      bestN = (Vector2){0.0f, -1.0f};
      hit = true;
    }
  } else if (dir.y < -0.0001f) {
    float t = (yMin - pos.y) / dir.y;
    float x = pos.x + dir.x * t;
    if (t > 0.0001f && x >= xMin && x <= xMax && t < bestT) {
      bestT = t;
      bestN = (Vector2){0.0f, 1.0f};
      hit = true;
    }
  }

  if (!hit)
    return false;
  if (tHit)
    *tHit = bestT;
  if (normal)
    *normal = bestN;
  return true;
}

static bool ReadFloat(cJSON *obj, const char *key, float *outValue) {
  cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
  if (!cJSON_IsNumber(item))
    return false;
  *outValue = (float)item->valuedouble;
  return true;
}

static void ResetStage(StageData *stage) {
  stage->rectCount = 0;
  stage->circleCount = 0;
  stage->goalPos = (Vector2){0.0f, 0.0f};
  stage->goalRadius = 0.0f;
  stage->hasGoal = false;
}

static bool LoadStage(const char *path, StageData *stage) {
  ResetStage(stage);
  char *text = LoadFileText(path);
  if (!text)
    return false;

  cJSON *root = cJSON_Parse(text);
  if (!root) {
    UnloadFileText(text);
    return false;
  }

  cJSON *rects = cJSON_GetObjectItemCaseSensitive(root, "rects");
  if (cJSON_IsArray(rects)) {
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, rects) {
      if (stage->rectCount >= MAX_STAGE_RECTS)
        break;
      float x, y, w, h;
      if (!ReadFloat(item, "x", &x) || !ReadFloat(item, "y", &y) ||
          !ReadFloat(item, "w", &w) || !ReadFloat(item, "h", &h))
        continue;
      stage->rects[stage->rectCount++] = (Rectangle){x, y, w, h};
    }
  }

  cJSON *circles = cJSON_GetObjectItemCaseSensitive(root, "circles");
  if (cJSON_IsArray(circles)) {
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, circles) {
      if (stage->circleCount >= MAX_STAGE_CIRCLES)
        break;
      float x, y, r;
      if (!ReadFloat(item, "x", &x) || !ReadFloat(item, "y", &y) ||
          !ReadFloat(item, "r", &r))
        continue;
      stage->circlePos[stage->circleCount] = (Vector2){x, y};
      stage->circleRadius[stage->circleCount] = r;
      stage->circleCount++;
    }
  }

  cJSON *goal = cJSON_GetObjectItemCaseSensitive(root, "goal");
  if (cJSON_IsObject(goal)) {
    float x, y, r;
    if (ReadFloat(goal, "x", &x) && ReadFloat(goal, "y", &y) &&
        ReadFloat(goal, "r", &r)) {
      stage->goalPos = (Vector2){x, y};
      stage->goalRadius = r;
      stage->hasGoal = true;
    }
  }

  cJSON_Delete(root);
  UnloadFileText(text);
  return true;
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
  const int wallThickness = 40;
  const float playerRadius = 35.0f;
  const Vector2 playerPos = {(float)screenWidth / 2.0f,
                             (float)screenHeight / 2.0f};
  float facingAngle = -PI / 2.0f; // up
  const float arrowLength = 55.0f;
  const float arrowWidth = 14.0f;
  const float rotationStep = PI / 24.0f; // 7.5 degrees per click
  const float rotationSpeed = PI / 2.0f; // 90 degrees per second while held
  StageData stage = {0};
  bool stageLoaded = false;
  Vector2 defaultGoalPos = {(float)screenWidth * 0.75f,
                            (float)screenHeight * 0.35f};
  const float defaultGoalRadius = 30.0f;

  const int rotateBtnW = 90;
  const int rotateBtnH = 60;
  const int rotateBtnPad = 20;
  const int fireBtnW = 110;
  const int fireBtnH = 60;
  Rectangle leftRotateBtn = {
      (float)(screenWidth - rotateBtnPad * 2 - rotateBtnW * 2),
      (float)(screenHeight - rotateBtnH - rotateBtnPad), (float)rotateBtnW,
      (float)rotateBtnH};
  Rectangle rightRotateBtn = {(float)(screenWidth - rotateBtnPad - rotateBtnW),
                              (float)(screenHeight - rotateBtnH - rotateBtnPad),
                              (float)rotateBtnW, (float)rotateBtnH};
  Rectangle fireBtn = {
      (float)(screenWidth - rotateBtnPad * 3 - rotateBtnW * 2 - fireBtnW),
      (float)(screenHeight - fireBtnH - rotateBtnPad), (float)fireBtnW,
      (float)fireBtnH};
  float beamTimer = 0.0f;
  const float beamDuration = 0.4f;
  const float beamLength = 10000.0f;
  float beamProgress = 0.0f;
  const float beamSpeed = 1200.0f;
  Vector2 beamDir = {1.0f, 0.0f};
  bool goalCleared = false;
  const float rippleDuration = 0.5f;
  const float rippleMinRadius = 6.0f;
  const float rippleMaxRadius = 28.0f;
  const int maxRipples = 16;
  Ripple ripples[16];
  int rippleNext = 0;
  const int maxParticles = 64;
  Particle particles[64];
  const int maxStars = 24;
  Star stars[24];
  float starSpawnTimer = 0.0f;

  Sound clickSound = LoadSound("決定ボタンを押す2.mp3");
  Sound wallHitSound = LoadSound("カーソル移動12.mp3");
  const float hueSpeed = 100.0f;         // degrees per second for hue shift
  const float transitionDuration = 0.6f; // seconds
  bool inGame = false;
  bool transitioning = false;
  bool fadeOut = true;
  float transitionAlpha = 0.0f;

  for (int i = 0; i < maxRipples; i++) {
    ripples[i].age = -1.0f;
  }
  for (int i = 0; i < maxParticles; i++) {
    particles[i].age = -1.0f;
  }
  for (int i = 0; i < maxStars; i++) {
    stars[i].life = -1.0f;
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
      goalCleared = false;
      stageLoaded = false;
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

    if (inGame && !stageLoaded) {
      bool loaded = LoadStage("stages/stage1.json", &stage);
      if (!loaded)
        ResetStage(&stage);
      if (!stage.hasGoal) {
        stage.goalPos = defaultGoalPos;
        stage.goalRadius = defaultGoalRadius;
        stage.hasGoal = true;
      }
      goalCleared = false;
      stageLoaded = true;
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
      Color wallColor = (Color){90, 110, 140, 255};
      DrawRectangle(0, 0, screenWidth, wallThickness, wallColor); // top
      DrawRectangle(0, screenHeight - wallThickness, screenWidth, wallThickness,
                    wallColor);                                    // bottom
      DrawRectangle(0, 0, wallThickness, screenHeight, wallColor); // left
      DrawRectangle(screenWidth - wallThickness, 0, wallThickness, screenHeight,
                    wallColor); // right

      Color rectColor = (Color){130, 130, 150, 255};
      for (int i = 0; i < stage.rectCount; i++) {
        DrawRectangleRec(stage.rects[i], rectColor);
      }
      Color circleColor = (Color){120, 160, 190, 255};
      for (int i = 0; i < stage.circleCount; i++) {
        DrawCircleV(stage.circlePos[i], stage.circleRadius[i], circleColor);
      }

      bool leftHovered = CheckCollisionPointRec(mouse, leftRotateBtn);
      bool rightHovered = CheckCollisionPointRec(mouse, rightRotateBtn);
      bool leftHeld = leftHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
      bool rightHeld = rightHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
      bool fireHovered = CheckCollisionPointRec(mouse, fireBtn);
      bool fireHeld = fireHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (leftHovered)
          facingAngle -= rotationStep;
        if (rightHovered)
          facingAngle += rotationStep;
        if (fireHovered) {
          beamTimer = beamDuration;
          beamProgress = 0.0f;
          Vector2 dir = {cosf(facingAngle), sinf(facingAngle)};
          beamDir = dir;
          goalCleared = false;
        }
      }
      if (leftHeld)
        facingAngle -= rotationSpeed * dt;
      if (rightHeld)
        facingAngle += rotationSpeed * dt;
      if (fireHeld) {
        if (beamTimer <= 0.0f)
          beamProgress = 0.0f;
        beamTimer = beamDuration;
        beamDir = (Vector2){cosf(facingAngle), sinf(facingAngle)};
        goalCleared = false;
      }
      if (facingAngle > PI)
        facingAngle -= 2.0f * PI;
      if (facingAngle < -PI)
        facingAngle += 2.0f * PI;

      Vector2 facingDir = {cosf(facingAngle), sinf(facingAngle)};
      Vector2 tip = {playerPos.x + facingDir.x * arrowLength,
                     playerPos.y + facingDir.y * arrowLength};
      Vector2 perp = {-facingDir.y, facingDir.x};
      Vector2 left = {tip.x + perp.x * (arrowWidth / 2.0f),
                      tip.y + perp.y * (arrowWidth / 2.0f)};
      Vector2 right = {tip.x - perp.x * (arrowWidth / 2.0f),
                       tip.y - perp.y * (arrowWidth / 2.0f)};

      DrawCircleV(playerPos, playerRadius, (Color){220, 220, 255, 255});
      DrawLineEx(playerPos, tip, 4.0f, (Color){40, 60, 120, 255});
      DrawTriangle(tip, left, right, (Color){240, 140, 80, 255});
      if (stage.hasGoal) {
        Color goalColor =
            goalCleared ? (Color){60, 180, 90, 255} : (Color){40, 140, 80, 255};
        DrawCircleV(stage.goalPos, stage.goalRadius, goalColor);
        if (goalCleared) {
          DrawText("CLEAR!", (int)(stage.goalPos.x - 50),
                   (int)(stage.goalPos.y - 10), 28, BLACK);
        }
      }

      if (beamTimer > 0.0f) {
        beamTimer -= dt;
        float prevProgress = beamProgress;
        beamProgress += beamSpeed * dt;
        if (beamProgress > beamLength)
          beamProgress = beamLength;
        float beamHue =
            fmodf((float)GetTime() * 180.0f + beamProgress * 0.05f, 360.0f);
        Color beamColor = ColorFromHSV(beamHue, 0.75f, 1.0f);
        beamColor.a = 200;
        Vector2 dir = beamDir;
        float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (dirLen > 0.0001f) {
          dir.x /= dirLen;
          dir.y /= dirLen;
          Vector2 pos = playerPos;
          float remaining = beamProgress;
          float traveled = 0.0f;
          const float xMin = (float)wallThickness;
          const float xMax = (float)(screenWidth - wallThickness);
          const float yMin = (float)wallThickness;
          const float yMax = (float)(screenHeight - wallThickness);
          int bounces = 0;
          const int maxBounces = 6;
          while (remaining > 0.0f && bounces <= maxBounces) {
            float bestT = remaining;
            Vector2 bestNormal = {0.0f, 0.0f};
            bool hitReflect = false;

            float tWall = 0.0f;
            Vector2 nWall = {0.0f, 0.0f};
            if (RayIntersectWalls(pos, dir, xMin, xMax, yMin, yMax, &tWall,
                                  &nWall) &&
                tWall < bestT) {
              bestT = tWall;
              bestNormal = nWall;
              hitReflect = true;
            }

            for (int i = 0; i < stage.rectCount; i++) {
              float tRect = 0.0f;
              Vector2 nRect = {0.0f, 0.0f};
              if (RayIntersectRect(pos, dir, stage.rects[i], &tRect, &nRect) &&
                  tRect < bestT) {
                bestT = tRect;
                bestNormal = nRect;
                hitReflect = true;
              }
            }

            for (int i = 0; i < stage.circleCount; i++) {
              float tCircle = 0.0f;
              Vector2 nCircle = {0.0f, 0.0f};
              if (RayIntersectCircle(pos, dir, stage.circlePos[i],
                                     stage.circleRadius[i], &tCircle,
                                     &nCircle) &&
                  tCircle < bestT) {
                bestT = tCircle;
                bestNormal = nCircle;
                hitReflect = true;
              }
            }

            if (!goalCleared && stage.hasGoal) {
              float tGoal = 0.0f;
              if (RayIntersectCircle(pos, dir, stage.goalPos, stage.goalRadius,
                                     &tGoal, NULL) &&
                  tGoal <= bestT && tGoal <= remaining) {
                Vector2 goalHit = {pos.x + dir.x * tGoal,
                                   pos.y + dir.y * tGoal};
                DrawLineEx(pos, goalHit, 6.0f, beamColor);
                goalCleared = true;
                remaining = 0.0f;
                break;
              }
            }

            Vector2 hitPos = {pos.x + dir.x * bestT, pos.y + dir.y * bestT};
            DrawLineEx(pos, hitPos, 6.0f, beamColor);

            remaining -= bestT;
            traveled += bestT;

            if (!hitReflect || bestT <= 0.0001f)
              break;

            float hitDist = traveled;
            if (hitDist > prevProgress && hitDist <= beamProgress) {
              AddRipple(ripples, maxRipples, &rippleNext, hitPos);
              AddParticles(particles, maxParticles, 8, hitPos);
              PlaySound(wallHitSound);
            }

            float dot = dir.x * bestNormal.x + dir.y * bestNormal.y;
            dir.x = dir.x - 2.0f * dot * bestNormal.x;
            dir.y = dir.y - 2.0f * dot * bestNormal.y;
            pos = hitPos;
            bounces++;
          }
        }
      }

      for (int i = 0; i < maxRipples; i++) {
        if (ripples[i].age < 0.0f)
          continue;
        ripples[i].age += dt;
        float t = ripples[i].age / rippleDuration;
        if (t >= 1.0f) {
          ripples[i].age = -1.0f;
          continue;
        }
        float radius =
            rippleMinRadius + (rippleMaxRadius - rippleMinRadius) * t;
        float inner = radius > 2.0f ? radius - 2.0f : 1.0f;
        float outer = radius + 2.0f;
        unsigned char alpha = (unsigned char)(180 * (1.0f - t));
        Color rippleColor = (Color){80, 150, 220, alpha};
        DrawRing(ripples[i].pos, inner, outer, 0.0f, 360.0f, 48, rippleColor);
      }

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

      Color btnBase = (Color){60, 70, 100, 255};
      Color btnHover = (Color){80, 100, 140, 255};
      Color fireColor =
          fireHovered ? (Color){200, 80, 80, 255} : (Color){160, 60, 60, 255};
      DrawRectangleRounded(fireBtn, 0.2f, 6, fireColor);
      DrawRectangleRoundedLines(fireBtn, 0.2f, 6, 2, (Color){30, 20, 20, 255});
      const char *fireTxt = "FIRE";
      int fireFont = 24;
      DrawText(
          fireTxt,
          (int)(fireBtn.x + (fireBtnW - MeasureText(fireTxt, fireFont)) / 2),
          (int)(fireBtn.y + (fireBtnH - fireFont) / 2), fireFont, WHITE);

      DrawRectangleRounded(leftRotateBtn, 0.2f, 6,
                           leftHovered ? btnHover : btnBase);
      DrawRectangleRoundedLines(leftRotateBtn, 0.2f, 6, 2,
                                (Color){20, 20, 30, 255});
      DrawRectangleRounded(rightRotateBtn, 0.2f, 6,
                           rightHovered ? btnHover : btnBase);
      DrawRectangleRoundedLines(rightRotateBtn, 0.2f, 6, 2,
                                (Color){20, 20, 30, 255});
      const char *leftTxt = "<";
      const char *rightTxt = ">";
      int rotFont = 28;
      DrawText(leftTxt,
               (int)(leftRotateBtn.x +
                     (rotateBtnW - MeasureText(leftTxt, rotFont)) / 2),
               (int)(leftRotateBtn.y + (rotateBtnH - rotFont) / 2), rotFont,
               WHITE);
      DrawText(rightTxt,
               (int)(rightRotateBtn.x +
                     (rotateBtnW - MeasureText(rightTxt, rotFont)) / 2),
               (int)(rightRotateBtn.y + (rotateBtnH - rotFont) / 2), rotFont,
               WHITE);
    }

    if (transitioning || fadeOut) {
      unsigned char alpha =
          (unsigned char)(255 * (transitionAlpha < 0 ? 0 : transitionAlpha));
      DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, alpha});
    }

    EndDrawing();
  }

  UnloadSound(wallHitSound);
  UnloadSound(clickSound);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
