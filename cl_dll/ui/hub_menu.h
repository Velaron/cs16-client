#pragma once

/*
 * HUB_MENU unico do client (tecla F6), no estilo do menu do Sakura.
 */

void Hub_Init( void );
void Hub_Open( int open );
int  Hub_IsOpen( void );

/* seleciona categoria e sub-tab (ex: Hub_ShowTab( 3, 1 ) = Misc > Servers). */
void Hub_ShowTab( int cat, int sub );

/* 0 = tecla consumida, 1 = passa para o engine. */
int  Hub_Key( int down, int keynum );
