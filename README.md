# Empilha Blocos (Tetris)

Projeto acadêmico em C/SDL2 com interface gráfica. Jogo estilo Tetris moderno (SRS), com **hold**, **ghost**, fila 7-bag, HUD, animações e **ranking** local.

- **Vídeo demonstrativo:** https://youtu.be/o8UMokPb4Mo  
- **Executável pronto (repositório/ZIP):** <[jogo.exe](https://github.com/JoaoPNobrega/EmpilhaBlocos.Exe)>

## Integrantes
- João Pedro Nóbrega
- Marco Antonio Veras

---

## Compilação (código-fonte)

### Windows (MSYS2 – UCRT64)
1. Instale MSYS2: https://www.msys2.org  
2. Abra **MSYS2 UCRT64** e execute:
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf \
     mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf
   gcc -std=c17 -O2 main_tetris.c core/tetris.c core/highscore.c ui/ui_tetris.c \
     $(pkg-config --cflags --libs sdl2 SDL2_ttf) -o tetris.exe
   ./tetris.exe

Controles: A/D mover | S descer | E/Q girar | ESPAÇO hard drop | C hold | ESC sair.


