#include "common.h"
#include "sprites.h"
#include "pool.h"
#include "dungeon.h"

#include <body-4x4.png.h>
#include <head-6x6.png.h>
#include <larm-1x3.png.h>
#include <rarm-1x3.png.h>
#include <lleg-2x2.png.h>
#include <rleg-2x2.png.h>
#include <tiles-16x16.png.h>
#include <cover.png.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Register game info for menu carousel
sdk_game_info("puntog", &image_cover_png);

static Player p;
static GameState game_state = STATE_TITLE;
static GameState previous_state = STATE_TITLE;
static GameState pause_return_state = STATE_DUNGEON;
static int menu_selected = 0;
static int pause_selected = 0;
static int shop_selected = 0;
static int wardrobe_selected = 0;
static int info_page = 0;
static int tutorial_step = 0;
static float tutorial_timer = 0.0f;
static char lobby_hint[48] = { 0 };
static float global_timer = 0.0f;
static float autosave_timer = 0.0f;
static float screen_shake = 0.0f;
static int message_timer = 0;
static char message_text[32] = { 0 };

// BGM Music tracks & state
typedef enum {
	BGM_NONE = 0,
	BGM_TITLE,
	BGM_LOBBY,
	BGM_DUNGEON,
	BGM_TUTORIAL,
	BGM_GAMEOVER,
} BgmTrack;

static sdk_melody_t *current_bgm_handle = NULL;
static BgmTrack current_bgm_track = BGM_NONE;
static int music_enabled = 1;
static int music_volume = 3; // 0=25%, 1=50%, 2=75%, 3=100%

// Dynamic Palettes (216-color RGB spectrum)
static Palette4 pal_zombie_walker;
static Palette4 pal_zombie_runner;
static Palette4 pal_zombie_brute;
static Palette4 pal_zombie_burn;
static Palette4 pal_zombie_freeze;
static Palette4 pal_coin;
static Palette4 pal_potion;
static Palette4 pal_heart;
static Palette4 pal_spell_icons[NUM_SPELLS];

// Spells definition table
static const SpellDef spells[NUM_SPELLS] = {
	[SPELL_FIREBALL] = {
		.name = "FIREBALL",
		.aspect = "FIRE",
		.heat_cost = 22.0f,
		.damage = 34,
		.cooldown = 0.32f,
		.desc = "Blast & Burn",
	},
	[SPELL_ICE_DAGGER] = {
		.name = "ICE DAGGER",
		.aspect = "ICE",
		.heat_cost = 16.0f,
		.damage = 24,
		.cooldown = 0.26f,
		.desc = "Pierce & Freeze",
	},
	[SPELL_MAGIC_MISSILE] = {
		.name = "MISSILE",
		.aspect = "ARCANE",
		.heat_cost = 20.0f,
		.damage = 28,
		.cooldown = 0.30f,
		.desc = "Homing Bolt",
	},
	[SPELL_ARCANE_SHIELD] = {
		.name = "SHIELD",
		.aspect = "CHARM",
		.heat_cost = 32.0f,
		.damage = 20,
		.cooldown = 1.00f,
		.desc = "Deflect Barrier",
	},
	[SPELL_RUNE_TRAP] = {
		.name = "RUNE TRAP",
		.aspect = "EARTH",
		.heat_cost = 26.0f,
		.damage = 60,
		.cooldown = 0.75f,
		.desc = "Ground Hazard",
	},
};

// Character creation / Keypad
static const char keypad[4][4] = {
	{ 'A', 'B', 'C', 'D' },
	{ 'E', 'F', 'G', 'H' },
	{ 'I', 'J', 'K', 'L' },
	{ 'M', 'N', 'O', 'P' },
};
static const char keypad2[4][4] = {
	{ 'Q', 'R', 'S', 'T' },
	{ 'U', 'V', 'W', 'X' },
	{ 'Y', 'Z', ' ', ' ' },
	{ ' ', ' ', ' ', ' ' },
};
static int key_cur_x = 0;
static int key_cur_y = 0;
static int key_page = 0;

static void show_message(const char *msg, float dur_sec)
{
	snprintf(message_text, sizeof(message_text), "%s", msg);
	message_timer = (int)(dur_sec * 60.0f);
}

static void bgm_play(BgmTrack track)
{
	if (!music_enabled || track == BGM_NONE) {
		if (current_bgm_handle) {
			sdk_melody_stop_and_release(current_bgm_handle);
			current_bgm_handle = NULL;
		}
		current_bgm_track = BGM_NONE;
		return;
	}

	if (current_bgm_track == track && current_bgm_handle) {
		return;
	}

	if (current_bgm_handle) {
		sdk_melody_stop_and_release(current_bgm_handle);
		current_bgm_handle = NULL;
	}

	current_bgm_track = track;

	const char *vol_mark = "(p)";
	if (music_volume == 0) vol_mark = "(ppp)";
	else if (music_volume == 1) vol_mark = "(pp)";
	else if (music_volume == 2) vol_mark = "(p)";
	else if (music_volume == 3) vol_mark = "(mp)";

	static char mel_bufs[2][192];
	static int mel_buf_idx = 0;
	mel_buf_idx = 1 - mel_buf_idx;
	char *mel_buf = mel_bufs[mel_buf_idx];
	const char *body = NULL;

	switch (track) {
	case BGM_TITLE:
		body = "/bpm:100 /i:flute %s { < a c e < a > c e < b > d f < b > d f < a c e a c e g# c e g# c e }";
		break;
	case BGM_LOBBY:
		body = "/bpm:110 /i:string %s { c e g > c < g e d f a > d < a f c e g > c < g e < b > d g > d < g d }";
		break;
	case BGM_DUNGEON:
		body = "/bpm:135 /i:phi %s { < a a > c < a a > d < a a > e < a a > d < a a > c < g g > c < f f > c < e e > c }";
		break;
	case BGM_TUTORIAL:
		body = "/bpm:125 /i:square %s { c e g e d f a f e g > c < g d f a f }";
		break;
	case BGM_GAMEOVER:
		body = "/bpm:75 /i:sine %s < g# f d < a# < a----";
		break;
	default:
		return;
	}

	snprintf(mel_buf, sizeof(mel_bufs[0]), body, vol_mark);
	current_bgm_handle = sdk_melody_play_get(mel_buf);
}

static void bgm_update(void)
{
	if (!music_enabled) {
		if (current_bgm_handle) {
			sdk_melody_stop_and_release(current_bgm_handle);
			current_bgm_handle = NULL;
			current_bgm_track = BGM_NONE;
		}
		return;
	}

	BgmTrack target = BGM_NONE;
	switch (game_state) {
	case STATE_TITLE:
	case STATE_NAME_MENU:
	case STATE_WARDROBE:
	case STATE_INFO:
		target = BGM_TITLE;
		break;
	case STATE_LOBBY:
	case STATE_SHOP:
		target = BGM_LOBBY;
		break;
	case STATE_DUNGEON:
	case STATE_SPELL_MENU:
		target = BGM_DUNGEON;
		break;
	case STATE_TUTORIAL:
		target = BGM_TUTORIAL;
		break;
	case STATE_GAMEOVER:
		target = BGM_GAMEOVER;
		break;
	case STATE_PAUSE:
		if (pause_return_state == STATE_LOBBY) target = BGM_LOBBY;
		else if (pause_return_state == STATE_TUTORIAL) target = BGM_TUTORIAL;
		else target = BGM_DUNGEON;
		break;
	default:
		target = BGM_LOBBY;
		break;
	}

	if (target != current_bgm_track || !current_bgm_handle) {
		bgm_play(target);
	}
}

static void init_palettes(void)
{
	// Decayed olive green zombie with pale red eyes
	pal_zombie_walker = (Palette4){ .c = { 0, rgb216(0, 2, 0), rgb216(2, 4, 1), rgb216(5, 0, 0) } };
	// Crimson ghoul runner
	pal_zombie_runner = (Palette4){ .c = { 0, rgb216(3, 1, 0), rgb216(5, 2, 0), rgb216(5, 5, 1) } };
	// Armored iron brute
	pal_zombie_brute  = (Palette4){ .c = { 0, rgb216(1, 1, 2), rgb216(2, 3, 4), rgb216(5, 5, 5) } };
	// Burning zombie overlay palette
	pal_zombie_burn   = (Palette4){ .c = { 0, rgb216(2, 0, 0), rgb216(5, 2, 0), rgb216(5, 5, 0) } };
	// Frozen zombie overlay palette
	pal_zombie_freeze = (Palette4){ .c = { 0, rgb216(0, 1, 3), rgb216(1, 4, 5), rgb216(5, 5, 5) } };

	// Pickups
	pal_coin   = (Palette4){ .c = { 0, rgb216(2, 1, 0), rgb216(5, 4, 0), rgb216(5, 5, 2) } };
	pal_potion = (Palette4){ .c = { 0, rgb216(0, 2, 3), rgb216(0, 4, 5), rgb216(5, 5, 5) } }; // Glass vial with blue/cyan elixir
	pal_heart  = (Palette4){ .c = { 0, rgb216(2, 0, 0), rgb216(5, 0, 1), rgb216(5, 4, 4) } };

	// Spell Icons
	pal_spell_icons[0] = (Palette4){ .c = { 0, rgb216(3, 1, 0), rgb216(5, 2, 0), rgb216(5, 5, 1) } }; // Fire
	pal_spell_icons[1] = (Palette4){ .c = { 0, rgb216(0, 2, 4), rgb216(2, 4, 5), rgb216(5, 5, 5) } }; // Ice
	pal_spell_icons[2] = (Palette4){ .c = { 0, rgb216(2, 0, 3), rgb216(4, 1, 5), rgb216(5, 4, 5) } }; // Arcane
	pal_spell_icons[3] = (Palette4){ .c = { 0, rgb216(0, 2, 3), rgb216(2, 4, 4), rgb216(5, 5, 5) } }; // Shield
	pal_spell_icons[4] = (Palette4){ .c = { 0, rgb216(3, 2, 0), rgb216(4, 3, 1), rgb216(5, 5, 2) } }; // Trap
}

