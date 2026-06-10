# Makefile for Project Duris!

SHELL = /bin/sh
#EFENCE = 1

CC	= g++
DEBUG	= -ggdb3
#LDFLAGS	= -ggdb3

LDFLAGS += -L/usr/lib/mysql

CFLAGS = -g -std=c++20 -D_FORTIFY_SOURCE=0 -Wno-write-strings -Wno-format-truncation -Wno-register -pthread
#CFLAGS += -D__NO_MYSQL__
CFLAGS += -DTEST_MUD

# to turn on/off chaos mud, change the flag at the top of db.h
#CFLAGS += -DCHAOS_MUD=1
#CFLAGS += -DCTF_MUD=1

INCLUDES	= -I. -I/usr/include/libxml2 -I/usr/include/mysql -I./ships

GENERAL_LIBS    = -lm -lstdc++ -lz -lxml2 -lcrypt -lmysqlclient -lgnutls -lssl -lcrypto -lcjson -lhiredis -pthread

ifdef EFENCE
	LDFLAGS += -L.
	LIBS += -lefence -lrt
	CFLAGS += -DEFENCE
endif

# GENERAL_LIBS += -lcygipc
LIBS += $(GENERAL_LIBS)

# Put all .o/.d files here (Makefile lives in src/)
OBJDIR   := ../obj
DEPFLAGS := -MMD -MP

# files that would be compiled as dynamic libs if USE_SHARED was defined
#    make sure that they are NOT included in ANY form in OBJS!!!
# The makefile will deal with compiling SH_LIBS as normal files
# if USE_SHARED isn't defined

SH_LIBS 	=

SHIP_SRCS = \
	ship_auto.c \
	ship_base.c \
	ship_cargo.c \
	ship_combat.c \
	ship_control.c \
	ship_npc.c \
	ship_npc_ai.c \
	ship_shop.c \
	ship_utils.c \
	ship_variables.c \

SHIP_OBJS = $(SHIP_SRCS:%.c=ships/%.o)

OBJS =  $(SHIP_OBJS) \
				 magic.o \
				 achievements.o \
				 account.o \
				 actcomm.o \
				 actinf.o \
				 actmove.o \
				 actnew.o \
				 actobj.o \
				 actoff.o \
				 actoth.o \
				 actset.o \
				 actwiz.o \
				 ai_mobs.o \
				 specs.ailvio.o \
				 alliances.o \
				 automatic_rules.o \
				 affects.o \
				 arena.o \
				 artifact.o \
				 assocs.o \
				 auction_houses.o \
				 bard.o \
				 beh_magic.o \
         blispells.o \
				 boards.o \
				 boon.o \
				 breath_weapons.o \
				 buildings.o \
         cardgames.o \
				 comm.o \
				 copyover.o \
				 common.o \
				 condition.o \
				 constant.o \
				 ctf.o \
				 db.o \
				 debug.o \
				 disguise.o \
				 drannak.o \
				 dreadlord.o \
				 editor.o \
				 encounters.o \
				 epic.o \
				 epic_bonus.o \
				 epic_skills.o \
				 ethermancer.o \
				 events.o \
				 new_events.o \
				 fight.o \
				 files.o \
				 fraglist.o \
				 forge_items.o \
				 genrand.o \
				 graph.o \
				 group.o \
				 guild.o \
				 guildhall.o \
				 guildhall_rooms.o \
				 guildhall_cmds.o \
				 guildhall_procs.o \
				 guildhall_db.o \
				 grapple.o \
				 handler.o \
				 hardcore.o \
				 innates.o \
				 interp.o \
				 justice.o \
				 kick.o \
				 language.o \
				 limits.o \
				 linked_objects.o\
				 listen.o\
				 mail.o \
				 map.o \
				 mccp.o \
				 memorize.o \
				 memory.o \
				 mm.o \
				 mobact.o \
				 world_quest.o \
				 mobcombat.o \
				 mobconv.o \
				 mobpatrol.o \
				 modify.o \
				 mount.o \
				 name_gen.o \
				 nanny.o \
				 necromancy.o \
				 new_combat_util.o \
				 new_skills.o \
				 nq.o \
				 objconv.o \
				 objmisc.o \
				 outposts.o \
				 period.list.o \
				 persistence_queue.o \n				 sql_persistence_raw.o \
				 poll.o \
				 prompt.o \
				 properties.o \
				 psionics.o \
				 quest.o \
				 random.o \
				 randomeq.o \
				 random.mob.o \
				 random.zone.o \
				 randobj.o \
				 range.o \
				 rogues.o \
				 shaman.o \
				 shop.o \
				 signals.o \
				 skills.o \
				 smagic.o \
				 salchemist.o \
				 sillusionist.o \
				 siege.o \
				 sound.o \
				 sparser.o \
				 specials.o \
				 specializations.o \
				 specs.assign.o \
				 specs.barovia.o \
				 specs.clwcvrn.o \
				 specs.dragonnia.o \
				 specs.eth2.o \
				 specs.halfcut.o \
				 specs.ravenloft.o \
				 specs.snogres.o \
				 specs.ioun.o \
				 specs.jot.o \
				 specs.library.o \
				 specs.lohrr.o \
				 specs.mobile.o \
				 specs.morgs.o \
				 specs.mist.o \
				 specs.object.o \
				 specs.realm.o \
				 specs.room.o \
				 specs.snikzone.o \
				 specs.grove.o \
				 specs.hoa.o \
				 specs.highway.o \
				 specs.tharnadia.o \
				 specs.twintowers.o \
				 specs.undermountain.o \
				 specs.underworld.o \
				 specs.vecna.o \
				 specs.venthix.o \
				 specs.verzanan.o \
				 specs.zalrix.o \
				 specs.set.o \
				 specs.shabo.o \
				 specs.jubilex.o \
				 specs.keleks.o \
				 specs.caertannad.o \
				 specs.firep.o \
				 specs.winterhaven.o \
				 specs.zion.o \
				 spells.o \
				 redis.o \
				 wizredis.o \
				 sql.o	\
				 sql_player.o \
				 ssl.o \
				 statistics.o \
				 storage_lockers.o \
				 tether.o \
				 timers.o \
				 track.o \
				 tradeskill.o \
				 trap.o \
				 transport.o \
				 trophy.o \
				 unicode.o \
				 utility.o \
				 weather.o \
				 ferry.o \
				 ferryact.o \
				 paladins.o \
				 guard.o \
				 makeexit.o \
				 racewar_stat_mods.o \
				 nexus_stones.o \
				 testcmd.o \
				 wikihelp.o \
				 reavers.o \
				 avengers.o \
				 specs.lucrot.o \
				 specs.gellz.o \
				 smoke.o \
				 multiplay_whitelist.o \
				 home.o \
				 websocket.o \
				 ws_handlers.o \
				 json_utils.o \
				 gmcp.o \
				 ttype.o \
				 wiz_newchar.o

