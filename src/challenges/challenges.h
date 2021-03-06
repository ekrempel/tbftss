/*
Copyright (C) 2015-2018 Parallel Realities

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

#include "../common.h"
#include "../json/cJSON.h"

extern Mission *loadMissionMeta(char *filename);
extern char **getFileList(char *dir, int *count);
extern void selectWidget(const char *name, const char *group);
extern char *getTranslatedString(char *string);
extern char *getLookupName(char *prefix, long num);
extern char *timeToString(long millis, int showHours);
extern void updateAccuracyStats(unsigned int *stats);
extern int getJSONValue(cJSON *node, char *name, int defValue);
extern long lookup(char *name);
extern void awardStatsTrophies(void);
extern void retreatAllies(void);
extern void retreatEnemies(void);
extern void awardCraftTrophy(void);

extern Dev dev;
extern Battle battle;
extern Entity *player;
extern Game game;
