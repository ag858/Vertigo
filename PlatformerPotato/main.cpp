#include <raylib.h>
#include <stdio.h>
#include <math.h>
#include <random>
#include "idle_sheet.h"
#include "jump_sheet.h"
#include "run_sheet.h"
#include "beam.h"
#include "high_rise.h"


#define W 800
#define H 450
#define G 800
#define JUMP_SPEED 500.0f
#define HORIZONTAL_SPEED 250.0f
#define NUM_PLATFORMS 1000
#define COUNTDOWN_MAX 5

static enum Screen { titlescreen, game, endscreen };
static Screen currentScreen;
static int countdown = 5;
static int frame = 0;
static int frameCounter = 0;
static int frameMax = 0;
static int heightPB = 0;
static float TimeGameStartedInSeconds = 0;
static int numRegensUsed = -1;
static bool animating = false;
static float fadeAmount = 1.0f;
static Sound jumpSounds[4];
static Sound regenSounds[2];
static Sound bonusSound;
static Sound tickSound;
static Camera2D camera;
static Rectangle platforms[NUM_PLATFORMS];
static Texture2D* sheetToUse;
static Texture2D runSheet;
static Texture2D jumpSheet;
static Texture2D idleSheet;
static Texture2D highRise;
static Texture2D beam;


struct Player
{
	float x, y, speed;
	bool canJump, facingRight, horMoving;
} player;
	


static void RegeneratePlatforms()
{
	numRegensUsed++;
	PlaySound(regenSounds[rand() % 2]);
	fadeAmount = 1.0f;

	for (int n = 0; n < 10; n++)
	{
		platforms[n] = {n * 100.0f, 400, 100, 20 };
	}

	for (int n = 10; n < NUM_PLATFORMS; n++)
	{
		platforms[n] = { (float)GetRandomValue(0, 800), -75.0f * n + 1000, 100 , 1 };
	}
}


static void UpdatePlayer(float Xmin, float Xmax)
{
	if (IsKeyReleased(KEY_ENTER) || IsGamepadButtonReleased(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
	{
		RegeneratePlatforms();
	}

	player.horMoving = false;

	if (player.x <= Xmin)
		player.x = Xmin;

	if (player.x >= Xmax)
		player.x = Xmax;

	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
	{
		player.x -= HORIZONTAL_SPEED * GetFrameTime();
		player.facingRight = false;
		player.horMoving = true;
	}

	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D) || (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)))
	{
		player.x += HORIZONTAL_SPEED * GetFrameTime();
		player.facingRight = true;
		player.horMoving = true;
	}

	if ((IsKeyDown(KEY_SPACE) || (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) && player.canJump)
	{
		PlaySound(jumpSounds[rand() % 4]);
		player.speed = -JUMP_SPEED;
		player.canJump = false;
	}

	int hitObstacle = 0;
	for (int i = 0; i < NUM_PLATFORMS; i++)
	{
		if (platforms[i].x <= player.x &&
			platforms[i].x + platforms[i].width >= player.x &&
			platforms[i].y >= player.y &&
			platforms[i].y <= player.y + player.speed * GetFrameTime())
		{
			hitObstacle = 1;
			player.speed = 0.0f;
			player.y = platforms[i].y;
		}
	}

	if (!hitObstacle)
	{
		player.canJump = false;
		player.y += player.speed * GetFrameTime();
		player.speed += G * GetFrameTime();
		sheetToUse = &jumpSheet;
		frame = 48;
	}
	else
	{
		player.canJump = true;
		if (player.horMoving)
		{
			sheetToUse = &runSheet;
			frame = frameCounter % 23;
		}
		else
		{
			sheetToUse = &idleSheet;
			frame = frameCounter % 35;
		}
	}
}


static float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}


static void UpdateFollowCamera()
{
	camera.offset = { W * 0.5f, H * 0.7f };
	camera.target = { lerp(camera.target.x, player.x, 0.05f), lerp(camera.target.y, player.y, 0.1f) };
	if (camera.target.x <= camera.offset.x)
	{
		camera.target.x = camera.offset.x;;
	}
	else if (camera.target.x >= 1000 - camera.offset.x)
	{
		camera.target.x = 1000 - camera.offset.x;;
	}
}


