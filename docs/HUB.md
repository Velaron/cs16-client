# HUB (F6)

Menu ImGui unico do fork, aberto com **F6**. Visual no estilo Sakura: 720x490,
header de 50 px, footer de 20 px, sub-tabs de 150x25 e area de conteudo 530x400.
Paleta `rgb(19,22,26)` / `rgb(25,30,35)` / `rgb(45,50,55)` com destaque `rgb(255,20,147)`.

## Estrutura

Quatro categorias, cada uma com sub-tabs:

| Categoria | Sub-tabs |
| --- | --- |
| **Rage** | Aimbot, Hitboxes, Anti Aim, Fake Lag, Speedhack |
| **Legit** | Aimbot, Triggerbot, Perfect Silent, Recoil, Knifebot, Hitboxes |
| **Visuals** | Player, Local, World, Radar, Skins, Removals, Sound, Menu |
| **Misc** | Movement, Servers, Keybindings, Player List, Config, AntiAFK, Notify, Route, Backtrack |

## Secoes por tab

| Tab | Secoes |
| --- | --- |
| Aimbot (Legit) | Aimbot legit, Ativacao, Mira, Humanizacao, Extras |
| Triggerbot | Triggerbot, Tempo, Rajada, Precisao |
| Perfect Silent | Silent, Perfect silent |
| Recoil | Recoil compensation, Rage, Precisao |
| Knifebot | Knifebot |
| Aimbot (Rage) | Rage aimbot, Hitboxes |
| Hitboxes | Partes do corpo (uma linha por hitbox, por modo) |
| Anti Aim | Anti aim |
| Fake Lag | Fake lag |
| Player | ESP, Chams |
| Local | Camera, Crosshair |
| World | Mundo |
| Radar | Radar |
| Skins | Skins |
| Removals | Remocoes |
| Sound | Sound ESP, Sons |
| Menu | Cores do menu, Teclas |
| Movement | Movimento, HNS |
| Servers | Lista e conexao em servidores |
| Keybindings | Teclas do HUB |
| Player List | Jogadores |
| Config | Configuracao |
| AntiAFK | Anti AFK, Nome, Chat spam |
| Notify | Notify |
| Route | Route |
| Backtrack | Backtrack, Janela de tempo, Alvo, Fakelag, Visual, Avancado |

## Bindings e cvars

- Todas as cvars do HUB vivem em `tools/hub_cvars.txt` (manifesto).
- `python3 tools/gen_hub_cvars.py` gera `cl_dll/ui/hub_cvars_list.h`.
- `bash tools/check_hub_bindings.sh` falha quando uma cvar do manifesto nao aparece
  em `cl_dll/ui/hub_menu.cpp`. Hoje: `total=196 missing=0`.

O backtrack tem documento proprio em [BACKTRACK.md](BACKTRACK.md).

## AntiAFK

`cl_sak_antiafk` da um passo curto (80 ms de `sidemove`) a cada 20 s em vez de deixar
o movimento ligado continuamente, que fazia o player deslizar parado.
