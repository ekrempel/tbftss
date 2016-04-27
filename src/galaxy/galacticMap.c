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

#include "galacticMap.h"

static void logic(void);
static void draw(void);
static void handleKeyboard(void);
static void handleMouse(void);
static void scrollGalaxy(void);
static void drawStarSystemDetail(void);
static void selectStarSystem(void);
static void drawGalaxy(void);
static void centerOnSelectedStarSystem(void);
static void doStarSystems(void);
void destroyGalacticMap(void);
static void drawPulses(void);
static void drawInfoBars(void);
static void doPulses(void);
static void addPulses(void);
static void drawMenu(void);
static void resume(void);
static void stats(void);
static void trophies(void);
static void options(void);
static void ok(void);
static void quit(void);
static void startMission(void);
static void returnFromOptions(void);
static void doStarSystemView(void);
static void updatePandoranAdvance(void);
static void fallenOK(void);

static StarSystem *selectedStarSystem;
static SDL_Texture *background;
static SDL_Texture *starSystemTexture;
static SDL_Texture *arrowTexture;
static SDL_Point camera;
static Pulse pulseHead = {0};
static Pulse *pulseTail;
static int pulseTimer;
static float ssx, ssy;
static float arrowPulse;
static int show;
static int scrollingMap;
static PointF cameraMin, cameraMax;
static Widget *startMissionButton;

void initGalacticMap(void)
{
	show = SHOW_GALAXY;

	startSectionTransition();

	stopMusic();

	app.delegate.logic = &logic;
	app.delegate.draw = &draw;
	memset(&app.keyboard, 0, sizeof(int) * MAX_KEYBOARD_KEYS);

	background = getTexture("gfx/backgrounds/background02.jpg");

	starSystemTexture = getTexture("gfx/galaxy/starSystem.png");

	arrowTexture = getTexture("gfx/galaxy/arrow.png");

	selectedStarSystem = getStarSystem(game.selectedStarSystem);

	centerOnSelectedStarSystem();

	updateAllMissions();

	updatePandoranAdvance();

	awardCampaignTrophies();

	awardStatsTrophies();

	saveGame();

	pulseTimer = 0;

	arrowPulse = 0;

	/* clear the pulses */
	destroyGalacticMap();

	initBackground();

	startMissionButton = getWidget("startMission", "starSystem");
	startMissionButton->action = startMission;

	getWidget("resume", "galacticMap")->action = resume;
	getWidget("stats", "galacticMap")->action = stats;
	getWidget("trophies", "galacticMap")->action = trophies;
	getWidget("options", "galacticMap")->action = options;
	getWidget("quit", "galacticMap")->action = quit;

	getWidget("ok", "stats")->action = ok;
	getWidget("ok", "trophies")->action = ok;

	getWidget("ok", "fallen")->action = fallenOK;

	endSectionTransition();

	playMusic("music/main/Pressure.ogg");
}

static void updatePandoranAdvance(void)
{
	StarSystem *starSystem, *fallenStarSystem;

	fallenStarSystem = NULL;

	for (starSystem = game.starSystemHead.next ; starSystem != NULL ; starSystem = starSystem->next)
	{
		if (starSystem->side != SIDE_PANDORAN && starSystem->fallsToPandorans && starSystem->completedMissions == starSystem->totalMissions && starSystem->totalMissions > 0)
		{
			starSystem->side = SIDE_PANDORAN;

			fallenStarSystem = starSystem;
		}
	}

	if (fallenStarSystem)
	{
		showOKDialog(&fallenOK, _("%s has fallen to the Pandorans"), fallenStarSystem->name);
	}
}

static void logic(void)
{
	handleKeyboard();

	handleMouse();

	switch (show)
	{
		case SHOW_GALAXY:
			doStarSystems();
			scrollGalaxy();
			scrollBackground(-ssx, -ssy);
			doStars(ssx, ssy);
			break;

		case SHOW_STAR_SYSTEM:
			doStarSystemView();
			break;
	}

	doPulses();

	if (pulseTimer % FPS == 0)
	{
		addPulses();
	}

	pulseTimer++;
	pulseTimer %= (FPS * 60);

	arrowPulse += 0.1;

	doWidgets();
	
	doTrophyAlerts();
	
	doTrophies();
}

