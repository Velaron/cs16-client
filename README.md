# CS16Client ENCHANCED

Fork do CS16Client focado em Counter-Strike 1.6 no Windows e Linux usando Xash3D. O objetivo deste fork e melhorar compatibilidade com servidores, estabilidade do HUD/client DLL e recursos visuais configuraveis.

Este README documenta apenas Windows e Linux. Plataformas fora desse alvo foram removidas do guia principal.

## Plataformas Alvo

- Windows 32-bit.
- Linux 32-bit em sistemas x86/x86_64.
- Uso com servidores CS 1.6 Steam/non-Steam compativeis com GoldSrc/Xash3D.

## Alteracoes Do Fork Por Data

### 2026-10-19

#### HUB unico (F6)

- Menu ImGui no estilo Sakura com 4 categorias (Rage, Legit, Visuals, Misc) e 26 sub-tabs.
- Tema, widgets (check, combo, slider, keybind) e layout centralizados em `cl_dll/ui/`.
- Manifesto `tools/hub_cvars.txt` + `tools/gen_hub_cvars.py` + `tools/check_hub_bindings.sh`
  garantem que toda cvar do manifesto tem widget no HUB.

#### Backtrack

- Novo modulo em duas camadas: `cl_dll/backtrack_core.{h,cpp}` (puro, testavel no host) e
  `cl_dll/backtrack.{h,cpp}` (glue com engine/HUD).
- Ring buffer de 96 quadros (~1 s) com gravacao por frame das 21 hitboxes.
- 37 cvars `cl_bt_*` numa tabela unica (`cl_dll/backtrack_cfg.h`), com tab propria no HUB.
- Busca por proximidade temporal (nao por igualdade de delay), interpolacao com peso
  (`cl_bt_smooth`), janela automatica por ping (`cl_bt_lag_comp`), mascara de hitbox
  (`cl_bt_parts`), FOV proprio (`cl_bt_fov`), cores/HUD configuraveis.
- Sync opcional de `cl_fakelag` apenas enquanto atira, restaurando os valores originais
  ao sair.
- Validacao por codigo em `./tools/bt_check.sh` (sem abrir o jogo).

#### Aimbot, triggerbot e extras

- Aimbot legit/rage com hitbox por modo, humanize (spiral), reaction time, flash check,
  fast zoom e block attack.
- Triggerbot com rajada/precisao, perfect silent, recoil compensation e knifebot.
- ESP de player/hitbox, chams, sound ESP, killsound e notify.

#### Correcoes

- AntiAFK: o `sidemove` ficava ligado em todo frame parado e o player deslizava para o
  lado; agora e um passo de 80 ms a cada 20 s.
- HUB: dump dentro do menu, sem a janela de teste do ImGui; historico validado e thread
  de dump sem `join()` duplicado.

### 2026-05-26

#### Hitbox e visibilidade

- `cl_hitbox_outline` desenha hitboxes 3D no caminho do Studio renderer, respeitando profundidade e paredes.
- `cl_hitbox_outline_head_only` mantem o modo limpo desenhando apenas a cabeca por padrao.
- `cl_hitbox_outline_max_dist` agora aceita `0` como distancia ilimitada.
- A hitbox da cabeca ficou 20% maior apenas no grupo oficial `HITGROUP_HEAD`.
- A cabeca ganhou brilho ciano aditivo mais forte, com uma passada larga de glow e uma linha principal mais clara por cima.
- Corrigida a deteccao da cabeca: deixou de usar tamanho da caixa e passou a usar `mstudiobbox_t.group == 1`, evitando aumentar hitboxes do corpo por engano.
- `cl_player_outline` adiciona contorno/silhueta visivel no modelo sem atravessar paredes.

#### Estabilidade do client DLL

- Corrigido crash em `CHud::Redraw` quando `cl_charset` ou `con_charset` ainda nao estavam prontos.
- Corrigido erro de simbolo indefinido para `CBeam::GetStartPos` e `CBeam::GetEndPos`, mantendo os accessors inline no header.
- Adicionados valores `BEAM_*` ausentes usados por efeitos antigos.
- Tratamento de sprites ausentes ficou mais tolerante em areas do HUD que antes podiam produzir comportamento instavel.

#### Limites internos de texto e caminhos

- `MAX_VA_STRING` aumentado para `4096`.
- `MAX_SYSPATH` aumentado para `4096`.
- Strings de menu aumentadas para `2048`.
- Text messages aumentadas para `2048`.
- Campos de HUD/scoreboard ampliados com copias usando `sizeof` e terminacao nula explicita.
- Buffer de caminhos de sprites no HUD aumentado para reduzir truncamentos.

#### Canvas e desenho 2D

