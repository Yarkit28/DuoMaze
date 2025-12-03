#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX

#include "raylib.h"
#include <array>
#include <string>
#include <fstream>

// Configuración
namespace CreatorConstants {
    constexpr int MAP_WIDTH = 20;
    constexpr int MAP_HEIGHT = 15;
    constexpr int TILE_SIZE = 40;
    constexpr int SCREEN_WIDTH = 1000;
    constexpr int SCREEN_HEIGHT = 700;
    constexpr int UI_PANEL_WIDTH = 200;
    
    // Bordes automáticos siempre activos
    constexpr bool AUTO_BORDES = true;
}

// Enumeraciones (igual que en el juego)
enum TileType {
    VACIO = 0,
    PARED = 1,
    START_MASTER = 2,
    START_SLAVE = 3,
    BOTON_1 = 4,
    BOTON_2 = 5,
    BOTON_3 = 6,
    PUERTA_1 = 7,
    PUERTA_2 = 8,
    PUERTA_3 = 9,
    OBSTACULO_ROJO = 10,
    OBSTACULO_AZUL = 11,
    META = 12,
    TOTAL_TILE_TYPES = 13
};

class TextureManager {
private:
    Texture2D loadTexture(const char* fileName, int size) {
        Image image = LoadImage(fileName);
        if (image.data == nullptr) {
            Image fallback = GenImageColor(size, size, MAGENTA);
            Texture2D texture = LoadTextureFromImage(fallback);
            UnloadImage(fallback);
            return texture;
        }
        
        ImageResize(&image, size, size);
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture;
    }
    
public:
    Texture2D piso, pared, master, slave;
    Texture2D boton1, boton2, boton3;
    Texture2D puerta1, puerta2, puerta3;
    Texture2D meta;
    
    bool loadAllTextures() {
        int size = CreatorConstants::TILE_SIZE;
        
        piso = loadTexture("resources/piso.png", size);
        pared = loadTexture("resources/pared.png", size);
        master = loadTexture("resources/master.png", size);
        slave = loadTexture("resources/slave.png", size);
        boton1 = loadTexture("resources/boton1.png", size);
        boton2 = loadTexture("resources/boton2.png", size);
        boton3 = loadTexture("resources/boton3.png", size);
        puerta1 = loadTexture("resources/puerta_roja_cerrada.png", size);
        puerta2 = loadTexture("resources/puerta_azul_cerrada.png", size);
        puerta3 = loadTexture("resources/puerta_morada_cerrada.png", size);
        meta = loadTexture("resources/meta.png", size);
        
        return true;
    }
    
    void unloadAll() {
        UnloadTexture(piso);
        UnloadTexture(pared);
        UnloadTexture(master);
        UnloadTexture(slave);
        UnloadTexture(boton1);
        UnloadTexture(boton2);
        UnloadTexture(boton3);
        UnloadTexture(puerta1);
        UnloadTexture(puerta2);
        UnloadTexture(puerta3);
        UnloadTexture(meta);
    }
};

class LevelCreator {
private:
    std::array<std::array<int, CreatorConstants::MAP_WIDTH>, CreatorConstants::MAP_HEIGHT> nivel;
    TextureManager& textures;
    bool gridVisible;
    
public:
    LevelCreator(TextureManager& tm) : textures(tm), gridVisible(true) {
        initializeWithBordes();
    }
    
    void initializeWithBordes() {
        // Limpiar todo
        for (int y = 0; y < CreatorConstants::MAP_HEIGHT; y++) {
            for (int x = 0; x < CreatorConstants::MAP_WIDTH; x++) {
                nivel[y][x] = VACIO;
            }
        }
        
        // Crear bordes automáticos
        if (CreatorConstants::AUTO_BORDES) {
            createBordes();
        }
    }
    
