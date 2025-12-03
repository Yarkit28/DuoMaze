#!/bin/bash
echo "🎮 Compilando DuoMaze para Windows desde Linux..."
echo "=================================================="

# Configuración
RAYLIB_DIR="$HOME/mingw_libraries/raylib-4.5.0_win64_mingw-w64"
OUTPUT_EXE="DuoMaze.exe"
MAIN_FILE="main_a.cpp"

# Verificar archivo fuente
if [ ! -f "$MAIN_FILE" ]; then
    echo "❌ Error: No se encuentra $MAIN_FILE"
    exit 1
fi

# Verificar Raylib
if [ ! -d "$RAYLIB_DIR" ]; then
    echo "❌ Raylib no encontrado en: $RAYLIB_DIR"
    echo "📥 Descargando Raylib para Windows..."
    mkdir -p $HOME/mingw_libraries
    cd $HOME/mingw_libraries
    wget -q https://github.com/raysan5/raylib/releases/download/4.5.0/raylib-4.5.0_win64_mingw-w64.zip
    unzip -q raylib-4.5.0_win64_mingw-w64.zip
    cd -
fi

echo "🔨 Compilando $OUTPUT_EXE..."
x86_64-w64-mingw32-g++ -o "$OUTPUT_EXE" "$MAIN_FILE" \
    -I"$RAYLIB_DIR/include" \
    -L"$RAYLIB_DIR/lib" \
    -lraylib -lopengl32 -lgdi32 -lwinmm \
    -static -lpthread -std=c++17 -O2 \
    -DWIN32_LEAN_AND_MEAN -DNOGDI -DNOUSER -DNOMINMAX

