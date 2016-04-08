// A small test program to exercise most of TIGR's features.

#include "tigr.h"
#include <math.h>

float playerx = 160, playery = 200;
float playerxs = 0, playerys = 0;
bool standing = true;
float remaining = 0;
Tigr *backdrop, *screen;

// Some simple platformer-esque physics.
// I do not necessarily recommend this as a good way of implementing a platformer :)
void update(float dt)
{
	if (remaining > 0)
		remaining -= dt;

	// Read the keyboard and move the player.
	if (standing && tigrKeyDown(screen, TK_SPACE))
		playerys -= 200;
	if (tigrKeyHeld(screen, TK_LEFT) || tigrKeyHeld(screen, 'A'))
		playerxs -= 10;
	if (tigrKeyHeld(screen, TK_RIGHT) || tigrKeyHeld(screen, 'D'))
		playerxs += 10;

	float oldx = playerx, oldy = playery;

	// Apply simply physics.
	playerxs *= exp(-10.0f*dt);
	playerys *= exp(-2.0f*dt);
	playerys += dt * 200.0f;
	playerx += dt * playerxs;
	playery += dt * playerys;

	// Apply collision.
	if (playerx < 8)
	{
		playerx = 8;
		playerxs = 0;
	}

	if (playerx > screen->w - 8)
	{
		playerx = screen->w - 8.0f;
		playerxs = 0;
	}

	// Apply playfield collision and stepping.
	float dx = (playerx - oldx) / 10;
	float dy = (playery - oldy) / 10;
	standing = false;
	for (int n=0;n<10;n++)
	{
		TPixel p = tigrGet(backdrop, (int)oldx, (int)oldy-1);
		if (p.r == 0 && p.g == 0 && p.b == 0)
			oldy -= 1;
		p = tigrGet(backdrop, (int)oldx, (int)oldy);
		if (p.r == 0 && p.g == 0 && p.b == 0 && playerys > 0) {
			playerys = 0;
			dy = 0;
			standing = true;
		}
		oldx += dx; oldy += dy;
	}

	playerx = oldx; playery = oldy;
}

int main(int argc, char *argv[])
{
	// Load our sprite.
	Tigr *squinkle = tigrLoadImage("squinkle.png");
	if (!squinkle)
		tigrError(0, "Cannot load squinkle.png");

	// Load some UTF-8 text.
	char *greeting = (char *)tigrReadFile("greeting.txt", 0);
	if (!greeting)
		tigrError(0, "Cannot load greeting.txt");

	// Make a window and an off-screen backdrop.
	screen = tigrWindow(320, 240, greeting, TIGR_2X);
	backdrop = tigrBitmap(screen->w, screen->h);

	// Fill in the background.
	tigrClear(backdrop, tigrRGB(80,180,255));
	tigrFill(backdrop, 0, 200, 320, 40, tigrRGB(60, 120, 60));
	tigrFill(backdrop, 0, 200, 320, 3,  tigrRGB(0, 0, 0));
	tigrLine(backdrop, 0, 201, 320, 201, tigrRGB(255,255,255));

	// Enable post fx
	tigrSetPostFX(screen, 1, 1, 1, 2.0f);

	int prevx=0, prevy=0, prev = 0;

	// Maintain a list of characters entered.
	int chars[16];
	for (int n=0;n<16;n++) chars[n] = '_';

	// Repeat till they close the window.
	while (!tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE))
	{
		// Update the game.
		float dt = tigrTime();
		update(dt);

		// Read the mouse and draw lines when pressed.
		int x, y, b;
		tigrMouse(screen, &x, &y, &b);
		if (b&1)
		{
			if (prev)
				tigrLine(backdrop, prevx, prevy, x, y, tigrRGB(0,0,0));
			prevx = x; prevy = y; prev = 1;
		} else {
			prev = 0;
		}

		// Composite the backdrop and sprite onto the screen.
		tigrBlit(screen, backdrop, 0,0,0,0, backdrop->w, backdrop->h);
		tigrBlitAlpha(screen, squinkle,
			(int)playerx-squinkle->w/2, (int)playery-squinkle->h, 0, 0, squinkle->w, squinkle->h, 1.0f);

		tigrPrint(screen, tfont, 10, 10, tigrRGBA(0xc0,0xd0,0xff,0xc0), greeting);
		tigrPrint(screen, tfont, 10, 222, tigrRGBA(0xff,0xff,0xff,0xff), "A D + SPACE");

		// Grab any chars and add them to our buffer.
		for (;;) {
			int c = tigrReadChar(screen);
			if (c == 0)
				break;
			for (int n=1;n<16;n++)
				chars[n-1] = chars[n];
			chars[15] = c;
		}

		// Print out the character buffer too.
		char tmp[100], *p = tmp;
		for (int n=0;n<16;n++)
			p = tigrEncodeUTF8(p, chars[n]);
		*p = 0;
		tigrPrint(screen, tfont, 160, 222, tigrRGB(255,255,255), "Chars: %s", tmp);

		// Update the window.
		tigrUpdate(screen);
	}

	tigrFree(squinkle);
	tigrFree(backdrop);
	tigrFree(screen);
	return 0;
}
