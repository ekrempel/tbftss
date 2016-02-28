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

#include "missionInfo.h"

static void drawMissionSummary(SDL_Texture *title);
static void drawObjectives(void);
static void drawChallenges(void);

static SDL_Texture *missionStartTexture;
static SDL_Texture *missionInProgressTexture;
static SDL_Texture *missionCompleteTexture;
static SDL_Texture *missionFailedTexture;
static const char *objectiveStatus[] = {"Incomplete", "Complete", "Failed", "Condition"};

void initMissionInfo(void)
{
	missionStartTexture = !battle.challengeData.isChallenge ? getTexture("gfx/battle/missionStart.png") : getTexture("gfx/battle/challengeStart.png");
	missionInProgressTexture = !battle.challengeData.isChallenge ? getTexture("gfx/battle/missionInProgress.png") : getTexture("gfx/battle/challengeInProgress.png");
	missionCompleteTexture = !battle.challengeData.isChallenge ? getTexture("gfx/battle/missionComplete.png") : getTexture("gfx/battle/challengeComplete.png");
	missionFailedTexture = !battle.challengeData.isChallenge ? getTexture("gfx/battle/missionFailed.png") : getTexture("gfx/battle/challengeFailed.png");
}

void drawMissionInfo(void)
{
	switch (battle.status)
	{
		case MS_START:
			drawMissionSummary(missionStartTexture);
			drawWidgets("startBattle");
			break;
			
		case MS_PAUSED:
			drawMissionSummary(missionInProgressTexture);
			drawWidgets("startBattle");
			break;
			
		case MS_COMPLETE:
		case MS_FAILED:
			if (!battle.unwinnable)
			{
				if (battle.missionFinishedTimer <= -FPS)
				{
					drawMissionSummary(battle.status == MS_COMPLETE ? missionCompleteTexture : missionFailedTexture);
				
					if (battle.missionFinishedTimer <= -(FPS * 2))
					{
						drawWidgets(battle.status == MS_COMPLETE ? "battleWon" : "battleLost");
					}
				}
			}
			break;
	}
}

static void drawMissionSummary(SDL_Texture *header)
{
	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 128);
	SDL_RenderFillRect(app.renderer, NULL);
	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_NONE);
	
	blit(header, SCREEN_WIDTH / 2, 150, 1);
	
	if (!battle.challengeData.isChallenge)
	{
		drawObjectives();
	}
	else
	{
		drawChallenges();
	}
}

static void drawObjectives(void)
{
	Objective *o;
	SDL_Color color;
	int y = 215;
	
	drawText(SCREEN_WIDTH / 2, y, 28, TA_CENTER, colors.white, _("OBJECTIVES"));
		
	y += 10;
	
	for (o = battle.objectiveHead.next ; o != NULL ; o = o->next)
	{
		if (o->active)
		{
			y += 50;
			
			switch (o->status)
			{
				case OS_INCOMPLETE:
					color = colors.white;
					break;
					
				case OS_COMPLETE:
					color = colors.green;
					break;
					
				case OS_FAILED:
					color = colors.red;
					break;
			}
			
			drawText(SCREEN_WIDTH / 2 - 100, y, 22, TA_RIGHT, colors.white, o->description);
			if (o->targetValue > 1 && !o->isCondition)
			{
				drawText(SCREEN_WIDTH / 2, y, 22, TA_CENTER, colors.white, "%d / %d", o->currentValue, o->targetValue);
			}
			drawText(SCREEN_WIDTH / 2 + 100, y, 22, TA_LEFT, color, objectiveStatus[o->status]);
		}
	}
	
	if (!battle.objectiveHead.next)
	{
		y += 50;
		
		drawText(SCREEN_WIDTH / 2, y, 22, TA_CENTER, colors.white, _("(none)"));
	}
	
	y += 75;
}

static void drawChallenges(void)
{
	Challenge *c;
	char *challengeStatus;
	SDL_Color color;
	int y = 215;
	
	drawText(SCREEN_WIDTH / 2, y, 24, TA_CENTER, colors.white, game.currentMission->description);
	
	if (battle.status == MS_START && battle.challengeData.timeLimit)
	{
		y+= 50;
		
		drawText(SCREEN_WIDTH / 2, y, 20, TA_CENTER, colors.white, "Time Limit: %s", timeToString(battle.challengeData.timeLimit, 0));
	}
		
	y += 25;
	
	for (c = game.currentMission->challengeHead.next ; c != NULL ; c = c->next)
	{
		y += 50;
		
		color = colors.white;
		
		challengeStatus = _("Incomplete");
		
		if (c->passed)
		{
			color = colors.green;
			
			challengeStatus = _("Complete");
		}
		else if (battle.status == MS_COMPLETE ||battle.status == MS_FAILED)
		{
			color = colors.red;
			
			challengeStatus = _("Failed");
		}
		
		drawText(SCREEN_WIDTH / 2 - 50, y, 22, TA_RIGHT, colors.white, "%s", getChallengeDescription(c));
		drawText(SCREEN_WIDTH / 2 + 50, y, 22, TA_LEFT, color, challengeStatus);
	}
}