    void createBordes() {
        // Bordes superior e inferior
        for (int x = 0; x < CreatorConstants::MAP_WIDTH; x++) {
            nivel[0][x] = PARED;
            nivel[CreatorConstants::MAP_HEIGHT - 1][x] = PARED;
        }
        
        // Bordes izquierdo y derecho
        for (int y = 0; y < CreatorConstants::MAP_HEIGHT; y++) {
            nivel[y][0] = PARED;
            nivel[y][CreatorConstants::MAP_WIDTH - 1] = PARED;
        }
    }
    
    void handleInput() {
        Vector2 mousePos = GetMousePosition();
        
        // Solo procesar clicks en el área del mapa
        if (mousePos.x < CreatorConstants::MAP_WIDTH * CreatorConstants::TILE_SIZE) {
            int tileX = static_cast<int>(mousePos.x) / CreatorConstants::TILE_SIZE;
            int tileY = static_cast<int>(mousePos.y) / CreatorConstants::TILE_SIZE;
            
            // No permitir modificar bordes si AUTO_BORDES está activado
            bool isBorderTile = CreatorConstants::AUTO_BORDES && 
                              (tileX == 0 || tileX == CreatorConstants::MAP_WIDTH - 1 ||
                               tileY == 0 || tileY == CreatorConstants::MAP_HEIGHT - 1);
            
            if (!isBorderTile && tileX >= 0 && tileX < CreatorConstants::MAP_WIDTH && 
                tileY >= 0 && tileY < CreatorConstants::MAP_HEIGHT) {
                
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    // Ciclo: 0->1->2->...->12->0
                    nivel[tileY][tileX] = (nivel[tileY][tileX] + 1) % TOTAL_TILE_TYPES;
                }
                
                // Click derecho para borrar (poner VACIO)
                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                    nivel[tileY][tileX] = VACIO;
                }
            }
        }
        
        // Controles de teclado
        if (IsKeyPressed(KEY_G)) gridVisible = !gridVisible;
        if (IsKeyPressed(KEY_C)) clearLevel();
        if (IsKeyPressed(KEY_S)) saveLevel();
    }
    
    void draw() {
        // Dibujar mapa
        for (int y = 0; y < CreatorConstants::MAP_HEIGHT; y++) {
            for (int x = 0; x < CreatorConstants::MAP_WIDTH; x++) {
                Rectangle destRect = {
                    static_cast<float>(x * CreatorConstants::TILE_SIZE),
                    static_cast<float>(y * CreatorConstants::TILE_SIZE),
                    static_cast<float>(CreatorConstants::TILE_SIZE),
                    static_cast<float>(CreatorConstants::TILE_SIZE)
                };
                
                // Dibujar piso
                DrawTexturePro(textures.piso, {0,0,(float)textures.piso.width,(float)textures.piso.height}, 
                              destRect, {0,0}, 0, WHITE);
                
                // Dibujar elemento
                drawTile(nivel[y][x], destRect);
                
                // Grid
                if (gridVisible) {
                    DrawRectangleLines((int)destRect.x, (int)destRect.y, 
                                     CreatorConstants::TILE_SIZE, CreatorConstants::TILE_SIZE, 
                                     Fade(BLACK, 0.3f));
                }
                
                // Resaltar bordes si están bloqueados
                if (CreatorConstants::AUTO_BORDES && 
                    (x == 0 || x == CreatorConstants::MAP_WIDTH - 1 ||
                     y == 0 || y == CreatorConstants::MAP_HEIGHT - 1)) {
                    DrawRectangleLines((int)destRect.x, (int)destRect.y, 
                                     CreatorConstants::TILE_SIZE, CreatorConstants::TILE_SIZE, 
                                     RED);
                }
                
                // Coordenadas
                DrawText(TextFormat("%d,%d", x, y), 
                        (int)destRect.x + 2, (int)destRect.y + 2, 8, BLACK);
            }
        }
        
        drawUI();
    }
    
    void clearLevel() {
        initializeWithBordes();
    }
    
    void saveLevel() {
        std::ofstream file("nivel_generado.txt");
        if (!file.is_open()) {
            return;
        }
        
        file << "// Nivel generado automaticamente - DuoMaze Level Creator\n";
        file << "// Copiar y pegar en LevelSystem::initializeLevelX()\n";
        file << "// Cambiar 'X' por el número de nivel correspondiente\n";
        file << "static void initializeLevelX(GameState& state) {\n";
        file << "    constexpr int nivelX[GameConstants::MAP_HEIGHT][GameConstants::MAP_WIDTH] = {\n";
        
        for (int y = 0; y < CreatorConstants::MAP_HEIGHT; y++) {
            file << "        {";
            for (int x = 0; x < CreatorConstants::MAP_WIDTH; x++) {
                file << nivel[y][x];
                if (x < CreatorConstants::MAP_WIDTH - 1) file << ", ";
            }
            file << "}";
            if (y < CreatorConstants::MAP_HEIGHT - 1) file << ",";
            file << "\n";
        }
        
        file << "    };\n";
        file << "    \n";
        file << "    loadLevelData(state, nivelX);\n";
        file << "}\n";
        
        file.close();
    }
    
