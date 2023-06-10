#include <raylib.h>
#include <vector>
#include <iostream>

// 800 x 450
static constexpr int W = 800;
static constexpr int H = 450;
static constexpr float G = 800.0f;
static constexpr float JUMP_SPEED = 500.0f;
static constexpr float HORIZONTAL_SPEED = 250.0f;
static int frame = 0;
static int frameCounter = 0;
static int frameMax = 0;
static int countdownMax = 5;
static int countdown = countdownMax;
static int heightPB = 0;
static bool animating = false;
static bool titlescreen = true;
static float fadeAmount = 1.0f;
static Sound jumpSounds[4];
static Sound regenSounds[2];


struct Player
{
	float x, y, speed;
	bool canJump, facingRight, horMoving;
};


struct EnvItem
{
	Rectangle rect;
	bool blocking;
	Color colour;
};


static void RegeneratePlatforms(std::vector<EnvItem>& envItems)
{
	PlaySound(regenSounds[rand() % 2]);
	fadeAmount = 1.0f;
	envItems.clear();
	envItems.push_back({ { 0, 400, 1000, 400 }, 1, DARKGRAY });

	for (float n = 300; n > -20000; n -= 75)
	{
		envItems.push_back({ { (float)GetRandomValue(0, 800), n, 100 , 1}, 1, WHITE });
	}
}


static void UpdatePlayer(Player& player, std::vector<EnvItem>& envItems, float delta, float Xmin, float Xmax)
{
	if (IsKeyReleased(KEY_ENTER) || IsGamepadButtonReleased(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
	{
		RegeneratePlatforms(envItems);
	}

	player.horMoving = false;

	if (player.x <= Xmin)
		player.x = Xmin;

	if (player.x >= Xmax)
		player.x = Xmax;

	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
	{
		player.x -= HORIZONTAL_SPEED * delta;
		player.facingRight = false;
		player.horMoving = true;
	}

	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D) || (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)))
	{
		player.x += HORIZONTAL_SPEED * delta;
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
	for (const auto& ei : envItems)
	{
		if (ei.blocking &&
			ei.rect.x <= player.x &&
			ei.rect.x + ei.rect.width >= player.x &&
			ei.rect.y >= player.y &&
			ei.rect.y <= player.y + player.speed * delta)
		{
			hitObstacle = 1;
			player.speed = 0.0f;
			player.y = ei.rect.y;
		}
	}

	if (!hitObstacle)
	{
		player.y += player.speed * delta;
		player.speed += G * delta;
		player.canJump = false;
	}
	else
	{
		player.canJump = true;
	}
}


static float lerp(const float& a, const float& b, const float& t)
{
	return a + (b - a) * t;
}


