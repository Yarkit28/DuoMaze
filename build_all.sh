#!/bin/bash
echo "=== COMPILADOR UNIVERSAL DUOMAZE ==="

# Función para compilar para Linux
compile_linux() {
    echo ""
    echo "--- COMPILANDO PARA LINUX ---"
    if ! command -v g++ &> /dev/null; then
        echo "Error: g++ no está instalado."
        return 1
    fi
    
    if ! pkg-config --exists raylib; then
        echo "Error: raylib no está instalado."
        return 1
    fi
    
    g++ -o DuoMaze_linux main_a.cpp -lraylib -lm -lpthread -ldl -lX11 -O2
    if [ $? -eq 0 ]; then
        echo "✅ Linux: DuoMaze_linux compilado exitosamente"
        return 0
    else
        echo "❌ Error compilando para Linux"
        return 1
    fi
}

# Función para compilar para Windows
compile_windows() {
    echo ""
    echo "--- COMPILANDO PARA WINDOWS ---"
    if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
        echo "Advertencia: mingw-w64 no está instalado. Saltando compilación Windows."
        echo "Instala con: sudo apt-get install mingw-w64"
        return 1
    fi
    
    x86_64-w64-mingw32-g++ -o DuoMaze.exe main_a.cpp -lraylib -lwinmm -lgdi32 -static -O2 -s
    if [ $? -eq 0 ]; then
        echo "✅ Windows: DuoMaze.exe compilado exitosamente"
        return 0
    else
        echo "❌ Error compilando para Windows"
        return 1
    fi
}

# Crear directorios de distribución
mkdir -p dist/linux dist/windows

# Compilar para ambas plataformas
linux_success=0
windows_success=0

compile_linux
if [ $? -eq 0 ]; then
    cp DuoMaze_linux dist/linux/
    cp Maze_Quest.mp3 dist/linux/ 2>/dev/null || echo "Advertencia: Maze_Quest.mp3 no encontrado para Linux"
    chmod +x dist/linux/DuoMaze_linux
    linux_success=1
fi

compile_windows
if [ $? -eq 0 ]; then
    cp DuoMaze.exe dist/windows/
    cp Maze_Quest.mp3 dist/windows/ 2>/dev/null || echo "Advertencia: Maze_Quest.mp3 no encontrado para Windows"
    windows_success=1
fi

# Resultados
echo ""
echo "=== RESULTADOS DE COMPILACIÓN ==="
if [ $linux_success -eq 1 ]; then
    echo "✅ Linux: dist/linux/DuoMaze_linux"
fi
if [ $windows_success -eq 1 ]; then
    echo "✅ Windows: dist/windows/DuoMaze.exe"
fi

if [ $linux_success -eq 0 ] && [ $windows_success -eq 0 ]; then
    echo "❌ No se pudo compilar para ninguna plataforma"
    exit 1
fi

echo ""
echo "🎮 ¡Compilación completada!"
