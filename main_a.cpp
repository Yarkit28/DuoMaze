#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX

#include "raylib.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <array>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <cmath>

// Configuración multiplataforma
#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

// Constantes del juego
namespace GameConstants {
    constexpr int MAP_WIDTH = 20;
    constexpr int MAP_HEIGHT = 15;
    constexpr int TILE_SIZE = 40;
    constexpr int SCREEN_WIDTH = 800;
    constexpr int SCREEN_HEIGHT = 600;
    constexpr int PLAYER_RADIUS = 15;
    constexpr int PLAYER_SPEED = 3;
    constexpr int FPS_TARGET = 60;
    
    // Tiempos de actualización de hilos (en ms)
    constexpr int PHYSICS_UPDATE_RATE = 10;
    constexpr int VALIDATION_UPDATE_RATE = 15;
    constexpr int AUDIO_UPDATE_RATE = 10;
    
    // NUEVO: Sistema de niveles
    constexpr int TOTAL_LEVELS = 4;
}

// Enumeraciones
enum GameScreen { MENU = 0, GAMEPLAY = 1 };
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
    META = 12
};

// Sistema de logging optimizado
class Logger {
private:
    std::ofstream logfile;
    std::mutex logMutex;
    
public:
    Logger() {
        logfile.open("debug_log.txt", std::ios::app);
    }
    
    ~Logger() {
        if (logfile.is_open()) {
            logfile.close();
        }
    }
    
    void write(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logfile.is_open()) {
            logfile << message << std::endl;
        }
    }
};

static Logger logger;

// Sistema de audio optimizado CON HILO DEDICADO
// SISTEMA DE AUDIO MEJORADO CON MÚSICAS DIFERENTES POR PANTALLA
class AudioSystem {
private:
    Music menuMusic;
    Music gameplayMusic;
    Sound sfxOpenDoor;
    Sound sfxLevelComplete;
    Sound sfxClick;
    std::atomic<bool> audioRunning{true};
    std::atomic<bool> musicPaused{false};
    std::atomic<float> volume{0.7f};
    std::atomic<bool> isMenuMusic{true};  // true = menú, false = gameplay
    std::thread musicThread;
    
    void playSFX(Sound sound, float volumeMultiplier = 1.0f) {
        if (sound.frameCount > 0) {
            SetSoundVolume(sound, volume.load() * volumeMultiplier);
            PlaySound(sound);
        }
    }
    
public:
    
    
    bool cargarMusicas() {
        if (!IsAudioDeviceReady()) {
            InitAudioDevice();
        }
        
        // Cargar música del menú (NUEVA)
        menuMusic = LoadMusicStream("resources/sound/music/Maze_Quest_Echoes.ogg");
        if (menuMusic.frameCount == 0) {
            logger.write("❌ Error: No se pudo cargar Maze_Quest_Echoes.ogg (música de menú)");
            return false;
        }
        
        // Cargar música de gameplay (ORIGINAL)
        gameplayMusic = LoadMusicStream("resources/sound/music/Maze_Quest.ogg");
        if (gameplayMusic.frameCount == 0) {
            logger.write("❌ Error: No se pudo cargar Maze_Quest.ogg (música de gameplay)");
            UnloadMusicStream(menuMusic);
            return false;
        }
        
        //Cargar efectos de sonido
        sfxOpenDoor = LoadSound("resources/sound/sfx/abrir_puerta.wav");
        if (sfxOpenDoor.frameCount == 0) {
            logger.write("⚠️  Advertencia: No se pudo cargar abrir_puerta.wav");
        } else {
            logger.write("✅ SFX cargado: abrir_puerta.wav");
        }
        
        sfxLevelComplete = LoadSound("resources/sound/sfx/zelda_headlift.wav");
        if (sfxLevelComplete.frameCount == 0) {
            logger.write("⚠️  Advertencia: No se pudo cargar zelda_headlift.wav");
        } else {
            logger.write("✅ SFX cargado: zelda_headlift.wav");
        }
        
        sfxClick = LoadSound("resources/sound/sfx/clic.wav");
        if (sfxClick.frameCount == 0) {
            logger.write("⚠️  Advertencia: No se pudo cargar clic.wav");
        } else {
            logger.write("✅ SFX cargado: clic.wav");
        }
        
        logger.write("✅ Audio cargado correctamente");
        
        logger.write("✅ Ambas músicas cargadas correctamente");
        logger.write("   - Menú: Maze_Quest_Echoes.ogg");
        logger.write("   - Gameplay: Maze_Quest.ogg");
        
        // Iniciar hilo de música
        audioRunning = true;
        musicThread = std::thread(&AudioSystem::musicThreadFunction, this);
        
        return true;
    }
    
    // NUEVO: Métodos públicos para reproducir SFX
    void playDoorOpen() {
        playSFX(sfxOpenDoor, 0.8f); // Un poco más bajo que la música
    }
    
    void playLevelComplete() {
        playSFX(sfxLevelComplete, 1.0f);
    }
    
    void playClick() {
        playSFX(sfxClick, 0.6f); // Click más suave
    }
    
    void musicThreadFunction() {
        logger.write("🎵 MusicThread started - Reproduciendo música de menú");
        
        Music* currentMusic = &menuMusic;  // Empezar con música del menú
        SetMusicVolume(*currentMusic, volume.load());
        PlayMusicStream(*currentMusic);
        
        while (audioRunning.load()) {
            if (!musicPaused.load()) {
                UpdateMusicStream(*currentMusic);
                
                // Cambiar música si es necesario
                if (isMenuMusic.load()) {
                    if (currentMusic != &menuMusic) {
                        logger.write("🎵 Cambiando a música de menú");
                        StopMusicStream(*currentMusic);
                        currentMusic = &menuMusic;
                        PlayMusicStream(*currentMusic);
                        SetMusicVolume(*currentMusic, volume.load());
                    }
                } else {
                    if (currentMusic != &gameplayMusic) {
                        logger.write("🎵 Cambiando a música de gameplay");
                        StopMusicStream(*currentMusic);
                        currentMusic = &gameplayMusic;
                        PlayMusicStream(*currentMusic);
                        SetMusicVolume(*currentMusic, volume.load());
                    }
                }
                
                // Reiniciar música si llegó al final
                if (GetMusicTimePlayed(*currentMusic) >= GetMusicTimeLength(*currentMusic)) {
                    StopMusicStream(*currentMusic);
                    PlayMusicStream(*currentMusic);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(GameConstants::AUDIO_UPDATE_RATE));
        }
        
        StopMusicStream(*currentMusic);
        logger.write("🎵 MusicThread finished");
    }
    
    void cambiarAMusicaMenu() {
        isMenuMusic = true;
    }
    
    void cambiarAMusicaGameplay() {
        isMenuMusic = false;
    }
    
    void togglePausa() {
        musicPaused = !musicPaused;
        if (isMenuMusic) {
            if (musicPaused) {
                PauseMusicStream(menuMusic);
            } else {
                ResumeMusicStream(menuMusic);
            }
        } else {
            if (musicPaused) {
                PauseMusicStream(gameplayMusic);
            } else {
                ResumeMusicStream(gameplayMusic);
            }
        }
    }
    
    void setVolume(float newVolume) {
        volume.store(newVolume);
        SetMusicVolume(menuMusic, newVolume);
        SetMusicVolume(gameplayMusic, newVolume);
    }
    
    float getVolume() const { return volume.load(); }
    bool isPaused() const { return musicPaused.load(); }
    
    void cerrarAudio() {
        audioRunning = false;
        if (musicThread.joinable()) {
            musicThread.join();
        }
        //Se libera la música
        if (menuMusic.frameCount > 0) {
            UnloadMusicStream(menuMusic);
        }
        if (gameplayMusic.frameCount > 0) {
            UnloadMusicStream(gameplayMusic);
        }
        
        // NUEVO: Liberar efectos de sonido
        if (sfxOpenDoor.frameCount > 0) {
            UnloadSound(sfxOpenDoor);
        }
        if (sfxLevelComplete.frameCount > 0) {
            UnloadSound(sfxLevelComplete);
        }
        if (sfxClick.frameCount > 0) {
            UnloadSound(sfxClick);
        }
        if (IsAudioDeviceReady()) {
            CloseAudioDevice();
        }
    }
};

//Sistemas de confeti
// ----------------------------------------------------------------------------
// Sistema de Partículas de Confeti - BASE CLASS
// ----------------------------------------------------------------------------

struct ConfettiParticle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float rotation;
    float angularVelocity;
    float lifetime; // Tiempo de vida restante en segundos
    float totalLifetime;
    float size;
};

