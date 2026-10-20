# Backtrack

Rewind de posicao no client: guarda o ultimo segundo de hitboxes de cada jogador e
deixa o aimbot/triggerbot mirar no ponto onde o alvo estava `N` milissegundos atras.
Servidores GoldSrc e ReHLDS ja fazem `sv_unlag`; o backtrack do client escolhe o ponto
historico e, opcionalmente, sincroniza o `cl_fakelag` para que o servidor aceite o tiro.

## Arquitetura

Duas camadas, para que a logica possa ser testada sem engine e sem jogo:

| Camada | Arquivos | Responsabilidade |
| --- | --- | --- |
| Core puro | `cl_dll/backtrack_core.h`, `cl_dll/backtrack_core.cpp` | ring buffer de quadros, busca por janela de tempo, filtros, mascara de hitbox, trim por idade, selftest |
| Glue | `cl_dll/backtrack.h`, `cl_dll/backtrack.cpp` | cvars, gravacao por frame, desenho dos ghosts, sync de fakelag, comandos |
| Configuracao | `cl_dll/backtrack_cfg.h`, `cl_dll/backtrack_cfg.cpp` | tabela unica das 37 cvars (`BT_CVAR_LIST`), usada pelo registro e pelos testes |

Fluxo por frame:

1. `HUD_Redraw` -> `EspHitbox_Flush()` preenche `Aimbot_GetBoxes()`.
2. `Backtrack_Tick()` grava um `bt_frame_s` com a posicao de cada hitbox
   (respeitando `cl_bt_record_ms`), aplica `Bt_TrimOlder()` e o `cl_bt_max_frames`.
3. `Aimbot_FindTarget()` / `Trigger_Run()` chamam `Backtrack_GetPoint(player, part)`.
4. `Backtrack_GetPoint()` monta o `bt_filter_s` (time, ping, distancia, visivel,
   modo de ativacao) e o `bt_query_s` (janela, passo, modo de escolha, smooth),
   chama `Bt_FilterAccepts()` e `Bt_Lookup()`.
5. `Backtrack_Draw()` desenha os ghosts e o indicador de delay.

Constantes do core: `BT_PARTS 21` (20 hitboxes do Sakura + `GENERIC`),
`BT_MAX_PLAYERS 33`, `BT_MAX_FRAMES 96` (~1 s a 100 Hz).

## Comandos

| Comando | Efeito |
| --- | --- |
| `bt_selftest` | roda o selftest do core e imprime `BT SELFTEST: total=10 fail=N` |
| `bt_clear` | limpa o historico |

## CVars (37)

| CVar | Default | Faixa | Descricao |
| --- | --- | --- | --- |
| `cl_bt_enable` | `0` | 0 .. 1 | liga o backtrack |
| `cl_bt_mode` | `1` | 0 .. 3 | 0 off, 1 legit, 2 rage, 3 adaptativo |
| `cl_bt_trigger_mode` | `0` | 0 .. 2 | 0 sempre, 1 so ao atirar, 2 tecla |
| `cl_bt_key` | `0` | 0 .. 255 | tecla de ativacao |
| `cl_bt_team` | `0` | 0 .. 1 | mirar no proprio time |
| `cl_bt_min_ms` | `40` | 0 .. 400 | delay minimo |
| `cl_bt_max_ms` | `200` | 0 .. 800 | delay maximo |
| `cl_bt_step_ms` | `20` | 0 .. 100 | resolucao do delay |
| `cl_bt_prefer` | `0` | 0 .. 3 | 0 recente, 1 antigo, 2 angulo, 3 perto |
| `cl_bt_visible_only` | `0` | 0 .. 1 | so ponto visivel |
| `cl_bt_ignore_walls` | `0` | 0 .. 1 | ignorar paredes na visibilidade |
| `cl_bt_max_dist` | `2500` | 100 .. 20000 | distancia maxima |
| `cl_bt_ping_min` | `0` | 0 .. 300 | ping minimo para usar |
| `cl_bt_sync_fakelag` | `0` | 0 .. 1 | sincronizar com cl_fakelag |
| `cl_bt_fakelag_extra` | `0` | 0 .. 100 | ms extras no fakelag |
| `cl_bt_draw` | `0` | 0 .. 1 | desenhar historico |
| `cl_bt_draw_ms` | `120` | 0 .. 800 | janela do desenho |
| `cl_bt_draw_line` | `1` | 0 .. 1 | marcar pontos |
| `cl_bt_draw_target_only` | `0` | 0 .. 1 | so o alvo atual |
| `cl_bt_interp` | `1` | 0 .. 1 | interpolar com o quadro vizinho |
| `cl_bt_record_ms` | `10` | 5 .. 50 | intervalo de gravacao |
| `cl_bt_max_frames` | `96` | 8 .. 96 | quadros usados |
| `cl_bt_clear_round` | `1` | 0 .. 1 | limpar ao morrer |
| `cl_bt_debug` | `0` | 0 .. 3 | 0 off, 1 resumo, 2 alvo, 3 tudo |
| `cl_bt_fov` | `0` | 0 .. 180 | fov proprio (0 = fov do aimbot) |
| `cl_bt_parts` | `0` | 0 .. 3 | 0 todos, 1 cabeca, 2 corpo, 3 cabeca+corpo |
| `cl_bt_lag_comp` | `0` | 0 .. 1 | janela automatica pelo ping |
| `cl_bt_smooth` | `0.5` | 0 .. 1 | peso da interpolacao |
| `cl_bt_color_r` | `255` | 0 .. 255 | cor do ghost - vermelho |
| `cl_bt_color_g` | `20` | 0 .. 255 | cor do ghost - verde |
| `cl_bt_color_b` | `147` | 0 .. 255 | cor do ghost - azul |
| `cl_bt_draw_alpha` | `200` | 0 .. 255 | transparencia do ghost |
| `cl_bt_draw_style` | `0` | 0 .. 2 | 0 caixa, 1 pontos, 2 ambos |
| `cl_bt_hud` | `0` | 0 .. 1 | indicador de delay na tela |
| `cl_bt_history_ms` | `1200` | 100 .. 3000 | idade maxima dos quadros |
| `cl_bt_reset_key` | `0` | 0 .. 255 | tecla que limpa o historico |
| `cl_bt_notify` | `0` | 0 .. 1 | avisar quando aplica rewind |

