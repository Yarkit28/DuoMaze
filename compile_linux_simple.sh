#!/bin/bash
echo "Compilando DuoMaze para Linux..."

g++ -o DuoMaze main_a.cpp -lraylib -lm -lpthread -ldl -lX11 -O2 -Wno-narrowing

if [ $? -eq 0 ]; then
    echo "✅ ¡Compilación exitosa!"
    echo "🎮 Ejecuta: ./DuoMaze"
else
    echo "❌ Error en la compilación"
fi
