[file name]: compile_windows_corregido.sh
[file content begin]
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
    
    # Crear directorio temporal para empaquetado
    TEMP_DIR="DuoMaze_Windows_Final"
    FINAL_ZIP="DuoMaze_Entrega_Final.zip"
    
    # Limpiar directorio temporal si existe
    if [ -d "$TEMP_DIR" ]; then
        rm -rf "$TEMP_DIR"
    fi
    
    # Crear estructura completa de carpetas
    echo "📦 Creando paquete de entrega..."
    mkdir -p "$TEMP_DIR"
    
    # Copiar ejecutable al directorio raíz del paquete
    cp "$OUTPUT_EXE" "$TEMP_DIR/"
    
    # Copiar archivos de recursos manteniendo estructura
    if [ -d "resources" ]; then
        echo "📁 Copiando recursos..."
        cp -r "resources" "$TEMP_DIR/"
        
        # Verificar estructura completa de carpetas
        mkdir -p "$TEMP_DIR/resources/fonts"
        mkdir -p "$TEMP_DIR/resources/sprites"
        mkdir -p "$TEMP_DIR/resources/sound/music"
        mkdir -p "$TEMP_DIR/resources/sound/sfx"
        mkdir -p "$TEMP_DIR/resources/backgrounds"
    else
        echo "⚠️  Advertencia: No se encontró carpeta 'resources'"
        # Crear estructura vacía para evitar errores
        mkdir -p "$TEMP_DIR/resources/fonts"
        mkdir -p "$TEMP_DIR/resources/sprites"
        mkdir -p "$TEMP_DIR/resources/sound/music"
        mkdir -p "$TEMP_DIR/resources/sound/sfx"
        mkdir -p "$TEMP_DIR/resources/backgrounds"
    fi
    
    # BACKWARD COMPATIBILITY: Buscar recursos en ubicaciones alternativas
    echo "🔍 Buscando recursos en ubicaciones alternativas..."
    
    # Buscar músicas en ubicación antigua
    if [ -f "Maze_Quest.ogg" ] && [ ! -f "$TEMP_DIR/resources/sound/music/Maze_Quest.ogg" ]; then
        echo "📥 Copiando Maze_Quest.ogg de ubicación antigua"
        cp "Maze_Quest.ogg" "$TEMP_DIR/resources/sound/music/"
    fi
    
    if [ -f "Maze_Quest_Echoes.ogg" ] && [ ! -f "$TEMP_DIR/resources/sound/music/Maze_Quest_Echoes.ogg" ]; then
        echo "📥 Copiando Maze_Quest_Echoes.ogg de ubicación antigua"
        cp "Maze_Quest_Echoes.ogg" "$TEMP_DIR/resources/sound/music/"
    fi
    
    # Verificar si hay archivos en las carpetas del paquete
    echo "📊 Verificando estructura del paquete..."
    echo "   - Ejecutable: $( [ -f "$TEMP_DIR/DuoMaze.exe" ] && echo "✅" || echo "❌" )"
    echo "   - Fuentes: $( [ -d "$TEMP_DIR/resources/fonts" ] && echo "✅ ($(ls "$TEMP_DIR/resources/fonts" 2>/dev/null | wc -l) fuentes)" || echo "❌" )"
    echo "   - Sprites: $( [ -d "$TEMP_DIR/resources/sprites" ] && echo "✅ ($(ls "$TEMP_DIR/resources/sprites" 2>/dev/null | wc -l) sprites)" || echo "❌" )"
    echo "   - Música: $( [ -d "$TEMP_DIR/resources/sound/music" ] && echo "✅ ($(ls "$TEMP_DIR/resources/sound/music" 2>/dev/null | wc -l) archivos)" || echo "❌" )"
    echo "   - SFX: $( [ -d "$TEMP_DIR/resources/sound/sfx" ] && echo "✅ ($(ls "$TEMP_DIR/resources/sound/sfx" 2>/dev/null | wc -l) efectos)" || echo "❌" )"
    
    # Verificar sprites críticos para el nuevo sistema de niveles
    echo "🔍 Verificando sprites críticos..."
    CRITICAL_SPRITES=("piso.png" "pared.png" "master.png" "slave.png" "boton1.png" "boton2.png" "boton3.png" 
                     "puerta_roja_cerrada.png" "puerta_roja_abierta.png" "puerta_azul_cerrada.png" 
                     "puerta_azul_abierta.png" "puerta_morada_cerrada.png" "puerta_morada_abierta.png"
                     "obstaculo_rojo.png" "obstaculo_azul.png" "meta.png")
    
    missing_sprites=0
    for sprite in "${CRITICAL_SPRITES[@]}"; do
        if [ ! -f "$TEMP_DIR/resources/sprites/$sprite" ]; then
            echo "   ⚠️  $sprite - FALTANTE"
            missing_sprites=$((missing_sprites + 1))
        else
            echo "   ✅ $sprite - PRESENTE"
        fi
    done
    
    if [ $missing_sprites -gt 0 ]; then
        echo "⚠️  Advertencia: Faltan $missing_sprites sprites críticos"
    else
        echo "✅ Todos los sprites críticos están presentes"
    fi
    
    # Crear archivo de instrucciones
    echo "📝 Creando archivo de instrucciones..."
    cat > "$TEMP_DIR/INSTRUCCIONES.txt" << 'EOF'