static void game_save(void)
{
	Save s = {
		.magic = SAVE_MAGIC,
		.head = p.head,
		.body = p.body,
		.Larm = p.Larm,
		.Rarm = p.Rarm,
		.Lleg = p.Lleg,
		.Rleg = p.Rleg,
		.set = p.nameset,
		.timespend = global_timer,
		.coins = p.coins,
		.max_hp = p.max_hp,
		.cooling_level = (int)((p.cooling_rate - 26.0f) / 5.0f),
		.spell_power = p.spell_power,
		.potions = p.potions,
		.highest_floor = current_floor,
		.music_enabled = music_enabled,
		.music_volume = music_volume,
	};
	memcpy(s.name, p.name, sizeof(s.name));
	sdk_save_write(0, &s, sizeof(s));
}

static void game_load(void)
{
	Save s;
	memset(&s, 0, sizeof(s));
	sdk_save_read(0, &s, sizeof(s));

	if (s.magic != SAVE_MAGIC) {
		p.head = 0;
		p.body = 0;
		p.Larm = 0;
		p.Rarm = 0;
		p.Lleg = 0;
		p.Rleg = 0;
		p.nameset = false;
		strcpy(p.name, "MAGE");
		p.coins = 0;
		p.max_hp = 100;
		p.hp = 100;
		p.cooling_rate = 26.0f;
		p.spell_power = 0;
		p.potions = 1;
		music_enabled = 1;
		music_volume = 3;
		return;
	}

	p.head = s.head % PLAYER_SPRITES;
	p.body = s.body % PLAYER_SPRITES;
	p.Larm = s.Larm % PLAYER_SPRITES;
	p.Rarm = s.Rarm % PLAYER_SPRITES;
	p.Lleg = s.Lleg % PLAYER_SPRITES;
	p.Rleg = s.Rleg % PLAYER_SPRITES;
	p.nameset = s.set;
	if (s.name[0] != 0 && (unsigned char)s.name[0] != 0xFF) {
		memcpy(p.name, s.name, sizeof(p.name));
	} else {
		strcpy(p.name, "MAGE");
	}

	p.coins = (s.coins >= 0 && s.coins < 99999) ? s.coins : 0;
	p.max_hp = (s.max_hp >= 60 && s.max_hp <= 300) ? s.max_hp : 100;
	p.hp = p.max_hp;

	int cool_lvl = (s.cooling_level >= 0 && s.cooling_level <= 10) ? s.cooling_level : 0;
	p.cooling_rate = 26.0f + cool_lvl * 5.0f;
	p.spell_power = (s.spell_power >= 0 && s.spell_power <= 20) ? s.spell_power : 0;
	p.potions = (s.potions >= 0 && s.potions <= 5) ? s.potions : 1;

	music_enabled = (s.music_enabled == 0) ? 0 : 1;
	music_volume = (s.music_volume >= 0 && s.music_volume <= 3) ? s.music_volume : 3;
}

static void player_reset(void)
{
	p.hp = p.max_hp;
	p.heat = 0.0f;
	p.overheated = false;
	p.selected_spell = SPELL_FIREBALL;
	p.spell_cooldown = 0.0f;
	p.dodge_timer = 0.0f;
	p.dodge_cd = 0.0f;
	p.shield_timer = 0.0f;
	p.invuln_timer = 0.0f;
	p.direction = 0;
	p.vx = 0.0f;
	p.vy = 0.0f;
	p.speed = 65.0f;
}

static void use_potion(void)
{
	if (p.potions <= 0) {
		show_message("NO POTIONS!", 1.0f);
		return;
	}
	p.potions--;
	p.heat = 0.0f;
	p.overheated = false;
	p.hp += 35;
	if (p.hp > p.max_hp) p.hp = p.max_hp;
	spawn_particles(p.x + 3, p.y + 6, 16, CYAN, 50.0f);
	sdk_melody_play("/i:square > g e c");
	show_message("ELIXIR RESTORED!", 1.5f);
}

static void cast_spell(void)
{
	if (p.overheated) {
		sdk_melody_play("/i:square < e c < a-");
		show_message("OVERHEATED!", 0.8f);
		return;
	}

	if (p.spell_cooldown > 0.0f)
		return;

	const SpellDef *sp = &spells[p.selected_spell];
	p.heat += sp->heat_cost;
	p.spell_cooldown = sp->cooldown;

	if (game_state == STATE_TUTORIAL && tutorial_step == 1) {
		tutorial_step = 2;
	}

	// Inverted Heat threshold check
	if (p.heat >= 100.0f) {
		p.heat = 100.0f;
		p.overheated = true;
		screen_shake = 0.25f;
		spawn_particles(p.x + 3, p.y + 6, 16, WHITE, 50.0f);
		sdk_melody_play("/i:noise c- /i:square < e d c");
		show_message("! SYSTEM OVERHEAT !", 1.8f);
	}

	float dir_x = 0, dir_y = 0;
	switch (p.direction) {
	case 0: dir_y = 1.0f; break;  // Down
	case 1: dir_x = -1.0f; break; // Left
	case 2: dir_x = 1.0f; break;  // Right
	case 3: dir_y = -1.0f; break; // Up
	}

	switch (p.selected_spell) {
	case SPELL_FIREBALL: {
		Projectile *proj = projectile_alloc();
		if (proj) {
			proj->x = p.x + 3.0f;
			proj->y = p.y + 6.0f;
			proj->vx = dir_x * 145.0f;
			proj->vy = dir_y * 145.0f;
			proj->spell_type = SPELL_FIREBALL;
			proj->damage = sp->damage + p.spell_power;
			proj->life = 1.8f;
			proj->pierces_left = 1;
			sdk_melody_play("/i:square > c e g");
		}
		break;
	}
	case SPELL_ICE_DAGGER: {
		Projectile *proj = projectile_alloc();
		if (proj) {
			proj->x = p.x + 3.0f;
			proj->y = p.y + 6.0f;
			proj->vx = dir_x * 210.0f;
			proj->vy = dir_y * 210.0f;
			proj->spell_type = SPELL_ICE_DAGGER;
			proj->damage = sp->damage + p.spell_power;
			proj->life = 1.2f;
			proj->pierces_left = 3;
			sdk_melody_play("/i:square > d f a");
		}
		break;
	}
	case SPELL_MAGIC_MISSILE: {
		Projectile *proj = projectile_alloc();
		if (proj) {
			proj->x = p.x + 3.0f;
			proj->y = p.y + 6.0f;
			proj->vx = dir_x * 125.0f;
			proj->vy = dir_y * 125.0f;
			proj->spell_type = SPELL_MAGIC_MISSILE;
			proj->damage = sp->damage + p.spell_power;
			proj->life = 2.5f;
			proj->pierces_left = 1;
			proj->homing_strength = 240.0f;
			sdk_melody_play("/i:sine > e a > c#");
		}
		break;
	}
	case SPELL_ARCANE_SHIELD: {
		p.shield_timer = 4.0f;
		spawn_particles(p.x + 3, p.y + 6, 12, CYAN, 40.0f);
		sdk_melody_play("/i:square > c g > c");
		show_message("SHIELD ACTIVE", 1.2f);
		break;
	}
	case SPELL_RUNE_TRAP: {
		Trap *tr = trap_spawn(p.x, p.y);
		if (tr) {
			spawn_particles(p.x + 3, p.y + 6, 8, YELLOW, 30.0f);
			sdk_melody_play("/i:sine > c d e");
		}
		break;
	}
	default:
		break;
	}
}

static void dodge_roll(void)
{
	if (p.dodge_cd > 0.0f || p.dodge_timer > 0.0f)
		return;

	float dx = 0, dy = 0;
	if (fabsf(p.vx) > 1.0f || fabsf(p.vy) > 1.0f) {
		float len = sqrtf(p.vx * p.vx + p.vy * p.vy);
		dx = p.vx / len;
		dy = p.vy / len;
	} else {
		switch (p.direction) {
		case 0: dy = 1.0f; break;
		case 1: dx = -1.0f; break;
		case 2: dx = 1.0f; break;
		case 3: dy = -1.0f; break;
		}
	}

	p.dodge_timer = 0.22f;
	p.dodge_cd = 0.50f;
	p.dodge_vx = dx * 140.0f;
	p.dodge_vy = dy * 140.0f;
	p.invuln_timer = 0.26f;

	if (game_state == STATE_TUTORIAL && tutorial_step == 4) {
		tutorial_step = 5;
	}

	spawn_particles(p.x + 3, p.y + 10, 6, LIGHT_GRAY, 25.0f);
	sdk_melody_play("/i:noise c");
}

void game_start(void)
{
	init_palettes();
	pools_init();
	game_load();

	// Boot up to Title Menu!
	game_state = STATE_TITLE;
	menu_selected = 0;
	bgm_update();

#if RUNNING_ON_HOST
	const char *start_env = getenv("PUNTOG_START");
	if (start_env) {
		if (strcmp(start_env, "lobby") == 0) {
			dungeon_init_lobby(&p);
			player_reset();
			game_state = STATE_LOBBY;
		} else if (strcmp(start_env, "tutorial") == 0) {
			dungeon_init_tutorial(&p);
			player_reset();
			game_state = STATE_TUTORIAL;
		} else if (strcmp(start_env, "dungeon") == 0) {
			dungeon_generate_floor(1, &p);
			player_reset();
			game_state = STATE_DUNGEON;
		} else if (strcmp(start_env, "pause") == 0) {
			dungeon_init_lobby(&p);
			player_reset();
			game_state = STATE_PAUSE;
			pause_return_state = STATE_LOBBY;
		}
		bgm_update();
	}
#endif
}