static void UpdateFollowCamera(Camera2D& camera, Player& player, float delta, const int& width, const int& height)
{
	camera.offset = { width * 0.5f, height * 0.7f };
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
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
	InitWindow(W, H, "Jumping Dude");
	InitAudioDevice();
	std::cout << IsAudioDeviceReady() << std::endl;
	HideCursor();
	SetTargetFPS(60);
	SetExitKey(NULL);
	//ToggleFullscreen();
	Font vcr = LoadFont("VCR_MONO.ttf");

	for (int i = 0; i < 4; i++)
	{
		jumpSounds[i] = LoadSound(FormatText("jump%i.ogg", i + 1));
		SetSoundVolume(jumpSounds[i], 0.8f);
	}

	regenSounds[0] = LoadSound("regenerate1.ogg");
	regenSounds[1] = LoadSound("regenerate2.ogg");

	Music timerMusic = LoadMusicStream("plinkypluck.ogg");
	SetMusicPitch(timerMusic, 0.5f);
	PlayMusicStream(timerMusic);
	
	Sound newHeightsSound;

	Texture2D idleSheet = LoadTexture("idle_sheet.png");
	Texture2D runSheet = LoadTexture("run_sheet.png");
	Texture2D jumpSheet = LoadTexture("jump_sheet.png");
	Texture2D highRise = LoadTexture("high_rise.png");
	Texture2D beam = LoadTexture("beam.png");

	Player player;
	player.x = 400;
	player.y = 400;
	player.speed = 0;
	player.canJump = false;
	
	std::vector<EnvItem> envItems;
	RegeneratePlatforms(envItems);

	Camera2D camera;
	camera.target = { player.x, player.y };
	camera.offset = { W * 0.5f, H *  0.7f };
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;


	while (!WindowShouldClose())
	{
		UpdateMusicStream(timerMusic);

		if (titlescreen)
		{
			if (IsKeyReleased(KEY_ENTER))
			{
				fadeAmount = 1.0f;
				titlescreen = false;
			}
			
			if (fadeAmount > 0)
			{
				fadeAmount -= GetFrameTime() * 0.3f;
				SetMusicVolume(timerMusic, fadeAmount);
			}
			
			camera.zoom = 1.0f - fadeAmount;
			camera.rotation = sin(GetTime()) * 1.0f;

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

			DrawTexturePro(jumpSheet, { 0, frame * 80.0f, 64, 80 }, { 200, 200, 128, 160 }, {0, 0}, 0, Fade(GRAY, 0.5f));

			DrawRectangle(camera.target.x - camera.offset.x, camera.target.y - camera.offset.y, W + 100, H + 100, Fade(BLACK, fadeAmount));
			EndMode2D();
			EndDrawing();
			frameCounter++;
			frame = (frameCounter / 4) % 30;
		}
		else
		{
			if (fadeAmount > 0)
			{
				fadeAmount -= GetFrameTime() * 2.0f;
			}
			UpdatePlayer(player, envItems, GetFrameTime(), 20.0f, 900.0f);
			UpdateFollowCamera(camera, player, GetFrameTime(), W, H);
	
			Texture2D* sheetToUse = nullptr;
			if (player.canJump)
			{
				animating = true;
				if (player.horMoving)
				{
					sheetToUse = &runSheet;
					frameMax = 23;
				}
				else
				{
					sheetToUse = &idleSheet;
					frameMax = 35;
				}
			}
			else
			{
				animating = false;
				sheetToUse = &jumpSheet;
				frameMax = 67;
			}
		
			BeginDrawing();
			BeginMode2D(camera);

			ClearBackground(BLACK);

			for (float i = 0.0f; i > -100000.0f; i -= 1024.0f)
			{
				DrawTexturePro(highRise, { 0.0f, 0.0f, 2048.0f, 2048.0f }, { 0.0f, i + 74, 1024.0f, 1024.0f }, { 0, 0 }, 0, DARKGRAY);
			}

			if (!envItems.empty())
			{
				for (const auto& envItem : envItems)
				{
					if (envItem.rect.y == 400)
					{
						DrawRectangleGradientV(envItem.rect.x, envItem.rect.y, envItem.rect.width, envItem.rect.height, BLACK, DARKGRAY);
						for (int i = 0; i < 1000; i += 100)
						{
							DrawTexture(beam, i, 400, WHITE);
						}
					}
					else
					{
						DrawTexture(beam, envItem.rect.x, envItem.rect.y, WHITE);
					}
				}
			}

			if (animating)
			{
				frame = (frameCounter) % frameMax;
			}
			else
			{
				frame = 48;
			}


			Rectangle playerRect = { player.x - 32, player.y - 80, 64, 80 };
			if (player.facingRight)
			{
				DrawTexturePro(*sheetToUse, {0, frame * 80.0f, 64, 80}, playerRect, { 0,0 }, 0, WHITE);
			}
			else
			{
				DrawTexturePro(*sheetToUse, {64, frame * 80.0f, -64, 80}, playerRect, {0,0}, 0, WHITE);
			}

			DrawRectangleGradientV(camera.target.x - camera.offset.x - 5, camera.target.y - camera.offset.y - 10, W, H/10, BLACK, Fade(BLACK, 0.0f));
		
			const char* heightText = TextFormat("(%0.0f ft)", ( - player.y + 400) * 0.1f);
			heightText = TextToUpper(heightText);
			DrawTextEx(vcr, heightText, { player.x - 30, player.y - 100 }, 12, 1, RAYWHITE);

			Color colourPB = static_cast<int>((-player.y + 400) * 0.1f) > heightPB ? GREEN : RAYWHITE;
			const char* recordText = TextFormat("highest: %i ft", heightPB);
			recordText = TextToUpper(recordText);
			DrawTextEx(vcr, recordText, { camera.target.x - camera.offset.x, camera.target.y - camera.offset.y}, 20, 1, colourPB);
			heightPB = static_cast<int>((-player.y + 400) * 0.1f) > heightPB ? static_cast<int>((-player.y + 400) * 0.1f) : heightPB;
		
			DrawTextEx(vcr, "(PRESS ENTER TO REGENERATE PLATFORMS)", { camera.target.x, camera.target.y - camera.offset.y }, 10, 1, WHITE);

			DrawRectangle(camera.target.x - camera.offset.x, camera.target.y - camera.offset.y, W, H + 100, Fade(RAYWHITE, fadeAmount));
			frameCounter++;

			camera.zoom = 1.0f + sin(GetTime() * abs(player.y) / 100) * 0.0001f;
			camera.rotation = sin(GetTime() * 5.0f) * abs(player.y) * 0.0001f;
			EndMode2D();
			EndDrawing();
		}

	}
}