class ConfettiSystem {
private:
    static constexpr float DURATION = 5.0f; // Duración del efecto en segundos
    static constexpr int MAX_PARTICLES = 150;

protected:
    std::vector<ConfettiParticle> particles;
    std::vector<ConfettiParticle>& getParticles() { return particles; }
    const std::vector<ConfettiParticle>& getParticles() const { return particles; }
    bool isActive = false;
    double startTime = 0.0;
    
    Color getRandomColor() {
        // Colores típicos de confeti (Rojo, Azul, Verde, Amarillo, Morado)
        std::array<Color, 5> colors = {RED, BLUE, LIME, GOLD, VIOLET};
        return colors[GetRandomValue(0, static_cast<int>(colors.size()) - 1)];
    }

    void createParticle(Vector2 center) {
        ConfettiParticle p;
        p.position = {center.x, center.y - 100.0f}; 
        
        // Ángulo aleatorio entre 90 (arriba) +/- 60 grados (rango entre 30 y 150)
        float angle = static_cast<float>(GetRandomValue(60, 150)) * DEG2RAD; 
        float speed = static_cast<float>(GetRandomValue(250, 400)) / 100.0f; // Velocidad entre 1.5 y 3.0
        
        p.velocity.x = speed * cosf(angle);
        p.velocity.y = -speed * sinf(angle);
        
        p.color = getRandomColor();
        p.rotation = static_cast<float>(GetRandomValue(0, 360));
        p.angularVelocity = static_cast<float>(GetRandomValue(-200, 200));
        p.totalLifetime = p.lifetime = static_cast<float>(GetRandomValue(300, 500)) / 100.0f; // Vida entre 1.0 y 2.0s
        p.size = static_cast<float>(GetRandomValue(5, 10));

        particles.push_back(p);
    }

public:
    virtual ~ConfettiSystem() = default;
    
    void startEffect(Vector2 center, int count = MAX_PARTICLES) {
        particles.clear();
        for (int i = 0; i < count; ++i) {
            createParticle(center);
        }
        isActive = true;
        startTime = GetTime();
        logger.write("🎉 Confetti effect started!");
    }

    virtual void update(float dt) {
        if (!isActive) return;

        // Limitar la duración del efecto
        if (GetTime() - startTime > DURATION) {
            // El efecto visual principal termina, pero permitimos que las partículas sigan cayendo
            // hasta que su lifetime termine.
        }

        for (auto it = particles.begin(); it != particles.end(); ) {
            it->lifetime -= dt;

            if (it->lifetime <= 0) {
                it = particles.erase(it);
            } else {
                // Aplicar gravedad (simulación simple)
                it->velocity.y += 9.8f * 0.008f; 
                
                // Actualizar posición
                it->position.x += it->velocity.x;
                it->position.y += it->velocity.y;
                
                // Actualizar rotación
                it->rotation += it->angularVelocity * dt;
                
                ++it;
            }
        }
        
        if (particles.empty() && GetTime() - startTime > DURATION) {
            isActive = false;
        }
    }

    virtual void draw() {
        if (!isActive) return;

        for (const auto& p : particles) {
            // Fading out effect near the end of life
            float lifeRatio = p.lifetime / p.totalLifetime;
            
            // Usar DrawRectanglePro para dibujar el confeti como pequeños rectángulos rotatorios
            DrawRectanglePro(
                (Rectangle){p.position.x, p.position.y, p.size, p.size / 2.0f}, // Rectángulo base
                (Vector2){p.size / 2.0f, p.size / 4.0f},                        // Origen (centro)
                p.rotation,                                                     // Rotación
                Fade(p.color, lifeRatio)                                        // Color con fade
            );
        }
    }
    
    bool isActiveEffect() const { return isActive; }
    void reset() { 
        particles.clear(); 
        isActive = false; 
    }
};


class EnhancedConfettiSystem : public ConfettiSystem {
private:
    Vector2 windForce = {0, 0};
    bool useWind = false;
    
public:
    void setWind(float x, float y) {
        windForce = {x, y};
        useWind = true;
    }
    
    void update(float dt) override {
        ConfettiSystem::update(dt);
        
        if (useWind) {
            for (auto& p : getParticles()) {
                p.velocity.x += windForce.x * dt;
                p.velocity.y += windForce.y * dt;
            }
        }
    }
    
    void drawWithGlow() {
        // Primera pasada: sombra/difuminado
        for (const auto& p : getParticles()) {
            float glowSize = p.size * 1.5f;
            Color glowColor = Fade(p.color, 0.3f);
            DrawCircleV(p.position, glowSize, glowColor);
        }
        
        // Segunda pasada: partícula principal
        ConfettiSystem::draw();
    }
};

// Estado del juego optimizado - CON SISTEMA DE NIVELES
struct GameState {
    mutable std::mutex mtx;
    
    using MapArray = std::array<std::array<int, GameConstants::MAP_WIDTH>, GameConstants::MAP_HEIGHT>;
    MapArray laberinto;
    
    Vector2 masterPos;
    Vector2 slavePos;
    
    // Estados atómicos
    std::atomic<bool> button1Active{false};
    std::atomic<bool> button2Active{false};
    std::atomic<bool> button3Active{false};
    
    std::atomic<bool> masterInGoal{false};
    std::atomic<bool> slaveInGoal{false};
    std::atomic<bool> bothInGoal{false};
    
    std::atomic<bool> gameRunning{true};
    
    // NUEVO: Sistema de niveles
    std::atomic<int> currentLevel{0};
    std::atomic<bool> levelCompleted{false};
};

// Gestor de texturas optimizado
class TextureManager {
private:
    std::unordered_map<std::string, Texture2D> textures;
    std::unordered_map<std::string, Font> fonts;
    bool texturesLoaded = false;
    
public:
    Font loadFont(const char* fileName, int fontSize = 32, int fontCharsCount = 250) {
        std::string key = std::string(fileName) + "_" + std::to_string(fontSize);
        
        auto it = fonts.find(key);
        if (it != fonts.end()) {
            return it->second;
        }
        
        Font font = LoadFontEx(fileName, fontSize, 0, fontCharsCount);
        if (font.texture.id == 0) {
            logger.write("❌ Error: No se pudo cargar la fuente: " + std::string(fileName));
            // Fallback a fuente por defecto
            font = GetFontDefault();
        } else {
            logger.write("✅ Fuente cargada: " + std::string(fileName));
        }
        
        fonts[key] = font;
        return font;
    }
    
    Font getFont(const std::string& name, int fontSize = 32) const {
        std::string key = name + "_" + std::to_string(fontSize);
        auto it = fonts.find(key);
        return (it != fonts.end()) ? it->second : GetFontDefault();
    }

    Texture2D loadAndRescaleTexture(const char* fileName, int targetWidth, int targetHeight) {
        std::string key = fileName;
        
        auto it = textures.find(key);
        if (it != textures.end()) {
            return it->second;
        }
        
        Image image = LoadImage(fileName);
        if (image.data == nullptr) {
            logger.write("❌ Error: No se pudo cargar la textura: " + std::string(fileName));
            
            Image fallback = GenImageColor(targetWidth, targetHeight, MAGENTA);
            Texture2D texture = LoadTextureFromImage(fallback);
            UnloadImage(fallback);
            
            textures[key] = texture;
            return texture;
        }
        
        ImageResize(&image, targetWidth, targetHeight);
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        
        textures[key] = texture;
        logger.write("✅ Textura cargada: " + std::string(fileName));
        
        return texture;
    }
    
    Texture2D getTexture(const std::string& name) const {
        auto it = textures.find(name);
        return (it != textures.end()) ? it->second : Texture2D{};
    }
    