DUO MAZE - INSTRUCCIONES (SISTEMA DE NIVELES AVANZADO)
======================================================

CONTROLES:
- Personaje ROJO (Master): W-A-S-D
- Personaje AZUL (Slave): Flechas direccionales
- Audio: P (pausar), M (mutear), U (volumen alto), H (volumen bajo)
- Controles audio: V (mostrar/ocultar)
- Niveles: ENTER (avanzar al siguiente nivel)

OBJETIVO:
Llevar a ambos personajes a la meta cooperando en cada nivel.

SISTEMA DE NIVELES MEJORADO:
- 4 niveles completos con mecánicas únicas
- Botones cooperativos (individuales y conjuntos)
- Obstáculos específicos por jugador
- Puertas que requieren cooperación
- Transición automática entre niveles
- Sistema de confeti en victoria

NUEVAS MECÁNICAS:
- Botón 3: Requiere que AMBOS jugadores estén encima
- Obstáculos rojos: Solo el jugador ROJO puede pasar
- Obstáculos azules: Solo el jugador AZUL puede pasar
- Puertas moradas: Requieren botón cooperativo

SISTEMA DE AUDIO MEJORADO:
- Música diferente en menú y gameplay
- Efectos de sonido para acciones específicas
- Controles de audio mejorados (4 niveles de volumen)

REQUISITOS:
✅ Zero instalación - solo ejecutar DuoMaze.exe
✅ Windows 7/8/10/11 (64 bits)

ESTRUCTURA ACTUALIZADA:
├── DuoMaze.exe
├── INSTRUCCIONES.txt
└── resources/
    ├── fonts/          # Fuentes del juego (Arrows.ttf, upheavtt.ttf, etc.)
    ├── sprites/        # Gráficos y texturas (ESENCIAL)
    ├── backgrounds/    # Fondos de pantalla
    └── sound/
        ├── music/      # Música de fondo (2 músicas diferentes)
        └── sfx/        # Efectos de sonido (abrir puertas, victoria, clics)

INSTRUCCIONES DE USO:
1. Extraer todo el contenido del ZIP
2. Ejecutar DuoMaze.exe
3. ¡Jugar!

NOTA: Mantener todos los archivos en la misma carpeta.

DESARROLLADO CON:
- Raylib 4.5.0
- Sistema multihilo concurrente
- Sistema de niveles progresivo (4 niveles)
- Sistema de partículas para confeti
- Compilado estáticamente

¡Disfruta el juego cooperativo!
EOF

    # Crear README adicional en la raíz del proyecto (opcional)
    cat > "README_ENTREGA.txt" << 'EOF'
DUO MAZE - PAQUETE DE ENTREGA
=============================

Este paquete contiene:
1. DuoMaze_Entrega_Final.zip - Paquete completo listo para entregar
2. DuoMaze.exe - Ejecutable compilado
3. main_a.cpp - Código fuente principal

Para probar el juego:
1. Extraer DuoMaze_Entrega_Final.zip
2. Ejecutar DuoMaze.exe desde la carpeta extraída
3. Seguir instrucciones en INSTRUCCIONES.txt

Estructura del ZIP:
DuoMaze_Windows_Final/
├── DuoMaze.exe
├── INSTRUCCIONES.txt
└── resources/ (todos los archivos necesarios)
EOF
    
    # Crear ZIP CORREGIDO - Usando el directorio como raíz del ZIP
    echo "🗜️  Creando archivo ZIP '$FINAL_ZIP'..."
    
    # Cambiar al directorio temporal para crear ZIP con estructura correcta
    cd "$TEMP_DIR"
    zip -r "../$FINAL_ZIP" ./*
    cd ..
    
    echo "✅ ZIP creado correctamente: $FINAL_ZIP"
    
    # Mostrar información del paquete
    echo ""
    echo "🎉 ¡PAQUETE LISTO PARA ENTREGAR!"
    echo "📦 Archivo: $FINAL_ZIP"
    echo "📁 Tamaño: $(du -h "$FINAL_ZIP" | cut -f1)"
    echo ""
    echo "📋 Contenido del ZIP:"
    echo "======================"
    unzip -l "$FINAL_ZIP" | head -30
    
    echo ""
    echo "🔧 PARA PROBAR:"
    echo "   1. Extraer el ZIP: unzip $FINAL_ZIP"
    echo "   2. Entrar en la carpeta: cd DuoMaze_Windows_Final"
    echo "   3. Ejecutar con Wine: wine DuoMaze.exe"
    echo ""
    echo "📝 PARA ENTREGAR:"
    echo "   - Entregar solo: $FINAL_ZIP"
    echo "   - El ZIP contiene estructura completa con carpeta contenedora"
    echo "   - Al extraer se creará: DuoMaze_Windows_Final/"
    
    # Limpiar directorio temporal (opcional, comentar para debug)
    # echo "🧹 Limpiando directorio temporal..."
    # rm -rf "$TEMP_DIR"
    
else
    echo "❌ Error en la compilación"
    exit 1
fi
[file content end]