static void update_projectiles(float dt)
{
	for (int i = 0; i < active_projectile_count; i++) {
		Projectile *proj = active_projectiles[i];
		proj->life -= dt;
		if (proj->life <= 0) {
			projectile_free(proj);
			i--;
			continue;
		}

		// Homing logic for Magic Missile
		if (proj->spell_type == SPELL_MAGIC_MISSILE && active_zombie_count > 0) {
			Zombie *best_z = NULL;
			float min_dist_sq = 220.0f * 220.0f;
			for (int j = 0; j < active_zombie_count; j++) {
				Zombie *z = active_zombies[j];
				float dx = (z->x + 4) - proj->x;
				float dy = (z->y + 6) - proj->y;
				float dist_sq = dx * dx + dy * dy;
				if (dist_sq < min_dist_sq) {
					min_dist_sq = dist_sq;
					best_z = z;
				}
			}

			if (best_z) {
				float dx = (best_z->x + 4) - proj->x;
				float dy = (best_z->y + 6) - proj->y;
				float len = sqrtf(dx * dx + dy * dy);
				if (len > 1.0f) {
					float target_vx = (dx / len) * 135.0f;
					float target_vy = (dy / len) * 135.0f;
					proj->vx += (target_vx - proj->vx) * 7.0f * dt;
					proj->vy += (target_vy - proj->vy) * 7.0f * dt;
				}
			}
		}

		proj->x += proj->vx * dt;
		proj->y += proj->vy * dt;

		// Trail particles
		if (proj->spell_type == SPELL_FIREBALL && (rand() % 2 == 0)) {
			spawn_particles(proj->x, proj->y, 1, ORANGE, 15.0f);
		} else if (proj->spell_type == SPELL_ICE_DAGGER && (rand() % 3 == 0)) {
			spawn_particles(proj->x, proj->y, 1, CYAN, 12.0f);
		} else if (proj->spell_type == SPELL_MAGIC_MISSILE && (rand() % 2 == 0)) {
			spawn_particles(proj->x, proj->y, 1, MAGENTA, 15.0f);
		}

		// Wall collision
		if (dungeon_check_collision(proj->x, proj->y, 3.0f)) {
			if (proj->spell_type == SPELL_FIREBALL) {
				spawn_particles(proj->x, proj->y, 8, ORANGE, 35.0f);
			}
			projectile_free(proj);
			i--;
			continue;
		}

		// Zombie collision check
		bool consumed = false;
		for (int j = 0; j < active_zombie_count; j++) {
			Zombie *z = active_zombies[j];
			float dx = (z->x + 4) - proj->x;
			float dy = (z->y + 6) - proj->y;
			if (dx * dx + dy * dy < 9.0f * 9.0f) {
				z->hp -= proj->damage;
				z->knockback_x = (proj->vx > 0 ? 1 : -1) * 24.0f;
				z->knockback_y = (proj->vy > 0 ? 1 : -1) * 24.0f;

				// Apply status effects
				if (proj->spell_type == SPELL_FIREBALL) {
					z->burn_timer = 2.0f;
					spawn_particles(z->x + 4, z->y + 6, 10, ORANGE, 40.0f);
					// Area splash damage
					for (int k = 0; k < active_zombie_count; k++) {
						if (k == j) continue;
						Zombie *other = active_zombies[k];
						float sx = other->x - z->x;
						float sy = other->y - z->y;
						if (sx * sx + sy * sy < 22.0f * 22.0f) {
							other->hp -= proj->damage / 2;
							other->burn_timer = 1.5f;
						}
					}
				} else if (proj->spell_type == SPELL_ICE_DAGGER) {
					z->freeze_timer = 2.5f;
					spawn_particles(z->x + 4, z->y + 6, 8, CYAN, 35.0f);
				} else if (proj->spell_type == SPELL_MAGIC_MISSILE) {
					spawn_particles(z->x + 4, z->y + 6, 8, MAGENTA, 35.0f);
				}

				sdk_melody_play("/i:square < a");

				if (z->hp <= 0) {
					if (z->type == 3) {
						z->hp = 9999;
					} else {
						spawn_particles(z->x + 4, z->y + 6, 12, GREEN, 40.0f);
						int roll = rand() % 100;
						if (roll < 65) {
							pickup_spawn(z->x, z->y, 0, 1 + (rand() % 3) + current_floor);
						} else if (roll < 82) {
							pickup_spawn(z->x, z->y, 2, 25); // Heart
						} else if (roll < 95) {
							pickup_spawn(z->x, z->y, 1, 1); // Potion
						}
						zombie_free(z);
						j--;
					}
				}

				if (z->type == 3 && game_state == STATE_TUTORIAL && tutorial_step == 2) {
					tutorial_step = 3;
					tutorial_timer = 0.0f;
					sdk_melody_play("/i:square > c e g");
				}

				proj->pierces_left--;
				if (proj->pierces_left <= 0) {
					consumed = true;
					break;
				}
			}
		}

		if (consumed) {
			projectile_free(proj);
			i--;
		}
	}
}

static void update_zombies(float dt)
{
	for (int i = 0; i < active_zombie_count; i++) {
		Zombie *z = active_zombies[i];

		// Status: Burning
		if (z->burn_timer > 0.0f) {
			z->burn_timer -= dt;
			z->burn_tick += dt;
			if (z->burn_tick >= 0.4f) {
				z->burn_tick = 0.0f;
				z->hp -= 8;
				spawn_particles(z->x + 4, z->y + 4, 2, ORANGE, 15.0f);
				if (z->hp <= 0) {
					spawn_particles(z->x + 4, z->y + 6, 8, RED, 30.0f);
					pickup_spawn(z->x, z->y, 0, 2);
					zombie_free(z);
					i--;
					continue;
				}
			}
		}

		// Status: Freezing
		float spd_factor = 1.0f;
		if (z->freeze_timer > 0.0f) {
			z->freeze_timer -= dt;
			spd_factor = 0.30f;
			if (rand() % 4 == 0) {
				spawn_particles(z->x + 4, z->y + 6, 1, CYAN, 10.0f);
			}
		}

		// Knockback decay
		z->knockback_x *= 0.82f;
		z->knockback_y *= 0.82f;

		// Steering pursuit towards player
		float dx = p.x - z->x;
		float dy = p.y - z->y;
		float dist = sqrtf(dx * dx + dy * dy);

		float move_x = 0, move_y = 0;
		if (dist > 2.0f && z->type != 3) {
			move_x = (dx / dist) * z->speed * spd_factor;
			move_y = (dy / dist) * z->speed * spd_factor;
		}

		float total_vx = move_x + z->knockback_x;
		float total_vy = move_y + z->knockback_y;

		float new_x = z->x + total_vx * dt;
		float new_y = z->y + total_vy * dt;

		if (!dungeon_check_collision(new_x, z->y, 4.0f)) {
			z->x = new_x;
		}
		if (!dungeon_check_collision(z->x, new_y, 4.0f)) {
			z->y = new_y;
		}

		// Walk animation
		z->anim_timer += dt;
		if (z->anim_timer > 0.20f) {
			z->anim_timer = 0.0f;
			z->anim_frame = 1 - z->anim_frame;
		}

		// Contact damage with player
		if (dist < 8.0f && z->type != 3) {
			if (p.shield_timer > 0.0f) {
				z->hp -= 20;
				z->knockback_x = -(dx / dist) * 35.0f;
				z->knockback_y = -(dy / dist) * 35.0f;
				spawn_particles(p.x + 3, p.y + 6, 8, CYAN, 35.0f);
				sdk_melody_play("/i:square > c > c");
			} else if (p.invuln_timer <= 0.0f && p.dodge_timer <= 0.0f) {
				int dmg = (z->type == 2) ? 18 : 12;
				p.hp -= dmg;
				p.invuln_timer = 0.50f;
				screen_shake = 0.15f;
				spawn_particles(p.x + 3, p.y + 6, 8, RED, 30.0f);
				sdk_melody_play("/i:square < f d# c-");

				p.vx = (dx / dist) * 40.0f;
				p.vy = (dy / dist) * 40.0f;

				if (p.hp <= 0) {
					p.hp = 0;
					game_state = STATE_GAMEOVER;
					sdk_melody_play("/i:square < c- < g- < c--");
				}
			}
		}
	}
}

static void update_traps(float dt)
{
	for (int i = 0; i < active_trap_count; i++) {
		Trap *tr = &active_traps[i];
		if (!tr->armed) {
			tr->arm_timer -= dt;
			if (tr->arm_timer <= 0) {
				tr->armed = true;
			}
			continue;
		}

		bool triggered = false;
		for (int j = 0; j < active_zombie_count; j++) {
			Zombie *z = active_zombies[j];
			float dx = z->x - tr->x;
			float dy = z->y - tr->y;
			if (dx * dx + dy * dy < 14.0f * 14.0f) {
				triggered = true;
				break;
			}
		}

		if (triggered) {
			spawn_particles(tr->x + 4, tr->y + 4, 18, YELLOW, 50.0f);
			spawn_particles(tr->x + 4, tr->y + 4, 10, ORANGE, 35.0f);
			screen_shake = 0.20f;
			sdk_melody_play("/i:noise c-");

			for (int j = 0; j < active_zombie_count; j++) {
				Zombie *z = active_zombies[j];
				float dx = z->x - tr->x;
				float dy = z->y - tr->y;
				float dist_sq = dx * dx + dy * dy;
				if (dist_sq < 24.0f * 24.0f) {
					z->hp -= 60;
					z->knockback_x = (dx > 0 ? 1 : -1) * 35.0f;
					z->knockback_y = (dy > 0 ? 1 : -1) * 35.0f;
					if (z->hp <= 0) {
						pickup_spawn(z->x, z->y, 0, 2);
						zombie_free(z);
						j--;
					}
				}
			}
			trap_remove(i);
			i--;
		}
	}
}

