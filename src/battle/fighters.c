/*
Copyright (C) 2015-2016 Parallel Realities

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "fighters.h"

static void separate(void);
static void die(void);
static void immediateDie(void);
static void spinDie(void);
static void straightDie(void);
static void randomizeDart(Entity *dart);
static void randomizeDartGuns(Entity *dart);
static void loadFighterDef(char *filename);
static void loadFighterDefList(char *filename);
static Entity *getFighterDef(char *name);

static Entity defHead, *defTail;

Entity *spawnFighter(char *name, int x, int y, int side)
{
	Entity *e, *def;

	e = spawnEntity();

	def = getFighterDef(name);

	memcpy(e, def, sizeof(Entity));

	e->id = battle.entId;
	e->next = NULL;

	e->x = x;
	e->y = y;
	e->side = side;

	switch (side)
	{
		case SIDE_ALLIES:
			e->aiAggression = 2 + rand() % 2;
			if (!(e->aiFlags & AIF_FOLLOWS_PLAYER))
			{
				e->aiFlags |= AIF_MOVES_TO_PLAYER;
			}
			break;

		case SIDE_PIRATE:
			e->aiAggression = rand() % 3;
			break;

		case SIDE_PANDORAN:
			e->aiAggression = 3 + rand() % 2;
			break;

		case SIDE_REBEL:
			e->aiAggression = 1 + rand() % 3;
			break;
	}

	if (strcmp(name, "ATAF") == 0)
	{
		e->aiAggression = 4;
	}

	if (strcmp(name, "Dart") == 0)
	{
		randomizeDart(e);
	}

	if (strcmp(name, "Civilian") == 0 && rand() % 2 == 0)
	{
		e->texture = getTexture("gfx/craft/civilian02.png");
	}

	if (e->aiFlags & AIF_AGGRESSIVE)
	{
		e->aiAggression = 4;
	}

	e->action = doAI;
	e->die = die;

	return e;
}

static void randomizeDart(Entity *dart)
{
	char texture[MAX_DESCRIPTION_LENGTH];

	if (rand() % 5 == 0)
	{
		dart->health = dart->maxHealth = 5 + (rand() % 21);
	}

	if (rand() % 5 == 0)
	{
		dart->shield = dart->maxShield = 1 + (rand() % 16);
		dart->shieldRechargeRate = 30 + (rand() % 90);
	}

	if (rand() % 5 == 0)
	{
		dart->speed = 2 + (rand() % 3);
	}

	if (rand() % 5 == 0)
	{
		dart->reloadTime = 24 + (rand() % 11);
	}

	randomizeDartGuns(dart);

	dart->missiles = rand() % 3;

	sprintf(texture, "gfx/fighters/dart0%d.png", 1 + rand() % 7);

	dart->texture = getTexture(texture);
}

static void randomizeDartGuns(Entity *dart)
{
	int i;

	switch (rand() % 4)
	{
		/* Single plasma gun */
		case 0:
			dart->guns[0].type = BT_PLASMA;
			dart->guns[0].x = dart->guns[0].y = 0;

			for (i = 1 ; i < MAX_FIGHTER_GUNS ; i++)
			{
				if (dart->guns[i].type)
				{
					dart->guns[i].type = BT_NONE;
				}
			}
			break;

		/* Dual plasma guns */
		case 1:
			dart->guns[0].type = BT_PLASMA;
			dart->guns[1].type = BT_PLASMA;
			break;

		/* Triple particle guns */
		case 2:
			dart->guns[2].type = BT_PARTICLE;
			dart->guns[2].y = -10;
			break;


		/* Plasma / Laser cannons */
		case 3:
			dart->guns[0].type = BT_PLASMA;
			dart->guns[0].x = dart->guns[0].y = 0;

			dart->guns[1].type = BT_LASER;
			dart->guns[1].x = dart->guns[1].y = 0;
			break;

		/* Dual Laser cannons */
		case 4:
			dart->guns[0].type = BT_LASER;
			dart->guns[1].type = BT_LASER;
			break;
	}
}