private:
    void drawTile(int tileType, const Rectangle& destRect) {
        switch (tileType) {
            case PARED:
                DrawTexturePro(textures.pared, {0,0,(float)textures.pared.width,(float)textures.pared.height}, destRect, {0,0}, 0, WHITE);
                break;
            case START_MASTER:
                DrawTexturePro(textures.master, {0,0,(float)textures.master.width,(float)textures.master.height}, destRect, {0,0}, 0, WHITE);
                break;
            case START_SLAVE:
                DrawTexturePro(textures.slave, {0,0,(float)textures.slave.width,(float)textures.slave.height}, destRect, {0,0}, 0, WHITE);
                break;
            case BOTON_1:
                DrawTexturePro(textures.boton1, {0,0,(float)textures.boton1.width,(float)textures.boton1.height}, destRect, {0,0}, 0, WHITE);
                break;
            case BOTON_2:
                DrawTexturePro(textures.boton2, {0,0,(float)textures.boton2.width,(float)textures.boton2.height}, destRect, {0,0}, 0, WHITE);
                break;
            case BOTON_3:
                DrawTexturePro(textures.boton3, {0,0,(float)textures.boton3.width,(float)textures.boton3.height}, destRect, {0,0}, 0, WHITE);
                break;
            case PUERTA_1:
                DrawTexturePro(textures.puerta1, {0,0,(float)textures.puerta1.width,(float)textures.puerta1.height}, destRect, {0,0}, 0, WHITE);
                break;
            case PUERTA_2:
                DrawTexturePro(textures.puerta2, {0,0,(float)textures.puerta2.width,(float)textures.puerta2.height}, destRect, {0,0}, 0, WHITE);
                break;
            case PUERTA_3:
                DrawTexturePro(textures.puerta3, {0,0,(float)textures.puerta3.width,(float)textures.puerta3.height}, destRect, {0,0}, 0, WHITE);
                break;
            case OBSTACULO_ROJO:
                DrawRectangleRec(destRect, RED);
                DrawText("R", (int)destRect.x + 15, (int)destRect.y + 12, 20, WHITE);
                break;
            case OBSTACULO_AZUL:
                DrawRectangleRec(destRect, BLUE);
                DrawText("B", (int)destRect.x + 15, (int)destRect.y + 12, 20, WHITE);
                break;
            case META:
                DrawTexturePro(textures.meta, {0,0,(float)textures.meta.width,(float)textures.meta.height}, destRect, {0,0}, 0, WHITE);
                break;
        }
    }
    
    void drawUI() {
        int panelX = CreatorConstants::MAP_WIDTH * CreatorConstants::TILE_SIZE + 10;
        
        // Panel de información
        DrawRectangle(panelX, 0, CreatorConstants::UI_PANEL_WIDTH, CreatorConstants::SCREEN_HEIGHT, Fade(BLACK, 0.1f));
        
        DrawText("CREADOR DE NIVELES", panelX + 10, 20, 20, DARKBLUE);
        DrawText("DuoMaze - Herramienta Dev", panelX + 10, 45, 14, DARKGRAY);
        DrawText(TextFormat("Tamaño: %dx%d", CreatorConstants::MAP_WIDTH, CreatorConstants::MAP_HEIGHT), 
                panelX + 10, 65, 12, DARKGRAY);
        
        // Controles
        DrawText("CONTROLES:", panelX + 10, 90, 16, BLACK);
        DrawText("Click Izquierdo: Ciclar tile", panelX + 10, 115, 14, DARKGRAY);
        DrawText("Click Derecho: Borrar tile", panelX + 10, 135, 14, DARKGRAY);
        DrawText("G: Mostrar/ocultar grid", panelX + 10, 155, 14, DARKGRAY);
        DrawText("C: Limpiar nivel", panelX + 10, 175, 14, DARKGRAY);
        DrawText("S: Guardar nivel", panelX + 10, 195, 14, DARKGRAY);
        
        // Leyenda
        DrawText("LEYENDA:", panelX + 10, 230, 16, BLACK);
        DrawText("0: Vacio | 1: Pared", panelX + 10, 255, 12, DARKGRAY);
        DrawText("2: Master | 3: Slave", panelX + 10, 275, 12, DARKGRAY);
        DrawText("4: Boton1 | 5: Boton2", panelX + 10, 295, 12, DARKGRAY);
        DrawText("6: Boton3 | 7: Puerta1", panelX + 10, 315, 12, DARKGRAY);
        DrawText("8: Puerta2 | 9: Puerta3", panelX + 10, 335, 12, DARKGRAY);
        DrawText("10: Rojo | 11: Azul", panelX + 10, 355, 12, DARKGRAY);
        DrawText("12: Meta", panelX + 10, 375, 12, DARKGRAY);
        
        // Tile bajo el mouse
        Vector2 mousePos = GetMousePosition();
        int tileX = static_cast<int>(mousePos.x) / CreatorConstants::TILE_SIZE;
        int tileY = static_cast<int>(mousePos.y) / CreatorConstants::TILE_SIZE;
        
        if (tileX >= 0 && tileX < CreatorConstants::MAP_WIDTH && 
            tileY >= 0 && tileY < CreatorConstants::MAP_HEIGHT) {
            
            bool isBorderTile = CreatorConstants::AUTO_BORDES && 
                              (tileX == 0 || tileX == CreatorConstants::MAP_WIDTH - 1 ||
                               tileY == 0 || tileY == CreatorConstants::MAP_HEIGHT - 1);
            
            DrawText(TextFormat("Tile: [%d,%d]", tileX, tileY), panelX + 10, 410, 16, 
                    isBorderTile ? RED : DARKBLUE);
            DrawText(TextFormat("Tipo: %d", nivel[tileY][tileX]), panelX + 10, 430, 16, 
                    isBorderTile ? RED : DARKBLUE);
            
            if (isBorderTile) {
                DrawText("BORDE (Bloqueado)", panelX + 10, 450, 14, RED);
            }
        }
        
        // Estado
        DrawText(TextFormat("Grid: %s", gridVisible ? "ON" : "OFF"), panelX + 10, 490, 14, DARKGRAY);
        DrawText(TextFormat("Bordes: %s", CreatorConstants::AUTO_BORDES ? "AUTO" : "MANUAL"), 
                panelX + 10, 510, 14, DARKGRAY);
        DrawText("Listo para diseñar!", panelX + 10, 540, 16, GREEN);
    }
};

int main() {
    InitWindow(CreatorConstants::SCREEN_WIDTH, CreatorConstants::SCREEN_HEIGHT, 
               "Creador de Niveles - DuoMaze Dev Tool");
    SetTargetFPS(60);
    
    TextureManager textureManager;
    textureManager.loadAllTextures();
    
    LevelCreator creator(textureManager);
    
    while (!WindowShouldClose()) {
        creator.handleInput();
        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        creator.draw();
        
        EndDrawing();
    }
    
    textureManager.unloadAll();
    CloseWindow();
    
    return 0;
}