# Name lists (no paths) for pfile-related objs
SHARED_PFILE_OBJS_NAMES = pf-skills.o pf-files.o
PFILE_OBJS_NAMES        = pfile-stubs.o pfile.o

MIGRATE_OBJS=../src-migrate/migrate_main.o \
						 ../src-migrate/migrate_common.o \
						 ../src-migrate/migrate_objects.o \
						 ../src-migrate/migrate_accounts.o \
						 ../src-migrate/migrate_players.o \
						 ../src-migrate/migrate_lockers.o \
						 ../src-migrate/migrate_ships.o \
						 ../src-migrate/migrate_guilds.o \
						 ../src-migrate/migrate_misc.o \
						 ../src-migrate/migrate_stubs.o

#******************************************************************************
#******************************************************************************
#
# Don't mess around below this point.  Everything past here are makefile rules,
# and NOT dependency lists that need to be changed

PROGS 	= dms_new

# Build object paths under OBJDIR (preserve subdirs like ships/)
DMS_OBJS = $(addprefix $(OBJDIR)/,$(OBJS))
PFILE_OBJS        = $(addprefix $(OBJDIR)/,$(PFILE_OBJS_NAMES))
SHARED_PFILE_OBJS = $(addprefix $(OBJDIR)/,$(SHARED_PFILE_OBJS_NAMES))

ifndef USE_SHARED
	DMS_OBJS   += $(addprefix $(OBJDIR)/,$(SH_LIBS))
	PFILE_OBJS += $(addprefix $(OBJDIR)/,$(SH_LIBS))
else
	PROGS += $(SH_LIBS:.o=.so)
endif

all: $(PROGS)

dms_new : $(DMS_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -rdynamic -g -o dms_new $(DMS_OBJS) $(LIBS)
	chmod a+rx dms_new

pfile : $(PFILE_OBJS) $(SHARED_PFILE_OBJS) $(OBJDIR)/mm.o $(OBJDIR)/common.o
	$(CC) $(CFLAGS) $(LDFLAGS) -g -o pfile \
		$(PFILE_OBJS) $(SHARED_PFILE_OBJS) \
		$(OBJDIR)/common.o $(OBJDIR)/mm.o $(LIBS)

migrate_pfiles : $(MIGRATE_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -g -o migrate_pfiles $(MIGRATE_OBJS) $(LIBS)

# Build any C object into OBJDIR, creating folders as needed.
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) $(DEPFLAGS) -c $< -o $@

# Build any C++ object into OBJDIR, creating folders as needed.
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) $(DEPFLAGS) -c $< -o $@

$(MIGRATE_OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) -c $< -o $@

# Shared pfile objects (pf-*.o) also go into OBJDIR
$(OBJDIR)/pf-%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -D_PFILE_ $(INCLUDES) $(DEBUG) $(DEPFLAGS) -c $< -o $@

# Shared libs (if you ever use SH_LIBS / USE_SHARED)
$(SH_LIBS:.o=.so): %.so: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(DEBUG) -fpic -c $< -o $(OBJDIR)/$*.o
	ld -shared -o $@ /usr/lib/crtbeginS.o $(OBJDIR)/$*.o /usr/lib/crtendS.o

clean:
	rm -f *.o *~ dms_new pfile migrate_pfiles .depend
	rm -f ships/*.a ships/*.o ships/*~ ships/.depend
	rm -f ../src-migrate/*.o
	rm -rf $(OBJDIR)

debug: $(PROGS)
	cd ..; ./gdbdms

# Auto-generated dependency files alongside objects
DEPFILES := $(DMS_OBJS:.o=.d) $(PFILE_OBJS:.o=.d) $(SHARED_PFILE_OBJS:.o=.d)
-include $(DEPFILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.