int main()
{
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(W, H, "Jumping Dude");
	InitAudioDevice();
	HideCursor();
	SetTargetFPS(60);
	SetExitKey(NULL);
	ToggleFullscreen();
	Font vcr = LoadFont("VCR_MONO.ttf");

	//Image idle = LoadImage("idle_sheet.png");
	//ExportImageAsCode(idle, "idle_sheet.h");
	//Image jump = LoadImage("jump_sheet.png");
	//ExportImageAsCode(jump, "jump_sheet.h");
	//Image run = LoadImage("run_sheet.png");
	//ExportImageAsCode(run, "run_sheet.h");
	/*Image highRiseImage = LoadImage("high_rise.png");
	ExportImageAsCode(highRiseImage, "high_rise.h");
	Image beamImage = LoadImage("beam.png");
	ExportImageAsCode(beamImage, "beam.h");*/

	jumpSounds[0] = LoadSound("jump1.ogg");
	jumpSounds[1] = LoadSound("jump2.ogg");
	jumpSounds[2] = LoadSound("jump3.ogg");
	jumpSounds[3] = LoadSound("jump4.ogg");

	regenSounds[0] = LoadSound("regenerate1.ogg");
	regenSounds[1] = LoadSound("regenerate2.ogg");

	bonusSound = LoadSound("bonusPluck.ogg");
	SetSoundVolume(bonusSound, 0.2f);
	tickSound = LoadSound("tick.ogg");

	Music timerMusic = LoadMusicStream("plinkypluck.ogg");
	SetMusicPitch(timerMusic, 0.5f);
	PlayMusicStream(timerMusic);
	Music wind = LoadMusicStream("wind.ogg");
	Music wind2 = LoadMusicStream("wind.ogg");
	SetMusicPitch(wind, 0.5f);
	PlayMusicStream(wind);
	
	idleSheet = LoadTextureFromImage( LoadImagePro(IDLE_SHEET_DATA, 64, 2480, 7));
	runSheet = LoadTextureFromImage( LoadImagePro(RUN_SHEET_DATA, 64, 1840, 7));
	jumpSheet = LoadTextureFromImage( LoadImagePro(JUMP_SHEET_DATA, 64, 2720, 7));
	highRise = LoadTextureFromImage(LoadImagePro(HIGH_RISE_DATA, 1024, 1024, 7));
	beam = LoadTextureFromImage(LoadImagePro(BEAM_DATA, 100, 30, 7));
	
	RegeneratePlatforms();

	player.x = 400;
	player.y = 400;
	camera.target = { player.x, player.y };
	camera.offset = { W * 0.5f, H *  0.7f };


	while (!WindowShouldClose())
	{
		UpdateMusicStream(timerMusic);
		UpdateMusicStream(wind);

		if (currentScreen == titlescreen)
		{
			if (IsKeyReleased(KEY_ENTER))
			{
				fadeAmount = 1.0f;
				currentScreen = game;
				TimeGameStartedInSeconds = GetTime();
			}
			
			if (fadeAmount > 0)
			{
				fadeAmount -= GetFrameTime() * 0.3f;
				SetMusicVolume(timerMusic, fadeAmount);
			}
			
			camera.zoom = 1.0f - fadeAmount;
			camera.rotation = sin(GetTime()) * 1.0f;
			frameCounter++;
			frame = (frameCounter / 4) % 30;

			BeginDrawing();
			BeginMode2D(camera);
			ClearBackground(RAYWHITE);

			DrawTextEx(vcr, "VERTIGO", { camera.target.x - camera.offset.x + 30, camera.target.y - camera.offset.y + 30 }, 32, 0, BLACK);
			const char* instructions1 = "USE THE PLATFORMS TO SCALE THE BUILDING.";
			DrawTextEx(vcr, instructions1, { camera.target.x - camera.offset.x + 10, camera.target.y}, 12, 0, BLACK);
			const char* instructions2 = "THE COUNTDOWN TIMER WILL BE EXTENDED AS LONG AS YOU KEEP REACHING NEW HEIGHTS.";
			DrawTextEx(vcr, instructions2, { camera.target.x - camera.offset.x + 10, camera.target.y + 12}, 12, 0, BLACK);
			const char* instructions3 = "WHEN THE COUNTDOWN REACHES ZERO, IT'S GAME OVER.";
			DrawTextEx(vcr, instructions3, { camera.target.x - camera.offset.x + 10, camera.target.y + 24 }, 12, 0, BLACK);
			const char* instructions4 = "YOU CAN REGENERATE THE PLATFORMS TO POSSIBLY CREATE A BETTER PATH.";
			DrawTextEx(vcr, instructions4, { camera.target.x - camera.offset.x + 10, camera.target.y + 36 }, 12, 0, BLACK);
			const char* instructions5 = "[PRESS ENTER TO PLAY]";
			DrawTextEx(vcr, instructions5, { camera.target.x - camera.offset.x + 10, camera.target.y + 48 }, 16, 0, BLACK);
			const char* instructions6 = "Copyright Quad Corps 2023";
			DrawText(instructions6, camera.target.x - camera.offset.x + 10, camera.target.y + 150, 10, BLACK);

			DrawTexturePro(jumpSheet, { 0, frame * 80.0f, 64, 80 }, { 100, 100, 128, 160 }, {0, 0}, 0, Fade(GRAY, 0.5f));

			DrawRectangle(camera.target.x - camera.offset.x, camera.target.y - camera.offset.y, W + 100, H + 100, Fade(BLACK, fadeAmount));
			EndMode2D();
			EndDrawing();
		}
		
		if (currentScreen == game)
		{
			SetMusicVolume(timerMusic, 0.5f);
			
			if (fadeAmount > 0)
			{
				fadeAmount -= GetFrameTime() * 2.0f;
			}

			if (frame > 20 && !IsMusicPlaying(wind2))
			{
				PlayMusicStream(wind2);
			}

			UpdatePlayer(20.0f, 900.0f);
			UpdateFollowCamera();

			camera.rotation = sin(GetTime()) * -player.y / 10000.0f;
			camera.zoom = 1.0f + sin(GetTime() * -player.y * 0.1f) / 10000.0f;
			frameCounter++;

			BeginDrawing();
			BeginMode2D(camera);

			ClearBackground(BLACK);

			for (int i = 0; i < 200; i++)
			{
				DrawTexturePro(highRise, { 0.0f, 0.0f, 2048.0f, 2048.0f }, { 0.0f, -1024.0f * i + 74, 1024.0f, 1024.0f }, { 0, 0 }, 0, DARKGRAY);
			}

			for (int i = 0; i < NUM_PLATFORMS; i++)
			{
				DrawTexture(beam, platforms[i].x, platforms[i].y, WHITE);
			}


			{
				const Rectangle playerRect = { player.x - 32, player.y - 80, 64, 80 };
				if (player.facingRight)
				{
					DrawTexturePro(*sheetToUse, {0, frame * 80.0f, 64, 80}, playerRect, { 0,0 }, 0, WHITE);
				}
				else
				{
					DrawTexturePro(*sheetToUse, {64, frame * 80.0f, -64, 80}, playerRect, {0,0}, 0, WHITE);
				}
			}

		
			{
				DrawRectangleGradientV(camera.target.x - camera.offset.x - 5, camera.target.y - camera.offset.y - 10, W, H/10, BLACK, Fade(BLACK, 0.0f));
			
				const float currentFT = (-player.y + 400.0f) * 0.1f;
				DrawTextEx(vcr, TextFormat("(%0.0f FT)", currentFT), { player.x - 30, player.y - 100 }, 12, 1, RAYWHITE);

				Color colourPB = RAYWHITE;
				if (currentFT > heightPB)
				{
					PlaySound(bonusSound);
					colourPB = LIME;
					heightPB = currentFT;
				}
				
				DrawTextEx(vcr, TextFormat("HIGHEST: %i ft", heightPB), { camera.target.x - camera.offset.x, camera.target.y - camera.offset.y}, 20, 1, colourPB);
			
				DrawTextEx(vcr, "(PRESS ENTER TO REGENERATE PLATFORMS)", { camera.target.x, camera.target.y - camera.offset.y }, 10, 1, WHITE);
			}
			
			
			DrawRectangle(camera.target.x - camera.offset.x, camera.target.y - camera.offset.y, W, H + 100, Fade(RAYWHITE, fadeAmount));

			EndMode2D();
			EndDrawing();			
		}

	}
}