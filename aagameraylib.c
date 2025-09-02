#include "raylib.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.1415926535
#endif
Rectangle exitBtn = { 500, 20, 80, 40 };  // Sağ üst köşeye minik kare bir çıkış butonu
Rectangle bestScoresBtn = { 300, 210, 160, 40 };
Rectangle levelStartBtn = { 200, 500, 200, 50 };
Rectangle levelResetBtn = { 250, 570, 100, 30 };
Vector2 collidedBallPos1 = { 0 };
Vector2 collidedBallPos2 = { 0 };
bool animatingCollision = false;
float collisionStartTime = 0.0f;
int collidedIndex1 = -1;
int collidedIndex2 = -1;
Vector2 lastThrownBallPos;
bool drawFrozenBall = false;
Vector2 thrownBallPos;
Texture2D bgLight;
Texture2D bgDark;
Texture2D nameBg;
void DrawExitButton(Font gameFont) {
    float fontSize = 22.0f;
    float cornerRadius = 0.3f;

    // Gölge efekti
    DrawRectangleRounded((Rectangle) {
        exitBtn.x + 2, exitBtn.y + 2,
            exitBtn.width, exitBtn.height
    }, cornerRadius, 8, Fade(BLACK, 0.3f));

    // Kırmızı buton
    DrawRectangleRounded(exitBtn, cornerRadius, 8, RED);

    // Yazıyı ortala
    const char* btnText = "EXIT";
    Vector2 textSize = MeasureTextEx(gameFont, btnText, fontSize, 0);
    Vector2 textPos = {
        exitBtn.x + (exitBtn.width - textSize.x) / 2,
        exitBtn.y + (exitBtn.height - textSize.y) / 2
    };

    DrawTextEx(gameFont, btnText, textPos, fontSize, 0, WHITE);
}


void CheckExitButtonClick() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), exitBtn)) {
        CloseWindow();
        exit(0);
    }
}


#define MAX_USERS 50
#define MAX_NAME_LEN 32
const char* scoresFile = "scores.txt";
char currentPlayer[MAX_NAME_LEN] = "";
bool nameEntered = false;


#define CONFETTI_COUNT 100
int GetTargetNeedles(int level) {
    if (level <= 2) return 10;      // ilk iki seviye kolay
    else if (level <= 5) return 10; // orta
    else if (level <= 8) return 10;
    else if (level <= 10) return 10;
    else return 10 + level % 5;    // sonrası zaten random olacak
}



int GetStartingNeedles(int level) {
    if (level <= 2) return 3;
    else if (level <= 5) return 4;
    else if (level <= 8) return 5;
    else if (level <= 10) return 6;
    else return 3 + (level % 4); // sonrası random zaten
}


const char* saveFile = "save.txt"; // kaldığın leveli buraya yazacağız

typedef struct Confetti {
    Vector2 position;
    Vector2 speed;
    Color color;
    float radius;
}Confetti;

Confetti confetti[CONFETTI_COUNT];
bool showConfetti = false;
bool showLevelTransition = false;
float levelTransitionStartTime = 0.0f;

typedef struct {
    float angle;
} Needle;

// #define MAX_LEVELS 4
// float bestTimes[MAX_LEVELS];
float levelStartTime;
bool showResetConfirm = false;  // Sıfırlama onay ekranı gösterilsin mi?

void SaveProgress(int level, const char* playerName) {
    FILE* file = fopen(saveFile, "w");
    if (file != NULL) {
        fprintf(file, "%s\n", playerName);
        fprintf(file, "%d\n", level);
        fclose(file);
    }
}

int LoadProgress(char* lastPlayer) {
    FILE* file = fopen(saveFile, "r");
    int savedLevel = 1;
    char savedName[MAX_NAME_LEN] = "";

    if (file != NULL) {
        fscanf(file, "%s", savedName);
        fscanf(file, "%d", &savedLevel);
        fclose(file);
    }

    // Eğer yeni kullanıcıysa, level 1'den başlat
    if (strcmp(savedName, lastPlayer) != 0) {
        savedLevel = 1;
        SaveProgress(savedLevel, lastPlayer); // Yeni kullanıcı için sıfırla
    }

    return savedLevel;
}


void AddStartingNeedles(Needle* needleAngles, int* needleCount, int count)
{
    for (int i = 0; i < count; i++)
    {
        needleAngles[*needleCount].angle = (360.0f / count) * i;
        (*needleCount)++;
    }
}

void ThrowNeedle(bool* bulletActive, float* bulletY, float screenHeight)
{
    *bulletActive = true;
    *bulletY = screenHeight / 2;
}