# Verificar resultado
if [ $? -eq 0 ] && [ -f "$OUTPUT_EXE" ]; then
    echo "✅ ¡Compilación exitosa!"
    
    # Crear paquete completo con estructura actualizada de recursos
    echo "📦 Creando paquete de entrega..."
    PACKAGE_DIR="DuoMaze_Windows_Final"
    
    # Crear estructura completa de carpetas según el nuevo código
    mkdir -p "$PACKAGE_DIR/resources/fonts"
    mkdir -p "$PACKAGE_DIR/resources/sprites"
    mkdir -p "$PACKAGE_DIR/resources/sound/music"
    mkdir -p "$PACKAGE_DIR/resources/sound/sfx"
    mkdir -p "$PACKAGE_DIR/resources/backgrounds"
    
    # Copiar ejecutable
    cp "$OUTPUT_EXE" "$PACKAGE_DIR/"
    
    # Copiar recursos con estructura actualizada
    if [ -d "resources" ]; then
        # Copiar fuentes
        if [ -d "resources/fonts" ]; then
            cp -r resources/fonts/* "$PACKAGE_DIR/resources/fonts/" 2>/dev/null || echo "⚠️  No hay fuentes aún"
        else
            echo "⚠️  Carpeta 'resources/fonts' no encontrada"
        fi
        
        # Copiar sprites (ahora más críticos con el nuevo sistema de niveles)
        if [ -d "resources/sprites" ]; then
            cp -r resources/sprites/* "$PACKAGE_DIR/resources/sprites/"
            echo "✅ Sprites incluidos (esenciales para niveles)"
        else
            echo "❌ ERROR: Carpeta 'resources/sprites' no encontrada - EL JUEGO NO FUNCIONARÁ CORRECTAMENTE"
        fi
        
        # Copiar música
        if [ -d "resources/sound/music" ]; then
            cp -r resources/sound/music/* "$PACKAGE_DIR/resources/sound/music/" 2>/dev/null || echo "⚠️  No hay música en nueva ubicación"
        fi
        
        # Copiar efectos de sonido
        if [ -d "resources/sound/sfx" ]; then
            cp -r resources/sound/sfx/* "$PACKAGE_DIR/resources/sound/sfx/" 2>/dev/null || echo "⚠️  No hay SFX aún"
        fi
        
        # Copiar fondos (nuevo en esta versión)
        if [ -d "resources/backgrounds" ]; then
            cp -r resources/backgrounds/* "$PACKAGE_DIR/resources/backgrounds/" 2>/dev/null || echo "⚠️  No hay fondos aún"
        fi
    else
        echo "❌ ERROR: Carpeta 'resources' no encontrada - EL JUEGO NO FUNCIONARÁ"
    fi
    
    # BACKWARD COMPATIBILITY: Copiar música en ubicación antigua si existe
    if [ -f "Maze_Quest.ogg" ]; then
        mkdir -p "$PACKAGE_DIR/resources/sound/music"
        cp Maze_Quest.ogg "$PACKAGE_DIR/resources/sound/music/"
        echo "✅ Música incluida (ubicación antigua)"
    fi
    
    # Verificar sprites críticos para el nuevo sistema de niveles
    echo "🔍 Verificando sprites críticos..."
    CRITICAL_SPRITES=("piso.png" "pared.png" "master.png" "slave.png" "boton1.png" "boton2.png" "boton3.png" 
                     "puerta_roja_cerrada.png" "puerta_roja_abierta.png" "puerta_azul_cerrada.png" 
                     "puerta_azul_abierta.png" "puerta_morada_cerrada.png" "puerta_morada_abierta.png"
                     "obstaculo_rojo.png" "obstaculo_azul.png" "meta.png")
    
    for sprite in "${CRITICAL_SPRITES[@]}"; do
        if [ ! -f "$PACKAGE_DIR/resources/sprites/$sprite" ]; then
            echo "⚠️  Sprite crítico faltante: $sprite"
        fi
    done
    
    # Crear archivo de instrucciones actualizado
    cat > "$PACKAGE_DIR/INSTRUCCIONES.txt" << 'EOF'
DUO MAZE - INSTRUCCIONES (SISTEMA DE NIVELES)
=============================================

CONTROLES:
- Personaje ROJO (Master): W-A-S-D
- Personaje AZUL (Slave): Flechas direccionales
- Audio: P (pausar), M (mutear), U (subir volumen), V (controles audio)
- Niveles: ENTER (avanzar al siguiente nivel)

OBJETIVO:
Llevar a ambos personajes a la meta cooperando en cada nivel.

SISTEMA DE NIVELES MEJORADO:
- 2 niveles completos con mecánicas únicas
- Botones cooperativos (individuales y conjuntos)
- Obstáculos específicos por jugador
- Puertas que requieren cooperación
- Transición automática entre niveles

NUEVAS MECÁNICAS:
- Botón 3: Requiere que AMBOS jugadores estén encima
- Obstáculos rojos: Solo el jugador ROJO puede pasar
- Obstáculos azules: Solo el jugador AZUL puede pasar
- Puertas moradas: Requieren botón cooperativo

REQUISITOS:
✅ Zero instalación - solo ejecutar DuoMaze.exe
✅ Windows 7/8/10/11 (64 bits)

ESTRUCTURA ACTUALIZADA:
├── DuoMaze.exe
└── resources/
    ├── fonts/          # Fuentes del juego (Arrows.ttf)
    ├── sprites/        # Gráficos y texturas (ESENCIAL)
    ├── backgrounds/    # Fondos de pantalla
    └── sound/
        ├── music/      # Música de fondo
        └── sfx/        # Efectos de sonido

DESARROLLADO CON:
- Raylib 4.5.0
- Sistema multihilo concurrente
- Sistema de niveles progresivo
- Compilado estáticamente

¡Disfruta el juego cooperativo!
EOF

    # Comprimir todo
    echo "🗜️  Comprimiendo paquete final..."
    zip -r "DuoMaze_Entrega_Final.zip" "$PACKAGE_DIR/"
    
    echo ""
    echo "🎉 ¡PAQUETE LISTO PARA ENTREGAR!"
    echo "📁 Archivo: DuoMaze_Entrega_Final.zip"
    echo ""
    echo "📋 Contenido del paquete:"
    tree "$PACKAGE_DIR/" 2>/dev/null || ls -la "$PACKAGE_DIR/"
    echo ""
    echo "🚀 Para probar en Linux: wine DuoMaze.exe"
    echo ""
    echo "🔍 ESTADO DEL PAQUETE:"
    if [ -d "$PACKAGE_DIR/resources/sprites" ] && [ "$(ls -A $PACKAGE_DIR/resources/sprites/ 2>/dev/null | wc -l)" -gt 5 ]; then
        echo "✅ Sprites: OK (sistema de niveles funcionará)"
    else
        echo "❌ Sprites: FALTANTES - el juego no funcionará correctamente"
    fi
    
    if [ -f "$PACKAGE_DIR/resources/sound/music/Maze_Quest.ogg" ]; then
        echo "✅ Música: OK"
    else
        echo "⚠️  Música: Faltante (el juego funcionará pero sin audio)"
    fi
    
else
    echo "❌ Error en la compilación"
    exit 1
fi
