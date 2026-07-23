/*
 * gmcp.h - gmcp protocol for durismud
 *
 * sends game data to clients via telnet (mudlet/cmud) or websocket.
 * same data, different transport.
 *
 * mudlet/ire compatible packages:
 *   - Room.Info, Char.Vitals, Char.Status, Char.Affects
 *   - Combat.Update, Comm.Channel, Ship.Contacts
 */

#ifndef GMCP_H
#define GMCP_H

#include "structs.h"

/* forward declarations */
struct ShipData;

/* telnet codes (rfc 1572) */
#define TELNET_IAC  255
#define TELNET_SB   250 /* subnegotiation begin */
#define TELNET_SE   240 /* subnegotiation end */
#define TELNET_WILL 251
#define TELNET_WONT 252
#define TELNET_DO   253
#define TELNET_DONT 254
#define TELOPT_GMCP 201 /* gmcp option code */

/* gmcp package names */
#define GMCP_PKG_ROOM_INFO     "Room.Info"
#define GMCP_PKG_ROOM_MAP      "Room.Map"
#define GMCP_PKG_CHAR_VITALS   "Char.Vitals"
#define GMCP_PKG_CHAR_STATUS   "Char.Status"
#define GMCP_PKG_CHAR_AFFECTS  "Char.Affects"
#define GMCP_PKG_COMBAT_UPDATE "Combat.Update"
#define GMCP_PKG_COMM_CHANNEL  "Comm.Channel"
#define GMCP_PKG_CHAR_SKILLS   "Char.Skills"
#define GMCP_PKG_CHAR_ITEMS    "Char.Items"
#define GMCP_PKG_QUEST_STATUS  "Quest.Status"
#define GMCP_PKG_QUEST_MAP     "Quest.Map"
#define GMCP_PKG_GROUP_STATUS  "Group.Status"
#define GMCP_PKG_SHIP_CONTACTS "Ship.Contacts"
#define GMCP_PKG_SHIP_INFO     "Ship.Info"

#define GMCP_MAX_INPUT_SIZE (1024 * 1024)
#define GMCP_MAX_PACKAGE_SIZE 128

/* negotiation */

/* send gmcp will to start negotiation (called during connection setup) */
void gmcp_negotiate(struct descriptor_data *d);

/* handle gmcp do/dont response from client */
void gmcp_handle_negotiation(struct descriptor_data *d, int cmd);

/* parse incoming gmcp data from telnet client */
void gmcp_handle_input(struct descriptor_data *d, const char *data, size_t len);

/* core send - handles both telnet (iac sb gmcp) and websocket (json) */
void gmcp_send(struct descriptor_data *d, const char *package, const char *json);

/* room info - sends when character enters a room */
void gmcp_room_info(struct char_data *ch);

/* room map - sends ascii map for wilderness zones */
void gmcp_room_map(struct char_data *ch);

/* send pre-generated map buffer via gmcp (called from map.c) */
void gmcp_send_room_map(struct char_data *ch, const char *map_buf);

/* room update throttling - mark rooms dirty, flush periodically */
void gmcp_mark_room_dirty(int room_number);
void gmcp_flush_dirty_rooms(void);

/* ship contacts - periodically flush to gmcp players on bridge */
void gmcp_flush_dirty_ship_contacts(void);

/* ship info - static/slow-changing ship data, also flushed periodically */
void gmcp_flush_dirty_ship_info(void);

/* char vitals - sends hp/mana/move when they change */
void gmcp_char_vitals(struct char_data *ch);

/* send vitals to everyone in the group */
void gmcp_group_vitals(struct char_data *ch);

/* group status - sends member list with hp/move/position */
void gmcp_send_group_status(struct char_data *ch);

/* char status - sends level, class, race on login/level up */
void gmcp_char_status(struct char_data *ch);

/* char affects - sends active buffs/debuffs when they change */
void gmcp_char_affects(struct char_data *ch);

/* combat updates */
void gmcp_combat_update(struct char_data *ch, struct char_data *victim, int damage, const char *damage_type, int critical);

/* send target health update */
void gmcp_combat_target(struct char_data *ch, struct char_data *victim);

/* combat ended */
void gmcp_combat_end(struct char_data *ch);

/* channel messages (gossip, guild, tell, etc) */
void gmcp_comm_channel(struct char_data *ch, const char *channel, const char *sender, const char *text);

/* channel with alignment (for nchat) */
void gmcp_comm_channel_ex(struct char_data *ch, const char *channel, const char *sender, const char *text, const char *alignment);

/* quest status - sends bartender quest info */
void gmcp_quest_status(struct char_data *ch);
void gmcp_quest_map(struct char_data *ch);
void gmcp_send_quest_map(struct char_data *ch, const char *map_buf);

/* send to all who can hear the channel */
void gmcp_broadcast_channel(const char *channel, const char *sender, const char *text, struct char_data *exclude);

/* utility macros */

/* check if character has gmcp enabled (negotiated and not toggled off) */
#define GMCP_ENABLED(ch) ((ch) && (ch)->desc && (ch)->desc->gmcp_enabled && !PLR3_FLAGGED(ch, PLR3_NOGMCP))

/* check if descriptor has gmcp enabled */
#define DESC_GMCP_ENABLED(d) ((d) && (d)->gmcp_enabled)

/* send gmcp to character if enabled */
#define GMCP_TO_CHAR(ch, pkg, json)                                                                                                                                                                    \
	do                                                                                                                                                                                                 \
	{                                                                                                                                                                                                  \
		if (GMCP_ENABLED(ch))                                                                                                                                                                          \
			gmcp_send((ch)->desc, (pkg), (json));                                                                                                                                                      \
	} while (0)

#endif /* GMCP_H */