void ShowHowManyLeft(int targetNeedles, int needleCount)
{
    int remaining = targetNeedles - needleCount;
    printf("Kalan igne: %d\n", remaining);
}
void PromptPlayerName(Font gameFont) {
    int charIndex = 0;

    while (!nameEntered && !WindowShouldClose()) {
        BeginDrawing();
        DrawTexture(nameBg, 0, 0, WHITE);  // Arka plan olarak resmi koy
        DrawExitButton(gameFont);
        CheckExitButtonClick();
        //  ClearBackground(WHITE);
        DrawTextEx(gameFont, "aa", (Vector2) { 220, 100 }, 150, 0, WHITE);


        DrawTextEx(gameFont, "INPUT YOUR NAME AND PRESS THE ENTER BUTTON:", (Vector2) { 75, 300 }, 20, 0, WHITE);
        // Ovalleştirilmiş arka plan kutusu
        DrawRectangleRounded((Rectangle) { 100, 340, 400, 50 }, 0.3f, 16, GRAY);

        // Kullanıcı ismini çiz
        DrawTextEx(gameFont, currentPlayer, (Vector2) { 110, 350 }, 30, 0, WHITE);

        EndDrawing();

        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (charIndex < MAX_NAME_LEN - 1)) {
                currentPlayer[charIndex++] = (char)key;
                currentPlayer[charIndex] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && charIndex > 0) {
            currentPlayer[--charIndex] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER) && charIndex > 0) {
            nameEntered = true;
        }
    }
}
void UpdateScore(const char* username, int level) {
    FILE* file = fopen(scoresFile, "r");
    char names[MAX_USERS][MAX_NAME_LEN];
    int levels[MAX_USERS];
    int count = 0;
    bool updated = false;

    if (file) {
        while (fscanf(file, "%s %d", names[count], &levels[count]) == 2 && count < MAX_USERS) {
            if (strcmp(names[count], username) == 0) {
                if (level > levels[count]) levels[count] = level;
                updated = true;
            }
            count++;
        }
        fclose(file);
    }

    if (!updated && count < MAX_USERS) {
        strcpy(names[count], username);
        levels[count] = level;
        count++;
    }

    file = fopen(scoresFile, "w");
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %d\n", names[i], levels[i]);
    }
    fclose(file);
}


void ShowBestScores(Font font) {
    FILE* file = fopen(scoresFile, "r");
    char name[MAX_NAME_LEN];
    int level;

    const int headerHeight = 60;
    const int rowHeight = 40;         // Her satıra 40px
    const int maxDisplay = 15;        // MAX 15 kişi göster
    int rowCount = 0;

    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Başlık efekti
    DrawTextEx(font, "BEST SCORES", (Vector2) { 146, 46 }, 50, 0, GRAY);  // Gölge
    DrawTextEx(font, "BEST SCORES", (Vector2) { 140, 50 }, 50, 0, BLACK); // Ana yazı
    DrawLine(100, 110, 500, 110, DARKGRAY); // Alt çizgi

    // Başlık kutusu
    DrawRectangleRec((Rectangle) { 120, 120, 360, headerHeight }, DARKGRAY);
    DrawTextEx(font, "NAME", (Vector2) { 140, 135 }, 22, 0, RAYWHITE);
    DrawTextEx(font, "LEVEL", (Vector2) { 380, 135 }, 22, 0, RAYWHITE);

    // Gövde arka plan kutusu
    DrawRectangleRounded(
        (Rectangle) {
        120, 180, 360, maxDisplay* rowHeight + 20
    },
        0.2f, 12, Fade(GRAY, 0.15f)
    );

    if (file) {
        int y = 190;
        while (fscanf(file, "%s %d", name, &level) == 2 && rowCount < maxDisplay) {
            Color rowColor = (rowCount % 2 == 0) ? Fade(LIGHTGRAY, 0.5f) : Fade(GRAY, 0.4f);
            DrawRectangleRec((Rectangle) { 120, y - 5, 360, rowHeight }, rowColor);
            DrawTextEx(font, name, (Vector2) { 140, y + 5 }, 20, 0, BLACK);
            DrawTextEx(font, TextFormat("Level %d", level), (Vector2) { 380, y + 5 }, 20, 0, BLACK);
            y += rowHeight;
            rowCount++;
        }
        fclose(file);
    }
    else {
        DrawTextEx(font, "No scores found.", (Vector2) { 140, 190 }, 22, 0, RED);
    }


    // --- BACK TO MENU BUTTON ---
    Rectangle backBtn = { 200, 820, 200, 50 };

    // Gölge efekti
    DrawRectangleRounded((Rectangle) { backBtn.x + 3, backBtn.y + 3, backBtn.width, backBtn.height }, 0.5f, 10, Fade(BLACK, 0.3f));
    // Ana buton
    DrawRectangleRounded(backBtn, 0.5f, 10, RED);
    // Yazı
    DrawTextEx(font, "BACK TO MENU",
        (Vector2) {
        backBtn.x + 20, backBtn.y + 12
    }, 24, 0, RAYWHITE);



    EndDrawing();
}


