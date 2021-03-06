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

#include "draw.h"

static PointF backgroundPoint[4];

void initBackground(void)
{
	backgroundPoint[0].x = -SCREEN_WIDTH / 2;
	backgroundPoint[0].y = -SCREEN_HEIGHT / 2;
	
	backgroundPoint[1].x = SCREEN_WIDTH / 2;
	backgroundPoint[1].y = -SCREEN_HEIGHT / 2;
	
	backgroundPoint[2].x = -SCREEN_WIDTH / 2;
	backgroundPoint[2].y = SCREEN_HEIGHT / 2;
	
	backgroundPoint[3].x = SCREEN_WIDTH / 2;
	backgroundPoint[3].y = SCREEN_HEIGHT / 2;
}

void prepareScene(void)
{
	SDL_SetRenderTarget(app.renderer, app.backBuffer);
	SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(app.renderer);
}

void presentScene(void)
{
	if (dev.debug)
	{
		drawText(5, SCREEN_HEIGHT - 25, 14, TA_LEFT, colors.white, "DEBUG MODE");
		if (dev.showFPS)
		{
			drawText(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 25, 14, TA_CENTER, colors.white, "FPS: %d", dev.fps);
		}
	}
	
	if (!app.hideMouse)
	{
		drawMouse();
	}
	
	SDL_SetRenderTarget(app.renderer, NULL);
	SDL_RenderCopy(app.renderer, app.backBuffer, NULL, NULL);
	SDL_RenderPresent(app.renderer);
}

void blit(SDL_Texture *texture, int x, int y, int center)
{
	SDL_Rect dstRect;
	
	dstRect.x = x;
	dstRect.y = y;
	SDL_QueryTexture(texture, NULL, NULL, &dstRect.w, &dstRect.h);

	if (center)
	{
		dstRect.x -= (dstRect.w / 2);
		dstRect.y -= (dstRect.h / 2);
	}

	SDL_RenderCopy(app.renderer, texture, NULL, &dstRect);
}

void blitScaled(SDL_Texture *texture, int x, int y, int w, int h, int center)
{
	SDL_Rect dstRect;
	
	dstRect.x = x;
	dstRect.y = y;
	dstRect.w = w;
	dstRect.h = h;
	
	if (center)
	{
		dstRect.x -= (dstRect.w / 2);
		dstRect.y -= (dstRect.h / 2);
	}

	SDL_RenderCopy(app.renderer, texture, NULL, &dstRect);
}

void blitRotated(SDL_Texture *texture, int x, int y, float angle)
{
	SDL_Rect dstRect;
	
	dstRect.x = x;
	dstRect.y = y;
	SDL_QueryTexture(texture, NULL, NULL, &dstRect.w, &dstRect.h);
	dstRect.x -= (dstRect.w / 2);
	dstRect.y -= (dstRect.h / 2);

	SDL_RenderCopyEx(app.renderer, texture, NULL, &dstRect, angle, NULL, SDL_FLIP_NONE);
}

void drawCircle(int cx, int cy, int radius, int r, int g, int b, int a)
{
	int x = radius;
	int y = 0;
	int radiusError = 1 - x;
	SDL_Point p[8];

	SDL_SetRenderDrawColor(app.renderer, r, g, b, a);
	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);

	while (x >= y)
	{
		p[0].x =  x + cx; p[0].y = y + cy;
		p[1].x =  y + cx; p[1].y =  x + cy;
		p[2].x = -x + cx; p[2].y =  y + cy;
		p[3].x = -y + cx; p[3].y =  x + cy;
		p[4].x = -x + cx; p[4].y = -y + cy;
		p[5].x = -y + cx; p[5].y = -x + cy;
		p[6].x =  x + cx; p[6].y = -y + cy;
		p[7].x =  y + cx; p[7].y = -x + cy;
		SDL_RenderDrawPoints(app.renderer, p, 8);

		y++;

		if (radiusError < 0)
		{
			radiusError += 2 * y + 1;
		}
		else
		{
			x--;
			radiusError += 2 * (y - x) + 1;
		}
	}

	SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_NONE);
}

void scrollBackground(float x, float y)
{
	int i;
	
	for (i = 0 ; i < 4 ; i++)
	{
		backgroundPoint[i].x += x;
		backgroundPoint[i].y += y;
		
		if (backgroundPoint[i].x < 0)
		{
			backgroundPoint[i].x += (SCREEN_WIDTH * 2);
		}
		
		if (backgroundPoint[i].x >= SCREEN_WIDTH)
		{
			backgroundPoint[i].x -= (SCREEN_WIDTH * 2);
		}
		
		if (backgroundPoint[i].y < 0)
		{
			backgroundPoint[i].y += (SCREEN_HEIGHT * 2);
		}
		
		if (backgroundPoint[i].y >= SCREEN_HEIGHT)
		{
			backgroundPoint[i].y -= (SCREEN_HEIGHT * 2);
		}
	}
}

void drawBackground(SDL_Texture *texture)
{
	int i;
	
	for (i = 0 ; i < 4 ; i++)
	{
		blitScaled(texture, backgroundPoint[i].x, backgroundPoint[i].y, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	}
}

int isOnBattleScreen(int x, int y, int w, int h)
{
	x -= (w / 2);
	x -= (SCREEN_WIDTH / 2);
	x -= battle.camera.x;
	
	y -= (h / 2);
	y -= (SCREEN_HEIGHT / 2);
	y -= battle.camera.y;
	
	w *= 2;
	w += SCREEN_WIDTH;
	
	h *= 2;
	h += SCREEN_HEIGHT;
	
	return collision(x, y, w, h, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void saveScreenshot(void)
{
	static int i = 0;
	char filename[MAX_NAME_LENGTH];
	SDL_Surface *sshot;
	
	sprintf(filename, "/tmp/tbftss/%d.bmp", ++i);
	
	sshot = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	SDL_RenderReadPixels(app.renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
	SDL_SaveBMP(sshot, filename);
	SDL_FreeSurface(sshot);
	
	if (!dev.takeScreenshots)
	{
		printf("Saved '%s'\n", filename);
	}
}