static void doStarSystems(void)
{
	StarSystem *starSystem;
	int cx, cy;

	if (!scrollingMap)
	{
		cx = app.mouse.x - 32;
		cy = app.mouse.y - 32;

		cameraMin.x = cameraMin.y = 99999;
		cameraMax.x = cameraMax.y = -99999;

		selectedStarSystem = NULL;

		for (starSystem = game.starSystemHead.next ; starSystem != NULL ; starSystem = starSystem->next)
		{
			cameraMin.x = MIN(cameraMin.x, starSystem->x);
			cameraMin.y = MIN(cameraMin.y, starSystem->y);

			cameraMax.x = MAX(cameraMax.x, starSystem->x);
			cameraMax.y = MAX(cameraMax.y, starSystem->y);

			if (starSystem->availableMissions > 0 && collision(cx, cy, 64, 64, starSystem->x - camera.x, starSystem->y - camera.y, 4, 4))
			{
				if (selectedStarSystem != starSystem)
				{
					selectedStarSystem = starSystem;

					if (app.mouse.button[SDL_BUTTON_LEFT])
					{
						selectStarSystem();

						app.mouse.button[SDL_BUTTON_LEFT] = 0;
					}
				}
			}
		}

		cameraMin.x -= SCREEN_WIDTH / 2;
		cameraMin.y -= SCREEN_HEIGHT / 2;

		cameraMax.x -= SCREEN_WIDTH / 2;
		cameraMax.y -= SCREEN_HEIGHT / 2;
	}
}

static void scrollGalaxy(void)
{
	int lastX, lastY;

	lastX = camera.x;
	lastY = camera.y;

	ssx = ssy = 0;

	if (scrollingMap)
	{
		camera.x -= app.mouse.dx * 1.5;
		camera.y -= app.mouse.dy * 1.5;

		ssx = -(app.mouse.dx / 3);
		ssy = -(app.mouse.dy / 3);

		camera.x = MAX(cameraMin.x, MIN(camera.x, cameraMax.x));
		camera.y = MAX(cameraMin.y, MIN(camera.y, cameraMax.y));
	}

	if (lastX == camera.x)
	{
		ssx = 0;
	}

	if (lastY == camera.y)
	{
		ssy = 0;
	}
}

static void doStarSystemView(void)
{
	Mission *mission;

	for (mission = selectedStarSystem->missionHead.next ; mission != NULL ; mission = mission->next)
	{
		if (mission->available && app.mouse.button[SDL_BUTTON_LEFT] && collision(app.mouse.x - app.mouse.w / 2, app.mouse.y - app.mouse.h / 2, app.mouse.w, app.mouse.h, mission->rect.x, mission->rect.y, mission->rect.w, mission->rect.h))
		{
			if (game.currentMission != mission)
			{
				playSound(SND_GUI_CLICK);
			}

			game.currentMission = mission;
			return;
		}
	}
}

static void addPulses(void)
{
	Pulse *pulse;
	StarSystem *starSystem;

	for (starSystem = game.starSystemHead.next ; starSystem != NULL ; starSystem = starSystem->next)
	{
		if (starSystem->completedMissions < starSystem->availableMissions)
		{
			pulse = malloc(sizeof(Pulse));
			memset(pulse, 0, sizeof(Pulse));

			pulse->x = starSystem->x;
			pulse->y = starSystem->y;
			pulse->life = 255;

			if (!starSystem->isSol)
			{
				pulse->r = 255;
			}
			else
			{
				pulse->g = 255;
			}

			pulseTail->next = pulse;
			pulseTail = pulse;
		}
	}
}

static void doPulses(void)
{
	Pulse *pulse, *prev;

	prev = &pulseHead;

	for (pulse = pulseHead.next ; pulse != NULL ; pulse = pulse->next)
	{
		pulse->size += 0.5;
		pulse->life--;

		if (pulse->life <= 0)
		{
			if (pulse == pulseTail)
			{
				pulseTail = prev;
			}

			prev->next = pulse->next;
			free(pulse);
			pulse = prev;
		}

		prev = pulse;
	}
}

static void draw(void)
{
	drawBackground(background);

	drawStars();

	drawGalaxy();

	drawPulses();

	drawInfoBars();

	switch (show)
	{
		case SHOW_STAR_SYSTEM:
			drawStarSystemDetail();
			break;

		case SHOW_MENU:
			drawMenu();
			break;

		case SHOW_STATS:
			drawStats();
			break;
			
		case SHOW_TROPHIES:
			drawTrophies();
			break;

		case SHOW_OPTIONS:
			drawOptions();
			break;
	}

	drawTrophyAlert();
}

static void drawPulses(void)
{
	Pulse *pulse;

	for (pulse = pulseHead.next ; pulse != NULL ; pulse = pulse->next)
	{
		drawCircle(pulse->x - camera.x, pulse->y - camera.y, pulse->size, pulse->r, pulse->g, pulse->b, pulse->life);
	}
}