- Adicionado wrapper local `cl_dll/canvas.cpp` e `cl_dll/canvas.h` para desenho 2D com linhas grossas, retangulos, circulos, scissor e helpers de projecao.
- Adicionado `engine_canvas_api.h` para declarar a ABI C do Canvas da engine sem conflito com a classe C++ local.
- O Canvas local salva/restaura estado OpenGL com `glPushAttrib`/`glPopAttrib`, reduzindo corrupcao de estado entre HUD, chuva, outlines e outros desenhos.

#### Build e instalacao testada

- Build 32-bit com CMake/Ninja validado.
- `client.so` gerado e testado como ELF i386 para uso com Xash3D Linux i386.

### 2026-05-02

Alteracoes ja documentadas anteriormente neste fork:

- Compatibilidade com servidores Windows/non-Steam.
- Correcao inicial de MOTD.
- Foco inicial em Linux.
- Comandos de skin model por arma:
  - `skin_model_ak47_list`
  - `skin_model_ak47_set`
  - `skin_model_(weapon)_list`
  - `skin_model_(weapon)_set`
- `aspect_ratio` para alterar proporcao sem trocar resolucao.
- `cl_spreaddot` para ponto de previsao de spread.

## Instalacao

1. Compile ou baixe a engine Xash3D ENCHANCED.
2. Tenha Counter-Strike 1.6 instalado legalmente.
3. Copie as pastas `valve` e `cstrike` para a pasta da engine.
4. Copie `client.so` ou `client.dll` para:

```text
cstrike/cl_dlls/
```

5. Execute a engine.

## Linux 32-bit

### Arch/Manjaro

Habilite `multilib` em `/etc/pacman.conf`:

```ini
[multilib]
Include = /etc/pacman.d/mirrorlist
```

Instale dependencias comuns:

```sh
sudo pacman -Syu
sudo pacman -S \
  git cmake ninja python \
  lib32-mesa \
  lib32-libglvnd \
  lib32-sdl2 \
  lib32-sdl2_image \
  lib32-openal \
  lib32-libpulse \
  lib32-zlib \
  lib32-libpng \
  lib32-libjpeg-turbo
```

Diagnostico:

```sh
ldd ./xash3d | grep "not found"
file ./xash3d
file cstrike/cl_dlls/client.so
```

### Debian/Ubuntu

```sh
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install -y \
  git cmake ninja-build python3 \
  libgl1-mesa-glx:i386 \
  libgl1-mesa-dri:i386 \
  libsdl2-2.0-0:i386 \
  libsdl2-image-2.0-0:i386 \
  libopenal1:i386 \
  libpulse0:i386 \
  zlib1g:i386 \
  libpng16-16:i386 \
  libjpeg-turbo8:i386 \
  file
```

Erros comuns:

- `No such file or directory`: normalmente falta libc/dependencia 32-bit.
- `wrong ELF class: ELFCLASS64`: algum binario 64-bit foi misturado com build 32-bit.
- Crash sem log claro: normalmente driver/OpenGL ou dependencia 32-bit ausente.

## Build

Clone:

```sh
git clone --recursive https://github.com/pescadordegoiaba/cs16-client-Enchanced.git
cd cs16-client-Enchanced
```

### Linux

```sh
cmake --preset linux-release-i386
cmake --build build -j$(nproc)
```

Se o preset nao estiver disponivel:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-m32" \
  -DCMAKE_CXX_FLAGS="-m32" \
  -DCMAKE_EXE_LINKER_FLAGS="-m32" \
  -DCMAKE_SHARED_LINKER_FLAGS="-m32"
cmake --build build -j$(nproc)
```

### Windows

```bat
git clone --recursive https://github.com/pescadordegoiaba/cs16-client-Enchanced.git
cd cs16-client-Enchanced
cmake -A Win32 -S . -B build
cmake --build build --config Release
cmake --install build --prefix C:\path\to\xash3d
```

## CVars Relevantes Do Fork

```cfg
cl_player_outline 0
cl_hitbox_outline 1
cl_hitbox_outline_head_only 1
cl_hitbox_outline_max_dist 0
aspect_ratio 1
cl_spreaddot 1
```

## Comandos De Skin Model

| Command | Description |
| --- | --- |
| `skin_model_ak47_list` | Lista skins disponiveis para AK-47. |
| `skin_model_ak47_set` | Define o skin model da AK-47. |
| `skin_model_(weapon)_list` | Lista skins para uma arma especifica. |
| `skin_model_(weapon)_set` | Define o skin model de uma arma especifica. |

## Documentacao

- [docs/HUB.md](docs/HUB.md) - categorias, sub-tabs e bindings do menu F6.
- [docs/BACKTRACK.md](docs/BACKTRACK.md) - arquitetura, 37 cvars, comandos e gate de testes.

## Reportando Problemas

Inclua no report:

- Sistema operacional.
- Saida de `file` para `xash3d` e `client.so`/`client.dll`.
- Mapa e servidor.
- Log do console.
- CVars visuais relevantes, principalmente `cl_hitbox_outline`, `cl_hitbox_outline_head_only` e `cl_hitbox_outline_max_dist`.
