#include "raylib.h"
#include "raymath.h"
#include <vector>

// Platform yapısı (Yeşil çizgilerden oluşan dikdörtgen bloklar)
struct Platform {
    Rectangle rect;
    float restitution; // Platformun zıplatma/sekme katsayısı (0.0 = sekmez, 1.0 = tam seker)
};

// Top/Karakter Yapısı
struct Player {
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool isGrounded;
};

// Fizik Sabitleri
const float GRAVITY = 1200.0f;       // Yerçekimi ivmesi
const float ACCELERATION = 1800.0f;  // Tuşa basıldığında karakterin ivmelenmesi
const float FRICTION = 8.0f;         // Yatay sürtünme (durmayı sağlar)
const float JUMP_FORCE = 550.0f;     // Zıplama gücü
const float RESTITUTION = 0.4f;      // Karakterin varsayılan sekme/esneklik katsayısı
const float MAX_SPEED_X = 500.0f;    // Maksimum yatay hız

void ResetPlayer(Player &player, Vector2 startPos) {
    player.position = startPos;
    player.velocity = (Vector2){ 0.0f, 0.0f };
    player.isGrounded = false;
}

int main() {
    // Ekran Boyutları
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "2D Neon Vector Platformer - Raylib C++");
    SetTargetFPS(60);

    Vector2 startPosition = { 100.0f, 300.0f };
    
    Player player;
    player.radius = 20.0f;
    ResetPlayer(player, startPosition);

    // Geniş Harita Tasarımı (Yeşil çizgilerden oluşan platformlar)
    std::vector<Platform> platforms = {
        // Başlangıç zemin platformu
        { { -200, 500, 800, 20 }, 0.2f },
        
        // Basamak platformları (Çıkış)
        { { 700, 420, 200, 20 }, 0.2f },
        { { 1000, 340, 200, 20 }, 0.2f },
        { { 1300, 260, 250, 20 }, 0.2f },
        
        // Yüksek Sekme (Trambolin) Platformu
        { { 1650, 550, 150, 20 }, 0.85f },
        
        // Yukarı seviye platformları
        { { 1900, 180, 400, 20 }, 0.2f },
        { { 2400, 280, 300, 20 }, 0.2f },
        
        // Engeller ve Duvarlar
        { { 2800, 100, 20, 400 }, 0.6f },  // Dikey duvar
        { { 2900, 480, 600, 20 }, 0.2f },  // Uzun zemin
        { { 3600, 380, 300, 20 }, 0.3f },
        { { 4000, 280, 500, 20 }, 0.1f }   // Bitiş bölgesi platformu
    };

    // 2D Kamera Kurulumu
    Camera2D camera = { 0 };
    camera.target = player.position;
    camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // Ana Oyun Döngüsü
    while (!WindowShouldClose()) {
        float dt = GetFrameTime(); // Kare süresi (Delta Time)

        // --- 1. GİRDİLERİ AL VE İVMELENME EKLENMESİ ---
        float moveDir = 0.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) moveDir += 1.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) moveDir -= 1.0f;

        // Yatay Hareket
        player.velocity.x += moveDir * ACCELERATION * dt;

        // Yatay Sürtünme (Friction / Damping)
        if (moveDir == 0.0f) {
            player.velocity.x -= player.velocity.x * FRICTION * dt;
        }

        // Maksimum Hız Sınırı
        if (player.velocity.x > MAX_SPEED_X) player.velocity.x = MAX_SPEED_X;
        if (player.velocity.x < -MAX_SPEED_X) player.velocity.x = -MAX_SPEED_X;

        // Yerçekimi Uygulama
        player.velocity.y += GRAVITY * dt;

        // Zıplama Kontrolü
        if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && player.isGrounded) {
            player.velocity.y = -JUMP_FORCE;
            player.isGrounded = false;
        }

        // 'R' Tuşuna Basılırsa Yeniden Başlat
        if (IsKeyPressed(KEY_R)) {
            ResetPlayer(player, startPosition);
        }

        // --- 2. FİZİK VE ÇARPIŞMA SİMÜLASYONU (Düşük Seviye AABB vs Circle) ---
        player.isGrounded = false;

        // Pozisyon Güncelleme (Yatay)
        player.position.x += player.velocity.x * dt;

        // Yatay Çarpmalar
        for (const auto& plat : platforms) {
            // Daire - Dikdörtgen En Yakın Nokta Tespiti
            float closestX = Clamp(player.position.x, plat.rect.x, plat.rect.x + plat.rect.width);
            float closestY = Clamp(player.position.y, plat.rect.y, plat.rect.y + plat.rect.height);

            float distX = player.position.x - closestX;
            float distY = player.position.y - closestY;
            float distance = sqrtf(distX * distX + distY * distY);

            if (distance < player.radius && distance > 0.0f) {
                // Çakışma derinliğini bul ve geri it
                float overlap = player.radius - distance;
                Vector2 normal = { distX / distance, distY / distance };

                // Sadece belirgin yatay çarpmalarda teğet tepki ver
                if (fabsf(normal.x) > fabsf(normal.y)) {
                    player.position.x += normal.x * overlap;
                    // Sekme fiziği (Yatay)
                    player.velocity.x = -player.velocity.x * (RESTITUTION * plat.restitution);
                }
            }
        }

        // Pozisyon Güncelleme (Dikey)
        player.position.y += player.velocity.y * dt;

        // Dikey Çarpmalar
        for (const auto& plat : platforms) {
            float closestX = Clamp(player.position.x, plat.rect.x, plat.rect.x + plat.rect.width);
            float closestY = Clamp(player.position.y, plat.rect.y, plat.rect.y + plat.rect.height);

            float distX = player.position.x - closestX;
            float distY = player.position.y - closestY;
            float distance = sqrtf(distX * distX + distY * distY);

            if (distance < player.radius && distance > 0.0f) {
                float overlap = player.radius - distance;
                Vector2 normal = { distX / distance, distY / distance };

                if (fabsf(normal.y) >= fabsf(normal.x)) {
                    player.position.y += normal.y * overlap;

                    // Yukarıdan düşüp platformun üstüne çarptıysa
                    if (normal.y < 0.0f) {
                        // Eğer dikey hız yüksekse sektir
                        if (fabsf(player.velocity.y) > 120.0f) {
                            player.velocity.y = -player.velocity.y * plat.restitution;
                        } else {
                            // Hız azaldıysa zeminde durdur
                            player.velocity.y = 0.0f;
                            player.isGrounded = true;
                        }
                    } 
                    // Aşağıdan platformun altına kafasını çarptıysa
                    else if (normal.y > 0.0f) {
                        player.velocity.y = -player.velocity.y * RESTITUTION;
                    }
                }
            }
        }

        // Düşme Kontrolü (Ekrandan/Haritadan aşağı düşerse başlangıca dön)
        if (player.position.y > 1000.0f) {
            ResetPlayer(player, startPosition);
        }

        // --- 3. KAMERA TAKİBİ ---
        // Pürüzsüz takip için Lerp kullanılır
        camera.target.x += (player.position.x - camera.target.x) * 5.0f * dt;
        camera.target.y += (player.position.y - camera.target.y) * 5.0f * dt;

        // --- 4. ÇİZİM (RENDER) ---
        BeginDrawing();
            ClearBackground(BLACK); // Retro hissi için siyah arka plan

            BeginMode2D(camera);

                // Grid (Izgara) Çizimi - Derinlik hissi katmak için
                for (int x = -1000; x < 5000; x += 100) {
                    DrawLine(x, -1000, x, 2000, Fade(DARKGREEN, 0.15f));
                }
                for (int y = -1000; y < 2000; y += 100) {
                    DrawLine(-1000, y, 5000, y, Fade(DARKGREEN, 0.15f));
                }

                // Platformların Çizimi (Yeşil Çizgilerden Oluşan Vektörel Stil)
                for (const auto& plat : platforms) {
                    // İçini hafif şeffaf yeşil, kenarlarını parlak yeşil çiz
                    DrawRectangleRec(plat.rect, Fade(GREEN, 0.08f));
                    DrawRectangleLinesEx(plat.rect, 2.0f, LIME);
                }

                // Karakter Çizimi (Yeşil Parlayan Çember)
                DrawCircleV(player.position, player.radius, Fade(GREEN, 0.3f)); // İç dolgu
                DrawCircleLines(player.position.x, player.position.y, player.radius, LIME); // Dış vektör çizgisi
                DrawCircleLines(player.position.x, player.position.y, player.radius - 3.0f, GREEN);

            EndMode2D();

            // Ekran Bilgi Arayüzü (HUD)
            DrawText("KONTROLLER: [A/D] veya [Yön Tuşları] Hareket | [SPACE/W] Zıpla | [R] Sıfırla", 20, 20, 18, LIME);
            DrawText(TextFormat("Hız X: %.1f | Hız Y: %.1f", player.velocity.x, player.velocity.y), 20, 50, 16, GREEN);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