int main(void)
{
    const int screenWidth = 600;
    const int screenHeight = 900;
    Image icon = LoadImage("Assets/images/aa.png");
    InitWindow(screenWidth, screenHeight, "aa");
    bgLight = LoadTexture("Assets/images/light2.jpg");  // Açık tema görseli
    bgDark = LoadTexture("Assets/images/dark.jpg");    // Koyu tema görseli
    nameBg = LoadTexture("Assets/images/dark5.jpg");


    SetWindowIcon(icon);
    InitAudioDevice(); // Ses sistemi başlatılıyor

    Music bgMusic = LoadMusicStream("Assets/sounds/AAGameMusic.mp3");  // Veya loop.wav ya da loop.mp3
    SetMusicVolume(bgMusic, 0.5f); // %50 ses
    Sound clickSound = LoadSound("Assets/sounds/click1.wav"); //menü için
    SetSoundVolume(clickSound, 1.0f);
    PlaySound(clickSound);  // preload
    StopSound(clickSound);  // buffer'ı hazırlasın
    Sound failsound = LoadSound("Assets/sounds/fail1.wav");
    PlaySound(failsound);  // preload
    StopSound(failsound);  // buffer'ı hazırlasın
    Sound victorysound = LoadSound("Assets/sounds/victory1.wav");
    PlaySound(victorysound);  // preload
    StopSound(victorysound);  // buffer'ı hazırlasın
    PlayMusicStream(bgMusic);



    Font gameFont = LoadFontEx("Assets/fonts/Archivo-Regular.ttf", 210, 0, 0); // yazı fontu için
    PromptPlayerName(gameFont); // Font ile çağır



    SetTargetFPS(50);

    // Tema ayarları
    bool isDarkMode = false;
    Color bgColor = RAYWHITE;
    Color textColor = BLACK;

    // Ses kontrolü
    //InitAudioDevice();
    bool isSoundOn = true;

    // Menü kontrolü
    bool menuOpen = false;
    Rectangle menuButton = { 250, 520, 100, 40 };
    Rectangle menuPanel = { 100, 150, 400, 300 };

    Vector2 center = { screenWidth / 2.0f, screenHeight / 2.0f - 100.0f };
    float circleRadius = 70.0f;
    float bulletLength = 150.0f;
    float ballRadius = 13.0f;

    float rotationAngle = 0.0f;
    float circleSpeedBase = 60.0f;
    float circleSpeed = circleSpeedBase;
    float direction = 1.0f;

    // int level = 1;
    int level = LoadProgress(currentPlayer);



    int targetNeedles = GetRandomValue(14, 20);  // Her level için hedef random

    Needle needleAngles[100];
    int needleCount = 0;

    bool bulletActive = false;
    float bulletX = center.x;
    float bulletY = screenHeight;
    float bulletSpeed = 740.0f;

    bool win = false;
    bool gameStarted = false;
    bool gameOver = false;

    int startNeedles = GetRandomValue(4, 6);  // Başlangıç iğneleri de random
    AddStartingNeedles(needleAngles, &needleCount, startNeedles);

    float baseY = screenHeight - 80.0f;
    float spacing = (targetNeedles > 12) ? 34.0f : 34.0f;

    while (!WindowShouldClose())
    {
        UpdateMusicStream(bgMusic);

        int remaining = targetNeedles - needleCount;
        float totalHeight = remaining * spacing;
        baseY = screenHeight - 80.0f;

        // Eğer çok yukarı çıkıyorsa, aşağı çek
        if (totalHeight > 500) {
            baseY += (totalHeight - 500);  // İstersen 400 yap, oyun ekranına göre
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), menuButton))
        {
            PlaySound(clickSound);

            menuOpen = !menuOpen;
        }

        if (menuOpen)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Vector2 mouse = GetMousePosition();

                Rectangle themeToggle = { menuPanel.x + 20, menuPanel.y + 60, 160, 40 };
                Rectangle soundToggle = { menuPanel.x + 20, menuPanel.y + 120, 160, 40 };
                Rectangle closeButton = { menuPanel.x + 20, menuPanel.y + 180, 160, 40 };

                if (CheckCollisionPointRec(mouse, themeToggle)) {
                    PlaySound(clickSound);
                    isDarkMode = !isDarkMode;
                    bgColor = isDarkMode ? BLACK : RAYWHITE;
                    textColor = isDarkMode ? RAYWHITE : BLACK;

                }
                else if (CheckCollisionPointRec(mouse, soundToggle)) {
                    PlaySound(clickSound);
                    isSoundOn = !isSoundOn;
                    if (!isSoundOn) SetMasterVolume(0.0f);
                    else SetMasterVolume(1.0f);
                }
                else if (CheckCollisionPointRec(mouse, closeButton)) {
                    PlaySound(clickSound);
                    menuOpen = false;
                }
                else if (CheckCollisionPointRec(mouse, bestScoresBtn)) {
                    bool viewingScores = true;
                    while (!WindowShouldClose() && viewingScores) {

                        ShowBestScores(gameFont);  // tabloyu çiziyor zaten içinde Begin/End var

                        // Butona tıklama kontrolü
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            Vector2 mouse = GetMousePosition();
                            Rectangle backBtn = { 200, 820, 200, 50 };
                            if (CheckCollisionPointRec(mouse, backBtn)) {
                                PlaySound(clickSound); // istersen efekti de çal
                                viewingScores = false; // döngüden çık
                            }
                        }


                    }

                }
            }

        }
        if (showLevelTransition)
        {

            float elapsed = GetTime() - levelTransitionStartTime;
            // ClearBackground(bgColor);
            DrawTexture(isDarkMode ? bgDark : bgLight, 0, 0, WHITE);
            DrawExitButton(gameFont);
            CheckExitButtonClick();


            DrawTextEx(gameFont, TextFormat("LEVEL % d", level - 1),
                (Vector2) {
                screenWidth / 2 - 180, screenHeight / 2 - 240
            }, 90, 0, DARKGREEN);
            DrawTextEx(gameFont, TextFormat("PASSED!"),
                (Vector2) {
                screenWidth / 2 - 120, screenHeight / 2 - 140
            }, 60, 0, DARKGREEN);



            // START düğmesi
            Rectangle startBtn = (Rectangle){ screenWidth / 2 - 100, screenHeight / 2 - 40, 200, 80 };

            // Gölge
            DrawRectangleRounded((Rectangle) { startBtn.x + 4, startBtn.y + 4, startBtn.width, startBtn.height }, 0.5f, 10, Fade(BLACK, 0.3f));
            // Buton
            DrawRectangleRounded(startBtn, 0.5f, 10, DARKGRAY);
            // Yazı
            DrawTextEx(gameFont, "CONTINUE",
                (Vector2) {
                startBtn.x + 30, startBtn.y + 25
            }, 30, 0, RAYWHITE);

            Rectangle levelMenuBtn = { 200, 570, 200, 50 };

            // Gölge
            DrawRectangleRounded((Rectangle) { levelMenuBtn.x + 3, levelMenuBtn.y + 3, levelMenuBtn.width, levelMenuBtn.height }, 0.5f, 10, Fade(BLACK, 0.3f));
            // Butonun kendisi
            DrawRectangleRounded(levelMenuBtn, 0.5f, 10, GRAY);
            // Yazı
            DrawTextEx(gameFont, "MENU", (Vector2) { levelMenuBtn.x + 60, levelMenuBtn.y + 10 }, 30, 0, WHITE);



            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, levelStartBtn)) {
                    showLevelTransition = false;
                    showConfetti = false;
                    menuOpen = false;  // Menüyü kapat
                }
                if (CheckCollisionPointRec(mouse, levelMenuBtn)) {
                    showLevelTransition = false;
                    showConfetti = false;
                    menuOpen = true;
                }
            }

            for (int i = 0; i < CONFETTI_COUNT; i++)
            {
                confetti[i].position.y += confetti[i].speed.y;
                if (confetti[i].position.y > screenHeight) {
                    confetti[i].position.y = GetRandomValue(-20, -10);
                    confetti[i].position.x = GetRandomValue(0, screenWidth);
                }
                DrawCircleV(confetti[i].position, confetti[i].radius, confetti[i].color);
            }

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                showLevelTransition = false;
                showConfetti = false;
            }

            EndDrawing();
            continue;
        }

        BeginDrawing();
        // ClearBackground(bgColor);
        DrawTexture(isDarkMode ? bgDark : bgLight, 0, 0, WHITE);  // BUNU EKLE



        if (animatingCollision)
        {
            float animTime = GetTime() - collisionStartTime; // levelStartTime değil, çarpışma zamanı

            ClearBackground(RED);
            DrawExitButton(gameFont);
            CheckExitButtonClick();

            Vector2 zoomTarget = {
                (collidedBallPos1.x + collidedBallPos2.x) / 2.0f,
                (collidedBallPos1.y + collidedBallPos2.y) / 2.0f
            };

            float zoomScale = (animTime < 1.0f) ? 1.0f + animTime * 2.0f : 3.0f;

            BeginMode2D((Camera2D) {
                .offset = (Vector2){ screenWidth / 2, screenHeight / 2 },
                    .target = zoomTarget,
                    .rotation = 0.0f,
                    .zoom = zoomScale
            });
            if (!isDarkMode) {
                if (drawFrozenBall)
                {
                    DrawCircleV(lastThrownBallPos, ballRadius, BLACK); // renk neyse ona göre
                }
                // çember ve iğneler
                DrawCircleV(center, circleRadius, BLACK);
                for (int i = 0; i < needleCount; i++)
                {
                    float angleWithRotation = needleAngles[i].angle + rotationAngle;
                    float rad = (angleWithRotation - 90.0f) * (M_PI / 180.0f);
                    float startX = center.x + cosf(rad) * (circleRadius - ballRadius);
                    float startY = center.y + sinf(rad) * (circleRadius - ballRadius);
                    float endX = center.x + cosf(rad) * (circleRadius + bulletLength);
                    float endY = center.y + sinf(rad) * (circleRadius + bulletLength);
                    DrawLineEx((Vector2) { startX, startY }, (Vector2) { endX, endY }, 4.0f, BLACK);

                    DrawCircle(endX, endY, ballRadius, RED);

                }


            }
            else {
                if (drawFrozenBall)
                {
                    DrawCircleV(lastThrownBallPos, ballRadius, WHITE); // renk neyse ona göre
                }
                // çember ve iğneler
                DrawCircleV(center, circleRadius, WHITE);
                for (int i = 0; i < needleCount; i++)
                {
                    float angleWithRotation = needleAngles[i].angle + rotationAngle;
                    float rad = (angleWithRotation - 90.0f) * (M_PI / 180.0f);
                    float startX = center.x + cosf(rad) * (circleRadius - ballRadius);
                    float startY = center.y + sinf(rad) * (circleRadius - ballRadius);
                    float endX = center.x + cosf(rad) * (circleRadius + bulletLength);
                    float endY = center.y + sinf(rad) * (circleRadius + bulletLength);
                    DrawLineEx((Vector2) { startX, startY }, (Vector2) { endX, endY }, 4.0f, WHITE);

                    DrawCircle(endX, endY, ballRadius, WHITE);

                }

            }


            EndMode2D();


            if (animTime > 2.0f)
            {
                animatingCollision = false;
                // mesela: topları sıfırla ya da yazı göster
            }

            EndDrawing();
            continue; // diğer çizim kodlarını atla
        }



        // Menü butonu
        if (!gameStarted && !menuOpen) {
            DrawRectangleRounded((Rectangle) { menuButton.x + 3, menuButton.y + 3, menuButton.width, menuButton.height }, 0.5f, 10, Fade(BLACK, 0.3f));
            DrawRectangleRounded(menuButton, 0.5f, 10, GRAY);
            DrawTextEx(gameFont, "MENU", (Vector2) { menuButton.x + 15, menuButton.y + 10 }, 25, 0, textColor);
        }

        // Menü paneli açık ise çiz
        if (menuOpen)
        {
            Vector2 mouse = GetMousePosition();
            BeginDrawing();
            DrawExitButton(gameFont);
            CheckExitButtonClick();

            // Gölge efekti
            DrawRectangleRounded((Rectangle) { menuPanel.x + 4, menuPanel.y + 4, menuPanel.width, menuPanel.height }, 0.15f, 20, Fade(BLACK, 0.3f));

            // Oval panelin kendisi
            DrawRectangleRounded(menuPanel, 0.15f, 20, BLUE);

            // Gölge
            DrawRectangleRounded((Rectangle) { bestScoresBtn.x + 3, bestScoresBtn.y + 3, bestScoresBtn.width, bestScoresBtn.height }, 0.5f, 10, Fade(BLACK, 0.3f));
            // Oval buton
            DrawRectangleRounded(bestScoresBtn, 0.5f, 10, VIOLET);
            // Yazı
            DrawTextEx(gameFont, "BEST SCORES", (Vector2) { bestScoresBtn.x + 10, bestScoresBtn.y + 10 }, 20, 0, WHITE);



            DrawTextEx(gameFont, "MENU",
                (Vector2) {
                menuPanel.x + menuPanel.width / 2 - MeasureTextEx(gameFont, "MENU", 30, 0).x / 2, menuPanel.y + 20
            },
                30, 0, RAYWHITE);


            DrawRectangleRounded((Rectangle) { menuPanel.x + 23, menuPanel.y + 63, 160, 40 }, 0.5f, 10, Fade(BLACK, 0.3f));
            DrawRectangleRounded((Rectangle) { menuPanel.x + 20, menuPanel.y + 60, 160, 40 }, 0.5f, 10, isDarkMode ? BLACK : WHITE);
            if (!isDarkMode) {
                DrawTextEx(gameFont, "Colour Theme",
                    (Vector2) {
                    menuPanel.x + 30, menuPanel.y + 70
                },
                    20, 0, BLACK);


            }
            else {
                DrawTextEx(gameFont, "Colour Theme",
                    (Vector2) {
                    menuPanel.x + 30, menuPanel.y + 70
                },
                    20, 0, WHITE);

            }



            Rectangle resetBtn = { menuPanel.x + 20, menuPanel.y + 240, 160, 40 };
            DrawRectangleRounded((Rectangle) { resetBtn.x + 3, resetBtn.y + 3, resetBtn.width, resetBtn.height }, 0.5f, 10, Fade(BLACK, 0.3f));
            DrawRectangleRounded(resetBtn, 0.5f, 10, RED);
            DrawTextEx(gameFont, "RESET", (Vector2) { resetBtn.x + 30, resetBtn.y + 10 }, 20, 0, WHITE);


            DrawRectangleRounded((Rectangle) { menuPanel.x + 23, menuPanel.y + 123, 160, 40 }, 0.5f, 10, Fade(BLACK, 0.3f));
            DrawRectangleRounded((Rectangle) { menuPanel.x + 20, menuPanel.y + 120, 160, 40 }, 0.5f, 10, isSoundOn ? GREEN : RED);
            DrawTextEx(gameFont, isSoundOn ? "Sound: ON" : "Sound: OFF",
                (Vector2) {
                menuPanel.x + 30, menuPanel.y + 130
            },
                20, 0, BLACK);


            DrawRectangleRounded((Rectangle) { menuPanel.x + 23, menuPanel.y + 183, 160, 40 }, 0.5f, 10, Fade(BLACK, 0.3f));
            DrawRectangleRounded((Rectangle) { menuPanel.x + 20, menuPanel.y + 180, 160, 40 }, 0.5f, 10, BLACK);
            DrawTextEx(gameFont, "Close Menu",
                (Vector2) {
                menuPanel.x + 30, menuPanel.y + 190
            },
                20, 0, WHITE);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mouse, resetBtn)) {
                    // Sıfırlama işlemi
                    PlaySound(clickSound);

                    level = 1;
                    SaveProgress(level, currentPlayer);
                    UpdateScore(currentPlayer, level);
                    targetNeedles = GetTargetNeedles(level);
                    needleCount = 0;
                    AddStartingNeedles(needleAngles, &needleCount, GetStartingNeedles(level));
                    rotationAngle = 0.0f;
                    circleSpeed = circleSpeedBase;
                    baseY = screenHeight - 100.0f;
                    gameStarted = false;
                    gameOver = false;
                    win = false;
                    showConfetti = false;
                    showLevelTransition = false;
                    menuOpen = false; // Menüden çık
                }
            }

            EndDrawing();
            continue;
        }

        if ((!gameStarted || gameOver) && !win)
        {
            Rectangle startBtn = { screenWidth / 2 - 100, screenHeight / 2 - 40, 200, 80 };
            DrawRectangleRounded((Rectangle) { startBtn.x + 3, startBtn.y + 3, startBtn.width, startBtn.height }, 0.5f, 10, Fade(BLACK, 0.3f));
            DrawRectangleRounded(startBtn, 0.5f, 10, DARKGRAY);
            DrawTextEx(gameFont, "START",
                (Vector2) {
                screenWidth / 2 - MeasureTextEx(gameFont, "START", 30, 0).x / 2, screenHeight / 2 - 15
            },
                30, 0, RAYWHITE);
            DrawExitButton(gameFont);
            CheckExitButtonClick();

            if (gameOver) {
                const char* title = "FAIL!";
                const char* title1 = "RETRY LEVEL";
                DrawTextEx(gameFont, "FAIL!",
                    (Vector2) {
                    screenWidth / 2 - MeasureTextEx(gameFont, "FAIL!", 130, 0).x / 2, screenHeight / 2 - 270
                },
                    130, 0, RED);
                DrawTextEx(gameFont, "RETRY LEVEL",
                    (Vector2) {
                    screenWidth / 2 - MeasureTextEx(gameFont, "RETRY LEVEL", 40, 0).x / 2, screenHeight / 2 - 140
                },
                    40, 0, RED);

            }
            else {
                const char* title = "aa";
                if (!isDarkMode) {
                    DrawTextEx(gameFont, "aa",
                        (Vector2) {
                        screenWidth / 2.22 - MeasureTextEx(gameFont, "aa", 200, 0).x / 2, screenHeight / 2 - 270
                    },
                        250, 0, BLACK);

                }
                else {
                    DrawTextEx(gameFont, "aa",
                        (Vector2) {
                        screenWidth / 2.22 - MeasureTextEx(gameFont, "aa", 200, 0).x / 2, screenHeight / 2 - 270
                    },
                        250, 0, WHITE);
                }


            }


            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                CheckCollisionPointRec(GetMousePosition(), (Rectangle) { screenWidth / 2 - 100, screenHeight / 2 - 40, 200, 80 }))
            {
                PlaySound(clickSound);
                gameStarted = true;
                PlayMusicStream(bgMusic); // çarpışmadan sonra müziği tekrar başlat
                gameOver = false;
                win = false;
                needleCount = 0;
                bulletActive = false;
                bulletY = screenHeight;
                rotationAngle = 0.0f;

                int startNeedles;
                if (level <= 10) {
                    targetNeedles = GetTargetNeedles(level);
                    startNeedles = GetStartingNeedles(level);
                    circleSpeed = circleSpeedBase + 20.0f * logf(level + 1);
                    direction = 1.0f;
                }
                else {
                    targetNeedles = GetRandomValue(14, 20);
                    startNeedles = GetRandomValue(4, 6);
                    circleSpeed = GetRandomValue(70, 100);
                    direction = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
                }

                AddStartingNeedles(needleAngles, &needleCount, startNeedles);

                baseY = screenHeight - 100.0f;
            }




            EndDrawing();
            continue;
        }


        rotationAngle += direction * circleSpeed * GetFrameTime();
        if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
        if (rotationAngle < 0.0f) rotationAngle += 360.0f;

        if (!bulletActive && IsKeyPressed(KEY_SPACE))
        {
            ThrowNeedle(&bulletActive, &bulletY, screenHeight);
            baseY -= spacing;
        }

        if (bulletActive)
        {
            bulletY -= bulletSpeed * GetFrameTime();
            if (bulletY <= center.y + circleRadius)
            {
                bulletActive = false;
                float attachAngle = 180.0f - rotationAngle;
                if (attachAngle < 0.0f) attachAngle += 360.0f;

                bool collision = false;
                for (int i = 0; i < needleCount; i++)
                {
                    float diff = fabsf(attachAngle - needleAngles[i].angle);
                    if (diff > 180.0f) diff = 360.0f - diff;
                    if (diff < 10.0f)
                    {
                        collision = true;
                        collidedIndex1 = i; // Çarpışan ilk iğnenin indeksini sakla
                        // Eğer ikinci bir iğneye ihtiyacınız varsa, en yakın iki tanesini bulmak için daha karmaşık bir mantık gerekir.
                        // Bu basit oyun için, sadece bir çarpışan iğnenin bilinmesi genellikle zoom'u ortalamak için yeterlidir.
                        // Şimdilik, collidedBallPos2'nin merminin son konumuna ayarlanacağını varsayalım.
                        break;
                    }
                }

                if (collision)
                {
                    StopMusicStream(bgMusic);  // MÜZİĞİ DURDUR
                    gameOver = true;
                    PlaySound(failsound);

                    gameStarted = false;
                    circleSpeed = 0;
                    animatingCollision = true;
                    collisionStartTime = GetTime(); // Animasyon için başlangıç zamanını ayarla

                    // çarpışan topların pozisyonunu al
                    // Yeni atılan merminin konumu (tam olarak yapışmadan hemen önceki)
                    collidedBallPos1 = (Vector2){ bulletX, bulletY };
                    lastThrownBallPos = thrownBallPos;
                    drawFrozenBall = true;
                    // Çarpılan mevcut iğnenin konumu
                    float hitNeedleAngle = needleAngles[collidedIndex1].angle + rotationAngle;
                    float hitNeedleRad = (hitNeedleAngle - 90.0f) * (M_PI / 180.0f);
                    collidedBallPos2 = (Vector2){
                        center.x + cosf(hitNeedleRad) * (circleRadius + bulletLength),
                        center.y + sinf(hitNeedleRad) * (circleRadius + bulletLength)
                    };


                }
                else
                {

                    needleAngles[needleCount].angle = attachAngle;
                    needleCount++;
                    ShowHowManyLeft(targetNeedles, needleCount);

                    if (needleCount >= targetNeedles)
                    {
                        PlaySound(victorysound);

                        // float elapsed = GetTime() - levelStartTime;

                        level++;
                        SaveProgress(level, currentPlayer);
                        UpdateScore(currentPlayer, level);

                        int startNeedles;
                        if (level <= 10) {
                            targetNeedles = GetTargetNeedles(level);
                            startNeedles = GetStartingNeedles(level);
                            circleSpeed = GetRandomValue(40, 100);
                            direction = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
                        }
                        else {

                            targetNeedles = GetRandomValue(14, 20);
                            startNeedles = GetRandomValue(4, 6);

                            circleSpeed = GetRandomValue(70, 150);
                            direction = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
                        }
                        needleCount = 0; // DİKKAT: her yeni levelde sıfırla

                        AddStartingNeedles(needleAngles, &needleCount, startNeedles);

                        rotationAngle = 0.0f;
                        //  levelStartTime = GetTime();
                        baseY = screenHeight - 100.0f;

                        showLevelTransition = true;
                        levelTransitionStartTime = GetTime();
                        showConfetti = true;

                        for (int i = 0; i < CONFETTI_COUNT; i++) {
                            confetti[i].position = (Vector2){ GetRandomValue(0, screenWidth), GetRandomValue(-600, 0) };
                            confetti[i].speed = (Vector2){ 0, GetRandomValue(100, 300) / 100.0f };
                            confetti[i].color = (Color){ GetRandomValue(100,255), GetRandomValue(100,255), GetRandomValue(100,255), 255 };
                            confetti[i].radius = GetRandomValue(2, 5);
                        }
                    }


                }
            }
        }

        DrawCircleV(center, circleRadius, textColor);



        char levelText[8];
        sprintf(levelText, "%02d", level);  // Direkt level yaz
        DrawTextEx(gameFont, levelText,
            (Vector2) {
            center.x - MeasureTextEx(gameFont, levelText, 40, 0).x / 2, center.y - 20
        },
            40, 0, bgColor);

        for (int i = 0; i < needleCount; i++)
        {
            float angleWithRotation = needleAngles[i].angle + rotationAngle;
            if (angleWithRotation >= 360.0f) angleWithRotation -= 360.0f;

            float rad = (angleWithRotation - 90.0f) * (M_PI / 180.0f);
            float startX = center.x + cosf(rad) * (circleRadius - ballRadius);
            float startY = center.y + sinf(rad) * (circleRadius - ballRadius);
            float endX = center.x + cosf(rad) * (circleRadius + bulletLength);
            float endY = center.y + sinf(rad) * (circleRadius + bulletLength);
            DrawLineEx((Vector2) { startX, startY }, (Vector2) { endX, endY }, 4.0f, textColor);
            DrawCircle(endX, endY, ballRadius, textColor);
        }

        for (int i = 0; i < targetNeedles - needleCount; i++)
        {
            if (targetNeedles - startNeedles > 8) {
                float y = baseY - (i + needleCount) * spacing + 350.0f;  // <<< BU ÖNEMLİ DEĞİŞİKLİK
                DrawCircle(center.x, y, ballRadius, textColor);
                char num[4];
                sprintf(num, "%d", i + 1);
                DrawTextEx(gameFont, num,
                    (Vector2) {
                    center.x - MeasureTextEx(gameFont, num, 10, 0).x / 2, y - 5
                },
                    10, 0, bgColor);
            }
            else {
                float y = baseY - (i + needleCount) * spacing + 120.0f;  // <<< BU ÖNEMLİ DEĞİŞİKLİK
                DrawCircle(center.x, y, ballRadius, textColor);
                char num[4];
                sprintf(num, "%d", i + 1);
                DrawTextEx(gameFont, num,
                    (Vector2) {
                    center.x - MeasureTextEx(gameFont, num, 10, 0).x / 2, y - 5
                },
                    10, 0, bgColor);
            }

        }
        DrawExitButton(gameFont);
        CheckExitButtonClick();
        EndDrawing();
    }
    UnloadTexture(bgLight);
    UnloadTexture(bgDark);
    UnloadTexture(nameBg);
    UnloadMusicStream(bgMusic);
    CloseAudioDevice();
    UnloadSound(clickSound);
    UnloadSound(failsound);
    UnloadSound(victorysound);
    UnloadFont(gameFont); // yazı fontunu kaldırır.
    UnloadImage(icon);
    CloseWindow();
    return 0;
}