## Modos de escolha (`cl_bt_prefer`)

- `0` mais recente da janela (alvo para `min_ms`)
- `1` mais antigo da janela (alvo para `max_ms`; usado automaticamente no modo rage)
- `2` melhor angulo em relacao a mira atual
- `3` mais perto do olho

A janela e `min_ms .. max_ms` com resolucao `step_ms`; a busca escolhe o quadro mais
proximo do alvo temporal (nao exige igualdade exata), o que evita `hits=0` com jitter
real de 16.7 ms. Com `cl_bt_interp 1` o ponto e interpolado com o quadro vizinho pelo
peso `cl_bt_smooth` (0 = desligado, 0.5 = media).

Com `cl_bt_lag_comp 1` o `max_ms` vira `ping * 2`, limitado a `min_ms`/`max_ms`.

## Validacao por codigo

Nada disso precisa de jogo aberto:

```
./tools/bt_check.sh
```

O gate roda, em ordem:

1. `./tools/run_bt_tests.sh` - testes da logica pura (`tests/backtrack_core_test.cpp`).
2. `tests/bt_config_test.cpp` - invariantes da tabela (prefixo, unicidade, faixa,
   default, archive) e paridade com `tools/hub_cvars.txt`.
3. `bash tools/check_hub_bindings.sh` - toda cvar do manifesto existe na tab do HUB.
4. `nm`/`strings` no `build/cl_dll/client.so` - 37 cvars e os simbolos do core.

Saida esperada:

```
BT CORE TEST: 66 ok, 0 fail
BT CONFIG TEST: 927 ok, 0 fail
total=196 missing=0
OK: todas as cvars do manifesto estao no HUB
cvars no client.so: 37
simbolos: ok
BT CHECK: TUDO OK
```

## Build

O client e 32-bit (i386) e o `cl_dll/CMakeLists.txt` usa `file(GLOB "*.cpp")`, entao
arquivo `.cpp` novo exige re-rodar o configure:

```
cmake -S . -B build
cmake --build build -j$(nproc)
cp build/cl_dll/client.so <pasta do jogo>/cstrike/cl_dlls/client.so
```

Execucao com as cvars na linha de comando (o `+exec` nao aplica no fluxo `+connect`):

```
./xash3d -game cstrike -console +cl_bt_enable 1 +cl_bt_mode 2 \
  +cl_bt_min_ms 60 +cl_bt_max_ms 180 +cl_bt_sync_fakelag 1 +connect ip:porta
```

## Limitacoes conhecidas

- `cl_bt_trigger_mode 1/2` e `cl_bt_key` dependem do estado de ataque/tecla do cmd atual.
- `cl_bt_sync_fakelag` so liga enquanto atira e devolve `cl_fakelag`,
  `cl_fakelag_limit` e `cl_fakelag_type` aos valores originais ao sair.
- `cl_bt_visible_only` usa o trace do aimbot (`Aimbot_PointVisible`); `cl_bt_ignore_walls`
  ignora a parede nesse teste.