void doFighter(void)
{
	if (self->alive == ALIVE_ALIVE)
	{
		if (self != player)
		{
			separate();
		}

		attachRope();

		if (self->thrust > 0.25)
		{
			addEngineEffect();
		}

		if (self->health <= 0)
		{
			self->health = 0;
			self->alive = ALIVE_DYING;
			self->die();

			if (self == battle.missionTarget)
			{
				battle.missionTarget = NULL;
			}
		}
		else if (self->systemPower <= 0 || (self->flags & EF_DISABLED))
		{
			self->dx *= 0.99;
			self->dy *= 0.99;
			self->thrust = 0;
			self->shield = self->maxShield = 0;
			self->action = NULL;

			if ((self->flags & EF_DISABLED) == 0)
			{
				playBattleSound(SND_POWER_DOWN, self->x, self->y);

				self->flags |= EF_DISABLED;
				self->flags |= EF_SECONDARY_TARGET;

				battle.stats[STAT_ENEMIES_DISABLED]++;

				updateObjective(self->name, TT_DISABLE);
			}
		}

		if (self->target != NULL && self->target->alive != ALIVE_ALIVE)
		{
			self->target = NULL;

			if (self != player)
			{
				self->action = doAI;
			}
		}
	}

	if (self->alive == ALIVE_ESCAPED)
	{
		if (self == player)
		{
			completeMission();
		}

		if (self->side != SIDE_ALLIES && (!(self->flags & EF_DISABLED)))
		{
			addHudMessage(colors.red, _("Mission target has escaped."));
			battle.stats[STAT_ENEMIES_ESCAPED]++;
		}

		if (strcmp(self->defName, "Civilian") == 0)
		{
			battle.stats[STAT_CIVILIANS_RESCUED]++;
		}

		/* if you did not escape under your own volition, or with the aid of a friend, you've been stolen */
		if (!self->owner || self->side == self->owner->side)
		{
			updateObjective(self->name, TT_ESCAPED);
			updateCondition(self->name, TT_ESCAPED);
		}
		else
		{
			updateObjective(self->name, TT_STOLEN);
			updateCondition(self->name, TT_STOLEN);
		}
	}

	if (self->alive == ALIVE_DEAD)
	{
		if (self == player)
		{
			battle.stats[STAT_PLAYER_KILLED]++;

			/* the player is known as "Player", so we need to check the craft they were assigned to */
			if (strcmp(game.currentMission->craft, "ATAF") == 0)
			{
				awardTrophy("ATAF_DESTROYED");
			}
		}
		else if (player != NULL)
		{
			if (player->alive == ALIVE_ALIVE)
			{
				if (self->side != SIDE_ALLIES)
				{
					battle.stats[STAT_ENEMIES_KILLED]++;

					runScriptFunction("ENEMIES_KILLED %d", battle.stats[STAT_ENEMIES_KILLED]);
				}
				else
				{
					if (strcmp(self->name, "Civilian") == 0)
					{
						battle.stats[STAT_CIVILIANS_KILLED]++;
						if (!battle.isEpic || game.currentMission->challengeData.isChallenge)
						{
							addHudMessage(colors.red, _("Civilian has been killed"));
						}
					}
					else
					{
						battle.stats[STAT_ALLIES_KILLED]++;
						if (!battle.isEpic || game.currentMission->challengeData.isChallenge)
						{
							addHudMessage(colors.red, _("Ally has been killed"));
						}

						runScriptFunction("ALLIES_KILLED %d", battle.stats[STAT_ALLIES_KILLED]);
					}
				}
			}

			updateObjective(self->name, TT_DESTROY);
			updateObjective(self->groupName, TT_DESTROY);

			adjustObjectiveTargetValue(self->name, TT_ESCAPED, -1);

			updateCondition(self->name, TT_DESTROY);
		}
	}
}

static void separate(void)
{
	int angle;
	int distance;
	float dx, dy, force;
	int count;
	Entity *e, **candidates;
	int i;

	dx = dy = 0;
	count = 0;
	force = 0;

	candidates = getAllEntsWithin(self->x - (self->w / 2), self->y - (self->h / 2), self->w, self->h, self);

	for (i = 0, e = candidates[i] ; e != NULL ; e = candidates[++i])
	{
		if (e->flags & EF_TAKES_DAMAGE)
		{
			distance = getDistance(e->x, e->y, self->x, self->y);

			if (distance > 0 && distance < self->separationRadius)
			{
				angle = getAngle(self->x, self->y, e->x, e->y);

				dx += sin(TO_RAIDANS(angle));
				dy += -cos(TO_RAIDANS(angle));
				force += (self->separationRadius - distance) * 0.005;

				count++;
			}
		}
	}

	if (count > 0)
	{
		dx /= count;
		dy /= count;

		dx *= force;
		dy *= force;

		self->dx -= dx;
		self->dy -= dy;
	}
}

void applyFighterThrust(void)
{
	float v;

	self->dx += sin(TO_RAIDANS(self->angle)) * 0.1;
	self->dy += -cos(TO_RAIDANS(self->angle)) * 0.1;
	self->thrust = sqrt((self->dx * self->dx) + (self->dy * self->dy));

	if (self->thrust > self->speed * self->speed)
	{
		v = (self->speed / sqrt(self->thrust));
		self->dx = v * self->dx;
		self->dy = v * self->dy;
		self->thrust = sqrt((self->dx * self->dx) + (self->dy * self->dy));
	}
}