static void centerOnSelectedStarSystem(void)
{
	camera.x = selectedStarSystem->x;
	camera.x -= SCREEN_WIDTH / 2;

	camera.y = selectedStarSystem->y;
	camera.y -= SCREEN_HEIGHT / 2;
}

static void drawGalaxy(void)
{
	SDL_Rect r;
	StarSystem *starSystem;
	SDL_Color color;
	float ax, ay, aa;

	for (starSystem = game.starSystemHead.next ; starSystem != NULL ; starSystem = starSystem->next)
	{
		r.x = starSystem->x - camera.x;
		r.y = starSystem->y - camera.y;

		blit(starSystemTexture, r.x, r.y, 1);

		switch (starSystem->side)
		{
			case SIDE_CSN:
				color = colors.cyan;
				break;

			case SIDE_UNF:
				color = colors.white;
				break;

			case SIDE_PANDORAN:
				color = colors.red;
				break;
		}

		drawText(r.x, r.y + 12, 14, TA_CENTER, color, starSystem->name);

		if (starSystem->completedMissions < starSystem->availableMissions)
		{
			ax = r.x;
			ay = r.y;
			aa = -1;

			ax = MAX(MIN(SCREEN_WIDTH - 64, ax), 64);
			ay = MAX(MIN(SCREEN_HEIGHT - 64, ay), 64);

			if (r.x < 0)
			{
				ax = 64 + (sin(arrowPulse) * 10);
				aa = 270;
			}
			else if (r.x > SCREEN_WIDTH)
			{
				ax = SCREEN_WIDTH - 64 + (sin(arrowPulse) * 10);
				aa = 90;
			}
			else if (r.y < 0)
			{
				ay = 64 + (sin(arrowPulse) * 10);
				aa = 0;
			}
			else if (r.y > SCREEN_HEIGHT)
			{
				ay = SCREEN_HEIGHT - 64 + (sin(arrowPulse) * 10);
				aa = 180;
			}

			if (aa != -1)
			{
				if (!starSystem->isSol)
				{
					SDL_SetTextureColorMod(arrowTexture, 255, 0, 0);
				}
				else
				{
					SDL_SetTextureColorMod(arrowTexture, 0, 255, 0);
				}

				blitRotated(arrowTexture, ax, ay, aa);
			}
		}
	}
}

static void drawInfoBars(void)
{
	SDL_Rect r;

	if (show != SHOW_STAR_SYSTEM && selectedStarSystem != NULL)
	{
		r.x = 0;
		r.y = SCREEN_HEIGHT - 35;
		r.w = SCREEN_WIDTH;
		r.h = 35;

		SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 200);
		SDL_RenderFillRect(app.renderer, &r);
		SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_NONE);

		drawText(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, 18, TA_CENTER, colors.white, selectedStarSystem->description);
	}

	r.x = 0;
	r.y = 0;
	r.w = SCREEN_WIDTH;
	r.h = 35;
	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 200);
	SDL_RenderFillRect(app.renderer, &r);
	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_NONE);

	drawText((SCREEN_WIDTH / 2), 5, 18, TA_CENTER, colors.white, _("Missions: %d / %d"), game.completedMissions, game.availableMissions);
}

static void selectStarSystem(void)
{
	if (selectedStarSystem->availableMissions > 0)
	{
		show = SHOW_STAR_SYSTEM;
		STRNCPY(game.selectedStarSystem, selectedStarSystem->name, MAX_NAME_LENGTH);
		game.currentMission = selectedStarSystem->missionHead.next;
		playSound(SND_GUI_SELECT);
	}
}

