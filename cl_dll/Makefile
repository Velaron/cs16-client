CC= gcc -m32
CXX = g++ -m32
SRCS+=../dlls/crossbow.cpp
SRCS+=../dlls/crowbar.cpp
SRCS+=../dlls/egon.cpp
SRCS+=./ev_hldm.cpp
SRCS+=../dlls/gauss.cpp
SRCS+=../dlls/handgrenade.cpp
SRCS+=./hl/hl_baseentity.cpp
SRCS+=./hl/hl_events.cpp
SRCS+=./hl/hl_objects.cpp
SRCS+=./hl/hl_weapons.cpp
SRCS+=../dlls/wpn_shared/hl_wpn_glock.cpp
SRCS+=../dlls/hornetgun.cpp
SRCS+=../common/interface.cpp
SRCS+=../dlls/mp5.cpp
SRCS+=../dlls/python.cpp
SRCS+=../dlls/rpg.cpp
SRCS+=../dlls/satchel.cpp
SRCS+=../dlls/shotgun.cpp
SRCS+=../dlls/squeakgrenade.cpp
SRCS+=../dlls/tripmine.cpp
#SRCS+=../game_shared/voice_banmgr.cpp
#SRCS+=../game_shared/voice_status.cpp
SRCS+=./ammo.cpp
SRCS+=./ammo_secondary.cpp
SRCS+=./ammohistory.cpp
SRCS+=./battery.cpp
SRCS+=./cdll_int.cpp
SRCS+=./com_weapons.cpp
SRCS+=./death.cpp
SRCS+=./demo.cpp
SRCS+=./entity.cpp
SRCS+=./ev_common.cpp
SRCS+=./events.cpp
SRCS+=./flashlight.cpp
SRCS+=./GameStudioModelRenderer.cpp
SRCS+=./geiger.cpp
SRCS+=./health.cpp
SRCS+=./hud.cpp
SRCS+=./hud_msg.cpp
SRCS+=./hud_redraw.cpp
#SRCS+=./hud_servers.cpp
SRCS+=./hud_spectator.cpp
SRCS+=./hud_update.cpp
SRCS+=./in_camera.cpp
SRCS+=./input.cpp
#SRCS+=./inputw32.cpp
SRCS+=./menu.cpp
SRCS+=./message.cpp
SRCS+=./overview.cpp
SRCS+=./parsemsg.cpp
SRCS_C+=../pm_shared/pm_debug.c
SRCS_C+=../pm_shared/pm_math.c
SRCS_C+=../pm_shared/pm_shared.c
SRCS+=./saytext.cpp
SRCS+=./status_icons.cpp
SRCS+=./statusbar.cpp
SRCS+=./studio_util.cpp
SRCS+=./StudioModelRenderer.cpp
SRCS+=./text_message.cpp
SRCS+=./train.cpp
SRCS+=./tri.cpp
SRCS+=./util.cpp
SRCS+=./view.cpp
SRCS+=./input_stub.cpp
SRCS+=./scoreboard.cpp
DEFINES =  -I../common -I. -I../game_shared -I../pm_shared -I../engine -Wno-write-strings -DLINUX -D_LINUX -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -DCLIENT_WEAPONS -DCLIENT_DLL -I../dlls/ -fpermissive -w
CFLAGS = -Og -ggdb
OBJS = $(SRCS:.cpp=.o) $(SRCS_C:.c=.o)
%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -fPIC -c $< -o $@

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) $(DEFINES) -fPIC -c $< -o $@
client.so : $(OBJS)
	$(CXX) $(OBJS) -o client.so -shared -Wl,--no-undefined -fPIC -lm -ldl

clean:
	$(RM) $(OBJS)