void applyFighterBrakes(void)
{
	self->dx *= 0.95;
	self->dy *= 0.95;

	self->thrust = sqrt((self->dx * self->dx) + (self->dy * self->dy));
}

void damageFighter(Entity *e, int amount, long flags)
{
	int prevShield = e->shield;

	e->aiDamageTimer = FPS;
	e->aiDamagePerSec += amount;

	if (flags & BF_SYSTEM_DAMAGE)
	{
		playBattleSound(SND_MAG_HIT, e->x, e->y);

		e->systemPower = MAX(0, e->systemPower - amount);

		e->systemHit = 255;

		if (e->systemPower == 0)
		{
			e->shield = e->maxShield = 0;
			e->action = NULL;
		}
	}
	else if (flags & BF_SHIELD_DAMAGE)
	{
		e->shield -= amount;

		if (e->shield <= 0 && prevShield > 0)
		{
			playBattleSound(SND_SHIELD_BREAK, e->x, e->y);
			addShieldSplinterEffect(e);
			e->shield = -(FPS * 10);
		}

		e->shield = MAX(-(FPS * 10), e->shield);
	}
	else
	{
		if (e->shield > 0)
		{
			e->shield -= amount;

			if (e->shield < 0)
			{
				e->health += e->shield;
				e->shield = 0;
			}
		}
		else
		{
			e->health -= amount;
			e->armourHit = 255;

			playBattleSound(SND_ARMOUR_HIT, e->x, e->y);
		}
	}

	if (e->shield > 0)
	{
		e->shieldHit = 255;

		/* don't allow the shield to recharge immediately after taking a hit */
		e->shieldRecharge = e->shieldRechargeRate;

		playBattleSound(SND_SHIELD_HIT, e->x, e->y);
	}

	/*
	 * Sometimes run away if you take too much damage in a short space of time
	 */
	if (e->type == ET_FIGHTER && (!(e->aiFlags & AIF_EVADE)) && e != player && e->aiDamagePerSec >= (e->maxHealth + e->maxShield) * 0.1)
	{
		if ((rand() % 10) > 7)
		{
			e->action = doAI;
			e->aiFlags |= AIF_EVADE;
			e->aiActionTime = e->aiEvadeTimer = FPS * (1 + (rand() % 3));
		}
		else
		{
			e->aiDamagePerSec = 0;
		}
	}
}

static void die(void)
{
	int n = rand() % 3;

	switch (self->deathType)
	{
		case DT_ANY:
			n = rand() % 3;
			break;
		case DT_NO_SPIN:
			n = 1 + rand() % 2;
			break;
		case DT_INSTANT:
			n = 2;
			break;
	}

	if (self == player && battle.isEpic)
	{
		n = 1;
	}

	switch (n)
	{
		case 0:
			self->action = spinDie;
			break;

		case 1:
			self->action = straightDie;
			break;

		case 2:
			self->action = immediateDie;
			break;
	}
}

static void immediateDie(void)
{
	self->alive = ALIVE_DEAD;
	addSmallExplosion();
	playBattleSound(SND_EXPLOSION_1 + rand() % 4, self->x, self->y);
	addDebris(self->x, self->y, 3 + rand() % 6);
}

static void spinDie(void)
{
	self->health--;
	self->thinkTime = 0;
	self->armourHit = 0;
	self->shieldHit = 0;
	self->systemHit = 0;

	self->angle += 8;

	if (rand() % 2 == 0)
	{
		addSmallFighterExplosion();
	}

	if (self->health <= -(FPS * 1.5))
	{
		self->alive = ALIVE_DEAD;
		addSmallExplosion();
		playBattleSound(SND_EXPLOSION_1 + rand() % 4, self->x, self->y);
		addDebris(self->x, self->y, 3 + rand() % 6);
	}
}

static void straightDie(void)
{
	self->health--;
	self->thinkTime = 0;
	self->armourHit = 0;
	self->shieldHit = 0;
	self->systemHit = 0;

	if (rand() % 2 == 0)
	{
		addSmallFighterExplosion();
	}

	if (self->health <= -(FPS * 1.5))
	{
		self->alive = ALIVE_DEAD;
		addSmallExplosion();
		playBattleSound(SND_EXPLOSION_1 + rand() % 4, self->x, self->y);
		addDebris(self->x, self->y, 3 + rand() % 6);
	}
}

void retreatEnemies(void)
{
	Entity *e;

	for (e = battle.entityHead.next ; e != NULL ; e = e->next)
	{
		if (e->type == ET_FIGHTER && e->side != SIDE_ALLIES)
		{
			e->aiFlags |= AIF_AVOIDS_COMBAT;
		}
	}
}

