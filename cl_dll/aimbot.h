#pragma once

/*
 * Aimbot nativo do client (integrado ao HUD/CL_CreateMove).
 *
 * Tudo o que a mira precisa por comando vive aqui: selecao de alvo por
 * hitbox, smoothing, recoil, silent/perfect silent e triggerbot.
 */

struct usercmd_s;
struct local_state_s;

/* Partes do corpo na mesma ordem do menu do Sakura. */
enum aim_part_e
{
	AIM_PART_BELT = 0,
	AIM_PART_L_THIGH,
	AIM_PART_L_TIBIA,
	AIM_PART_L_FOOT,
	AIM_PART_R_THIGH,
	AIM_PART_R_TIBIA,
	AIM_PART_R_FOOT,
	AIM_PART_STOMACH,
	AIM_PART_LOWER_CHEST,
	AIM_PART_UPPER_CHEST,
	AIM_PART_NECK,
	AIM_PART_HEAD,
	AIM_PART_L_SHOULDER,
	AIM_PART_L_ELBOW,
	AIM_PART_L_ARM,
	AIM_PART_L_HAND,
	AIM_PART_R_SHOULDER,
	AIM_PART_R_ELBOW,
	AIM_PART_R_ARM,
	AIM_PART_R_HAND,
	AIM_PART_GENERIC,
	AIM_PART_COUNT
};

struct aim_box_t
{
	int player;
	int group;    /* studio hitbox group (1..7) */
	int part;     /* aim_part_e */
	float origin[3];
	float c[8][3]; /* cantos do hitbox, para desenho */
};

void Aimbot_Init( void );
void Aimbot_SetBoxes( const aim_box_t *boxes, int count );
void Aimbot_NoteMouse( float pitch, float yaw );

/* Estado local vindo da predicao (punchangle, arma, municao, seed). */
void Aimbot_PostRunCmd( struct local_state_s *from, struct local_state_s *to,
	struct usercmd_s *cmd, int runfuncs, unsigned int seed );

/* Aplica mira/trigger no comando. Retorna 1 quando os angulos da camera
   mudaram (nesse caso o chamador deve chamar SetViewAngles). */
int Aimbot_Apply( float viewangles[3], struct usercmd_s *cmd );

void Aimbot_Draw( void );
/* Caixas do ultimo frame (para ESP e outros modulos). */
const aim_box_t *Aimbot_GetBoxes( int *count );

/* acesso usado pelo backtrack */
int Aimbot_CurrentTarget( void );
int Aimbot_KeyHeld( void );
int Aimbot_PointVisible( const float *eye, const float *point, int player, int ignoreWalls );
/* 0 = tecla consumida, 1 = passa para a engine. */
int Aimbot_Key( int down, int keynum );