static void drawStarSystemDetail(void)
{
	int y;
	Mission *mission;
	SDL_Rect r;

	r.w = 900;
	r.h = 600;
	r.x = (SCREEN_WIDTH / 2) - (r.w / 2);
	r.y = (SCREEN_HEIGHT / 2) - (r.h / 2);
	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);

	SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 225);
	SDL_RenderFillRect(app.renderer, &r);

	SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 200);
	SDL_RenderDrawRect(app.renderer, &r);

	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_NONE);

	y = 70;

	drawText(SCREEN_WIDTH / 2, y, 28, TA_CENTER, colors.cyan, selectedStarSystem->name);

	SDL_RenderDrawLine(app.renderer, r.x, 120, r.x + r.w - 1, 120);

	SDL_RenderDrawLine(app.renderer, 515, 120, 515, 660);

	y += 80;

	for (mission = selectedStarSystem->missionHead.next ; mission != NULL ; mission = mission->next)
	{
		mission->rect.x = 200;
		mission->rect.y = y - 2;
		mission->rect.w = 300;
		mission->rect.h = 40;

		if (mission == game.currentMission)
		{
			SDL_SetRenderDrawColor(app.renderer, 32, 64, 128, 255);
			SDL_RenderFillRect(app.renderer, &mission->rect);

			SDL_SetRenderDrawColor(app.renderer, 64, 96, 196, 255);
			SDL_RenderDrawRect(app.renderer, &mission->rect);
		}

		if (mission->available)
		{
			drawText(210, y, 24, TA_LEFT, mission->completed ? colors.lightGrey : colors.yellow, mission->name);
		}

		y += 50;
	}

	if (game.currentMission->available)
	{
		drawText(525, 135, 18, TA_LEFT, colors.lightGrey, _("Pilot: %s"), game.currentMission->pilot);
		drawText(525, 160, 18, TA_LEFT, colors.lightGrey, _("Craft: %s"), game.currentMission->craft);
		drawText(525, 185, 18, TA_LEFT, colors.lightGrey, _("Squadron: %s"), game.currentMission->squadron);

		limitTextWidth(500);
		drawText(525, 230, 22, TA_LEFT, colors.white, game.currentMission->description);
		limitTextWidth(0);
	}

	if (game.currentMission->completed)
	{
		drawText(525, SCREEN_HEIGHT - 95, 18, TA_LEFT, colors.green, _("This mission has been completed."));
	}
	else if (game.currentMission->epic)
	{
		drawText(525, SCREEN_HEIGHT - 95, 18, TA_LEFT, colors.yellow, _("Note: this is an Epic Mission."));
	}

	startMissionButton->enabled = (!game.currentMission->completed || selectedStarSystem->isSol);

	drawWidgets("starSystem");
}

static void fallenOK(void)
{
	show = SHOW_GALAXY;

	app.modalDialog.type = MD_NONE;
}

static void handleKeyboard(void)
{
	if (app.keyboard[SDL_SCANCODE_ESCAPE] && !app.awaitingWidgetInput)
	{
		switch (show)
		{
			case SHOW_GALAXY:
				selectWidget("resume", "galacticMap");
				show = SHOW_MENU;
				playSound(SND_GUI_CLOSE);
				break;

			case SHOW_STAR_SYSTEM:
				show = SHOW_GALAXY;
				break;

			case SHOW_MENU:
				show = SHOW_GALAXY;
				break;

			case SHOW_OPTIONS:
			case SHOW_STATS:
				show = SHOW_MENU;
				selectWidget("resume", "galacticMap");
				break;
		}

		playSound(SND_GUI_CLOSE);

		clearInput();
	}
}

static void handleMouse(void)
{
	if (app.mouse.button[SDL_BUTTON_LEFT])
	{
		if (app.mouse.dx != 0 || app.mouse.dy != 0)
		{
			scrollingMap = 1;
		}
	}
	else
	{
		scrollingMap = 0;
	}
}

static void startMission(void)
{
	initBattle();

	loadMission(game.currentMission->filename);
}

static void drawMenu(void)
{
	SDL_Rect r;

	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 128);
	SDL_RenderFillRect(app.renderer, NULL);
	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_NONE);

	r.w = 400;
	r.h = 500;
	r.x = (SCREEN_WIDTH / 2) - r.w / 2;
	r.y = (SCREEN_HEIGHT / 2) - r.h / 2;

	SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 0);
	SDL_RenderFillRect(app.renderer, &r);
	SDL_SetRenderDrawColor(app.renderer, 200, 200, 200, 255);
	SDL_RenderDrawRect(app.renderer, &r);

	drawWidgets("galacticMap");
}

static void resume(void)
{
	show = SHOW_GALAXY;
}

static void options(void)
{
	show = SHOW_OPTIONS;

	initOptions(returnFromOptions);
}

static void stats(void)
{
	selectWidget("ok", "stats");

	show = SHOW_STATS;

	initStatsDisplay();
}

static void trophies(void)
{
	selectWidget("ok", "trophies");

	show = SHOW_TROPHIES;

	initTrophiesDisplay();
}

static void ok(void)
{
	selectWidget("resume", "galacticMap");

	show = SHOW_MENU;
}

static void returnFromOptions(void)
{
	show = SHOW_MENU;

	selectWidget("resume", "galacticMap");
}

static void quit(void)
{
	initTitle();
}

void destroyGalacticMap(void)
{
	Pulse *pulse;

	while (pulseHead.next)
	{
		pulse = pulseHead.next;
		pulseHead.next = pulse->next;
		free(pulse);
	}

	pulseTail = &pulseHead;
}