static void update_pickups(float dt)
{
	for (int i = 0; i < active_pickup_count; i++) {
		Pickup *pk = &active_pickups[i];
		pk->life -= dt;
		if (pk->life <= 0) {
			pickup_remove(i);
			i--;
			continue;
		}

		float dx = (p.x + 3) - pk->x;
		float dy = (p.y + 6) - pk->y;
		if (dx * dx + dy * dy < 10.0f * 10.0f) {
			if (pk->type == 0) { // Coin
				p.coins += pk->value;
				sdk_melody_play("/i:sine > e g C-");
				spawn_particles(pk->x + 3, pk->y + 3, 4, YELLOW, 20.0f);
			} else if (pk->type == 1) { // Potion
				if (p.potions < 5) p.potions++;
				sdk_melody_play("/i:square > g e c");
				show_message("+1 ELIXIR!", 1.2f);
			} else if (pk->type == 2) { // Heart
				p.hp += pk->value;
				if (p.hp > p.max_hp) p.hp = p.max_hp;
				sdk_melody_play("/i:sine > c e g");
			}
			pickup_remove(i);
			i--;
		}
	}
}

static void update_particles(float dt)
{
	for (int i = 0; i < active_particle_count; i++) {
		Particle *pt = active_particles[i];
		pt->life -= dt;
		if (pt->life <= 0) {
			particle_free(pt);
			i--;
			continue;
		}
		pt->x += pt->vx * dt;
		pt->y += pt->vy * dt;
		pt->vx *= 0.94f;
		pt->vy *= 0.94f;
	}
}

