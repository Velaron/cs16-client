#pragma once

/*
 * server_dump.h - dump de servidores (comando `dump_servidor`).
 *
 * Abre um painel ImGui onde o usuario escolhe:
 *   - fonte   : master server, varredura LAN ou lista manual de ip:porta
 *   - campos  : endereco, nome, mapa, jogadores, ping, regras, lista de jogadores
 *   - formato : txt, csv ou json
 *   - destino : arquivo ou diretorio (gera nome com data/hora)
 *
 * A consulta usa o protocolo de query do GoldSrc (A2S) por UDP, em thread
 * separada, e o painel mostra progresso e o caminho do arquivo salvo.
 */

void ServerDump_Init( void );
/* desenha o conteudo do dump dentro de uma tab do HUB (sem janela propria). */
void ServerDump_DrawContent( void );
/* chamado por frame no HUD: dispara cl_dump_auto depois do mapa carregar. */
void ServerDump_Tick( void );
/* encerra a thread do dump no shutdown do client. */
void ServerDump_Shutdown( void );