void retreatAllies(void)
{
	Entity *e;

	for (e = battle.entityHead.next ; e != NULL ; e = e->next)
	{
		if (e->type == ET_FIGHTER && e->side == SIDE_ALLIES)
		{
			e->flags |= EF_RETREATING;

			e->aiFlags |= AIF_AVOIDS_COMBAT;
			e->aiFlags |= AIF_UNLIMITED_RANGE;
			e->aiFlags |= AIF_GOAL_JUMPGATE;
			e->aiFlags &= ~AIF_FOLLOWS_PLAYER;
			e->aiFlags &= ~AIF_MOVES_TO_PLAYER;
		}
	}
}

static Entity *getFighterDef(char *name)
{
	Entity *e;

	for (e = defHead.next ; e != NULL ; e = e->next)
	{
		if (strcmp(e->name, name) == 0)
		{
			return e;
		}
	}

	printf("Error: no such fighter '%s'\n", name);
	exit(1);
}

void loadFighterDefs(void)
{
	memset(&defHead, 0, sizeof(Entity));
	defTail = &defHead;

	loadFighterDefList("data/fighters");
	loadFighterDefList("data/craft");
	loadFighterDefList("data/turrets");
}

static void loadFighterDefList(char *dir)
{
	char **filenames;
	char path[MAX_FILENAME_LENGTH];
	int count, i;

	filenames = getFileList(dir, &count);

	for (i = 0 ; i < count ; i++)
	{
		sprintf(path, "%s/%s", dir, filenames[i]);

		loadFighterDef(path);

		free(filenames[i]);
	}

	free(filenames);
}

static void loadFighterDef(char *filename)
{
	cJSON *root, *node;
	char *text;
	Entity *e;
	int i;

	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Loading %s", filename);

	text = readFile(filename);

	root = cJSON_Parse(text);

	if (root)
	{
		e = malloc(sizeof(Entity));
		memset(e, 0, sizeof(Entity));
		defTail->next = e;
		defTail = e;

		e->type = ET_FIGHTER;
		e->active = 1;

		STRNCPY(e->name, cJSON_GetObjectItem(root, "name")->valuestring, MAX_NAME_LENGTH);
		STRNCPY(e->defName, e->name, MAX_NAME_LENGTH);
		e->health = e->maxHealth = cJSON_GetObjectItem(root, "health")->valueint;
		e->shield = e->maxShield = cJSON_GetObjectItem(root, "shield")->valueint;
		e->speed = cJSON_GetObjectItem(root, "speed")->valuedouble;
		e->reloadTime = cJSON_GetObjectItem(root, "reloadTime")->valueint;
		e->shieldRechargeRate = cJSON_GetObjectItem(root, "shieldRechargeRate")->valueint;
		e->texture = getTexture(cJSON_GetObjectItem(root, "texture")->valuestring);

		SDL_QueryTexture(e->texture, NULL, NULL, &e->w, &e->h);

		if (cJSON_GetObjectItem(root, "guns"))
		{
			i = 0;

			for (node = cJSON_GetObjectItem(root, "guns")->child ; node != NULL ; node = node->next)
			{
				e->guns[i].type = lookup(cJSON_GetObjectItem(node, "type")->valuestring);
				e->guns[i].x = cJSON_GetObjectItem(node, "x")->valueint;
				e->guns[i].y = cJSON_GetObjectItem(node, "y")->valueint;

				i++;

				if (i >= MAX_FIGHTER_GUNS)
				{
					printf("ERROR: cannot assign more than %d guns to a fighter\n", MAX_FIGHTER_GUNS);
					exit(1);
				}
			}

			e->combinedGuns = getJSONValue(root, "combinedGuns", 0);
		}

		e->selectedGunType = e->guns[0].type;

		e->missiles = getJSONValue(root, "missiles", 0);

		if (cJSON_GetObjectItem(root, "flags"))
		{
			e->flags = flagsToLong(cJSON_GetObjectItem(root, "flags")->valuestring, NULL);
		}

		if (cJSON_GetObjectItem(root, "aiFlags"))
		{
			e->aiFlags = flagsToLong(cJSON_GetObjectItem(root, "aiFlags")->valuestring, NULL);
		}

		if (cJSON_GetObjectItem(root, "deathType"))
		{
			e->deathType = lookup(cJSON_GetObjectItem(root, "deathType")->valuestring);
		}

		e->separationRadius = MAX(e->w, e->h) * 3;

		e->systemPower = MAX_SYSTEM_POWER;

		cJSON_Delete(root);
	}

	free(text);
}

void destroyFighterDefs(void)
{
	Entity *e;

	while (defHead.next)
	{
		e = defHead.next;
		defHead.next = e->next;
		free(e);
	}
}