static void check_tile_interactions(void)
{
	int tx = (int)((p.x + 3.0f) / TILE_SIZE);
	int ty = (int)((p.y + 8.0f) / TILE_SIZE);

	if (tx < 0 || tx >= cur_map_cols || ty < 0 || ty >= cur_map_rows)
		return;

	uint8_t t = cur_map[ty][tx];

	// Dungeon Exit Ladder -> Next Floor!
	if (t == TILE_LADDER && game_state == STATE_DUNGEON) {
		current_floor++;
		p.coins += 25 + current_floor * 10;
		dungeon_generate_floor(current_floor, &p);
		player_reset();
		sdk_melody_play("/i:square > c e g > c-");
		char buf[32];
		snprintf(buf, sizeof(buf), "FLOOR %d DESCENDED!", current_floor);
		show_message(buf, 2.0f);
		return;
	}

	// Hazard Lava
	if (t == TILE_LAVA_0 || t == TILE_LAVA_1) {
		if (p.invuln_timer <= 0.0f) {
			p.hp -= 4;
			p.invuln_timer = 0.35f;
			spawn_particles(p.x + 3, p.y + 6, 4, ORANGE, 25.0f);
			if (p.hp <= 0) {
				p.hp = 0;
				game_state = STATE_GAMEOVER;
			}
		}
	}
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;
	if (dt > 0.05f) dt = 0.05f;
	global_timer += dt;

	if (message_timer > 0) message_timer--;
	if (screen_shake > 0.0f) screen_shake -= dt;

	autosave_timer += dt;
	if (autosave_timer > AUTOSAVE_TIME) {
		autosave_timer = 0.0f;
		game_save();
	}

	bgm_update();

	if (game_state == STATE_TITLE) {
		if (sdk_inputs_delta.vertical == 1) {
			menu_selected = (menu_selected + 1) % 6;
			sdk_melody_play("/i:square > g");
		}
		if (sdk_inputs_delta.vertical == -1) {
			menu_selected = (menu_selected + 5) % 6;
			sdk_melody_play("/i:square > g");
		}
		if (sdk_inputs_delta.select == 1) {
			music_enabled = !music_enabled;
			bgm_update();
			game_save();
			sdk_melody_play("/i:square > g");
		}
		if (sdk_inputs_delta.a == 1 || sdk_inputs_delta.start == 1) {
			sdk_melody_play("/i:square > c e g");
			previous_state = STATE_TITLE;
			if (menu_selected == 0) {
				dungeon_init_lobby(&p);
				player_reset();
				game_state = STATE_LOBBY;
				bgm_update();
				show_message("SANCTUARY LOBBY", 2.0f);
			} else if (menu_selected == 1) {
				dungeon_generate_floor(1, &p);
				player_reset();
				game_state = STATE_DUNGEON;
				bgm_update();
				show_message("DUNGEON FLOOR 1", 2.0f);
			} else if (menu_selected == 2) {
				dungeon_init_tutorial(&p);
				player_reset();
				tutorial_step = 0;
				tutorial_timer = 0.0f;
				game_state = STATE_TUTORIAL;
				bgm_update();
				show_message("TRAINING GROUNDS", 2.0f);
			} else if (menu_selected == 3) {
				info_page = 0;
				game_state = STATE_INFO;
			} else if (menu_selected == 4) {
				wardrobe_selected = 0;
				game_state = STATE_WARDROBE;
			} else if (menu_selected == 5) {
				music_enabled = !music_enabled;
				bgm_update();
				game_save();
			}
		}
		return;
	}

	if (game_state == STATE_PAUSE) {
		if (sdk_inputs_delta.vertical == 1) {
			pause_selected = (pause_selected + 1) % 7;
			sdk_melody_play("/i:square > g");
		}
		if (sdk_inputs_delta.vertical == -1) {
			pause_selected = (pause_selected + 6) % 7;
			sdk_melody_play("/i:square > g");
		}
		if (sdk_inputs_delta.start == 1 || sdk_inputs_delta.b == 1) {
			game_state = pause_return_state;
			return;
		}

		if (pause_selected == 2) {
			if (sdk_inputs_delta.horizontal == 1) {
				if (!music_enabled) {
					music_enabled = 1;
					music_volume = 0;
				} else if (music_volume < 3) {
					music_volume++;
				}
				bgm_play(current_bgm_track);
				game_save();
				sdk_melody_play("/i:square > g");
			} else if (sdk_inputs_delta.horizontal == -1) {
				if (music_volume > 0) {
					music_volume--;
				} else {
					music_enabled = 0;
				}
				bgm_play(current_bgm_track);
				game_save();
				sdk_melody_play("/i:square > e");
			}
		}

		if (sdk_inputs_delta.a == 1) {
			sdk_melody_play("/i:square > c e");
			if (pause_selected == 0) {
				game_state = pause_return_state;
			} else if (pause_selected == 1) {
				game_save();
				sdk_melody_play("/i:square > c e g > c");
				show_message("GAME SAVED!", 1.8f);
			} else if (pause_selected == 2) {
				music_enabled = !music_enabled;
				bgm_update();
				game_save();
				sdk_melody_play("/i:square > g");
			} else if (pause_selected == 3) {
				previous_state = STATE_PAUSE;
				info_page = 0;
				game_state = STATE_INFO;
			} else if (pause_selected == 4) {
				game_state = STATE_SPELL_MENU;
			} else if (pause_selected == 5) {
				dungeon_init_lobby(&p);
				player_reset();
				game_save();
				game_state = STATE_LOBBY;
				bgm_update();
				show_message("SANCTUARY LOBBY", 2.0f);
			} else if (pause_selected == 6) {
				game_save();
				game_state = STATE_TITLE;
				menu_selected = 0;
				bgm_update();
			}
		}
		return;
	}

	if (game_state == STATE_INFO) {
		if (sdk_inputs_delta.horizontal == 1) {
			if (info_page < 3) {
				info_page++;
				sdk_melody_play("/i:square > c");
			}
		}
		if (sdk_inputs_delta.horizontal == -1) {
			if (info_page > 0) {
				info_page--;
				sdk_melody_play("/i:square > c");
			}
		}
		if (sdk_inputs_delta.b == 1 || sdk_inputs_delta.a == 1 || sdk_inputs_delta.start == 1) {
			game_state = previous_state;
		}
		return;
	}

	if (game_state == STATE_SHOP) {
		if (sdk_inputs_delta.vertical == 1) {
			shop_selected = (shop_selected + 1) % 5;
			sdk_melody_play("/i:square > g");
		}
		if (sdk_inputs_delta.vertical == -1) {
			shop_selected = (shop_selected + 4) % 5;
			sdk_melody_play("/i:square > g");
		}
		if (sdk_inputs_delta.b == 1) {
			game_state = STATE_LOBBY;
			return;
		}

		int cost_hp = 50 + (p.max_hp - 100) * 3;
		int cool_lvl = (int)((p.cooling_rate - 26.0f) / 5.0f);
		int cost_cool = 40 + cool_lvl * 35;
		int cost_pwr = 60 + p.spell_power * 40;
		int cost_pot = 25;

		if (sdk_inputs_delta.a == 1) {
			if (shop_selected == 0 && p.coins >= cost_hp && p.max_hp < 260) {
				p.coins -= cost_hp;
				p.max_hp += 20;
				p.hp = p.max_hp;
				sdk_melody_play("/i:square > c e g > c");
				show_message("MAX HEALTH UPGRADED!", 1.5f);
				game_save();
			} else if (shop_selected == 1 && p.coins >= cost_cool && p.cooling_rate < 65.0f) {
				p.coins -= cost_cool;
				p.cooling_rate += 5.0f;
				sdk_melody_play("/i:square > c e g > c");
				show_message("COOLING RATE UPGRADED!", 1.5f);
				game_save();
			} else if (shop_selected == 2 && p.coins >= cost_pwr && p.spell_power < 15) {
				p.coins -= cost_pwr;
				p.spell_power += 2;
				sdk_melody_play("/i:square > c e g > c");
				show_message("SPELL POWER UPGRADED!", 1.5f);
				game_save();
			} else if (shop_selected == 3 && p.coins >= cost_pot && p.potions < 5) {
				p.coins -= cost_pot;
				p.potions++;
				sdk_melody_play("/i:square > g > c");
				show_message("PURCHASED MAGIC ELIXIR!", 1.5f);
				game_save();
			} else if (shop_selected == 4) {
				game_state = STATE_LOBBY;
			} else {
				sdk_melody_play("/i:square < f < d#");
			}
		}
		return;
	}

	if (game_state == STATE_WARDROBE) {
		if (sdk_inputs_delta.vertical == 1) {
			wardrobe_selected = (wardrobe_selected + 1) % 5;
			sdk_melody_play("/i:square > g");
		}
		if (sdk_inputs_delta.vertical == -1) {
			wardrobe_selected = (wardrobe_selected + 4) % 5;
			sdk_melody_play("/i:square > g");
		}

		if (sdk_inputs_delta.horizontal == 1) {
			if (wardrobe_selected == 0) p.head = (p.head + 1) % PLAYER_SPRITES;
			if (wardrobe_selected == 1) p.body = (p.body + 1) % PLAYER_SPRITES;
			if (wardrobe_selected == 2) { p.Larm = (p.Larm + 1) % PLAYER_SPRITES; p.Rarm = p.Larm; }
			if (wardrobe_selected == 3) { p.Lleg = (p.Lleg + 1) % PLAYER_SPRITES; p.Rleg = p.Lleg; }
			sdk_melody_play("/i:square > e");
		}
		if (sdk_inputs_delta.horizontal == -1) {
			if (wardrobe_selected == 0) p.head = (p.head + PLAYER_SPRITES - 1) % PLAYER_SPRITES;
			if (wardrobe_selected == 1) p.body = (p.body + PLAYER_SPRITES - 1) % PLAYER_SPRITES;
			if (wardrobe_selected == 2) { p.Larm = (p.Larm + PLAYER_SPRITES - 1) % PLAYER_SPRITES; p.Rarm = p.Larm; }
			if (wardrobe_selected == 3) { p.Lleg = (p.Lleg + PLAYER_SPRITES - 1) % PLAYER_SPRITES; p.Rleg = p.Lleg; }
			sdk_melody_play("/i:square > e");
		}

		if (sdk_inputs_delta.a == 1 || sdk_inputs_delta.b == 1 || sdk_inputs_delta.start == 1) {
			game_save();
			game_state = (previous_state == STATE_TITLE) ? STATE_TITLE : STATE_LOBBY;
		}
		return;
	}

	if (game_state == STATE_NAME_MENU) {
		if (sdk_inputs_delta.horizontal == -1 && key_cur_x > 0) key_cur_x--;
		if (sdk_inputs_delta.horizontal == 1 && key_cur_x < 3) key_cur_x++;
		if (sdk_inputs_delta.vertical == -1 && key_cur_y > 0) key_cur_y--;
		if (sdk_inputs_delta.vertical == 1 && key_cur_y < 3) key_cur_y++;

		if (sdk_inputs_delta.select == 1) {
			key_page = !key_page;
		}

		char s = (key_page ? keypad2 : keypad)[key_cur_y][key_cur_x];
		int nlen = strlen(p.name);

		if (sdk_inputs_delta.a == 1 && nlen < 14) {
			if (s >= 'A' && s <= 'Z') {
				p.name[nlen] = s;
				p.name[nlen + 1] = 0;
			}
		}
		if (sdk_inputs_delta.b == 1 && nlen > 0) {
			p.name[nlen - 1] = 0;
		}
		if (sdk_inputs_delta.start == 1 && nlen > 0) {
			p.nameset = true;
			dungeon_init_lobby(&p);
			player_reset();
			game_save();
			game_state = STATE_LOBBY;
		}
		return;
	}

	if (game_state == STATE_GAMEOVER) {
		if (sdk_inputs_delta.start == 1 || sdk_inputs_delta.a == 1) {
			dungeon_init_lobby(&p);
			player_reset();
			game_state = STATE_LOBBY;
			show_message("SANCTUARY LOBBY", 2.0f);
		}
		return;
	}

	if (game_state == STATE_SPELL_MENU) {
		if (sdk_inputs_delta.vertical == 1)
			p.selected_spell = (SpellType)((p.selected_spell + 1) % NUM_SPELLS);
		if (sdk_inputs_delta.vertical == -1)
			p.selected_spell = (SpellType)((p.selected_spell + NUM_SPELLS - 1) % NUM_SPELLS);

		if (sdk_inputs_delta.select == 1 || sdk_inputs_delta.a == 1 || sdk_inputs_delta.b == 1) {
			game_state = (pause_return_state == STATE_LOBBY) ? STATE_LOBBY : STATE_DUNGEON;
		}
		return;
	}

	// In-game Input (DUNGEON, LOBBY, TUTORIAL)
	if (sdk_inputs_delta.start == 1) {
		pause_return_state = game_state;
		game_state = STATE_PAUSE;
		pause_selected = 0;
		sdk_melody_play("/i:square > c e");
		return;
	}

	if (sdk_inputs_delta.select == 1) {
		pause_return_state = game_state;
		game_state = STATE_SPELL_MENU;
		return;
	}

	// Spell cycle buttons
	if (sdk_inputs_delta.x == 1) {
		p.selected_spell = (SpellType)((p.selected_spell + 1) % NUM_SPELLS);
		if (game_state == STATE_TUTORIAL && tutorial_step == 5) {
			tutorial_step = 6;
		}
	}
	// Consume Potion
	if (sdk_inputs_delta.y == 1) {
		use_potion();
		if (game_state == STATE_TUTORIAL && tutorial_step == 6) {
			tutorial_step = 7;
		}
	}

	// In Lobby: Check nearby interactive props
	if (game_state == STATE_LOBBY) {
		int ptx = (int)((p.x + 3.0f) / TILE_SIZE);
		int pty = (int)((p.y + 6.0f) / TILE_SIZE);
		uint8_t nearby = EMPTY;
		if (pty >= 0 && pty < cur_map_rows && ptx >= 0 && ptx < cur_map_cols &&
		    cur_map[pty][ptx] != EMPTY && cur_map[pty][ptx] != TILE_FLOOR) {
			nearby = cur_map[pty][ptx];
		} else {
			for (int dy = -1; dy <= 1; dy++) {
				for (int dx = -1; dx <= 1; dx++) {
					int nx = ptx + dx;
					int ny = pty + dy;
					if (nx >= 0 && nx < cur_map_cols && ny >= 0 && ny < cur_map_rows) {
						uint8_t t = cur_map[ny][nx];
						if (t == TILE_PORTAL || t == TILE_LADDER || t == TILE_SHOP_ALTAR ||
						    t == TILE_WARDROBE || t == TILE_CHEST_CLOSED) {
							nearby = t;
							break;
						}
					}
				}
				if (nearby != EMPTY) break;
			}
		}

		if (nearby == TILE_PORTAL) {
			snprintf(lobby_hint, sizeof(lobby_hint), "[A] ENTER DUNGEON");
			if (sdk_inputs_delta.a == 1) {
				dungeon_generate_floor(1, &p);
				player_reset();
				game_state = STATE_DUNGEON;
				show_message("DUNGEON FLOOR 1", 2.0f);
				return;
			}
		} else if (nearby == TILE_LADDER) {
			snprintf(lobby_hint, sizeof(lobby_hint), "[A] ENTER TUTORIAL");
			if (sdk_inputs_delta.a == 1) {
				dungeon_init_tutorial(&p);
				player_reset();
				tutorial_step = 0;
				tutorial_timer = 0.0f;
				game_state = STATE_TUTORIAL;
				show_message("TRAINING GROUNDS", 2.0f);
				return;
			}
		} else if (nearby == TILE_SHOP_ALTAR) {
			snprintf(lobby_hint, sizeof(lobby_hint), "[A] ARCANE ALTAR");
			if (sdk_inputs_delta.a == 1) {
				game_state = STATE_SHOP;
				shop_selected = 0;
				return;
			}
		} else if (nearby == TILE_WARDROBE) {
			snprintf(lobby_hint, sizeof(lobby_hint), "[A] WARDROBE");
			if (sdk_inputs_delta.a == 1) {
				previous_state = STATE_LOBBY;
				game_state = STATE_WARDROBE;
				wardrobe_selected = 0;
				return;
			}
		} else if (nearby == TILE_CHEST_CLOSED) {
			snprintf(lobby_hint, sizeof(lobby_hint), "[A] ARCHIVES & INFO");
			if (sdk_inputs_delta.a == 1) {
				previous_state = STATE_LOBBY;
				info_page = 0;
				game_state = STATE_INFO;
				return;
			}
		} else {
			lobby_hint[0] = 0;
		}
	}

	// In Tutorial: Progress tracking
	if (game_state == STATE_TUTORIAL) {
		tutorial_timer += dt;
		if (tutorial_step == 0) {
			static float stx = 0, sty = 0;
			if (tutorial_timer < 0.1f) { stx = p.x; sty = p.y; }
			if (fabsf(p.x - stx) + fabsf(p.y - sty) > 25.0f) {
				tutorial_step = 1;
				sdk_melody_play("/i:square > c e");
			}
		}
		if (tutorial_step == 3 && (tutorial_timer > 3.0f || p.heat > 25.0f)) {
			tutorial_step = 4;
		}

		int ptx = (int)((p.x + 3.0f) / TILE_SIZE);
		int pty = (int)((p.y + 6.0f) / TILE_SIZE);
		if (pty >= 0 && pty < cur_map_rows && ptx >= 0 && ptx < cur_map_cols) {
			if (tutorial_step >= 7 && cur_map[pty][ptx] == TILE_PORTAL) {
				p.coins += 50;
				sdk_melody_play("/i:square > c e g > c");
				dungeon_init_lobby(&p);
				player_reset();
				game_save();
				game_state = STATE_LOBBY;
				show_message("+50 GOLD! WELCOME TO LOBBY", 2.5f);
				return;
			}
		}
	}

	// Cast Spell (A button)
	if (sdk_inputs_delta.a == 1) {
		cast_spell();
	}

	// Dodge Roll (B button)
	if (sdk_inputs_delta.b == 1) {
		// Check if near closed chest to open it
		int tx = (int)((p.x + 3.0f) / TILE_SIZE);
		int ty = (int)((p.y + 8.0f) / TILE_SIZE);
		bool opened = false;
		for (int dy = -1; dy <= 1; dy++) {
			for (int dx = -1; dx <= 1; dx++) {
				if (cur_map[ty + dy][tx + dx] == TILE_CHEST_CLOSED && game_state == STATE_DUNGEON) {
					cur_map[ty + dy][tx + dx] = TILE_CHEST_OPEN;
					p.coins += 25 + rand() % 20;
					p.potions++;
					spawn_particles((tx + dx) * TILE_SIZE + 8, (ty + dy) * TILE_SIZE + 8, 16, YELLOW, 45.0f);
					sdk_melody_play("/i:sine > e g > c");
					show_message("CHEST: GOLD + ELIXIR!", 1.5f);
					opened = true;
					break;
				}
			}
			if (opened) break;
		}
		if (!opened) {
			dodge_roll();
		}
	}

	// Continuous Heat Dissipation
	if (p.heat > 0.0f) {
		p.heat -= p.cooling_rate * dt;
		if (p.heat < 0.0f) p.heat = 0.0f;
		if (p.overheated && p.heat <= 30.0f) {
			p.overheated = false;
			show_message("SYSTEM COOLED", 1.0f);
		}
	}

	// Timers
	if (p.spell_cooldown > 0.0f) p.spell_cooldown -= dt;
	if (p.dodge_cd > 0.0f) p.dodge_cd -= dt;
	if (p.invuln_timer > 0.0f) p.invuln_timer -= dt;
	if (p.shield_timer > 0.0f) p.shield_timer -= dt;

	// Player Movement
	if (p.dodge_timer > 0.0f) {
		p.dodge_timer -= dt;
		float new_x = p.x + p.dodge_vx * dt;
		float new_y = p.y + p.dodge_vy * dt;
		if (!dungeon_check_collision(new_x, p.y, 4.0f)) p.x = new_x;
		if (!dungeon_check_collision(p.x, new_y, 4.0f)) p.y = new_y;
		spawn_particles(p.x + 3, p.y + 10, 1, LIGHT_GRAY, 10.0f);
	} else {
		float in_x = 0.0f, in_y = 0.0f;
		if (abs(sdk_inputs.joy_x) > 400) in_x = (float)sdk_inputs.joy_x / 2048.0f;
		if (abs(sdk_inputs.joy_y) > 400) in_y = (float)sdk_inputs.joy_y / 2048.0f;
		if (sdk_inputs.horizontal) in_x = (float)sdk_inputs.horizontal;
		if (sdk_inputs.vertical) in_y = (float)sdk_inputs.vertical;

		if (fabsf(in_x) > 0.1f || fabsf(in_y) > 0.1f) {
			float len = sqrtf(in_x * in_x + in_y * in_y);
			if (len > 1.0f) {
				in_x /= len;
				in_y /= len;
			}
			p.vx = in_x * p.speed;
			p.vy = in_y * p.speed;

			if (fabsf(in_y) > fabsf(in_x)) {
				p.direction = (in_y > 0) ? 0 : 3;
			} else {
				p.direction = (in_x > 0) ? 2 : 1;
			}

			p.walk_anim += dt * 6.0f;
			p.walk_frame = (int)p.walk_anim & 1;
		} else {
			p.vx = 0;
			p.vy = 0;
			p.walk_frame = 0;
		}

		float new_x = p.x + p.vx * dt;
		float new_y = p.y + p.vy * dt;
		if (!dungeon_check_collision(new_x, p.y, 4.0f)) p.x = new_x;
		if (!dungeon_check_collision(p.x, new_y, 4.0f)) p.y = new_y;
	}

	check_tile_interactions();
	dungeon_update(dt, &p);
	update_projectiles(dt);
	update_zombies(dt);
	update_traps(dt);
	update_pickups(dt);
	update_particles(dt);
}

