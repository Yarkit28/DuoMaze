#!/bin/bash
echo "🛠️  Compilando Creador de Niveles - DuoMaze Dev Tool..."
echo "========================================================"

# Configuración
RAYLIB_DIR="$HOME/mingw_libraries/raylib-4.5.0_win64_mingw-w64"
OUTPUT_EXE="CreadorNiveles.exe"
MAIN_FILE="creador_niveles.cpp"

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
    
    # Crear paquete completo para el creador de niveles
    echo "📦 Creando paquete de herramientas..."
    PACKAGE_DIR="CreadorNiveles_Windows"
    
    # Crear estructura de carpetas
    mkdir -p "$PACKAGE_DIR/resources"
    
    # Copiar ejecutable
    cp "$OUTPUT_EXE" "$PACKAGE_DIR/"
    
    # Copiar recursos necesarios para el creador
    if [ -d "resources" ]; then
        # Lista de texturas necesarias para el creador
        REQUIRED_TEXTURES=(
            "piso.png" "pared.png" "master.png" "slave.png"
            "boton1.png" "boton2.png" "boton3.png"
            "puerta_roja_cerrada.png" "puerta_azul_cerrada.png" "puerta_morada_cerrada.png"
            "meta.png"
        )
        
        # Verificar y copiar texturas necesarias
        echo "📁 Copiando recursos del creador..."
        for texture in "${REQUIRED_TEXTURES[@]}"; do
            if [ -f "resources/$texture" ]; then
                cp "resources/$texture" "$PACKAGE_DIR/resources/"
                echo "  ✅ $texture"
            else
                echo "  ⚠️  $texture no encontrado"
            fi
        done
        
        # Si no se encontraron en resources/, buscar en resources/sprites/
        if [ ! -f "$PACKAGE_DIR/resources/piso.png" ] && [ -d "resources/sprites" ]; then
            echo "🔍 Buscando texturas en resources/sprites/"
            for texture in "${REQUIRED_TEXTURES[@]}"; do
                if [ -f "resources/sprites/$texture" ]; then
                    cp "resources/sprites/$texture" "$PACKAGE_DIR/resources/"
                    echo "  ✅ $texture (desde sprites)"
                fi
            done
        fi
    else
        echo "⚠️  Carpeta 'resources' no encontrada"
    fi
    
    # Crear archivo de instrucciones
    cat > "$PACKAGE_DIR/INSTRUCCIONES_CREADOR.txt" << 'EOF'
CREADOR DE NIVELES - DUOMAZE
============================

DESCRIPCIÓN:
Herramienta de desarrollo para crear y editar niveles para el juego DuoMaze.

CONTROLES:
- Click IZQUIERDO: Ciclar entre tipos de tile (0-12)
- Click DERECHO: Borrar tile (poner VACIO)
- G: Mostrar/ocultar grid
- C: Limpiar nivel completo
- S: Guardar nivel generado

TIPOS DE TILE:
 0: Vacio          1: Pared
 2: Start Master   3: Start Slave  
 4: Boton 1        5: Boton 2
 6: Boton 3        7: Puerta 1
 8: Puerta 2       9: Puerta 3
10: Obst. Rojo    11: Obst. Azul
12: Meta

CARACTERÍSTICAS:
✅ Bordes automáticos (siempre activos)
✅ Grid con coordenadas
✅ Panel informativo en tiempo real
✅ Exportación directa a código C++

USO:
1. Diseña el nivel haciendo click en los tiles
2. Coloca elementos especiales (start, meta, botones, puertas)
3. Presiona S para guardar
4. El código se guarda en 'nivel_generado.txt'
5. Copia el código en LevelSystem::initializeLevelX() del juego

NOTAS:
- Los bordes están bloqueados y no se pueden modificar
- Asegúrate de incluir al menos un START_MASTER, START_SLAVE y META
- Los botones y puertas deben coincidir (Boton1 -> Puerta1, etc.)

REQUISITOS:
✅ Windows 7/8/10/11 (64 bits)
✅ Zero instalación - solo ejecutar CreadorNiveles.exe

DESARROLLADO CON:
- Raylib 4.5.0
- C++17
- Compilado estáticamente

¡Feliz creación de niveles!
EOF

    # Comprimir todo
    echo "🗜️  Comprimiendo paquete del creador..."
    zip -r "CreadorNiveles_Entrega.zip" "$PACKAGE_DIR/"
    
    echo ""
    echo "🎉 ¡CREADOR DE NIVELES LISTO!"
    echo "📁 Archivo: CreadorNiveles_Entrega.zip"
    echo ""
    echo "📋 Contenido del paquete:"
    tree "$PACKAGE_DIR/" 2>/dev/null || ls -la "$PACKAGE_DIR/"
    echo ""
    echo "🚀 Para usar en Linux: wine CreadorNiveles.exe"
    echo "💡 El archivo 'nivel_generado.txt' se creará al guardar niveles"
    
else
    echo "❌ Error en la compilación"
    exit 1
fi
