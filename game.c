#include "raylib.h"
#include <math.h>

static bool SegmentHitsCircle(Vector2 a, Vector2 b, Vector2 center,
                              float radius, Vector2 *hitPoint) {
  Vector2 d = {b.x - a.x, b.y - a.y};
  float len2 = d.x * d.x + d.y * d.y;
  if (len2 <= 0.000001f)
    return false;
  Vector2 ac = {center.x - a.x, center.y - a.y};
  float t = (ac.x * d.x + ac.y * d.y) / len2;
  if (t < 0.0f)
    t = 0.0f;
  else if (t > 1.0f)
    t = 1.0f;
  Vector2 closest = {a.x + d.x * t, a.y + d.y * t};
  float dx = closest.x - center.x;
  float dy = closest.y - center.y;
  float dist2 = dx * dx + dy * dy;
  if (dist2 > radius * radius)
    return false;
  float len = sqrtf(len2);
  float offset = sqrtf(radius * radius - dist2);
  float tHit = t - offset / len;
  if (tHit < 0.0f)
    tHit = 0.0f;
  if (hitPoint) {
    hitPoint->x = a.x + d.x * tHit;
    hitPoint->y = a.y + d.y * tHit;
  }
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
  Vector2 goalPos = {(float)screenWidth * 0.75f, (float)screenHeight * 0.35f};
  const float goalRadius = 30.0f;

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

  Sound clickSound = LoadSound("決定ボタンを押す2.mp3");
  const float hueSpeed = 100.0f;         // degrees per second for hue shift
  const float transitionDuration = 0.6f; // seconds
  bool inGame = false;
  bool transitioning = false;
  bool fadeOut = true;
  float transitionAlpha = 0.0f;

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

    if (!inGame) {
      DrawRectangleGradientEx(
          (Rectangle){0, 0, (float)screenWidth, (float)screenHeight}, topLeft,
          topRight, bottomRight, bottomLeft);

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
      Color goalColor =
          goalCleared ? (Color){60, 180, 90, 255} : (Color){40, 140, 80, 255};
      DrawCircleV(goalPos, goalRadius, goalColor);
      if (goalCleared) {
        DrawText("CLEAR!", (int)(goalPos.x - 50), (int)(goalPos.y - 10), 28,
                 BLACK);
      }

      if (beamTimer > 0.0f) {
        beamTimer -= dt;
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
          const float xMin = (float)wallThickness;
          const float xMax = (float)(screenWidth - wallThickness);
          const float yMin = (float)wallThickness;
          const float yMax = (float)(screenHeight - wallThickness);
          int bounces = 0;
          const int maxBounces = 6;
          while (remaining > 0.0f && bounces <= maxBounces) {
            float tX = INFINITY;
            float tY = INFINITY;
            if (dir.x > 0.0f)
              tX = (xMax - pos.x) / dir.x;
            else if (dir.x < 0.0f)
              tX = (xMin - pos.x) / dir.x;
            if (dir.y > 0.0f)
              tY = (yMax - pos.y) / dir.y;
            else if (dir.y < 0.0f)
              tY = (yMin - pos.y) / dir.y;

            float tHit = tX < tY ? tX : tY;
            if (tHit <= 0.0f || !isfinite(tHit))
              break;

            float segment = (remaining < tHit) ? remaining : tHit;
            Vector2 hitPos = {pos.x + dir.x * segment, pos.y + dir.y * segment};
            Vector2 goalHit;
            bool hitGoal =
                !goalCleared &&
                SegmentHitsCircle(pos, hitPos, goalPos, goalRadius, &goalHit);
            if (hitGoal) {
              DrawLineEx(pos, goalHit, 6.0f, beamColor);
              goalCleared = true;
              remaining = 0.0f;
              break;
            } else {
              DrawLineEx(pos, hitPos, 6.0f, beamColor);
            }

            remaining -= segment;
            if (segment < tHit - 0.0001f)
              break;

            pos = hitPos;
            if (tX < tY)
              dir.x = -dir.x;
            else
              dir.y = -dir.y;

            bounces++;
          }
        }
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

  UnloadSound(clickSound);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