static void draw_player(float x, float y, int direction)
{
	if (p.invuln_timer > 0.0f && (((int)(global_timer * 15.0f)) & 1)) {
		return;
	}

	int leg_frame = p.walk_frame;

	sdk_draw_tile(x - 1, y - 1 + leg_frame, &ts_lleg_2x2_png, p.Lleg * PLAYER_DIRECTION + direction);
	sdk_draw_tile(x + 1, y - 1 + (1 - leg_frame), &ts_rleg_2x2_png, p.Rleg * PLAYER_DIRECTION + direction);
	sdk_draw_tile(x - 1, y - 5, &ts_body_4x4_png, p.body * PLAYER_DIRECTION + direction);
	sdk_draw_tile(x - 2, y - 11, &ts_head_6x6_png, p.head * PLAYER_DIRECTION + direction);
	sdk_draw_tile(x - 2, y - 5, &ts_rarm_1x3_png, p.Rarm * PLAYER_DIRECTION + direction);
	sdk_draw_tile(x + 3, y - 5, &ts_larm_1x3_png, p.Larm * PLAYER_DIRECTION + direction);

	// Arcane Shield Aura
	if (p.shield_timer > 0.0f) {
		float angle = global_timer * 5.0f;
		int ox = (int)(cosf(angle) * 11.0f);
		int oy = (int)(sinf(angle) * 11.0f);
		tft_draw_pixel(x + 3 + ox, y + oy, CYAN);
		tft_draw_pixel(x + 3 - ox, y - oy, CYAN);
		tft_draw_pixel(x + 3 + oy, y - ox, LIGHT_BLUE);
		tft_draw_pixel(x + 3 - oy, y + ox, LIGHT_BLUE);
	}
}

static void render_hud(void)
{
	// 1. Top HUD Bar (y=0..8)
	for (int y = 0; y <= 8; y++) {
		tft_draw_rect(0, y, 159, y, BLACK);
	}
	tft_draw_rect(0, 9, 159, 9, DARK_GRAY);

	if (game_state == STATE_TUTORIAL) {
		hud_draw_text_fmt(14, 2, GOLD, "TRAINING STEP %d/7", tutorial_step + 1);
		hud_draw_text_fmt(122, 2, CYAN, "P%d  $%d", p.potions, p.coins);
	} else {
		// HP text & bar
		hud_draw_text(2, 2, WHITE, "HP");
		tft_draw_rect(12, 2, 46, 6, DARK_RED);
		int hp_w = (p.hp * 33) / p.max_hp;
		if (hp_w < 0) hp_w = 0;
		if (hp_w > 33) hp_w = 33;
		color_t hp_col = (p.hp < p.max_hp / 3) ? RED : GREEN;
		for (int y = 3; y <= 5; y++) {
			if (hp_w > 0) tft_draw_rect(13, y, 13 + hp_w - 1, y, hp_col);
		}
		hud_draw_text_fmt(49, 2, WHITE, "%d", p.hp);

		// Heat bar
		tft_draw_rect(72, 2, 102, 6, DARK_BLUE);
		int heat_w = (int)((p.heat * 29.0f) / 100.0f);
		if (heat_w < 0) heat_w = 0;
		if (heat_w > 29) heat_w = 29;
		color_t heat_col = CYAN;
		if (p.heat > 75.0f) heat_col = RED;
		else if (p.heat > 45.0f) heat_col = ORANGE;
		for (int y = 3; y <= 5; y++) {
			if (heat_w > 0) tft_draw_rect(73, y, 73 + heat_w - 1, y, heat_col);
		}

		if (p.overheated) {
			if (((int)(global_timer * 8.0f)) & 1)
				hud_draw_text(105, 2, RED, "!HOT!");
			else
				hud_draw_text(105, 2, ORANGE, "!LOCK!");
		} else {
			hud_draw_text_fmt(105, 2, heat_col, "%d%%", (int)p.heat);
		}

		// Coins ($) and Potions (P)
		hud_draw_text_fmt(126, 2, GOLD, "$%d", p.coins);
		hud_draw_text_fmt(146, 2, CYAN, "P%d", p.potions);
	}

	// 2. Bottom HUD Bar (y=110..119)
	for (int y = 111; y < 120; y++) {
		tft_draw_rect(0, y, 159, y, BLACK);
	}
	tft_draw_rect(0, 110, 159, 110, DARK_GRAY);

	if (game_state == STATE_LOBBY) {
		if (lobby_hint[0] != 0) {
			hud_draw_text(6, 113, YELLOW, lobby_hint);
		} else {
			hud_draw_text(6, 113, WHITE, "SANCTUARY LOBBY");
		}
		hud_draw_text(110, 113, GRAY, "[START] MENU");
	} else if (game_state == STATE_TUTORIAL) {
		const char *tut_msgs[] = {
			"USE D-PAD TO MOVE AROUND",
			"PRESS [A] TO CAST SPELL",
			"AIM & HIT PRACTICE DUMMY!",
			"WATCH HEAT! DON'T OVERHEAT",
			"PRESS [B] TO DODGE ROLL",
			"PRESS [X] TO CYCLE SPELLS",
			"PRESS [Y] TO USE ELIXIR",
			"DONE! ENTER PORTAL (+50$)",
		};
		int s = (tutorial_step >= 0 && tutorial_step <= 7) ? tutorial_step : 7;
		color_t c = (s == 7) ? GREEN : YELLOW;
		hud_draw_text(6, 113, c, tut_msgs[s]);
	} else {
		// Dungeon mode HUD
		draw_sprite_2bpp(2, 111, sp_spell_icons[p.selected_spell], 8, 8,
				 pal_spell_icons[p.selected_spell], false);
		const SpellDef *sp = &spells[p.selected_spell];
		hud_draw_text_fmt(13, 113, WHITE, "%s +%d%%", sp->name, (int)sp->heat_cost);
		hud_draw_text_fmt(116, 113, YELLOW, "FL%d", current_floor);
		if (p.dodge_cd <= 0.0f) {
			hud_draw_text(138, 113, GREEN, "ROLL");
		} else {
			hud_draw_text(138, 113, DARK_GRAY, "WAIT");
		}
	}

	// Message Banner
	if (message_timer > 0) {
		int msg_len = strlen(message_text) * 4;
		int mx = (TFT_WIDTH - msg_len) / 2;
		if (mx < 4) mx = 4;
		tft_draw_rect(mx - 3, 52, mx + msg_len + 2, 60, BLACK);
		hud_draw_text(mx, 54, YELLOW, message_text);
	}
}