    bool loadAllTextures() {
        if (texturesLoaded) return true;
        
        logger.write("📥 Cargando y reescalando texturas...");
        
        textures["menu_background"] = LoadTexture("resources/backgrounds/menu_bg.png");
        if (textures["menu_background"].id == 0) {
            logger.write("❌ Error: No se pudo cargar el fondo del menú");
        } else {
            logger.write("✅ Fondo del menú cargado");
        }
        
        textures["piso"] = loadAndRescaleTexture("resources/sprites/piso.png", 
                                               GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["pared"] = loadAndRescaleTexture("resources/sprites/pared.png", 
                                                GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["master"] = loadAndRescaleTexture("resources/sprites/master.png", 
                                                 GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["slave"] = loadAndRescaleTexture("resources/sprites/slave.png", 
                                                GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["boton1"] = loadAndRescaleTexture("resources/sprites/boton1.png", 
                                                 GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["boton2"] = loadAndRescaleTexture("resources/sprites/boton2.png", 
                                                 GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["boton3"] = loadAndRescaleTexture("resources/sprites/boton3.png", 
                                                 GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["puerta1Cerrada"] = loadAndRescaleTexture("resources/sprites/puerta_roja_cerrada.png", 
                                                         GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["puerta2Cerrada"] = loadAndRescaleTexture("resources/sprites/puerta_azul_cerrada.png", 
                                                         GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["puerta1Abierta"] = loadAndRescaleTexture("resources/sprites/puerta_roja_abierta.png", 
                                                         GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["puerta2Abierta"] = loadAndRescaleTexture("resources/sprites/puerta_azul_abierta.png", 
                                                         GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["puerta3Cerrada"] = loadAndRescaleTexture("resources/sprites/puerta_morada_cerrada.png",
                                                         GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["puerta3Abierta"] = loadAndRescaleTexture("resources/sprites/puerta_morada_abierta.png", 
                                                         GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["ObstaculoRojo"] = loadAndRescaleTexture("resources/sprites/obstaculo_rojo.png",
                                                         GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["ObstaculoAzul"] = loadAndRescaleTexture("resources/sprites/obstaculo_azul.png", 
                                                         GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        textures["meta"] = loadAndRescaleTexture("resources/sprites/meta.png", 
                                               GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        loadFont("resources/fonts/Arrows.ttf", 20, 250);
        loadFont("resources/fonts/Arrows.ttf", 24, 250);
        loadFont("resources/fonts/upheavtt.ttf", 20, 250);
        loadFont("resources/fonts/upheavtt.ttf", 60, 250);
        loadFont("resources/fonts/upheavtt.ttf", 30, 250);
        // Para controles - estilo inversión/tecnológico
        loadFont("resources/fonts/Inversionz.ttf", 22, 250);      // Tamaño para controles
        loadFont("resources/fonts/Inversionz.ttf", 18, 250);      // Tamaño más pequeño
        loadFont("resources/fonts/Inversionz.ttf", 16, 250);      // Para texto pequeño
    
        // Para nivel - estilo espacial/futurista
        loadFont("resources/fonts/spaceranger.ttf", 28, 250);     // Tamaño principal nivel
        loadFont("resources/fonts/spaceranger.ttf", 32, 250);     // Tamaño más grande
        loadFont("resources/fonts/spaceranger.ttf", 24, 250);     // Tamaño alternativo
        loadFont("resources/fonts/spaceranger.ttf", 40, 250);
        loadFont("resources/fonts/spaceranger.ttf", 20, 250);
        
        texturesLoaded = true;
        logger.write("🎨 Texturas cargadas correctamente");
        return true;
    }
    
    void unloadAll() {
        for (auto& pair : textures) {
            UnloadTexture(pair.second);
        }
        textures.clear();
        texturesLoaded = false;
        logger.write("🧹 Todas las texturas liberadas");
    }
    
    bool areTexturesLoaded() const { return texturesLoaded; }
};

// Sistema de colisiones optimizado
class CollisionSystem {
private:
    static constexpr int COLLISION_CHECK_RADIUS = 1;
    
public:
    static bool canPassTile(int tileType, bool isMaster, const GameState& state) {
        switch (tileType) {
            case VACIO: case START_MASTER: case START_SLAVE: 
            case BOTON_1: case BOTON_2: case BOTON_3: case META:
                return true;
            case PARED:
                return false;
            case PUERTA_1:
                return state.button1Active.load();
            case PUERTA_2:
                return state.button2Active.load();
            case PUERTA_3:
                return state.button3Active.load();
            case OBSTACULO_ROJO:
                return isMaster;
            case OBSTACULO_AZUL:
                return !isMaster;
            default:
                return false;
        }
    }
    
    static bool checkCollisionWithLaberinto(Vector2 position, float radius, bool isMaster, const GameState& state) {
        using namespace GameConstants;
        
        if (position.x < radius || position.y < radius || 
            position.x >= MAP_WIDTH * TILE_SIZE - radius || 
            position.y >= MAP_HEIGHT * TILE_SIZE - radius) {
            return true;
        }
        
        int centerTileX = static_cast<int>(position.x / TILE_SIZE);
        int centerTileY = static_cast<int>(position.y / TILE_SIZE);
        
        for (int y = centerTileY - COLLISION_CHECK_RADIUS; y <= centerTileY + COLLISION_CHECK_RADIUS; y++) {
            for (int x = centerTileX - COLLISION_CHECK_RADIUS; x <= centerTileX + COLLISION_CHECK_RADIUS; x++) {
                if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
                    int tileType = state.laberinto[y][x];
                    
                    if (!canPassTile(tileType, isMaster, state)) {
                        Rectangle tileRect = {
                            static_cast<float>(x * TILE_SIZE), 
                            static_cast<float>(y * TILE_SIZE), 
                            static_cast<float>(TILE_SIZE), 
                            static_cast<float>(TILE_SIZE)
                        };
                        if (CheckCollisionCircleRec(position, radius, tileRect)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
};

// Sistema de movimiento optimizado
class MovementSystem {
private:
    static constexpr float BORDER_MARGIN = 1.0f;
    
    static float clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
    
public:
    static Vector2 calculateNewPosition(Vector2 currentPos, const std::vector<int>& keys) {
        Vector2 newPos = currentPos;
        
        for (size_t i = 0; i < keys.size(); i += 4) {
            if (IsKeyDown(keys[i])) newPos.x -= GameConstants::PLAYER_SPEED;
            if (IsKeyDown(keys[i+1])) newPos.x += GameConstants::PLAYER_SPEED;
            if (IsKeyDown(keys[i+2])) newPos.y -= GameConstants::PLAYER_SPEED;
            if (IsKeyDown(keys[i+3])) newPos.y += GameConstants::PLAYER_SPEED;
        }
        
        const float maxX = GameConstants::MAP_WIDTH * GameConstants::TILE_SIZE - BORDER_MARGIN;
        const float maxY = GameConstants::MAP_HEIGHT * GameConstants::TILE_SIZE - BORDER_MARGIN;
        
        newPos.x = clamp(newPos.x, BORDER_MARGIN, maxX);
        newPos.y = clamp(newPos.y, BORDER_MARGIN, maxY);
        
        return newPos;
    }
};

// Hilos del juego optimizados
void physicsThread(GameState& state, bool isMaster, const std::vector<int>& keys) {
    logger.write((isMaster ? "Master" : "Slave") + std::string("PhysicsThread started"));
    
    while (state.gameRunning) {
        Vector2 currentPos;
        {
            std::lock_guard<std::mutex> lock(state.mtx);
            currentPos = isMaster ? state.masterPos : state.slavePos;
        }
        
        Vector2 newPos = MovementSystem::calculateNewPosition(currentPos, keys);
        
        if (!CollisionSystem::checkCollisionWithLaberinto(newPos, GameConstants::PLAYER_RADIUS, isMaster, state)) {
            std::lock_guard<std::mutex> lock(state.mtx);
            if (isMaster) {
                state.masterPos = newPos;
            } else {
                state.slavePos = newPos;
            }
        }
        
        SLEEP_MS(GameConstants::PHYSICS_UPDATE_RATE);
    }
    logger.write((isMaster ? "Master" : "Slave") + std::string("PhysicsThread finished"));
}

void validationThread(GameState& state, AudioSystem& audio) {
    logger.write("ValidationThread started");
    
    bool prevButton1Active = false;
    bool prevButton2Active = false;
    bool prevButton3Active = false;
    bool prevLevelCompleted = false;
    
    while (state.gameRunning) {
        Vector2 masterPos, slavePos;
        {
            std::lock_guard<std::mutex> lock(state.mtx);
            masterPos = state.masterPos;
            slavePos = state.slavePos;
        }
        
        int masterTileX = static_cast<int>(masterPos.x / GameConstants::TILE_SIZE);
        int masterTileY = static_cast<int>(masterPos.y / GameConstants::TILE_SIZE);
        int slaveTileX = static_cast<int>(slavePos.x / GameConstants::TILE_SIZE);
        int slaveTileY = static_cast<int>(slavePos.y / GameConstants::TILE_SIZE);
        
        // Botones 1 y 2 (activación individual)
        if (masterTileX >= 0 && masterTileX < GameConstants::MAP_WIDTH && 
            masterTileY >= 0 && masterTileY < GameConstants::MAP_HEIGHT) {
            int tile = state.laberinto[masterTileY][masterTileX];
            if (tile == BOTON_1) state.button1Active = true;
        }
        
        if (slaveTileX >= 0 && slaveTileX < GameConstants::MAP_WIDTH && 
            slaveTileY >= 0 && slaveTileY < GameConstants::MAP_HEIGHT) {
            int tile = state.laberinto[slaveTileY][slaveTileX];
            if (tile == BOTON_2) state.button2Active = true;
        }
        if (!state.button3Active){
          // NUEVO: Botón 3 requiere AMBOS jugadores
          bool masterOnButton3 = false;
          bool slaveOnButton3 = false;
        
          if (masterTileX >= 0 && masterTileX < GameConstants::MAP_WIDTH && 
              masterTileY >= 0 && masterTileY < GameConstants::MAP_HEIGHT) {
              int tile = state.laberinto[masterTileY][masterTileX];
              if (tile == BOTON_3) masterOnButton3 = true;
          }
        
          if (slaveTileX >= 0 && slaveTileX < GameConstants::MAP_WIDTH && 
              slaveTileY >= 0 && slaveTileY < GameConstants::MAP_HEIGHT) {
              int tile = state.laberinto[slaveTileY][slaveTileX];
              if (tile == BOTON_3) slaveOnButton3 = true;
          }
        
          // Solo activar si AMBOS están en el botón
          state.button3Active = masterOnButton3 && slaveOnButton3;
        }
        
        //Detectar cuando una puerta se abre
        if (!prevButton1Active && state.button1Active) {
            audio.playDoorOpen();
            logger.write("🔊 SFX: Puerta 1 abierta");
        }
        if (!prevButton2Active && state.button2Active) {
            audio.playDoorOpen();
            logger.write("🔊 SFX: Puerta 2 abierta");
        }
        if (!prevButton3Active && state.button3Active) {
            audio.playDoorOpen();
            logger.write("🔊 SFX: Puerta 3 abierta");
        }
        
        // Actualizar estados anteriores
        prevButton1Active = state.button1Active.load();
        prevButton2Active = state.button2Active.load();
        prevButton3Active = state.button3Active.load();
        
        
        // Verificar victoria (se mantiene igual)
        bool masterOnGoal = (masterTileX >= 0 && masterTileX < GameConstants::MAP_WIDTH && 
                           masterTileY >= 0 && masterTileY < GameConstants::MAP_HEIGHT) &&
                           (state.laberinto[masterTileY][masterTileX] == META);
        bool slaveOnGoal = (slaveTileX >= 0 && slaveTileX < GameConstants::MAP_WIDTH && 
                          slaveTileY >= 0 && slaveTileY < GameConstants::MAP_HEIGHT) &&
                          (state.laberinto[slaveTileY][slaveTileX] == META);
        
        state.masterInGoal = masterOnGoal;
        state.slaveInGoal = slaveOnGoal;
        state.bothInGoal = masterOnGoal && slaveOnGoal;
        
        if (state.bothInGoal && !state.levelCompleted) {
            state.levelCompleted = true;
            logger.write("✅ Nivel " + std::to_string(state.currentLevel.load()) + " completado!");
        }
        
        //Reproducir sonido cuando se completa el nivel
        if (!prevLevelCompleted && state.levelCompleted) {
            audio.playLevelComplete();
            logger.write("🔊 SFX: Nivel completado");
        }
        
        prevLevelCompleted = state.levelCompleted.load();
        
        SLEEP_MS(GameConstants::VALIDATION_UPDATE_RATE);
    }
    logger.write("ValidationThread finished");
}

// Sistema de renderizado optimizado
class RenderSystem {
private:
    TextureManager& textureManager;
    
public:

    RenderSystem(TextureManager& tm) : textureManager(tm) {}
    
    // MÉTODO 1: Para arrows.ttf CON CONTORNO
    void drawArrowsText(const std::string& text, Vector2 position, float fontSize, 
                       Color textColor, Color outlineColor = BLACK) {
        
        Font font = textureManager.getFont("resources/fonts/Arrows.ttf", 
                                          static_cast<int>(fontSize));
        
        if (font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id) {
            // Contorno de 8 direcciones
            std::vector<Vector2> outlineOffsets = {
              {-1, 0}, {1, 0}, {0, -1}, {0, 1},  // Solo 4 direcciones, no diagonales
              // {-1, -1}, {1, -1}, {-1, 1}, {1, 1}  // Comentadas para hacerlo más sutil
            };
            
            // Dibujar contorno
            for (const auto& offset : outlineOffsets) {
                DrawTextEx(font, text.c_str(), 
                          Vector2{position.x + offset.x, position.y + offset.y}, 
                          fontSize, 1, outlineColor);
            }
            
            // Dibujar texto principal
            DrawTextEx(font, text.c_str(), position, fontSize, 1, textColor);
            
        } else {
            // Fallback: texto normal con contorno simple
            DrawText(text.c_str(), (int)position.x + 1, (int)position.y, (int)fontSize, outlineColor);
            DrawText(text.c_str(), (int)position.x - 1, (int)position.y, (int)fontSize, outlineColor);
            DrawText(text.c_str(), (int)position.x, (int)position.y + 1, (int)fontSize, outlineColor);
            DrawText(text.c_str(), (int)position.x, (int)position.y - 1, (int)fontSize, outlineColor);
            DrawText(text.c_str(), (int)position.x, (int)position.y, (int)fontSize, textColor);
        }
    }
    
    // MÉTODO 2: Para inversionz.ttf SIN CONTORNO
    void drawInversionzText(const std::string& text, Vector2 position, float fontSize, 
                           Color textColor) {
        
        Font font = textureManager.getFont("resources/fonts/Inversionz.ttf", 
                                          static_cast<int>(fontSize));
        
        if (font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id) {
            DrawTextEx(font, text.c_str(), position, fontSize, 1, textColor);
        } else {
            DrawText(text.c_str(), (int)position.x, (int)position.y, (int)fontSize, textColor);
        }
    }
    
    // MÉTODO 3: Para spaceranger.ttf CON CONTORNO (más grueso)
    void drawSpacerangerText(const std::string& text, Vector2 position, float fontSize, 
                            Color textColor, Color outlineColor = BLACK) {
        
        Font font = textureManager.getFont("resources/fonts/spaceranger.ttf", 
                                          static_cast<int>(fontSize));
        
        if (font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id) {
            // Contorno más grueso (12 direcciones)
            std::vector<Vector2> outlineOffsets = {
                {-3, 0}, {3, 0}, {0, -3}, {0, 3},
                {-3, -3}, {3, -3}, {-3, 3}, {3, 3},
                {-2, 0}, {2, 0}, {0, -2}, {0, 2}
            };
            
            // Dibujar contorno
            for (const auto& offset : outlineOffsets) {
                DrawTextEx(font, text.c_str(), 
                          Vector2{position.x + offset.x, position.y + offset.y}, 
                          fontSize, 1, outlineColor);
            }
            
            // Dibujar texto principal
            DrawTextEx(font, text.c_str(), position, fontSize, 1, textColor);
            
        } else {
            // Fallback
            DrawText(text.c_str(), (int)position.x + 2, (int)position.y, (int)fontSize, outlineColor);
            DrawText(text.c_str(), (int)position.x - 2, (int)position.y, (int)fontSize, outlineColor);
            DrawText(text.c_str(), (int)position.x, (int)position.y + 2, (int)fontSize, outlineColor);
            DrawText(text.c_str(), (int)position.x, (int)position.y - 2, (int)fontSize, outlineColor);
            DrawText(text.c_str(), (int)position.x, (int)position.y, (int)fontSize, textColor);
        }
    }
    
    
    
    
    /*
    bool drawTextWithFont(const std::string& fontName, const std::string& text, 
                         Vector2 position, float fontSize, Color color) {
        Font customFont = textureManager.getFont(fontName, static_cast<int>(fontSize));
        if (customFont.texture.id != 0 && customFont.texture.id != GetFontDefault().texture.id) {
            DrawTextEx(customFont, text.c_str(), position, fontSize, 2, color);
            return true;
        } else {
            // Fallback a fuente por defecto
            DrawText(text.c_str(), (int)position.x, (int)position.y, (int)fontSize, color);
            return false;
        }
    }*/
    void drawLaberinto(const GameState& state) {
        for (int y = 0; y < GameConstants::MAP_HEIGHT; y++) {
            for (int x = 0; x < GameConstants::MAP_WIDTH; x++) {
                Rectangle destRect = {
                    static_cast<float>(x * GameConstants::TILE_SIZE),
                    static_cast<float>(y * GameConstants::TILE_SIZE),
                    static_cast<float>(GameConstants::TILE_SIZE),
                    static_cast<float>(GameConstants::TILE_SIZE)
                };
                int tileType = state.laberinto[y][x];
                
                drawTexture("piso", destRect, WHITE);
                drawTileContent(tileType, destRect, state);
            }
        }
    }
    
    void drawPlayers(GameState& state) {
        std::lock_guard<std::mutex> lock(state.mtx);
        
        Rectangle masterDest = {
            state.masterPos.x - GameConstants::TILE_SIZE/2, 
            state.masterPos.y - GameConstants::TILE_SIZE/2, 
            static_cast<float>(GameConstants::TILE_SIZE), 
            static_cast<float>(GameConstants::TILE_SIZE)
        };
        drawTexture("master", masterDest, WHITE);
        
        Rectangle slaveDest = {
            state.slavePos.x - GameConstants::TILE_SIZE/2, 
            state.slavePos.y - GameConstants::TILE_SIZE/2, 
            static_cast<float>(GameConstants::TILE_SIZE), 
            static_cast<float>(GameConstants::TILE_SIZE)
        };
        drawTexture("slave", slaveDest, WHITE);
    }
    
private:
    void drawTexture(const std::string& textureName, const Rectangle& destRect, Color tint) {
        Texture2D texture = textureManager.getTexture(textureName);
        if (texture.id != 0) {
            DrawTexturePro(texture, 
                          {0, 0, (float)texture.width, (float)texture.height},
                          destRect, {0, 0}, 0, tint);
        }
    }
    
    void drawTileContent(int tileType, const Rectangle& destRect, const GameState& state) {
        switch (tileType) {
            case PARED:
                drawTexture("pared", destRect, WHITE);
                break;
                
            case BOTON_1:
                drawTexture("boton1", destRect, state.button1Active.load() ? GREEN : WHITE);
                break;
                
            case BOTON_2:
                drawTexture("boton2", destRect, state.button2Active.load() ? GREEN : WHITE);
                break;
                
            case BOTON_3:
                drawTexture("boton3", destRect, state.button3Active.load() ? GREEN : WHITE);
                break;
                
            case PUERTA_1:
                if (state.button1Active.load()) {
                    drawTexture("puerta1Abierta", destRect, WHITE);
                } else {
                    drawTexture("puerta1Cerrada", destRect, WHITE);
                }
                break;
                
            case PUERTA_2:
                if (state.button2Active.load()) {
                    drawTexture("puerta2Abierta", destRect, WHITE);
                } else {
                    drawTexture("puerta2Cerrada", destRect, WHITE);
                }
                break;
            
            case PUERTA_3:
                if (state.button3Active.load()) {
                    drawTexture("puerta3Abierta", destRect, WHITE);
                } else {
                    drawTexture("puerta3Cerrada", destRect, WHITE);
                }
                break;
                
            case OBSTACULO_ROJO:
                // NUEVO: Sprite temporal - reemplaza cuando tengas el sprite real
                drawTexture("ObstaculoRojo", destRect, WHITE);
                break;
                
            case OBSTACULO_AZUL:
                // NUEVO: Sprite temporal - reemplaza cuando tengas el sprite real
                drawTexture("ObstaculoAzul", destRect, WHITE);
                break;
                
            case META:
                drawTexture("meta", destRect, state.bothInGoal ? GREEN : WHITE);
                break;
                
            default:
                break;
        }
    }
};

// Sistema de menú optimizado
class MenuSystem {
private:
    Rectangle playButton;
    Rectangle exitButton;
    TextureManager& textureManager;
    AudioSystem& audioSystem;
    
    void drawTextWithOutline(Font font, const char* text, Vector2 position, 
                           float fontSize, float spacing, Color textColor) {
        // Efecto de contorno
        std::vector<Vector2> outlineOffsets = {
            {-3, 0}, {3, 0}, {0, -3}, {0, 3},
            {-3, -3}, {3, -3}, {-3, 3}, {3, 3}
        };
        
        for (const auto& offset : outlineOffsets) {
            DrawTextEx(font, text, 
                       Vector2{position.x + offset.x, position.y + offset.y}, 
                       fontSize, spacing, BLACK);
        }
        
        // Texto principal
        DrawTextEx(font, text, position, fontSize, spacing, textColor);
    }
    
    void drawButtonText(Font font, const char* text, Rectangle button, Color textColor) {
        if (font.texture.id != 0) {
            // Usar MeasureTextEx para centrar correctamente
            Vector2 textSize = MeasureTextEx(font, text, 30, 2);
            Vector2 textPosition = {
                button.x + (button.width - textSize.x) / 2,
                button.y + (button.height - textSize.y) / 2
            };
            
            drawTextWithOutline(font, text, textPosition, 30, 2, textColor);
        } else {
            // Fallback a la función original
            DrawText(text, 
                     button.x + button.width/2 - MeasureText(text, 30)/2,
                     button.y + button.height/2 - 15, 30, textColor);
        }
    }
    
public:
    MenuSystem(TextureManager& tm, AudioSystem& audio) : textureManager(tm), audioSystem(audio) {
        playButton = { GameConstants::SCREEN_WIDTH/2 - 100, GameConstants::SCREEN_HEIGHT/2, 200, 50 };
        exitButton = { GameConstants::SCREEN_WIDTH/2 - 100, GameConstants::SCREEN_HEIGHT/2 + 70, 200, 50 };
    }
    
    
    
    
    
    void draw() {
        Texture2D background = textureManager.getTexture("menu_background");
        if (background.id != 0) {
            // Escalar la imagen para que cubra toda la pantalla
            DrawTexturePro(background,
                          {0, 0, (float)background.width, (float)background.height},
                          {0, 0, (float)GameConstants::SCREEN_WIDTH, (float)GameConstants::SCREEN_HEIGHT},
                          {0, 0}, 0, WHITE);
        } else {
            // Fallback: fondo blanco si no hay imagen
            ClearBackground(RAYWHITE);
        }
        
        Font titleFont = textureManager.getFont("resources/fonts/upheavtt.ttf", 60);
        Font regularFont = textureManager.getFont("resources/fonts/upheavtt.ttf", 20);
        Font buttonFont = textureManager.getFont("resources/fonts/upheavtt.ttf", 30);
        
        if (titleFont.texture.id != 0) {
            const char* duoText = "DUO";
            const char* mazeText = "MAZE";
            float titleSize = 60;
            float spacing = 2;
            
            // Medir ambas partes
            Vector2 duoSize = MeasureTextEx(titleFont, duoText, titleSize, spacing);
            Vector2 mazeSize = MeasureTextEx(titleFont, mazeText, titleSize, spacing);
            
            // Posición central
            float totalWidth = duoSize.x + mazeSize.x;
            Vector2 basePos = Vector2{
                GameConstants::SCREEN_WIDTH/2 - totalWidth/2, 
                GameConstants::SCREEN_HEIGHT/4
            };
            
            // Dibujar cada parte
            drawTextWithOutline(titleFont, duoText, basePos, titleSize, spacing, BLUE);
            
            Vector2 mazePos = Vector2{basePos.x + duoSize.x, basePos.y};
            drawTextWithOutline(titleFont, mazeText, mazePos, titleSize, spacing, RED);
        
    } else {
        // Fallback
        DrawText("DuoMaze", 
                 GameConstants::SCREEN_WIDTH/2 - MeasureText("DuoMaze", 60)/2, 
                 GameConstants::SCREEN_HEIGHT/4, 60, DARKBLUE);
    }
        
        
        /*DrawText("Cooperación en el Laberinto", 
                 GameConstants::SCREEN_WIDTH/2 - MeasureText("Cooperación en el Laberinto", 20)/2, 
                 GameConstants::SCREEN_HEIGHT/3 + 20, 20, DARKGRAY);*/
        if (regularFont.texture.id != 0) {
        const char* subtitle = "Cooperación en el Laberinto";
        Vector2 subtitleSize = MeasureTextEx(regularFont, subtitle, 20, 1);
        Vector2 subtitlePos = Vector2{
            GameConstants::SCREEN_WIDTH/2 - subtitleSize.x/2,
            GameConstants::SCREEN_HEIGHT/3 + 20
        };
        drawTextWithOutline(regularFont, subtitle, subtitlePos, 20, 1, GRAY);
    } else {
        DrawText("Cooperación en el Laberinto", 
                 GameConstants::SCREEN_WIDTH/2 - MeasureText("Cooperación en el Laberinto", 20)/2, 
                 GameConstants::SCREEN_HEIGHT/3 + 20, 20, GRAY);
    }
        
        Vector2 mousePoint = GetMousePosition();
        
        /*DrawRectangleRec(playButton, 
            CheckCollisionPointRec(mousePoint, playButton) ? BLUE : SKYBLUE);
        DrawRectangleLinesEx(playButton, 2, DARKBLUE);
        DrawText("JUGAR", 
                 playButton.x + playButton.width/2 - MeasureText("JUGAR", 30)/2,
                 playButton.y + playButton.height/2 - 15, 30, WHITE);
        
        DrawRectangleRec(exitButton, 
            CheckCollisionPointRec(mousePoint, exitButton) ? RED : PINK);
        DrawRectangleLinesEx(exitButton, 2, MAROON);
        DrawText("SALIR", 
                 exitButton.x + exitButton.width/2 - MeasureText("SALIR", 30)/2,
                 exitButton.y + exitButton.height/2 - 15, 30, WHITE);*/
        // Botón JUGAR
        DrawRectangleRec(playButton, 
            CheckCollisionPointRec(mousePoint, playButton) ? BLUE : SKYBLUE);
        DrawRectangleLinesEx(playButton, 2, DARKBLUE);
        
        // Usar la nueva función para dibujar el texto del botón
        drawButtonText(buttonFont, "JUGAR", playButton, WHITE);
        
        // Botón SALIR
        DrawRectangleRec(exitButton, 
            CheckCollisionPointRec(mousePoint, exitButton) ? RED : PINK);
        DrawRectangleLinesEx(exitButton, 2, MAROON);
        
        // Usar la nueva función para dibujar el texto del botón
        drawButtonText(buttonFont, "SALIR", exitButton, WHITE);
        
        /*DrawText("Usa P: Pausar música, M: Mutear, U: Subir volumen", 
                 GameConstants::SCREEN_WIDTH/2 - MeasureText("Usa P: Pausar música, M: Mutear, U: Subir volumen", 16)/2,
                 GameConstants::SCREEN_HEIGHT - 50, 16, GRAY);*/
        if (regularFont.texture.id != 0) {
        const char* audioText = "Usa P: Pausar música, M: Mutear, U: Subir volumen, H: Alto/Bajo";
        Vector2 textSize = MeasureTextEx(regularFont, audioText, 16, 1);
        Vector2 textPos = Vector2{
            GameConstants::SCREEN_WIDTH/2 - textSize.x/2,
            GameConstants::SCREEN_HEIGHT - 50
        };
        drawTextWithOutline(regularFont, audioText, textPos, 16, 1, DARKGRAY);
    } else {
        DrawText("Usa P: Pausar música, M: Mutear, U: Subir volumen", 
                 GameConstants::SCREEN_WIDTH/2 - MeasureText("Usa P: Pausar música, M: Mutear, U: Subir volumen", 16)/2,
                 GameConstants::SCREEN_HEIGHT - 50, 16, GRAY);
    }
    }
    
    bool isPlayButtonPressed() {
        Vector2 mousePoint = GetMousePosition();
        bool pressed = CheckCollisionPointRec(mousePoint, playButton) && 
                      IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        if (pressed) {
            audioSystem.playClick();
        }
        return pressed;
    }
    bool isExitButtonPressed() {
        Vector2 mousePoint = GetMousePosition();
        bool pressed = CheckCollisionPointRec(mousePoint, exitButton) && 
                      IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        if (pressed) {
            audioSystem.playClick();
        }
        return pressed;
    }
};

// NUEVO: Sistema de niveles expandido
class LevelSystem {
public:
    static void initializeLevel(GameState& state, int level) {
        // Resetear estados
        state.button1Active = false;
        state.button2Active = false;
        state.button3Active = false;
        state.masterInGoal = false;
        state.slaveInGoal = false;
        state.bothInGoal = false;
        state.levelCompleted = false;
        
        state.currentLevel = level;
        
        switch(level) {
            case 0: initializeLevel0(state); break;
            case 1: initializeLevel1(state); break;
            case 2: initializeLevel2(state); break;  // NUEVO: Nivel 2
            case 3: initializeLevel3(state); break;  // NUEVO: Nivel 3
            default: initializeLevel0(state); break;
        }
        
        logger.write("🎮 Nivel " + std::to_string(level) + " cargado");
    }
    
private:
    static void loadLevelData(GameState& state, const int levelData[GameConstants::MAP_HEIGHT][GameConstants::MAP_WIDTH]) {
        for (int y = 0; y < GameConstants::MAP_HEIGHT; y++) {
            for (int x = 0; x < GameConstants::MAP_WIDTH; x++) {
                state.laberinto[y][x] = levelData[y][x];
                
                if (levelData[y][x] == START_MASTER) {
                    state.masterPos = {
                        static_cast<float>(x * GameConstants::TILE_SIZE + GameConstants::TILE_SIZE/2), 
                        static_cast<float>(y * GameConstants::TILE_SIZE + GameConstants::TILE_SIZE/2)
                    };
                }
                if (levelData[y][x] == START_SLAVE) {
                    state.slavePos = {
                        static_cast<float>(x * GameConstants::TILE_SIZE + GameConstants::TILE_SIZE/2), 
                        static_cast<float>(y * GameConstants::TILE_SIZE + GameConstants::TILE_SIZE/2)
                    };
                }
            }
        }
    }
    //Nivel 1
    static void initializeLevel0(GameState& state) {
        constexpr int nivel0[GameConstants::MAP_HEIGHT][GameConstants::MAP_WIDTH] = {
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 2, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 4, 0, 0, 0, 1},
            {1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1},
            {1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1},
            {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1},
            {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
            {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
            {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
            {1, 3, 0, 0, 0, 0, 0, 0, 7, 0, 5, 0, 0, 0, 0, 0, 0, 0, 12, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
          };
        
        loadLevelData(state, nivel0);
    }
    
    //Nivel 2
    static void initializeLevel1(GameState& state) {
    constexpr int nivel1[GameConstants::MAP_HEIGHT][GameConstants::MAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1},
        {1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
        {1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 11, 1},
        {1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 5, 1, 0, 1, 0, 1},
        {1, 1, 0, 0, 1, 1, 1, 0, 1, 6, 1, 0, 0, 0, 0, 10, 0, 1, 0, 1},
        {1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1},
        {1, 0, 1, 0, 1, 1, 1, 7, 8, 0, 1, 1, 0, 11, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 9, 1, 1, 1, 0, 1},
        {1, 10, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 1, 1, 1, 12, 1, 1, 1, 0, 1},
        {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };
    
    loadLevelData(state, nivel1);
}
    // Nivel 3
    static void initializeLevel2(GameState& state) {
        constexpr int nivel2[GameConstants::MAP_HEIGHT][GameConstants::MAP_WIDTH] = {
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 0, 0, 0, 11, 0, 0, 0, 0, 0, 6, 1, 0, 0, 0, 0, 4, 7, 0, 1},
            {1, 0, 1, 0, 1, 8, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1},
            {1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1},
            {1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
            {1, 0, 0, 0, 10, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
            {1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 9, 1, 1, 1, 1, 1, 1, 1, 0, 1},
            {1, 0, 5, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 10, 0, 0, 0, 0, 1},
            {1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 9, 1, 1, 1, 1, 1, 1, 1, 0, 1},
            {1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1},
            {1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 12, 0, 0, 1, 0, 1},
            {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
        };
        
        loadLevelData(state, nivel2);
    }
    
    //Nivel 4
    static void initializeLevel3(GameState& state) {
    constexpr int nivel3[GameConstants::MAP_HEIGHT][GameConstants::MAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 4, 1, 6, 0, 0, 8, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1},
        {1, 10, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 11, 0, 1, 0, 1, 0, 1},
        {1, 0, 0, 11, 0, 10, 0, 1, 0, 1, 0, 1, 0, 1, 0, 10, 0, 1, 0, 1},
        {1, 11, 10, 1, 0, 1, 0, 1, 0, 1, 0, 10, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 10, 11, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 0, 1, 0, 11, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 11, 10, 1, 10, 1, 1, 0, 0, 10, 0, 11, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 11, 5, 1, 0, 7, 0, 1, 0, 10, 0, 11, 0, 1, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 0, 9, 3, 0, 10, 0, 11, 0, 1, 0, 11, 0, 1, 0, 11, 0, 1, 0, 1},
        {1, 12, 1, 0, 0, 11, 0, 1, 0, 10, 0, 1, 0, 10, 0, 1, 0, 10, 0, 1},
        {1, 0, 9, 2, 0, 1, 0, 10, 0, 11, 0, 10, 0, 11, 0, 10, 0, 11, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };
    
    loadLevelData(state, nivel3);
}
};

// Controlador de audio en pantalla
class AudioOverlay {
private:
    std::atomic<bool> mostrarControles{false};
    double tiempoOcultarControles{0};
    
public:
    void update() {
        if (GetTime() > tiempoOcultarControles) {
            mostrarControles = false;
        }
    }
    
    void draw() {
        if (!mostrarControles) return;
        
        DrawRectangle(10, GameConstants::SCREEN_HEIGHT - 120, 250, 110, Fade(BLACK, 0.8f));
        DrawText("CONTROLES AUDIO:", 20, GameConstants::SCREEN_HEIGHT - 110, 16, YELLOW);
        DrawText("P: Pausar/Reanudar musica", 20, GameConstants::SCREEN_HEIGHT - 90, 14, WHITE);
        DrawText("M: Mutear", 20, GameConstants::SCREEN_HEIGHT - 70, 14, WHITE);
        DrawText("U: Subir volumen", 20, GameConstants::SCREEN_HEIGHT - 50, 14, WHITE);
        DrawText("H: Alto/Bajo volumen", 20, GameConstants::SCREEN_HEIGHT - 30, 14, WHITE);
        DrawText("V: Mostrar/ocultar controles", 20, GameConstants::SCREEN_HEIGHT - 10, 14, WHITE);
        
    }
    
    void showTemporarily(double seconds = 3.0) {
        mostrarControles = true;
        tiempoOcultarControles = GetTime() + seconds;
    }
    
    void toggle() {
        mostrarControles = !mostrarControles;
        if (mostrarControles) {
            tiempoOcultarControles = GetTime() + 3.0;
        }
    }
};

int main() {
    logger.write("=== DuoMaze Iniciado ===");
    
    InitWindow(GameConstants::SCREEN_WIDTH, GameConstants::SCREEN_HEIGHT, "DuoMaze - Sistema de Niveles");
    SetTargetFPS(GameConstants::FPS_TARGET);

    AudioSystem audio;
    GameState gameState;
    TextureManager textureManager;
    RenderSystem renderSystem(textureManager);
    MenuSystem menuSystem(textureManager, audio);
    AudioOverlay audioOverlay;
    
    //Sistema de confeti
    EnhancedConfettiSystem confettiSystem;
    bool confettiActive = false;
    
    GameScreen currentScreen = MENU;
    bool shouldClose = false;
    
    textureManager.loadAllTextures();
    audio.cargarMusicas();

    const std::vector<int> masterKeys = {KEY_A, KEY_D, KEY_W, KEY_S};
    const std::vector<int> slaveKeys = {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN};
    
    std::thread masterThread;
    std::thread slaveThread;
    std::thread validatorThread;

    while (!WindowShouldClose() && !shouldClose) {
        if (IsKeyPressed(KEY_P)) {
            audio.togglePausa();
            audioOverlay.showTemporarily();
            audio.playClick(); 
        }
        
        if (IsKeyPressed(KEY_M)) {
            audio.setVolume(0.0f);
            audioOverlay.showTemporarily();
            audio.playClick(); 
        }
        
        if (IsKeyPressed(KEY_U)) {
            audio.setVolume(0.7f);
            audioOverlay.showTemporarily();
            audio.playClick(); 
        }
        if (IsKeyPressed(KEY_H)) {
          if (audio.getVolume() > 0.35f) { // Si está en volumen alto
              audio.setVolume(0.15f);      // Cambia a bajo
          } else {
            audio.setVolume(0.7f);       // Cambia a alto
          }
          audioOverlay.showTemporarily();
          audio.playClick(); 
        }
        
        if (IsKeyPressed(KEY_V)) {
            audioOverlay.toggle();
            audio.playClick();
        }
        
        audioOverlay.update();
        
        float deltaTime = GetFrameTime();
        confettiSystem.update(deltaTime);
        
        // Reiniciar confeti si terminó y el nivel sigue completado
        if (!confettiSystem.isActiveEffect() && confettiActive && gameState.levelCompleted) {
            Vector2 screenCenter = {
                GameConstants::SCREEN_WIDTH / 2.0f,
                GameConstants::SCREEN_HEIGHT / 2.0f
            };
            confettiSystem.startEffect(screenCenter);
        }

        switch (currentScreen) {
            case MENU: {
                if (menuSystem.isPlayButtonPressed()) {
                    // NUEVO: Siempre empezar desde nivel 0
                    LevelSystem::initializeLevel(gameState, 0);
                    gameState.gameRunning = true;
                    confettiSystem.reset();  // Reiniciar confeti
                    confettiActive = false;
                    
                    
                    masterThread = std::thread(physicsThread, std::ref(gameState), true, std::ref(masterKeys));
                    slaveThread = std::thread(physicsThread, std::ref(gameState), false, std::ref(slaveKeys));
                    validatorThread = std::thread(validationThread, std::ref(gameState), std::ref(audio));
                    
                    currentScreen = GAMEPLAY;
                    audio.cambiarAMusicaGameplay();
                    logger.write("Juego iniciado - Nivel 0");
                }
                
                if (menuSystem.isExitButtonPressed()) {
                    shouldClose = true;
                    logger.write("Juego cerrado desde menú");
                }
            } break;
            
            case GAMEPLAY: {
                
                if (gameState.bothInGoal && !confettiActive) {
                    Vector2 screenCenter = {
                        GameConstants::SCREEN_WIDTH / 2.0f,
                        GameConstants::SCREEN_HEIGHT / 2.0f
                    };
                    confettiSystem.startEffect(screenCenter);
                    confettiActive = true;
                    logger.write("🎊 Confetti activado para victoria!");
                }
                
                // NUEVO: Lógica de transición entre niveles
                if (gameState.levelCompleted && IsKeyPressed(KEY_ENTER)) {
                    int nextLevel = gameState.currentLevel.load() + 1;
                    
                    if (nextLevel < GameConstants::TOTAL_LEVELS) {
                    // Detener hilos antes de seguir
                        gameState.gameRunning = false;
                        confettiSystem.reset();  // Detener confeti
                        confettiActive = false;
            
                    if (masterThread.joinable()) masterThread.join();
                    if (slaveThread.joinable()) slaveThread.join();
                    if (validatorThread.joinable()) validatorThread.join();
                        
                        // Cargar siguiente nivel
                        LevelSystem::initializeLevel(gameState, nextLevel);
                        
                        // Reiniciar flag para nuevos hilos
            gameState.gameRunning = true;
            
                        // Crear NUEVOS hilos para el nuevo nivel
            masterThread = std::thread(physicsThread, std::ref(gameState), true, std::ref(masterKeys));
            slaveThread = std::thread(physicsThread, std::ref(gameState), false, std::ref(slaveKeys));
            validatorThread = std::thread(validationThread, std::ref(gameState), std::ref(audio));
            
            audio.cambiarAMusicaGameplay(); 
            logger.write("Avanzando al nivel " + std::to_string(nextLevel));
                    } else {
                        // Volver al menú (reinicio tipo Super Mario)
                        gameState.gameRunning = false;
                        confettiSystem.reset();  // Detener confeti
                        confettiActive = false;
                        
                        if (masterThread.joinable()) masterThread.join();
                        if (slaveThread.joinable()) slaveThread.join();
                        if (validatorThread.joinable()) validatorThread.join();
                        
                        currentScreen = MENU;
                        audio.cambiarAMusicaMenu();
                        logger.write("Todos los niveles completados - Volviendo al menú");
                    }
                }
            } break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentScreen) {
            case MENU:
                menuSystem.draw();
                break;
                
            case GAMEPLAY:
                renderSystem.drawLaberinto(gameState);
                renderSystem.drawPlayers(gameState);
                
                confettiSystem.drawWithGlow();
                
                // Fondo para mejor legibilidad
                DrawRectangle(0, 0, GameConstants::SCREEN_WIDTH, 40, Fade(BLACK, 0.4f));
                
                
                // 1. CONTROLES - Usando inversionz.ttf (SIN contorno)
                renderSystem.drawInversionzText("master: wasd", Vector2{10, 10}, 22, RED);
    
                // 2. NIVEL - Usando spaceranger.ttf (CON contorno)
                std::string levelText = TextFormat("NIVEL %d", gameState.currentLevel.load() + 1);
                float levelTextWidth = MeasureText(levelText.c_str(), 28); // Estimación
                renderSystem.drawSpacerangerText(levelText, 
                  Vector2{GameConstants::SCREEN_WIDTH/2 - levelTextWidth/2, 5}, 
                    28, GOLD);
    
    // 3. SLAVE - Usando inversionz.ttf (SIN contorno) + arrows.ttf (CON contorno)
    renderSystem.drawInversionzText("slave:", 
        Vector2{(float)GameConstants::SCREEN_WIDTH - 260, 10}, 22, BLUE);
    
                // Flechas CON CONTORNO usando arrows.ttf
    renderSystem.drawArrowsText("cbda", 
        Vector2{(float)GameConstants::SCREEN_WIDTH - 110, 10}, 24, BLUE);
        
        
        
                /*
                DrawText(TextFormat("Botón 1 (Rojo): %s", gameState.button1Active ? "ACTIVADO" : "INACTIVO"), 
                         10, 40, 18, gameState.button1Active ? GREEN : RED);
                DrawText(TextFormat("Botón 2 (Azul): %s", gameState.button2Active ? "ACTIVADO" : "INACTIVO"), 
                         10, 70, 18, gameState.button2Active ? GREEN : BLUE);
                DrawText(TextFormat("Botón 3 (Ambos): %s", gameState.button3Active ? "ACTIVADO" : "INACTIVO"), 
                         10, 100, 18, gameState.button3Active ? GREEN : ORANGE);
                
                DrawText(TextFormat("Master: %s", gameState.masterInGoal ? "EN META" : "EN CAMINO"), 
                         10, 130, 18, gameState.masterInGoal ? GREEN : RED);
                DrawText(TextFormat("Slave: %s", gameState.slaveInGoal ? "EN META" : "EN CAMINO"), 
                         10, 160, 18, gameState.slaveInGoal ? GREEN : BLUE);
               */
                
                // NUEVO: Pantallas de victoria diferenciadas
if (gameState.levelCompleted) {
    DrawRectangle(0, GameConstants::SCREEN_HEIGHT/2 - 60, 
                  GameConstants::SCREEN_WIDTH, 120, Fade(BLACK, 0.8f));
    
    // Obtener la fuente una vez
    Font spacerangerFont = textureManager.getFont("resources/fonts/spaceranger.ttf", 40);
    Font spacerangerFontSmall = textureManager.getFont("resources/fonts/spaceranger.ttf", 20);
    
    if (gameState.currentLevel < GameConstants::TOTAL_LEVELS - 1) {
        // No es el último nivel
        std::string levelCompleteText = "¡NIVEL COMPLETADO!";
        std::string nextLevelText = "Presiona ENTER para siguiente nivel";
        
        // Medir texto con la fuente real
        Vector2 levelCompleteSize;
        if (spacerangerFont.texture.id != 0) {
            levelCompleteSize = MeasureTextEx(spacerangerFont, levelCompleteText.c_str(), 40, 1);
        } else {
            levelCompleteSize.x = MeasureText(levelCompleteText.c_str(), 40);
        }
        
        renderSystem.drawSpacerangerText(levelCompleteText, 
            Vector2{GameConstants::SCREEN_WIDTH/2 - levelCompleteSize.x/2, 
                    GameConstants::SCREEN_HEIGHT/2 - 40}, 
            40, GREEN);
        
        // Texto pequeño
        Vector2 nextLevelSize;
        if (spacerangerFontSmall.texture.id != 0) {
            nextLevelSize = MeasureTextEx(spacerangerFontSmall, nextLevelText.c_str(), 20, 1);
        } else {
            nextLevelSize.x = MeasureText(nextLevelText.c_str(), 20);
        }
        
        renderSystem.drawSpacerangerText(nextLevelText, 
            Vector2{GameConstants::SCREEN_WIDTH/2 - nextLevelSize.x/2, 
                    GameConstants::SCREEN_HEIGHT/2 + 10}, 
            20, WHITE);
    } else {
        // Último nivel
        std::string gameCompleteText = "¡JUEGO COMPLETADO!";
        std::string backToMenuText = "Presiona ENTER para volver al menú";
        
        // Medir texto
        Vector2 gameCompleteSize;
        if (spacerangerFont.texture.id != 0) {
            gameCompleteSize = MeasureTextEx(spacerangerFont, gameCompleteText.c_str(), 40, 1);
        } else {
            gameCompleteSize.x = MeasureText(gameCompleteText.c_str(), 40);
        }
        
        renderSystem.drawSpacerangerText(gameCompleteText, 
            Vector2{GameConstants::SCREEN_WIDTH/2 - gameCompleteSize.x/2, 
                    GameConstants::SCREEN_HEIGHT/2 - 40}, 
            40, GOLD);
        
        // Texto pequeño
        Vector2 backToMenuSize;
        if (spacerangerFontSmall.texture.id != 0) {
            backToMenuSize = MeasureTextEx(spacerangerFontSmall, backToMenuText.c_str(), 20, 1);
        } else {
            backToMenuSize.x = MeasureText(backToMenuText.c_str(), 20);
        }
        
        renderSystem.drawSpacerangerText(backToMenuText, 
            Vector2{GameConstants::SCREEN_WIDTH/2 - backToMenuSize.x/2, 
                    GameConstants::SCREEN_HEIGHT/2 + 10}, 
            20, WHITE);
    }
}
                
                DrawText("Controles: WASD (Master), Flechas (Slave)", 
                         GameConstants::SCREEN_WIDTH/2 - MeasureText("Controles: WASD (Master), Flechas (Slave)", 16)/2, 
                         GameConstants::SCREEN_HEIGHT - 25, 16, DARKGRAY);
                
                break;
        }
        
        audioOverlay.draw();
        
        EndDrawing();
    }

    logger.write("=== Cerrando DuoMaze ===");
    
    gameState.gameRunning = false;
    
    if (masterThread.joinable()) masterThread.join();
    if (slaveThread.joinable()) slaveThread.join();
    if (validatorThread.joinable()) validatorThread.join();
    
    audio.cerrarAudio();
    textureManager.unloadAll();
    CloseWindow();
    
    logger.write("=== DuoMaze Cerrado Correctamente ===");
    return 0;
}
