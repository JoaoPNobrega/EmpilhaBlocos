Empilha Blocos (Tetris)

Projeto acadêmico de um Tetris com regras modernas (SRS), fila 7-bag, hold, ghost, animação de limpeza de linhas, HUD e ranking local por jogador.

Estrutura do projeto
--------------------
jogoAED/
  core/
    tetris.c / tetris.h        - lógica do jogo (SRS, bag, hold, pontuação, animações)
    highscore.c / highscore.h  - ranking (carrega/salva em scores.txt)
  ui/
    ui_tetris.c / ui_tetris.h  - interface SDL2/SDL_ttf (menus, HUD, efeitos)
  main_tetris.c                - loop principal e estados (menu, nome, jogo, game over)

Controles
---------
A / D: mover para esquerda/direita
S: descer suave (soft drop)
E / Q: girar horário / anti-horário
ESPAÇO: hard drop
C: hold
ESC: sair

Requisitos
----------
SDL2 e SDL2_ttf
C17 (gcc/clang)

Como compilar e executar
------------------------
Windows (MSYS2 – ambiente UCRT64)
1) Instale MSYS2: https://www.msys2.org
2) Abra o terminal MSYS2 UCRT64 e execute:
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf
gcc -std=c17 -O2 main_tetris.c core/tetris.c core/highscore.c ui/ui_tetris.c $(pkg-config --cflags --libs sdl2 SDL2_ttf) -o tetris.exe
./tetris.exe

Obs.: Se preferir distribuir o .exe pronto, deixe tetris.exe na mesma pasta das DLLs: SDL2.dll, SDL2_ttf.dll.
Se faltar alguma DLL, copie também de C:\msys64\ucrt64\bin:
libwinpthread-1.dll, libgcc_s_seh-1.dll, libfreetype-6.dll, libbrotlicommon-1.dll, libbrotlidec-1.dll, zlib1.dll, libbz2-1.dll, libpng16-16.dll.

Linux (Debian/Ubuntu)
sudo apt update
sudo apt install build-essential pkg-config libsdl2-dev libsdl2-ttf-dev
gcc -std=c17 -O2 main_tetris.c core/tetris.c core/highscore.c ui/ui_tetris.c $(pkg-config --cflags --libs sdl2 SDL2_ttf) -o tetris
./tetris

macOS (Homebrew)
brew install sdl2 sdl2_ttf pkg-config
clang -std=c17 -O2 main_tetris.c core/tetris.c core/highscore.c ui/ui_tetris.c $(pkg-config --cflags --libs sdl2 SDL2_ttf) -o tetris
./tetris

Arquivos gerados em runtime
---------------------------
scores.txt (ranking local). Não é necessário versionar (adicione ao .gitignore).

Personalização rápida
---------------------
Altura do botão “REINICIAR”: altere GAME_BTN_Y em ui/ui_tetris.h.
Cores das peças: função color_for() em ui/ui_tetris.c.
Cores/fonte/posições: macros em ui/ui_tetris.h e chamadas em ui/ui_tetris.c.

Licença
-------
Uso acadêmico/educacional. Adapte a licença conforme a disciplina (ex.: MIT).