void game_paint(unsigned dt_usec)
{
	(void)dt_usec;
	tft_fill(BLACK);

	if (game_state == STATE_TITLE) {
		tft_set_origin(0, 0);
		tft_draw_rect(0, 0, 159, 119, BLACK);

		// Cover art on the left: size 84x84 at (6, 18) with 1px border
		tft_draw_rect(5, 17, 90, 102, DARK_GRAY);
		sdk_draw_image(6, 18, &image_cover_png);

		// Title header
		hud_draw_text(60, 4, GOLD, "P U N T O G");
		hud_draw_text(40, 10, GRAY, "THE DUNGEON EXPEDITION");

		char bgm_title_opt[16];
		snprintf(bgm_title_opt, sizeof(bgm_title_opt), "BGM: %s", music_enabled ? "ON" : "OFF");

		// Menu items on the right (x=96)
		const char *items[] = {
			"LOBBY",
			"DUNGEON",
			"TUTORIAL",
			"ARCHIVES",
			"WARDROBE",
			bgm_title_opt,
		};
		for (int i = 0; i < 6; i++) {
			int y = 18 + i * 14;
			color_t col = (menu_selected == i) ? YELLOW : WHITE;
			if (menu_selected == i) {
				tft_draw_rect(93, y - 2, 156, y + 8, DARK_GRAY);
				hud_draw_text_fmt(96, y, col, "> %s", items[i]);
			} else {
				hud_draw_text_fmt(100, y, col, "%s", items[i]);
			}
		}

		hud_draw_text(18, 108, YELLOW, "[A] SELECT    [UP/DN] NAVIGATE");
		return;
	}

	if (game_state == STATE_INFO) {
		tft_set_origin(0, 0);
		tft_draw_rect(4, 4, 155, 115, BLACK);
		tft_draw_rect(5, 5, 154, 114, DARK_GRAY);

		hud_draw_text_fmt(24, 8, GOLD, "ARCHIVES & GUIDE (%d/4)", info_page + 1);
		tft_draw_rect(8, 16, 151, 16, DARK_GRAY);

		if (info_page == 0) {
			hud_draw_text(10, 20, YELLOW, "-- CONTROLS --");
			hud_draw_text(10, 30, WHITE, "D-PAD: 8-WAY MOVEMENT");
			hud_draw_text(10, 42, WHITE, "[A]: CAST ACTIVE SPELL");
			hud_draw_text(10, 54, WHITE, "[B]: DODGE ROLL (INVULN)");
			hud_draw_text(10, 66, WHITE, "[X]: CYCLE SPELLS");
			hud_draw_text(10, 78, WHITE, "[Y]: DRINK ELIXIR (HEAL)");
			hud_draw_text(10, 90, WHITE, "[START]: PAUSE MENU");
		} else if (info_page == 1) {
			hud_draw_text(10, 20, YELLOW, "-- HEAT GAUGE --");
			hud_draw_text(10, 30, WHITE, "CASTING SPELLS RAISES HEAT.");
			hud_draw_text(10, 42, WHITE, "BLUE GAUGE AT TOP IS HEAT.");
			hud_draw_text(10, 56, RED,   "AT 100% HEAT: OVERHEATED!");
			hud_draw_text(10, 68, WHITE, "CANNOT CAST UNTIL COOLED.");
			hud_draw_text(10, 80, CYAN,  "PASSIVE COOLING WHEN IDLE.");
			hud_draw_text(10, 92, GREEN, "ELIXIRS CLEAR HEAT AT ONCE.");
		} else if (info_page == 2) {
			hud_draw_text(10, 20, YELLOW, "-- SPELL ARSENAL --");
			hud_draw_text(10, 30, ORANGE, "FIREBALL: BURST EXPLOSION");
			hud_draw_text(10, 42, CYAN,   "ICE DAGGER: SWIFT PIERCING");
			hud_draw_text(10, 54, MAGENTA,"MISSILE: HOMING BOLTS");
			hud_draw_text(10, 66, LIGHT_BLUE, "SHIELD: CONTACT SHOCK");
			hud_draw_text(10, 78, YELLOW, "RUNE TRAP: FLOOR EXPLOSIVE");
			hud_draw_text(10, 90, GOLD,   "UPGRADE SPELLS AT THE ALTAR");
		} else if (info_page == 3) {
			hud_draw_text(10, 20, YELLOW, "-- BESTIARY & DUNGEONS --");
			hud_draw_text(10, 30, GREEN,  "WALKER: SLOW CRYPT SLAVE");
			hud_draw_text(10, 42, RED,    "GHOUL: FAST SPRINTER");
			hud_draw_text(10, 54, GRAY,   "BRUTE: HEAVY ARMORED TANK");
			hud_draw_text(10, 66, YELLOW, "LADDER: ADVANCE DEEPER");
			hud_draw_text(10, 78, GOLD,   "CHESTS: GOLD & POTIONS");
			hud_draw_text(10, 90, WHITE,  "GOLD PERSISTS ACROSS RUNS");
		}

		hud_draw_text(18, 105, YELLOW, "[< >] PAGE    [B/A] RETURN");
		return;
	}

	if (game_state == STATE_SHOP) {
		tft_set_origin(0, 0);
		tft_draw_rect(6, 4, 153, 115, BLACK);
		tft_draw_rect(7, 5, 152, 114, DARK_GRAY);

		hud_draw_text(42, 8, GOLD, "ARCANE ALTAR");
		hud_draw_text_fmt(54, 16, YELLOW, "GOLD: $%d", p.coins);

		int cost_hp = 50 + (p.max_hp - 100) * 3;
		int cool_lvl = (int)((p.cooling_rate - 26.0f) / 5.0f);
		int cost_cool = 40 + cool_lvl * 35;
		int cost_pwr = 60 + p.spell_power * 40;
		int cost_pot = 25;

		for (int i = 0; i < 5; i++) {
			int y = 28 + i * 15;
			color_t col = (shop_selected == i) ? YELLOW : WHITE;
			if (i == 0) {
				hud_draw_text_fmt(12, y, col, "%s MAX HP (+20)    $%d", (shop_selected == i) ? ">" : " ", cost_hp);
			} else if (i == 1) {
				hud_draw_text_fmt(12, y, col, "%s COOLING RATE    $%d", (shop_selected == i) ? ">" : " ", cost_cool);
			} else if (i == 2) {
				hud_draw_text_fmt(12, y, col, "%s SPELL POWER     $%d", (shop_selected == i) ? ">" : " ", cost_pwr);
			} else if (i == 3) {
				hud_draw_text_fmt(12, y, col, "%s BUY ELIXIR (%d)  $%d", (shop_selected == i) ? ">" : " ", p.potions, cost_pot);
			} else {
				hud_draw_text_fmt(12, y, col, "%s EXIT ALTAR", (shop_selected == i) ? ">" : " ");
			}
		}

		hud_draw_text(16, 105, WHITE, "[A] PURCHASE   [B] EXIT");
		return;
	}

	if (game_state == STATE_WARDROBE) {
		tft_set_origin(0, 0);
		tft_draw_rect(6, 4, 153, 115, BLACK);
		tft_draw_rect(7, 5, 152, 114, DARK_GRAY);

		hud_draw_text(32, 8, GOLD, "WARDROBE CUSTOMIZER");

		for (int i = 0; i < 5; i++) {
			int y = 24 + i * 16;
			color_t col = (wardrobe_selected == i) ? YELLOW : WHITE;
			if (i == 0) {
				hud_draw_text_fmt(10, y, col, "%s HEAD STYLE: #%d", (wardrobe_selected == i) ? ">" : " ", p.head + 1);
			} else if (i == 1) {
				hud_draw_text_fmt(10, y, col, "%s ROBE BODY:  #%d", (wardrobe_selected == i) ? ">" : " ", p.body + 1);
			} else if (i == 2) {
				hud_draw_text_fmt(10, y, col, "%s SLEEVES:    #%d", (wardrobe_selected == i) ? ">" : " ", p.Larm + 1);
			} else if (i == 3) {
				hud_draw_text_fmt(10, y, col, "%s BOOTS:      #%d", (wardrobe_selected == i) ? ">" : " ", p.Lleg + 1);
			} else {
				hud_draw_text(10, y, col, (wardrobe_selected == i) ? "> CONFIRM & SAVE" : "  CONFIRM & SAVE");
			}
		}

		// Draw preview pedestal & player sprite
		tft_draw_rect(112, 40, 142, 76, BLACK);
		tft_draw_rect(113, 41, 141, 75, DARK_GRAY);
		hud_draw_text(115, 43, YELLOW, "PREVIEW");
		draw_player(127, 68, 0);

		hud_draw_text(14, 105, WHITE, "[< >] CHANGE   [A/B] SAVE");
		return;
	}

	if (game_state == STATE_NAME_MENU) {
		tft_set_origin(0, 0);
		hud_draw_text(48, 6, YELLOW, "ENTER YOUR NAME");

		const int cell_w = 20, cell_h = 15;
		const int start_x = 24, start_y = 18;

		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				int px = start_x + x * cell_w;
				int py = start_y + y * cell_h;
				char c = (key_page ? keypad2 : keypad)[y][x];

				if (x == key_cur_x && y == key_cur_y) {
					for (int r = py; r < py + cell_h - 1; r++)
						tft_draw_rect(px, r, px + cell_w - 1, r, WHITE);
					char str[2] = { c, 0 };
					hud_draw_text(px + 8, py + 5, BLACK, str);
				} else {
					tft_draw_rect(px, py, px + cell_w - 1, py + cell_h - 1, DARK_GRAY);
					char str[2] = { c, 0 };
					hud_draw_text(px + 8, py + 5, WHITE, str);
				}
			}
		}

		hud_draw_text_fmt(24, 82, CYAN, "NAME: %s", p.name);
		hud_draw_text(16, 96, WHITE, "A:SET  B:DEL  PAGE:SEL");
		hud_draw_text(16, 106, GREEN, "START:DESCEND INTO DUNGEON");
		return;
	}

	if (game_state == STATE_SPELL_MENU) {
		tft_set_origin(0, 0);
		hud_draw_text(40, 4, YELLOW, "SPELL GRIMOIRE");

		for (int i = 0; i < NUM_SPELLS; i++) {
			int y = 16 + i * 18;
			draw_sprite_2bpp(6, y, sp_spell_icons[i], 8, 8, pal_spell_icons[i], false);
			if ((SpellType)i == p.selected_spell) {
				hud_draw_text_fmt(18, y, YELLOW, "> %s [%s]", spells[i].name, spells[i].aspect);
			} else {
				hud_draw_text_fmt(22, y, WHITE, "%s [%s]", spells[i].name, spells[i].aspect);
			}
			hud_draw_text_fmt(22, y + 7, GRAY, "+%d%% HEAT: %s",
					  (int)spells[i].heat_cost, spells[i].desc);
		}

		hud_draw_text(12, 108, CYAN, "A/B/SEL:EQUIP & CLOSE");
		return;
	}

	if (game_state == STATE_GAMEOVER) {
		tft_set_origin(0, 0);
		hud_draw_text(52, 35, RED, "YOU PERISHED");
		hud_draw_text_fmt(36, 52, WHITE, "FLOOR REACHED: %d", current_floor);
		hud_draw_text_fmt(36, 64, GOLD, "GOLD RECOVERED: $%d", p.coins);
		hud_draw_text(32, 85, YELLOW, "PRESS START / A TO RETURN TO LOBBY");
		return;
	}

	// World Rendering (DUNGEON, LOBBY, TUTORIAL, PAUSE)
	float cam_x = p.x - TFT_WIDTH / 2.0f;
	float cam_y = p.y - TFT_HEIGHT / 2.0f;

	if (screen_shake > 0.0f) {
		cam_x += ((rand() % 5) - 2);
		cam_y += ((rand() % 5) - 2);
	}

	if (cam_x < 0) cam_x = 0;
	if (cam_y < 0) cam_y = 0;
	if (cam_x > cur_map_cols * TILE_SIZE - TFT_WIDTH)
		cam_x = cur_map_cols * TILE_SIZE - TFT_WIDTH;
	if (cam_y > cur_map_rows * TILE_SIZE - TFT_HEIGHT)
		cam_y = cur_map_rows * TILE_SIZE - TFT_HEIGHT;

	tft_set_origin(cam_x, cam_y);

	// 1. Tilemap
	dungeon_render_tiles(cam_x, cam_y, global_timer);

	// Lobby Station Labels
	if (game_state == STATE_LOBBY) {
		hud_draw_text(cur_map_cols / 2 * TILE_SIZE - 12, 2 * TILE_SIZE - 7, YELLOW, "DUNGEON");
		hud_draw_text(2 * TILE_SIZE - 12, 2 * TILE_SIZE - 7, CYAN, "WARDROBE");
		hud_draw_text((cur_map_cols - 3) * TILE_SIZE - 10, 2 * TILE_SIZE - 7, GOLD, "ALTAR");
		hud_draw_text(2 * TILE_SIZE - 12, (cur_map_rows - 3) * TILE_SIZE - 7, GREEN, "TUTORIAL");
		hud_draw_text((cur_map_cols - 3) * TILE_SIZE - 12, (cur_map_rows - 3) * TILE_SIZE - 7, WHITE, "ARCHIVES");
	}

	// 2. Traps
	for (int i = 0; i < active_trap_count; i++) {
		Trap *tr = &active_traps[i];
		draw_sprite_2bpp(tr->x, tr->y, sp_spell_icons[4], 8, 8, pal_spell_icons[4], false);
	}

	// 3. Pickups
	for (int i = 0; i < active_pickup_count; i++) {
		Pickup *pk = &active_pickups[i];
		if (pk->type == 0) {
			int f = ((int)(global_timer * 6.0f)) & 1;
			draw_sprite_2bpp(pk->x, pk->y, f ? sp_coin_f1 : sp_coin_f0, 8, 6, pal_coin, false);
		} else if (pk->type == 1) {
			draw_sprite_2bpp(pk->x, pk->y, sp_potion, 8, 8, pal_potion, false);
		} else if (pk->type == 2) {
			draw_sprite_2bpp(pk->x, pk->y, sp_heart, 8, 8, pal_heart, false);
		}
	}

	// 4. Projectiles
	for (int i = 0; i < active_projectile_count; i++) {
		Projectile *proj = active_projectiles[i];
		if (proj->spell_type == SPELL_FIREBALL) {
			tft_draw_pixel(proj->x, proj->y, RED);
			tft_draw_pixel(proj->x + 1, proj->y, ORANGE);
			tft_draw_pixel(proj->x, proj->y + 1, YELLOW);
			tft_draw_pixel(proj->x + 1, proj->y + 1, WHITE);
		} else if (proj->spell_type == SPELL_ICE_DAGGER) {
			tft_draw_pixel(proj->x, proj->y, CYAN);
			tft_draw_pixel(proj->x + 1, proj->y, WHITE);
			tft_draw_pixel(proj->x + 2, proj->y, LIGHT_BLUE);
		} else if (proj->spell_type == SPELL_MAGIC_MISSILE) {
			tft_draw_pixel(proj->x, proj->y, MAGENTA);
			tft_draw_pixel(proj->x + 1, proj->y, PINK);
			tft_draw_pixel(proj->x + 1, proj->y + 1, PURPLE);
			tft_draw_pixel(proj->x + 1, proj->y + 1, WHITE);
		}
	}

	// 5. Zombies (with dummy, dynamic palette & animation!)
	for (int i = 0; i < active_zombie_count; i++) {
		Zombie *z = active_zombies[i];
		const uint8_t *frame_data = (z->anim_frame == 0) ? sp_zombie_f0 : sp_zombie_f1;

		Palette4 z_pal;
		if (z->burn_timer > 0.0f) {
			z_pal = pal_zombie_burn;
		} else if (z->freeze_timer > 0.0f) {
			z_pal = pal_zombie_freeze;
		} else if (z->type == 1) {
			z_pal = pal_zombie_runner;
		} else if (z->type == 2) {
			z_pal = pal_zombie_brute;
		} else if (z->type == 3) {
			z_pal = (Palette4){ .c = { 0, rgb216(2, 1, 0), rgb216(4, 2, 0), rgb216(5, 4, 1) } };
		} else {
			z_pal = pal_zombie_walker;
		}

		bool flip = (z->vx < 0);
		draw_sprite_2bpp(z->x, z->y, frame_data, 8, 12, z_pal, flip);

		if (z->type == 3) {
			hud_draw_text(z->x - 6, z->y - 8, YELLOW, "DUMMY");
		} else if (z->hp < z->max_hp) {
			int bar_w = (z->hp * 8) / z->max_hp;
			if (bar_w < 0) bar_w = 0;
			tft_draw_rect(z->x, z->y - 2, z->x + 7, z->y - 2, DARK_RED);
			if (bar_w > 0)
				tft_draw_rect(z->x, z->y - 2, z->x + bar_w - 1, z->y - 2, GREEN);
		}
	}

	// 6. Particles
	for (int i = 0; i < active_particle_count; i++) {
		Particle *pt = active_particles[i];
		tft_draw_pixel(pt->x, pt->y, pt->color);
		if (pt->size > 1) {
			tft_draw_pixel(pt->x + 1, pt->y, pt->color);
		}
	}

	// 7. Player
	draw_player(p.x, p.y, p.direction);

	// 8. HUD on screen coordinates
	tft_set_origin(0, 0);
	render_hud();

	// 9. Pause Modal Menu
	if (game_state == STATE_PAUSE) {
		tft_set_origin(0, 0);
		tft_draw_rect(10, 8, 150, 114, BLACK);
		tft_draw_rect(11, 9, 149, 113, DARK_GRAY);
		tft_draw_rect(12, 10, 148, 112, BLACK);
		hud_draw_text(56, 13, YELLOW, "PAUSED");

		char bgm_label[24];
		if (!music_enabled) {
			snprintf(bgm_label, sizeof(bgm_label), "MUSIC: OFF");
		} else {
			const char *vol_str[] = { "25%", "50%", "75%", "100%" };
			snprintf(bgm_label, sizeof(bgm_label), "MUSIC: %s", vol_str[music_volume]);
		}

		const char *pause_opts[] = {
			"RESUME",
			"SAVE PROGRESS",
			bgm_label,
			"HOW TO PLAY & INFO",
			"SPELL GRIMOIRE",
			"RETURN TO LOBBY",
			"QUIT TO TITLE",
		};
		for (int i = 0; i < 7; i++) {
			color_t col = (pause_selected == i) ? YELLOW : WHITE;
			int y = 24 + i * 11;
			if (pause_selected == i) {
				hud_draw_text_fmt(16, y, col, "> %s", pause_opts[i]);
			} else {
				hud_draw_text_fmt(20, y, col, "%s", pause_opts[i]);
			}
		}
		hud_draw_text(24, 103, GRAY, "[A] SELECT   [< >] VOLUME");
	}
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = false,
		.off_on_select = false,
		.fps_color = rgb_to_rgb565(255, 255, 255),
	};

	sdk_main(&config);
}
