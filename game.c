//本ゲームはclaude sonnet 4.5の支援を受けながら作成しました.特にRaylib特有の関数や難しい数式などについてサポートいただきました。
#include "raylib.h"
#include <math.h>
#include <stddef.h>

typedef struct Particle { //パーティクル
  Vector2 pos;
  Vector2 vel;
  float age;
  float life;
} Particle;

typedef struct Star { //背景の星
  Vector2 pos;
  Vector2 vel;
  float life;
  float maxLife;
} Star;

typedef struct Obstacle { //障害物
  Rectangle rect;
  float speed;
  float vx;
  bool active;
} Obstacle;

typedef struct LaserItem { //レーザー
  Rectangle rect;
  float speed;
  bool active;
} LaserItem;

typedef struct StarItem { //星
  Rectangle rect;
  float speed;
  bool active;
} StarItem;

typedef struct Bullet { //弾
  Vector2 pos;
  Vector2 vel;
  bool active;
} Bullet;

typedef struct CircleObstacle { //円障害物
  Vector2 pos;
  float radius;
  float speed;
  float shootTimer;
  bool active;
  bool isFiring;
  float fireDelay;
  int currentAngle;
} CircleObstacle;

typedef struct Turret { //タレット
  float x;
  float vx;
  float shootTimer;
  bool active;
} Turret;

typedef struct TurretLaser { //タレットのレーザー
  float x;
  float width;
  float progress;
  float warningTimer;
  bool active;
  bool firing;
} TurretLaser;

typedef struct ZigzagTriangle { //Z字障害物
  Vector2 pos;
  float size;
  float speed;
  float amplitude;
  float frequency;
  float time;
  bool active;
} ZigzagTriangle;

typedef struct LaserSegment { //レーザーの線分
  Vector2 start;
  Vector2 end;
} LaserSegment;


