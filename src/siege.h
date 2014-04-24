#include "prototypes.h"
#include "files.h"

void init_towns();

void explode_ammo( P_char ch, P_obj ammo );
bool is_siege( P_obj weapon );
bool is_loading_siege( P_obj siege );
void damage_siege( P_obj siege, P_obj ammo );
P_obj get_siege_room( P_char ch, char *arg );
void save_towns();
void apply_zone_modifier(P_char ch);
int castlewall( P_obj obj, P_char ch, int cmd, char *arg );
bool has_gates( int room );
int calculate_attacks(P_char ch, int attacks[]);
void multihit_siege( P_char ch );
void kill_siege( P_char ch, P_obj obj );

void add_siege( P_obj siege );
void remove_siege( P_obj siege );
void save_siege_list( );
void init_siege_list( );
void list_siege( P_char ch );

struct siege {
  P_obj obj;
  siege *next_siege;
};

typedef struct siege *P_siege;