//線分と円の衝突チェック
static bool CheckLineCircleCollision(Vector2 lineStart, Vector2 lineEnd, Vector2 circlePos, float radius) {
  //線分の始点と終点を計算
  float dx = lineEnd.x - lineStart.x;
  float dy = lineEnd.y - lineStart.y;
  float lengthSquared = dx * dx + dy * dy;
  
  if (lengthSquared == 0.0f) {
    //線分が点の場合
    return CheckCollisionPointCircle(lineStart, circlePos, radius);
  }
  
  //最も近い点を計算
  float t = ((circlePos.x - lineStart.x) * dx + (circlePos.y - lineStart.y) * dy) / lengthSquared;
  t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
  
  // 最も近い点を計算
  Vector2 closest = {
    lineStart.x + t * dx,
    lineStart.y + t * dy
  };
  
  //最も近い点と円の中心との距離をチェック
  return CheckCollisionPointCircle(closest, circlePos, radius);
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

int main(void) {
  const int screenWidth = 1200; //画面の幅
  const int screenHeight = 900; //画面の高さ

  InitWindow(screenWidth, screenHeight, "Dodge Reflex"); //ウィンドウの初期化
  InitAudioDevice(); //音声の初期化
  SetTargetFPS(60); //フレームレートの設定

  const char *title = "DODGE REFLEX"; //タイトル
  const int titleFontSize = 80; //タイトルの文字サイズ

  const int buttonWidth = 200; //ボタンの幅
  const int buttonHeight = 60; //ボタンの高さ
  Rectangle startButton = {(float)(screenWidth - buttonWidth) / 2,
                           (float)screenHeight - buttonHeight - 60,
                           (float)buttonWidth, (float)buttonHeight}; //スタートボタン
  const int playerSize = 18; //プレイヤーの大きさ
  float playerX = screenWidth * 0.5f; //プレイヤーのX座標
  float playerY = screenHeight - 80.0f; //プレイヤーのY座標

  const int maxObstacles = 40; //障害物の最大数
  Obstacle obstacles[40];
  float obstacleSpawnTimer = 0.0f; //障害物の生成タイマー

  const int maxItems = 8; //レーザーの最大数
  LaserItem items[8];
  float itemSpawnTimer = 0.0f; //レーザーの生成タイマー
  int laserAmmo = 0; //レーザーの弾数
  const int maxLaserAmmo = 100; //レーザーの最大弾数

  const int maxStarItems = 3; //星の最大数
  StarItem starItems[3];
  float starSpawnTimer = 0.0f; //星の生成タイマー
  bool invincible = false; //無敵
  float invincibilityTimer = 0.0f; //無敵タイマー
  const float invincibilityDuration = 5.0f; //無敵持続時間

  const int maxCircleObstacles = 10; //円障害物の最大数
  CircleObstacle circleObstacles[10];
  float circleSpawnTimer = 0.0f; //円障害物の生成タイマー
  const int maxBullets = 200; //弾の最大数
  Bullet bullets[200];

  const int maxTurrets = 3; //タレットの最大数
  Turret turrets[3];
  const int maxTurretLasers = 3; //タレットのレーザーの最大数
  TurretLaser turretLasers[3];
  const float turretLaserWidth = 40.0f; //タレットのレーザーの幅
  const float turretWarningTime = 1.0f; //タレットの警告時間
  const float turretFireTime = 0.5f; //タレットの発射時間

  const int maxZigzagTriangles = 15; //Z字障害物の最大数
  ZigzagTriangle zigzagTriangles[15];
  float zigzagSpawnTimer = 0.0f; //Z字障害物の生成タイマー

  bool laserActive = false; //レーザーの有効化
  float laserTimer = 0.0f; //レーザーのタイマー
  float laserProgress = 0.0f; //レーザーの進行度
  float laserAngle = -90.0f;  //レーザーの角度
  const float laserDuration = 1.0f; //レーザーの持続時間
  const float laserSpeed = 5000.0f; //レーザーのスピード
  const float laserWidth = 6.0f; //レーザーの幅
  const int maxLaserReflections = 10; //レーザーの反射数
  LaserSegment laserSegments[10];  //レーザーの線分
  int laserSegmentCount = 0; //レーザーの線分の数
  const int maxParticles = 64; //パーティクルの最大数
  Particle particles[64];
  const int maxStars = 24; //星の最大数
  Star stars[24];

  Sound clickSound = LoadSound("決定ボタンを押す2.mp3"); //クリック音
  Sound wallHitSound = LoadSound("カーソル移動12.mp3"); //壁衝突音
  const int maxLaserSounds = 5; //レーザーの音の最大数
  Sound laserSounds[5];
  for (int i = 0; i < maxLaserSounds; i++) {
    laserSounds[i] = LoadSound("気弾2.mp3");
  }
  Sound bulletFireSound = LoadSound("決定ボタンを押す34.mp3"); //弾発射音
  Sound deathSound = LoadSound("チーン1.mp3"); //死亡音
  Sound itemGetSound = LoadSound("se_itemget_004.wav"); //アイテム取得音
  Music bgm = LoadMusicStream("maou_bgm_8bit27.mp3"); //BGM
  Texture2D laserItemTexture = LoadTexture("laser.gif"); //レーザーのアイテム
  Texture2D starItemTexture = LoadTexture("star.gif"); //星のアイテム
  const float hueSpeed = 100.0f;         //色相の変化速度
  const float transitionDuration = 0.6f; //秒
  bool inGame = false; //ゲーム中
  bool transitioning = false; //遷移中
  bool fadeOut = true; //フェードアウト
  float transitionAlpha = 0.0f; //遷移のアルファ
  bool loading = false; //ロード中
  float loadingProgress = 0.0f; //ロードの進行度
  const float loadingDuration = 1.5f; //ロードの持続時間
  bool dead = false; //死んだ
  float runTime = 0.0f; //プレイ時間
  float highScore = 0.0f; //最高スコア

  for (int i = 0; i < maxParticles; i++) {
    particles[i].age = -1.0f; //パーティクルの有効化
  }
  for (int i = 0; i < maxStars; i++) {
    stars[i].life = -1.0f; //星の有効化
  }
  for (int i = 0; i < maxObstacles; i++) {
    obstacles[i].active = false; //障害物の有効化
  }
  for (int i = 0; i < maxItems; i++) {
    items[i].active = false; //レーザーのアイテムの有効化
  }
  for (int i = 0; i < maxStarItems; i++) {
    starItems[i].active = false; //星のアイテムの有効化
  }
  for (int i = 0; i < maxCircleObstacles; i++) {
    circleObstacles[i].active = false; //円障害物の有効化
  }
  for (int i = 0; i < maxBullets; i++) {
    bullets[i].active = false; //弾の有効化
  }
  for (int i = 0; i < maxTurrets; i++) {
    //タレットを画面を等間隔に配置
    turrets[i].x = (float)(i + 1) * screenWidth / (maxTurrets + 1);
    //タレットの方向を交互に変更
    turrets[i].vx = (i % 2 == 0) ? 150.0f : -150.0f;
    //タレットの発射タイミングを交互に変更
    turrets[i].shootTimer = 5.0f + (float)i * 1.0f;
    turrets[i].active = true;
  }
  for (int i = 0; i < maxTurretLasers; i++) {
    turretLasers[i].active = false; //タレットのレーザーの有効化
  }
  for (int i = 0; i < maxZigzagTriangles; i++) {
    zigzagTriangles[i].active = false; //Z字障害物の有効化
  }


  while (!WindowShouldClose()) {
    Vector2 mouse = GetMousePosition(); //マウスの位置  
    bool hovered =
        !inGame && !transitioning && !loading && CheckCollisionPointRec(mouse, startButton); //スタートボタンの範囲内
    bool pressed = hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON); //スタートボタンを押した
    if (!inGame && !transitioning && !loading && hovered &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      PlaySound(clickSound); //クリック音
      loading = true;
      loadingProgress = 0.0f; //ロードの進行度
      dead = false;
      runTime = 0.0f;
      // High score is persistent across games
      laserAmmo = 0;
      laserActive = false;
      laserTimer = 0.0f;
      laserProgress = 0.0f;
      laserAngle = -90.0f; //レーザーの角度
      obstacleSpawnTimer = 0.0f; //障害物の生成タイマー
      itemSpawnTimer = 0.0f; //レーザーの生成タイマー
      circleSpawnTimer = 0.0f; //円障害物の生成タイマー
      starSpawnTimer = 0.0f; //星の生成タイマー
      invincible = false; //無敵
      invincibilityTimer = 0.0f; //無敵タイマー
      playerY = screenHeight - 80.0f; //プレイヤーのY座標
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
      for (int i = 0; i < maxZigzagTriangles; i++) {
        zigzagTriangles[i].active = false;
      }
    }

    float dt = GetFrameTime(); //フレーム時間
    
    if (loading) {
      loadingProgress += dt / loadingDuration; //ロードの進行度
      if (loadingProgress >= 1.0f) {
        loading = false;
        loadingProgress = 1.0f;
        transitioning = true;
        fadeOut = true;
        transitionAlpha = 0.0f;
        PlayMusicStream(bgm);
      }
    }
    
    if (transitioning) {
      float delta = dt / transitionDuration; //遷移の進行度
      if (fadeOut) {
        transitionAlpha += delta; //フェードアウト
        if (transitionAlpha >= 1.0f) {
          transitionAlpha = 1.0f;
          fadeOut = false;
          inGame = true;
        }
      } else {
        transitionAlpha -= delta; //フェードイン
        if (transitionAlpha <= 0.0f) {
          transitionAlpha = 0.0f;
          transitioning = false;
        }
      }
    }

    float t = (float)GetTime(); //時間    
    float hueTop = fmodf(t * hueSpeed, 360.0f); //色相
    float hueBottom = fmodf(t * hueSpeed + 60.0f, 360.0f); //色相
    Color topLeft = ColorFromHSV(hueTop, 0.45f, 0.35f); //色相
    Color topRight = ColorFromHSV(hueTop + 10.0f, 0.5f, 0.4f); //色相
    Color bottomLeft = ColorFromHSV(hueBottom, 0.5f, 0.55f); //色相
    Color bottomRight = ColorFromHSV(hueBottom + 15.0f, 0.55f, 0.6f); //色相    

    BeginDrawing();
    ClearBackground((Color){18, 18, 28, 255}); //背景色

    UpdateMusicStream(bgm); //音楽の更新

    if (!inGame) {
      starSpawnTimer -= dt; //星の生成タイマー
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
        starSpawnTimer = 0.35f + (float)GetRandomValue(0, 40) / 100.0f; //星の生成タイマー
      }

      DrawRectangleGradientEx(
          (Rectangle){0, 0, (float)screenWidth, (float)screenHeight}, topLeft,
          topRight, bottomRight, bottomLeft); //グラデーション

      for (int i = 0; i < maxStars; i++) { //星の更新
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

      int titleWidth = MeasureText(title, titleFontSize); //タイトルの幅        
      DrawText(title, (screenWidth - titleWidth) / 2, 50, titleFontSize, WHITE); //タイトルの表示

      Color buttonColor =
          hovered ? (Color){70, 160, 255, 255} : (Color){50, 130, 220, 255}; //ボタンの色
      if (pressed)
        buttonColor = (Color){30, 100, 200, 255}; //ボタンの色

      DrawRectangleRounded(startButton, 0.2f, 8, buttonColor); //ボタンの描画
      DrawRectangleRoundedLines(startButton, 0.2f, 8, 2,
                                (Color){10, 20, 30, 255}); //ボタンの枠

      const char *startText = "START";
      int startTextSize = 28;
      int startTextWidth = MeasureText(startText, startTextSize);
      DrawText(startText,
               (int)(startButton.x + (buttonWidth - startTextWidth) / 2),
               (int)(startButton.y + (buttonHeight - startTextSize) / 2),
               startTextSize, WHITE); //スタートテキストの表示
      
      //ロード画面の表示
      if (loading) {
        //半透明の暗いオーバーレイ
        DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 200});
        
        //アニメーションするロードテキスト
        int dotCount = ((int)(loadingProgress * 12.0f)) % 4;
        const char *loadingTexts[] = {"LOADING", "LOADING.", "LOADING..", "LOADING..."};
        const char *loadingText = loadingTexts[dotCount];
        int loadingFontSize = 48;
        int loadingTextWidth = MeasureText(loadingText, loadingFontSize);
        DrawText(loadingText, (screenWidth - loadingTextWidth) / 2, 
                 screenHeight / 2 - 80, loadingFontSize, WHITE); //ロードテキストの表示
        
        //プログレスバー
        int barWidth = 400;
        int barHeight = 30;
        int barX = (screenWidth - barWidth) / 2;
        int barY = screenHeight / 2;
        
        //プログレスバーの背景
        DrawRectangle(barX, barY, barWidth, barHeight, (Color){40, 40, 50, 255}); //プログレスバーの背景
        
        //プログレスバーの塗りつぶし
        int fillWidth = (int)(barWidth * loadingProgress); //プログレスバーの幅
        float hue = fmodf((float)GetTime() * 120.0f, 360.0f);
        Color fillColor = ColorFromHSV(hue, 0.7f, 0.9f);
        DrawRectangle(barX, barY, fillWidth, barHeight, fillColor); //プログレスバーの塗りつぶし
        
        //プログレスバーの枠
        DrawRectangleLines(barX, barY, barWidth, barHeight, WHITE); //プログレスバーの枠
        
        //回転する円
        float angle = (float)GetTime() * 180.0f;
        int circleX = screenWidth / 2;
        int circleY = screenHeight / 2 + 80;
        for (int i = 0; i < 8; i++) {
          float a = (angle + i * 45.0f) * DEG2RAD;
          float x = circleX + cosf(a) * 30.0f;
          float y = circleY + sinf(a) * 30.0f;
          float alpha = 255.0f * (1.0f - (float)i / 8.0f);
          DrawCircle((int)x, (int)y, 5, (Color){255, 255, 255, (unsigned char)alpha});  //回転する円
        }
        
        //パーセンテージテキスト
        const char *percentText = TextFormat("%.0f%%", loadingProgress * 100.0f); //パーセンテージテキスト
        int percentWidth = MeasureText(percentText, 24); //パーセンテージテキストの幅
        DrawText(percentText, (screenWidth - percentWidth) / 2, 
                 barY + barHeight + 15, 24, WHITE); //パーセンテージテキストの表示
      }
    } else {
      ClearBackground(WHITE);

      if (!dead) {
        runTime += dt; //プレイ時間
        playerX = mouse.x; //プレイヤーの位置
        playerY = mouse.y; //プレイヤーの位置
        if (playerX < playerSize / 2.0f)
          playerX = playerSize / 2.0f; //プレイヤーの位置
        if (playerX > screenWidth - playerSize / 2.0f)
          playerX = screenWidth - playerSize / 2.0f; //プレイヤーの位置
        if (playerY < playerSize / 2.0f)
          playerY = playerSize / 2.0f; //プレイヤーの位置
        if (playerY > screenHeight - playerSize / 2.0f)
          playerY = screenHeight - playerSize / 2.0f; //プレイヤーの位置
        
        //マウスホイールでレーザー角度調整
        float wheelMove = GetMouseWheelMove(); //マウスホイールの移動
        if (wheelMove != 0.0f) {
          laserAngle -= wheelMove * 10.0f;  //Scroll up = negative = counter-clockwise
          //角度を-180から180の範囲で制限
          while (laserAngle < -180.0f) laserAngle += 360.0f;
          while (laserAngle > 180.0f) laserAngle -= 360.0f;
        }
      }
      Vector2 playerPos = {playerX, playerY};
      Rectangle playerRect = {playerPos.x - playerSize / 2.0f,
                              playerPos.y - playerSize / 2.0f,
                              (float)playerSize, (float)playerSize};

      if (!dead && !laserActive && laserAmmo > 0 &&
          IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { //レーザーの発射
        laserActive = true;
        laserTimer = laserDuration;
        laserProgress = 0.0f;
        laserAmmo--;
      }

      if (!dead) {
        obstacleSpawnTimer -= dt; //障害物の生成
        if (obstacleSpawnTimer <= 0.0f) {
          float speedScale = 1.0f + runTime * 0.015f; //障害物のスピード
          for (int i = 0; i < maxObstacles; i++) {
            if (!obstacles[i].active) {
              float w = (float)GetRandomValue(30, 80); //障害物の幅
              float h = (float)GetRandomValue(20, 60); //障害物の高さ
              float speed = (float)GetRandomValue(160, 360) * speedScale; //障害物のスピード
              int mode = GetRandomValue(0, 2); //障害物の移動方向
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
          obstacleSpawnTimer = 0.10f + (float)GetRandomValue(0, 20) / 100.0f; //障害物の生成
        }

        itemSpawnTimer -= dt; //アイテムの生成
        if (itemSpawnTimer <= 0.0f) {
          for (int i = 0; i < maxItems; i++) {
            if (!items[i].active) {
              float size = (float)laserItemTexture.width; //アイテムの幅
              float x = (float)GetRandomValue(0, screenWidth - (int)size); //アイテムの位置
              items[i].rect = (Rectangle){x, -size, size, (float)laserItemTexture.height}; //アイテムの位置
              items[i].speed = (float)GetRandomValue(140, 240);
              items[i].active = true;
              break;
            }
          }
          itemSpawnTimer = 3.0f + (float)GetRandomValue(0, 200) / 100.0f; //アイテムの生成
        }

        starSpawnTimer -= dt; //スターアイテムの生成
        if (starSpawnTimer <= 0.0f) {
          for (int i = 0; i < maxStarItems; i++) {
            if (!starItems[i].active) {
              float size = (float)starItemTexture.width; //スターアイテムの幅
              float x = (float)GetRandomValue(0, screenWidth - (int)size); //スターアイテムの位置
              starItems[i].rect = (Rectangle){x, -size, size, (float)starItemTexture.height};
              starItems[i].speed = (float)GetRandomValue(100, 180);
              starItems[i].active = true;
              break;
            }
          }
          starSpawnTimer = 15.0f + (float)GetRandomValue(0, 1000) / 100.0f; //スターアイテムの生成
        }

        circleSpawnTimer -= dt; //円障害物の生成
        if (circleSpawnTimer <= 0.0f) {
          for (int i = 0; i < maxCircleObstacles; i++) {
            if (!circleObstacles[i].active) {
              float radius = (float)GetRandomValue(20, 35); //円障害物の半径
              float x = (float)GetRandomValue((int)radius, screenWidth - (int)radius); //円障害物の位置
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
          circleSpawnTimer = 5.0f + (float)GetRandomValue(0, 150) / 100.0f; //円障害物の生成
        }

        zigzagSpawnTimer -= dt; //くねくね障害物の生成
        if (zigzagSpawnTimer <= 0.0f) {
          for (int i = 0; i < maxZigzagTriangles; i++) {
            if (!zigzagTriangles[i].active) {
              float size = (float)GetRandomValue(15, 25); //くねくね障害物の幅
              float x = (float)GetRandomValue((int)size, screenWidth - (int)size); //くねくね障害物の位置
              zigzagTriangles[i].pos = (Vector2){x, -size};
              zigzagTriangles[i].size = size;
              zigzagTriangles[i].speed = (float)GetRandomValue(100, 180);
              zigzagTriangles[i].amplitude = (float)GetRandomValue(100,600);
              zigzagTriangles[i].frequency = (float)GetRandomValue(200, 400) / 100.0f;
              zigzagTriangles[i].time = 0.0f;
              zigzagTriangles[i].active = true;
              break;
            }
          }
          zigzagSpawnTimer = 1.5f + (float)GetRandomValue(0, 200) / 100.0f; //くねくね障害物の生成
        }

        //タレットの更新
        for (int i = 0; i < maxTurrets; i++) {
          if (!turrets[i].active)
            continue;
          
          //タレットの移動
          turrets[i].x += turrets[i].vx * dt;//タレットの移動
          
          //タレットの反射
          if (turrets[i].x < 30) {
            turrets[i].x = 30;
            turrets[i].vx = -turrets[i].vx;
          } else if (turrets[i].x > screenWidth - 30) {
            turrets[i].x = screenWidth - 30;
            turrets[i].vx = -turrets[i].vx;
          }
          
          turrets[i].shootTimer -= dt; //タレットの発射
          if (turrets[i].shootTimer <= 0.0f) {
            //空いているレーザースロットを探す
            for (int j = 0; j < maxTurretLasers; j++) {
              if (!turretLasers[j].active) {
                turretLasers[j].x = turrets[i].x;
                turretLasers[j].width = turretLaserWidth;
                turretLasers[j].progress = 0.0f;
                turretLasers[j].warningTimer = turretWarningTime;
                turretLasers[j].active = true;
                turretLasers[j].firing = false;
                //空いている音声インスタンスを探す
                for (int k = 0; k < maxLaserSounds; k++) {
                  if (!IsSoundPlaying(laserSounds[k])) {
                    PlaySound(laserSounds[k]);
                    break;
                  }
                }
                break;
              }
            }
            turrets[i].shootTimer = 5.0f; //タレットの発射
          }
        }
      }

      for (int i = 0; i < maxObstacles; i++) {
        if (!obstacles[i].active)
          continue;
        if (!dead) {
          obstacles[i].rect.y += obstacles[i].speed * dt; //障害物の移動
          obstacles[i].rect.x += obstacles[i].vx * dt;
        }
        if (obstacles[i].rect.y > screenHeight + obstacles[i].rect.height ||
            obstacles[i].rect.x < -obstacles[i].rect.width * 2 ||
            obstacles[i].rect.x > screenWidth + obstacles[i].rect.width * 2) { //障害物の削除
          obstacles[i].active = false;
          continue;
        }
        if (!dead && CheckCollisionRecs(playerRect, obstacles[i].rect)) {
          if (invincible) {
            obstacles[i].active = false; //障害物の削除
            Vector2 hitPos = {obstacles[i].rect.x + obstacles[i].rect.width / 2,
                              obstacles[i].rect.y + obstacles[i].rect.height / 2};
            AddParticles(particles, maxParticles, 10, hitPos);
            PlaySound(wallHitSound);
          } else {
            dead = true;
            laserActive = false;
            if (runTime > highScore) {
              highScore = runTime;
            }
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
        if (!dead) {
          items[i].rect.y += items[i].speed * dt; //アイテムの移動
        }
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
        if (!dead) {
          starItems[i].rect.y += starItems[i].speed * dt; //スターアイテムの移動
        }
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

      //インビジブルティタイマーの更新
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
          circleObstacles[i].pos.y += circleObstacles[i].speed * dt; //円障害物の移動
          
          if (!circleObstacles[i].isFiring) {
            //発射していない場合、発射タイミングをチェック
            circleObstacles[i].shootTimer -= dt;
            if (circleObstacles[i].shootTimer <= 0.0f) {
              circleObstacles[i].isFiring = true; //発射開始
              circleObstacles[i].currentAngle = 0; //角度を0にリセット
              circleObstacles[i].fireDelay = 0.0f; //発射遅延をリセット
              circleObstacles[i].shootTimer = 1.5f + (float)GetRandomValue(0, 100) / 100.0f; //次の発射タイミング
            }
          } else {
            //弾を順番に発射中
            circleObstacles[i].fireDelay -= dt; //発射遅延を減らす
            if (circleObstacles[i].fireDelay <= 0.0f) {
              //現在の角度で弾を1発発射
              for (int j = 0; j < maxBullets; j++) {
                if (!bullets[j].active) {
                  float rad = circleObstacles[i].currentAngle * DEG2RAD; //角度をラジアンに変換
                  float bulletSpeed = 200.0f; //弾の速度
                  bullets[j].pos = circleObstacles[i].pos; //弾の位置
                  bullets[j].vel = (Vector2){cosf(rad) * bulletSpeed, sinf(rad) * bulletSpeed}; //弾の速度
                  bullets[j].active = true; //弾を有効化
                  PlaySound(bulletFireSound); //発射音
                  break;
                }
              }
              
              //次の角度へ移動
              circleObstacles[i].currentAngle += 20; //20度ずつ回転
              if (circleObstacles[i].currentAngle >= 360) {
                //全方向に発射完了
                circleObstacles[i].isFiring = false; //発射終了
              } else {
                //次の弾の発射遅延を設定
                circleObstacles[i].fireDelay = 0.1f; //0.1秒後に次の弾
              }
            }
          }
        }
        if (circleObstacles[i].pos.y > screenHeight + circleObstacles[i].radius * 2) {
          circleObstacles[i].active = false; //画面外に出たら削除
          continue;
        }
        if (!dead && CheckCollisionCircleRec(circleObstacles[i].pos, circleObstacles[i].radius, playerRect)) {
          if (invincible) { //無敵の場合
            circleObstacles[i].active = false; //円障害物を削除
            AddParticles(particles, maxParticles, 15, circleObstacles[i].pos); //パーティクル生成
            PlaySound(wallHitSound); //衝突音
          } else { //無敵でない場合
            dead = true; //死亡
            laserActive = false; //レーザー無効化
            if (runTime > highScore) {
              highScore = runTime; //ハイスコア更新
            }
            StopMusicStream(bgm); //BGM停止
            for (int k = 0; k < maxLaserSounds; k++) {
              StopSound(laserSounds[k]); //レーザー音停止
            }
            PlaySound(deathSound); //死亡音
          }
        }
      }

      for (int i = 0; i < maxZigzagTriangles; i++) {
        if (!zigzagTriangles[i].active)
          continue;
        if (!dead) {
          //垂直位置の更新
          zigzagTriangles[i].pos.y += zigzagTriangles[i].speed * dt; //下に移動
          //水平位置をサイン波で更新
          zigzagTriangles[i].time += dt; //時間を更新
          zigzagTriangles[i].pos.x += sinf(zigzagTriangles[i].time * zigzagTriangles[i].frequency) * 
                                       zigzagTriangles[i].amplitude * dt; //ジグザグ移動
          //三角形を画面内に保つ
          if (zigzagTriangles[i].pos.x < zigzagTriangles[i].size) {
            zigzagTriangles[i].pos.x = zigzagTriangles[i].size; //左端で停止
          }
          if (zigzagTriangles[i].pos.x > screenWidth - zigzagTriangles[i].size) {
            zigzagTriangles[i].pos.x = screenWidth - zigzagTriangles[i].size; //右端で停止
          }
        }
        if (zigzagTriangles[i].pos.y > screenHeight + zigzagTriangles[i].size * 2) {
          zigzagTriangles[i].active = false; //画面外に出たら削除
          continue;
        }
        //三角形とプレイヤーの衝突判定
        if (!dead) {
          //三角形の簡易的な当たり判定用矩形
          Rectangle triangleBounds = {
            zigzagTriangles[i].pos.x - zigzagTriangles[i].size,
            zigzagTriangles[i].pos.y - zigzagTriangles[i].size,
            zigzagTriangles[i].size * 2,
            zigzagTriangles[i].size * 2
          };
          if (CheckCollisionRecs(playerRect, triangleBounds)) {
            if (invincible) { //無敵の場合
              zigzagTriangles[i].active = false; //三角形を削除
              AddParticles(particles, maxParticles, 12, zigzagTriangles[i].pos); //パーティクル生成
              PlaySound(wallHitSound); //衝突音
            } else { //無敵でない場合
              dead = true; //死亡
              laserActive = false; //レーザー無効化
              if (runTime > highScore) {
                highScore = runTime; //ハイスコア更新
              }
              StopMusicStream(bgm); //BGM停止
              for (int k = 0; k < maxLaserSounds; k++) {
                StopSound(laserSounds[k]); //レーザー音停止
              }
              PlaySound(deathSound); //死亡音
            }
          }
        }
      }


      for (int i = 0; i < maxBullets; i++) { //弾の更新
        if (!bullets[i].active)
          continue;
        if (!dead) {
          bullets[i].pos.x += bullets[i].vel.x * dt; //弾のX座標更新
          bullets[i].pos.y += bullets[i].vel.y * dt; //弾のY座標更新
        }
        if (bullets[i].pos.x < -10 || bullets[i].pos.x > screenWidth + 10 ||
            bullets[i].pos.y < -10 || bullets[i].pos.y > screenHeight + 10) {
          bullets[i].active = false; //画面外に出たら削除
          continue;
        }
        if (!dead && CheckCollisionCircleRec(bullets[i].pos, 4.0f, playerRect)) {
          if (invincible) { //無敵の場合
            bullets[i].active = false; //弾を削除
          } else { //無敵でない場合
            dead = true; //死亡
            laserActive = false; //レーザー無効化
            if (runTime > highScore) {
              highScore = runTime; //ハイスコア更新
            }
            StopMusicStream(bgm); //BGM停止
            for (int k = 0; k < maxLaserSounds; k++) {
              StopSound(laserSounds[k]); //レーザー音停止
            }
            PlaySound(deathSound); //死亡音
          }
        }
      }

      //タレットレーザーの更新
      for (int i = 0; i < maxTurretLasers; i++) {
        if (!turretLasers[i].active)
          continue;
        
        if (!dead) {
          if (!turretLasers[i].firing) {
            //警告フェーズ
            turretLasers[i].warningTimer -= dt; //警告タイマーを減らす
            if (turretLasers[i].warningTimer <= 0.0f) {
              turretLasers[i].firing = true; //発射開始
              turretLasers[i].progress = 0.0f; //進行度をリセット
            }
          } else {
            //発射フェーズ
            turretLasers[i].progress += dt; //進行度を更新
            if (turretLasers[i].progress >= turretFireTime) {
              turretLasers[i].active = false; //発射終了
              continue;
            }
            
            //発射中のプレイヤーとの衝突判定
            Rectangle laserRect = {
              turretLasers[i].x - turretLasers[i].width / 2.0f,
              0,
              turretLasers[i].width,
              (float)screenHeight
            };
            if (CheckCollisionRecs(laserRect, playerRect)) {
              if (!invincible) { //無敵でない場合
                dead = true; //死亡
                laserActive = false; //レーザー無効化
                if (runTime > highScore) {
                  highScore = runTime; //ハイスコア更新
                }
                StopMusicStream(bgm); //BGM停止
                for (int k = 0; k < maxLaserSounds; k++) {
                  StopSound(laserSounds[k]); //レーザー音停止
                }
                PlaySound(deathSound); //死亡音
              }
            }
          }
        }
      }

      if (laserActive) { //レーザーが有効な場合
        laserTimer -= dt; //レーザータイマーを減らす
        laserProgress += laserSpeed * dt; //レーザーの進行度を更新
        
        if (laserTimer <= 0.0f)
          laserActive = false; //タイマーが終了したら無効化

        //反射を考慮したレーザーセグメントを計算
        laserSegmentCount = 0; //セグメント数をリセット
        Vector2 currentPos = playerPos; //現在の位置
        float currentAngle = laserAngle * DEG2RAD; //現在の角度（ラジアン）
        float remainingLength = laserProgress; //残りの長さ
        
        for (int reflection = 0; reflection <= maxLaserReflections && remainingLength > 0.0f; reflection++) {
          float dirX = cosf(currentAngle); //X方向の単位ベクトル
          float dirY = sinf(currentAngle); //Y方向の単位ベクトル
          
          //各壁までの距離を計算
          float distToWall = 10000.0f; //壁までの距離
          int hitWall = -1; //0=上, 1=右, 2=下, 3=左
          
          if (dirY < 0) { //上に移動
            float dist = -currentPos.y / dirY; //上壁までの距離
            if (dist > 0 && dist < distToWall) { distToWall = dist; hitWall = 0; }
          }
          if (dirY > 0) { //下に移動
            float dist = (screenHeight - currentPos.y) / dirY; //下壁までの距離
            if (dist > 0 && dist < distToWall) { distToWall = dist; hitWall = 2; }
          }
          if (dirX > 0) { //右に移動
            float dist = (screenWidth - currentPos.x) / dirX; //右壁までの距離
            if (dist > 0 && dist < distToWall) { distToWall = dist; hitWall = 1; }
          }
          if (dirX < 0) { //左に移動
            float dist = -currentPos.x / dirX; //左壁までの距離
            if (dist > 0 && dist < distToWall) { distToWall = dist; hitWall = 3; }
          }
          
          //セグメントの終点を決定
          float segmentLength = (distToWall < remainingLength) ? distToWall : remainingLength; //セグメントの長さ
          Vector2 segmentEnd = {
            currentPos.x + dirX * segmentLength,
            currentPos.y + dirY * segmentLength
          };
          
          //セグメントを保存
          if (laserSegmentCount < 6) {
            laserSegments[laserSegmentCount].start = currentPos; //開始点
            laserSegments[laserSegmentCount].end = segmentEnd; //終点
            laserSegmentCount++; //セグメント数を増やす
          }
          
          remainingLength -= segmentLength; //残りの長さを減らす
          
          //壁に当たって残りの長さがある場合、反射
          if (distToWall < remainingLength + segmentLength && hitWall >= 0) {
            currentPos = segmentEnd; //現在位置を更新
            //当たった壁に応じて角度を反射
            if (hitWall == 0 || hitWall == 2) { //上または下の壁
              currentAngle = -currentAngle; //Y軸で反射
            } else { //左または右の壁
              currentAngle = PI - currentAngle; //X軸で反射
            }
          } else {
            break; //これ以上反射なし
          }
        }

        //全てのレーザーセグメントを描画
        float beamHue = fmodf((float)GetTime() * 180.0f + laserProgress * 0.2f, 360.0f); //色相を計算
        Color beamColor = ColorFromHSV(beamHue, 0.75f, 1.0f); //色相から色を生成
        beamColor.a = 200; //透明度を設定
        
        for (int i = 0; i < laserSegmentCount; i++) {
          DrawLineEx(laserSegments[i].start, laserSegments[i].end, laserWidth, beamColor); //セグメントを描画
        }
        
        //レーザーセグメントと障害物の衝突
        for (int seg = 0; seg < laserSegmentCount; seg++) {
          Vector2 segStart = laserSegments[seg].start;
          Vector2 segEnd = laserSegments[seg].end;
          
          for (int i = 0; i < maxObstacles; i++) {
            if (!obstacles[i].active)
              continue;
            //レーザーセグメントと障害物の衝突
            if (CheckCollisionLines(segStart, segEnd, 
                                     (Vector2){obstacles[i].rect.x, obstacles[i].rect.y},
                                     (Vector2){obstacles[i].rect.x + obstacles[i].rect.width, obstacles[i].rect.y}, NULL) ||
                CheckCollisionLines(segStart, segEnd,
                                     (Vector2){obstacles[i].rect.x + obstacles[i].rect.width, obstacles[i].rect.y},
                                     (Vector2){obstacles[i].rect.x + obstacles[i].rect.width, obstacles[i].rect.y + obstacles[i].rect.height}, NULL) ||
                CheckCollisionLines(segStart, segEnd,
                                     (Vector2){obstacles[i].rect.x + obstacles[i].rect.width, obstacles[i].rect.y + obstacles[i].rect.height},
                                     (Vector2){obstacles[i].rect.x, obstacles[i].rect.y + obstacles[i].rect.height}, NULL) ||
                CheckCollisionLines(segStart, segEnd,
                                     (Vector2){obstacles[i].rect.x, obstacles[i].rect.y + obstacles[i].rect.height},
                                     (Vector2){obstacles[i].rect.x, obstacles[i].rect.y}, NULL) ||
                CheckCollisionPointRec(segEnd, obstacles[i].rect)) {
              Vector2 hitPos = {obstacles[i].rect.x + obstacles[i].rect.width / 2,
                                obstacles[i].rect.y + obstacles[i].rect.height / 2};
              obstacles[i].active = false;
              AddParticles(particles, maxParticles, 10, hitPos);
              PlaySound(wallHitSound);
            }
          }
        }
        for (int seg = 0; seg < laserSegmentCount; seg++) {
          Vector2 segStart = laserSegments[seg].start;
          Vector2 segEnd = laserSegments[seg].end;
          
          for (int i = 0; i < maxCircleObstacles; i++) {
            if (!circleObstacles[i].active)
              continue;
            //線と円の衝突をチェック
            if (CheckLineCircleCollision(segStart, segEnd, circleObstacles[i].pos, circleObstacles[i].radius)) {
              circleObstacles[i].active = false;
              AddParticles(particles, maxParticles, 15, circleObstacles[i].pos);
              PlaySound(wallHitSound);
            }
          }
        }
        for (int seg = 0; seg < laserSegmentCount; seg++) {
          Vector2 segStart = laserSegments[seg].start;
          Vector2 segEnd = laserSegments[seg].end;
          
          for (int i = 0; i < maxBullets; i++) {
            if (!bullets[i].active)
              continue;
            //弾とレーザーセグメントの衝突をチェック
            if (CheckLineCircleCollision(segStart, segEnd, bullets[i].pos, 4.0f)) {
              bullets[i].active = false;
            }
          }
        }
        for (int seg = 0; seg < laserSegmentCount; seg++) {
          Vector2 segStart = laserSegments[seg].start;
          Vector2 segEnd = laserSegments[seg].end;
          
          for (int i = 0; i < maxZigzagTriangles; i++) {
            if (!zigzagTriangles[i].active)
              continue;
            Rectangle triangleBounds = {
              zigzagTriangles[i].pos.x - zigzagTriangles[i].size,
              zigzagTriangles[i].pos.y - zigzagTriangles[i].size,
              zigzagTriangles[i].size * 2,
              zigzagTriangles[i].size * 2
            };
            //三角形とレーザーセグメントの衝突をチェック
            if (CheckCollisionLines(segStart, segEnd,
                                     (Vector2){triangleBounds.x, triangleBounds.y},
                                     (Vector2){triangleBounds.x + triangleBounds.width, triangleBounds.y}, NULL) ||
                CheckCollisionLines(segStart, segEnd,
                                     (Vector2){triangleBounds.x + triangleBounds.width, triangleBounds.y},
                                     (Vector2){triangleBounds.x + triangleBounds.width, triangleBounds.y + triangleBounds.height}, NULL) ||
                CheckCollisionLines(segStart, segEnd,
                                     (Vector2){triangleBounds.x + triangleBounds.width, triangleBounds.y + triangleBounds.height},
                                     (Vector2){triangleBounds.x, triangleBounds.y + triangleBounds.height}, NULL) ||
                CheckCollisionLines(segStart, segEnd,
                                     (Vector2){triangleBounds.x, triangleBounds.y + triangleBounds.height},
                                     (Vector2){triangleBounds.x, triangleBounds.y}, NULL) ||
                CheckCollisionPointRec(segEnd, triangleBounds)) {
              zigzagTriangles[i].active = false;
              AddParticles(particles, maxParticles, 12, zigzagTriangles[i].pos);
              PlaySound(wallHitSound);
            }
          }
        }
      }

      Color obstacleColor = (Color){50, 60, 80, 255}; //障害物の色
      for (int i = 0; i < maxObstacles; i++) {
        if (obstacles[i].active)
          DrawRectangleRec(obstacles[i].rect, obstacleColor); //障害物を描画
      }

      for (int i = 0; i < maxItems; i++) { //レーザーアイテムの描画
        if (items[i].active) {
          DrawTexture(laserItemTexture, (int)items[i].rect.x, (int)items[i].rect.y, WHITE); //テクスチャを描画
        }
      }

      for (int i = 0; i < maxStarItems; i++) { //星アイテムの描画
        if (starItems[i].active) {
          DrawTexture(starItemTexture, (int)starItems[i].rect.x, (int)starItems[i].rect.y, WHITE); //テクスチャを描画
        }
      }

      Color circleColor = (Color){180, 60, 100, 255}; //円障害物の色
      for (int i = 0; i < maxCircleObstacles; i++) {
        if (circleObstacles[i].active)
          DrawCircleV(circleObstacles[i].pos, circleObstacles[i].radius, circleColor); //円を描画
      }

      Color triangleColor = (Color){50, 200, 80, 255}; //三角形の色
      for (int i = 0; i < maxZigzagTriangles; i++) {
        if (zigzagTriangles[i].active) {
          Vector2 v1 = {zigzagTriangles[i].pos.x, zigzagTriangles[i].pos.y - zigzagTriangles[i].size}; //頂点1
          Vector2 v2 = {zigzagTriangles[i].pos.x - zigzagTriangles[i].size, zigzagTriangles[i].pos.y + zigzagTriangles[i].size}; //頂点2
          Vector2 v3 = {zigzagTriangles[i].pos.x + zigzagTriangles[i].size, zigzagTriangles[i].pos.y + zigzagTriangles[i].size}; //頂点3
          DrawTriangle(v1, v2, v3, triangleColor); //三角形を描画
        }
      }


      Color bulletColor = (Color){220, 80, 80, 255}; //弾の色
      for (int i = 0; i < maxBullets; i++) {
        if (bullets[i].active)
          DrawCircleV(bullets[i].pos, 4.0f, bulletColor); //弾を描画
      }

      //タレットの描画
      Color turretColor = (Color){60, 60, 70, 255}; //タレットの色
      for (int i = 0; i < maxTurrets; i++) {
        if (turrets[i].active) {
          DrawRectangle((int)(turrets[i].x - 15), 0, 30, 20, turretColor); //タレット本体
          DrawCircle((int)turrets[i].x, 20, 8, (Color){80, 80, 90, 255}); //タレットの砲口
        }
      }

      //タレットレーザーの描画
      for (int i = 0; i < maxTurretLasers; i++) {
        if (!turretLasers[i].active)
          continue;
        
        if (!turretLasers[i].firing) {
          //警告フェーズ - 赤い半透明の警告
          float alpha = 100 + 155 * (1.0f - turretLasers[i].warningTimer / turretWarningTime); //透明度を計算
          Color warningColor = (Color){255, 50, 50, (unsigned char)alpha}; //警告色
          DrawRectangle(
            (int)(turretLasers[i].x - turretLasers[i].width / 2.0f),
            0,
            (int)turretLasers[i].width,
            screenHeight,
            warningColor
          );
        } else {
          //発射フェーズ - 明るいレーザービーム
          float intensity = 1.0f - (turretLasers[i].progress / turretFireTime); //強度を計算
          unsigned char alpha = (unsigned char)(255 * intensity); //透明度
          Color laserColor = (Color){255, 255, 100, alpha}; //レーザー色
          DrawRectangle(
            (int)(turretLasers[i].x - turretLasers[i].width / 2.0f),
            0,
            (int)turretLasers[i].width,
            screenHeight,
            laserColor
          );
        }
      }

      //無敵状態を視覚的にフィードバックしてプレイヤーを描画
      Color playerColor = invincible ? (Color){255, 215, 0, 255} : (Color){40, 50, 80, 255}; //無敵時は金色
      
      //無敵が切れる1秒前に点滅
      bool shouldDraw = true; //プレイヤーを描画するかどうか
      if (invincible && invincibilityTimer < 1.0f) { //無敵残り時間が1秒未満
        //0.1秒ごとに点滅（1秒間に10回点滅）
        float blinkInterval = 0.1f;
        int blinkCount = (int)(invincibilityTimer / blinkInterval);
        shouldDraw = (blinkCount % 2 == 0); //偶数回目は表示、奇数回目は非表示
      }
      
      if (shouldDraw) {
        DrawRectangleRec(playerRect, playerColor); //プレイヤーを描画
      }
      
      //レーザー角度インジケーターの描画
      if (!dead) {
        float angleRad = laserAngle * DEG2RAD; //角度をラジアンに変換
        float indicatorLength = 40.0f; //インジケーターの長さ
        Vector2 indicatorEnd = {
          playerX + cosf(angleRad) * indicatorLength,
          playerY + sinf(angleRad) * indicatorLength
        };
        DrawLineEx(playerPos, indicatorEnd, 2.0f, (Color){255, 100, 100, 200}); //矢印の線
        DrawCircleV(indicatorEnd, 4.0f, (Color){255, 100, 100, 255}); //矢印の先端
      }

      for (int i = 0; i < maxParticles; i++) { //パーティクルの描画
        if (particles[i].age < 0.0f)
          continue;
        particles[i].age += dt; //パーティクルの寿命を更新
        if (particles[i].age >= particles[i].life) {
          particles[i].age = -1.0f; //寿命が尽きたら無効化
          continue;
        }
        particles[i].vel.x *= 0.96f;
        particles[i].vel.y *= 0.96f;
        particles[i].pos.x += particles[i].vel.x * dt; //位置を更新
        particles[i].pos.y += particles[i].vel.y * dt;
        float t = particles[i].age / particles[i].life; //残りの寿命割合
        unsigned char alpha = (unsigned char)(200 * (1.0f - t)); //透明度を計算
        DrawCircleV(particles[i].pos, 2.5f, (Color){255, 170, 90, alpha}); //パーティクルを描画
      }

      DrawText(TextFormat("LASER: %d", laserAmmo), 20, 20, 22, BLACK); //レーザー弾数表示
      if (invincible) { //無敵時のUI
        DrawText(TextFormat("INVINCIBLE: %.1fs", invincibilityTimer), 20, 48, 22, (Color){255, 215, 0, 255}); //無敵時間
        DrawText("MOUSE: MOVE", 20, 76, 20, BLACK); //操作説明
        DrawText("LEFT CLICK: LASER", 20, 104, 20, BLACK);
        DrawText("SCROLL: ANGLE", 20, 132, 20, BLACK);
        DrawText(TextFormat("ANGLE: %.0f°", laserAngle), 20, 160, 20, (Color){255, 100, 100, 255}); //角度表示
      } else { //通常時のUI
        DrawText("MOUSE: MOVE", 20, 48, 20, BLACK); //操作説明
        DrawText("LEFT CLICK: LASER", 20, 76, 20, BLACK);
        DrawText("SCROLL: ANGLE", 20, 104, 20, BLACK);
        DrawText(TextFormat("ANGLE: %.0f°", laserAngle), 20, 132, 20, (Color){255, 100, 100, 255}); //角度表示
      }
      //生存時間を表示
      const char *timeLabel = TextFormat("TIME: %.1fs", runTime); //時間ラベル
      int timeWidth = MeasureText(timeLabel, 22); //テキスト幅を測定
      DrawText(timeLabel, screenWidth - timeWidth - 20, 20, 22, BLACK); //右上に表示
      

      //ハイスコアを表示
      const char *highScoreLabel = TextFormat("BEST: %.1fs", highScore); //ハイスコアラベル
      int highScoreWidth = MeasureText(highScoreLabel, 22); //テキスト幅を測定
      DrawText(highScoreLabel, screenWidth - highScoreWidth - 20, 48, 22, (Color){200, 100, 0, 255}); //右上に表示

      if (dead) { //死亡時の表示
        DrawText("GAME OVER", screenWidth / 2 - 140, screenHeight / 2 - 40,
                 40, BLACK); //ゲームオーバーテキスト
        DrawText("Press R to Restart", screenWidth / 2 - 150,
                 screenHeight / 2 + 10, 22, BLACK); //リスタート指示
        if (IsKeyPressed(KEY_R)) { //Rキーでリスタート
          dead = false; //死亡フラグをリセット
          runTime = 0.0f; //プレイ時間をリセット
          // High score persists
          laserAmmo = 0; //レーザー弾数をリセット
          laserActive = false; //レーザー無効化
          laserTimer = 0.0f; //レーザータイマーをリセット
          laserProgress = 0.0f; //レーザー進行度をリセット
          laserAngle = -90.0f; //レーザー角度をリセット
          obstacleSpawnTimer = 0.0f; //障害物生成タイマーをリセット
          itemSpawnTimer = 0.0f; //アイテム生成タイマーをリセット
          circleSpawnTimer = 0.0f; //円障害物生成タイマーをリセット
          starSpawnTimer = 0.0f; //星アイテム生成タイマーをリセット
          invincible = false; //無敵をリセット
          invincibilityTimer = 0.0f; //無敵タイマーをリセット
          playerX = screenWidth * 0.5f;
          playerY = screenHeight - 80.0f; //プレイヤー位置をリセット
          PlayMusicStream(bgm);
          for (int i = 0; i < maxObstacles; i++) {
            obstacles[i].active = false; //障害物を全て削除
          }
          for (int i = 0; i < maxItems; i++) {
            items[i].active = false; //アイテムを全て削除
          }
          for (int i = 0; i < maxStarItems; i++) {
            starItems[i].active = false; //星アイテムを全て削除
          }
          for (int i = 0; i < maxCircleObstacles; i++) {
            circleObstacles[i].active = false; //円障害物を全て削除
          }
          for (int i = 0; i < maxBullets; i++) {
            bullets[i].active = false; //弾を全て削除
          }
          for (int i = 0; i < maxTurretLasers; i++) {
            turretLasers[i].active = false;
          }
          for (int i = 0; i < maxZigzagTriangles; i++) {
            zigzagTriangles[i].active = false; //三角形を全て削除
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
