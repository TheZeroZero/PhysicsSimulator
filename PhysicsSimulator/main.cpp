#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <array>
#include <stdio.h>
#include <random> // for std::mt19937 and std::random_device
#include <vector>
#include <cmath>
#include <string>

#include "CircleDrawing.h"
#include "Constants.h"
#include "Vector_2d.h"


enum mouseButtons
{
	left,
	middle,
	right,
	max_mouseButtons
};
struct buttonStates
{
	bool down = false;
	bool held = false;
	bool up = false;
};

namespace Random // capital R to avoid conflicts with functions named random()
{
	std::random_device rd;
	std::seed_seq ss{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() }; // get 8 integers of random numbers from std::random_device for our seed
	std::mt19937 mt{ ss };

	int get(int min, int max)
	{
		std::uniform_int_distribution die{ min, max }; // we can create a distribution in any function that needs it
		return die(mt); // and then generate a random number from our global generator
	}
}

SDL_Color HSVtoRGB(float H, float S, float V) {
	if (H > 360 || H < 0 || S>100 || S < 0 || V>100 || V < 0) {
		printf("The given HSV values are not in valid range\n");
		return SDL_Color(0, 0, 0, 255);
	}
	float s = S / 100;
	float v = V / 100;
	float C = s * v;
	float X = C * (1 - std::abs(std::fmod(H / 60.0, 2) - 1));
	float m = v - C;
	float r, g, b;
	if (H >= 0 && H < 60) {
		r = C, g = X, b = 0;
	}
	else if (H >= 60 && H < 120) {
		r = X, g = C, b = 0;
	}
	else if (H >= 120 && H < 180) {
		r = 0, g = C, b = X;
	}
	else if (H >= 180 && H < 240) {
		r = 0, g = X, b = C;
	}
	else if (H >= 240 && H < 300) {
		r = X, g = 0, b = C;
	}
	else {
		r = C, g = 0, b = X;
	}
	int R = (r + m) * 255;
	int G = (g + m) * 255;
	int B = (b + m) * 255;
	SDL_Color rgba = { R, G, B, 255 };
	return rgba;
}






class Texture
{
public:
	Texture();
	~Texture();

	bool loadFromFile(std::string path);
#if defined(SDL_TTF_MAJOR_VERSION)
	bool loadFromRenderedText(std::string text, SDL_Color textColor);
#endif
	bool createFromSurface(SDL_Surface* surface);

	void free();

	void render(int x, int y, SDL_Rect* clip, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);
	void renderScaled(int x, int y, SDL_Rect* clip, double scale = 1.0, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

	void setColor(Uint8 r, Uint8 g, Uint8 b);
	void setBlendMode(SDL_BlendMode blending);
	void setAlpha(Uint8 a);

	int getWidth() { return m_w; }
	int getHeight() { return m_h; }

	Texture& operator=(Texture& other);
private:
	SDL_Texture* m_texture;
	int m_w;
	int m_h;
};

bool init();
bool loadMedia();
void close();

SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;
TTF_Font* g_font = NULL;

Texture g_background;







Texture::Texture()
{
	m_texture = NULL;
	m_w = 0;
	m_h = 0;
}
Texture::~Texture()
{
	free();
}

void Texture::free()
{
	SDL_DestroyTexture(m_texture);
	m_texture = NULL;
}

bool Texture::loadFromFile(std::string path)
{
	free();

	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Couldn't load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0x00, 0xff, 0xff));
		m_texture = SDL_CreateTextureFromSurface(g_renderer, loadedSurface);
		if (m_texture == NULL)
		{
			printf("Couldn't create texture from surface %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}
		else
		{
			m_w = loadedSurface->w;
			m_h = loadedSurface->h;
		}
		SDL_FreeSurface(loadedSurface);
	}

	return m_texture != NULL;
}

#if defined(SDL_TTF_MAJOR_VERSION)
bool Texture::loadFromRenderedText(std::string text, SDL_Color textColor)
{
	free();

	SDL_Surface* loadedText = TTF_RenderText_Solid(g_font, text.c_str(), textColor);
	if (loadedText == NULL)
	{
		printf("Couldn't create surface from text! SDL_ttf Error: %s\n", TTF_GetError());
	}
	else
	{
		m_texture = SDL_CreateTextureFromSurface(g_renderer, loadedText);
		if (m_texture == NULL)
		{
			printf("Couldn't create texture from surface text! SDL Error: %s\n", SDL_GetError());
		}
		else
		{
			m_w = loadedText->w;
			m_h = loadedText->h;
		}

		SDL_FreeSurface(loadedText);
	}

	return m_texture != NULL;
}
#endif

Texture& Texture::operator=(Texture& other)
{
	//self guard
	if (this == &other)
		return *this;

	m_w = other.m_w;
	m_h = other.m_h;
	m_texture = other.m_texture;

	return other;
}

bool Texture::createFromSurface(SDL_Surface* surface)
{
	free();

	m_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	if (m_texture == NULL)
	{
		printf("Couldn't create texture from surface text! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		m_w = surface->w;
		m_h = surface->h;
	}


	return m_texture != NULL;
}

void Texture::render(int x, int y, SDL_Rect* clip=NULL, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	SDL_Rect RenderedQuad = { x, y, m_w, m_h };
	if (clip != NULL)
	{
		RenderedQuad.w = clip->w;
		RenderedQuad.h = clip->h;
	}

	SDL_RenderCopyEx(g_renderer, m_texture, clip, &RenderedQuad, angle, center, flip);
}

//For scaling you need to provide a clip rect
void Texture::renderScaled(int x, int y, SDL_Rect* clip, double scale, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	SDL_Rect RenderedQuad = { x, y, m_w, m_h };
	if (clip != NULL)
	{
		RenderedQuad.w = clip->w * scale;
		RenderedQuad.h = clip->h * scale;
	}
	else
	{
		printf("Warning: Used Texture::renderScale(), without providing a clip rect.\n");
	}

	SDL_RenderCopyEx(g_renderer, m_texture, clip, &RenderedQuad, angle, center, flip);
}

void Texture::setColor(Uint8 r, Uint8 g, Uint8 b)
{
	SDL_SetTextureColorMod(m_texture, r, g, b);
}

void Texture::setBlendMode(SDL_BlendMode blending)
{
	SDL_SetTextureBlendMode(m_texture, blending);
}

void Texture::setAlpha(Uint8 a)
{
	SDL_SetTextureAlphaMod(m_texture, a);
}









bool init()
{
	bool success = true;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Couldn't initialize SDL! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering is not enabled\n");
		}

		g_window = SDL_CreateWindow("Physics Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (g_window == NULL)
		{
			printf("Couldn't create window! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create vsynced renderer for window
			//g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			//Create unvsynced renderer
			g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
			if (g_renderer == NULL)
			{
				printf("Couldn't create renderer! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				SDL_SetRenderDrawColor(g_renderer, 0xff, 0xff, 0xff, 0xff);

				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("Couldn't initialize SDL_image! SDL_image Error: %s\n", IMG_GetError());
					success = false;
				}
				if (TTF_Init() == -1)
				{
					printf("Couldn't initialize SDL_ttf! SDL_ttf Error: %s\n", TTF_GetError());
					success = false;
				}

			}
		}
	}

	return success;
}

bool loadMedia()
{
	bool success = true;
	/*
	g_font = TTF_OpenFont("Apple ][.ttf", FONT_SIZE);
	if (g_font == NULL)
	{
		printf("Couldn't open font! SDL_ttf Error: %s\n", TTF_GetError());
		success = false;
	}
	else
	{
	}
	*/
	if (!g_background.loadFromFile("background.bmp"))
	{
		printf("Couldn't load background!\n");
		success = false;
	}


	return success;
}

void close()
{
	g_background.free();

	TTF_CloseFont(g_font);
	g_font = NULL;

	SDL_DestroyRenderer(g_renderer);
	g_renderer = NULL;
	SDL_DestroyWindow(g_window);
	g_window = NULL;

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}








class PhysicsBall
{
public:
	PhysicsBall(double radius, SDL_Color color, Vector_2d position, Vector_2d velocity);

	bool checkCollision(const PhysicsBall& otherBall);
	bool checkCollisionWithPoint(Vector_2d point);

	Vector_2d getPosition() { return m_position; }
	void setRadius(double radius);

	void calculateForces(const std::vector<PhysicsBall>& otherBalls);
	void update(std::vector<PhysicsBall>& otherBalls, buttonStates mouseButtons[], Vector_2d mousePosition, PhysicsBall*& p_pickedUpBall, double elapsedTime);
	void show(SDL_Renderer* renderer, buttonStates mouseButtons[], Vector_2d mousePosition, PhysicsBall*& p_pickedUpBall);
private:
	Vector_2d m_position;
	Vector_2d m_velocity;
	Vector_2d m_force;
	double m_radius;
	double m_mass;
	SDL_Color m_color;
};

PhysicsBall::PhysicsBall(double radius, SDL_Color color, Vector_2d position, Vector_2d velocity = Vector_2d(0, 0))
{
	m_position = position;
	m_velocity = velocity;
	m_force = Vector_2d(0.0, 0.0);
	m_radius = radius;
	static double fourOverThree = 4 / 3.0;
	m_mass = std::_Pi * fourOverThree * m_radius * m_radius * m_radius;
	m_color = color;
}

void PhysicsBall::setRadius(double radius)
{
	m_radius = radius; 
	static double fourOverThree = 4 / 3.0;
	m_mass = std::_Pi * fourOverThree * m_radius * m_radius * m_radius;
}

bool PhysicsBall::checkCollision(const PhysicsBall& otherBall)
{
	return distanceSquared(otherBall.m_position, m_position) <= (otherBall.m_radius + m_radius) * (otherBall.m_radius + m_radius);
}

bool PhysicsBall::checkCollisionWithPoint(Vector_2d point)
{
	return distanceSquared(point, m_position) <= m_radius * m_radius;
}

void PhysicsBall::calculateForces(const std::vector<PhysicsBall>& otherBalls)
{
	m_force = Vector_2d(0.0, 0.0);
	
	for (const PhysicsBall& otherBall : otherBalls)
	{
		if (&otherBall != this)
		{
			Vector_2d force;
			double distSqr = distanceSquared(otherBall.m_position, m_position);
			double forceValue = otherBall.m_mass * m_mass / distSqr;

			//Normalize vector and scale by force value
			force = forceValue * (otherBall.m_position - m_position) * rsqrt(distSqr);

			m_force += GRAV * force;
		}
	}
}

void PhysicsBall::update(std::vector<PhysicsBall>& otherBalls, buttonStates mouseButtons[], Vector_2d mousePosition, PhysicsBall*& p_pickedUpBall, double elapsedTime)
{
	if (m_mass > 0.0)
	{
		m_velocity += m_force / m_mass * elapsedTime;
		m_position += m_velocity * elapsedTime;
	}

	//clamp down the velocity
	if (distanceSquared(m_velocity) <= 0.001)
		m_velocity = Vector_2d(0.0, 0.0);

	if (p_pickedUpBall != nullptr)
	{
		if(!(mouseButtons[left].down && mouseButtons[right].down)) //if both are pressed at the same time, do nothing
		{
			//Picking the ball up
			if (mouseButtons[left].down)
			{
				p_pickedUpBall->m_force = Vector_2d(0.0, 0.0);
				p_pickedUpBall->m_velocity = Vector_2d(0.0, 0.0);
			}
			if (mouseButtons[left].held)
			{
				p_pickedUpBall->m_force = Vector_2d(0.0, 0.0);
				p_pickedUpBall->m_velocity = Vector_2d(0.0, 0.0);
				p_pickedUpBall->m_position = mousePosition;
			}
			if (mouseButtons[left].up)
			{
				p_pickedUpBall = nullptr;
			}

			//Giving the ball velocity
			if (mouseButtons[right].up)
			{
				p_pickedUpBall->m_velocity = (p_pickedUpBall->m_position - mousePosition);
				p_pickedUpBall = nullptr;
			}
		}
	}

	//border collisions
	/*if (m_position.x - m_radius < 0 || m_position.x + m_radius > SCREEN_WIDTH)
	{
		m_position.x -= m_velocity.x;
		m_velocity.x = -m_velocity.x;
	}
	if (m_position.y - m_radius < 0 || m_position.y + m_radius > SCREEN_HEIGHT)
	{
		m_position.y -= m_velocity.y;
		m_velocity.y = -m_velocity.y;
	}*/

	//wraping the ball
	if (m_position.x < 0)
	{
		m_position.x = SCREEN_WIDTH;
	}
	if (m_position.x > SCREEN_WIDTH)
	{
		m_position.x = 0;
	}
	if (m_position.y < 0)
	{
		m_position.y = SCREEN_HEIGHT;
	}
	if (m_position.y > SCREEN_HEIGHT)
	{
		m_position.y = 0;
	}

	//ball collisions
	for (PhysicsBall& otherBall : otherBalls)
	{
		if (&otherBall != this && checkCollision(otherBall))
		{
			//Normal collisions
			Vector_2d normal = getVectorFromPositions(otherBall.m_position, m_position);

			Vector_2d displacement = normalizeVector(normal) * (m_radius + otherBall.m_radius - length(normal));
			m_position += displacement / 2;
			otherBall.m_position -= displacement / 2;

			normal = normalizeVector(normal);
			Vector_2d tangent = getPerpendicularVector(normal);

			Vector_2d normalVelocity = projectVector(m_velocity, normal);
			Vector_2d tangentialVelocity = projectVector(m_velocity, tangent);

			Vector_2d otherNormalVelocity = projectVector(otherBall.m_velocity, normal);
			Vector_2d otherTangentialVelocity = projectVector(otherBall.m_velocity, tangent);

	
			m_velocity = tangentialVelocity + ((m_mass - otherBall.m_mass) * normalVelocity + 2 * otherBall.m_mass * otherNormalVelocity) / (m_mass + otherBall.m_mass);
			otherBall.m_velocity = otherTangentialVelocity + RESTITUTION * (2 * m_mass * normalVelocity + (otherBall.m_mass - m_mass) * otherNormalVelocity) / (m_mass + otherBall.m_mass);

			m_velocity *= RESTITUTION;
			otherBall.m_velocity *= RESTITUTION;

			/*
			//Wonky collisions 
			
			double dist = std::sqrt(distanceSquared(m_position, otherBall.m_position));

			double nx = (otherBall.m_position.x - m_position.x) / dist;
			double ny = (otherBall.m_position.y - m_position.y) / dist;

			double tx = -ny;
			double ty = nx;

			double kx = (b1.m_velocity.x - b2.m_velocity.x);
			double ky = (b1.m_velocity.y - b2.m_velocity.y);
			double p = 2.0 * (nx * kx + ny * ky) / (b1.m_mass + b2.m_mass);
			b1.m_velocity.x = b1.m_velocity.x - p * b2.m_mass * nx;
			b1.m_velocity.y = b1.m_velocity.y - p * b2.m_mass * ny;
			b2.m_velocity.x = b2.m_velocity.x + p * b1.m_mass * nx;
			b2.m_velocity.y = b2.m_velocity.y + p * b1.m_mass * ny;
			*/

		}
	}
}

void PhysicsBall::show(SDL_Renderer* renderer, buttonStates mouseButtons[], Vector_2d mousePosition, PhysicsBall*& p_pickedUpBall)
{
	SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, m_color.a);
	SDL_RenderFillCircle(renderer, m_position.x, m_position.y, m_radius);
	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
	SDL_RenderDrawLine(renderer, m_position.x, m_position.y, m_position.x + LINE_SCALE_FORCE * m_force.x, m_position.y + LINE_SCALE_FORCE * m_force.y);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawLine(renderer, m_position.x, m_position.y, m_position.x + LINE_SCALE_VELOCITY * m_velocity.x, m_position.y + LINE_SCALE_VELOCITY * m_velocity.y);
	if (this == p_pickedUpBall && mouseButtons[right].held)
	{
		SDL_SetRenderDrawColor(renderer, 128, 128, 255, 255);
		SDL_RenderDrawLine(renderer, m_position.x, m_position.y, mousePosition.x, mousePosition.y);
	}
}




int main(int argc, char** argv)
{
	if (!init())
	{
		printf("Couldn't initialize!\n");
		return 1;
	}

	if (!loadMedia())
	{
		printf("Couldn't load media!\n");
		return 1;
	}

	bool quit = false;
	SDL_Event event;
	double deltaTime = 0.0;
	double elapsedTime = 0.0;
	double realDeltaTime = 0.0;
	double realElapsedTime = 0.0;

	Vector_2d mousePosition{ 0.0, 0.0 };
	buttonStates mouseButtons[max_mouseButtons];
	buttonStates spacebar;
	PhysicsBall* p_pickedUpBall = nullptr;

	std::vector<PhysicsBall> balls;

	/*for (int i = 0; i < BALLS_COUNT; ++i)
	{
		double radius = Random::get(25, 50);
		if (i == 0)
			radius = 25;
		if (i == 1)
			radius = 100;

		SDL_Color color = SDL_Color(Random::get(0, 255), Random::get(0, 255), Random::get(0, 255), 255);

		Vector_2d position = Vector_2d(Random::get(radius, SCREEN_WIDTH - radius), Random::get(radius, SCREEN_HEIGHT - radius));


		PhysicsBall b(radius, color, position);
		balls.push_back(b);
	}*/



	while (!quit)
	{
		//Start of the frame
		uint64_t startTime = SDL_GetPerformanceCounter();

		//Event loop
		while (SDL_PollEvent(&event))
		{
			if (SDL_QUIT == event.type)
			{
				quit = true;
			}

			if (SDL_KEYDOWN == event.type)
			{
				if (SDLK_SPACE == event.key.keysym.sym && spacebar.held == false)
				{
					spacebar.down = true;
					spacebar.held = true;
				}
			}

			if (SDL_KEYUP == event.type)
			{
				if (SDLK_SPACE == event.key.keysym.sym)
				{
					spacebar.up = true;
					spacebar.held = false;
				}
			}

			if (SDL_MOUSEMOTION == event.type)
			{
				int x, y;
				SDL_GetMouseState(&x, &y);
				mousePosition.x = x;
				mousePosition.y = y;
			}

			if (SDL_MOUSEBUTTONDOWN == event.type)
			{
				if (SDL_BUTTON_LEFT == event.button.button)
				{
					//printf("Left mouse button down.\n");
					mouseButtons[left].down = true;
				}
				if (SDL_BUTTON_RIGHT == event.button.button)
				{
					//printf("Right mouse button down.\n");
					mouseButtons[right].down = true;
				}
				if (SDL_BUTTON_MIDDLE == event.button.button)
				{
					//printf("Middle mouse button down.\n");
					mouseButtons[middle].down = true;
				}
			}
			if ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LMASK))
			{
				//printf("Left held.\n");
				mouseButtons[left].held = true;
			}
			if ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_RMASK))
			{
				//printf("Right held.\n");
				mouseButtons[right].held = true;
			}
			if ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MMASK))
			{
				;//printf("Middle held.\n");
				mouseButtons[middle].held = true;
			}


			if (SDL_MOUSEBUTTONUP == event.type)
			{
				if (SDL_BUTTON_LEFT == event.button.button)
				{
					//printf("Left mouse button up.\n");
					mouseButtons[left].up = true;
					mouseButtons[left].held = false;
				}
				if (SDL_BUTTON_RIGHT == event.button.button)
				{
					//printf("Right mouse button up.\n");
					mouseButtons[right].up = true;
					mouseButtons[right].held = false;
				}
				if (SDL_BUTTON_MIDDLE == event.button.button)
				{
					//printf("Middle mouse button up.\n");
					mouseButtons[middle].up = true;
					mouseButtons[middle].held = false;
				}
			}

			if (SDL_MOUSEWHEEL == event.type)
			{
				if (event.wheel.y > 0)
				{
					//printf("Mouse is scrolling up.\n");
				}
				else if (event.wheel.y < 0)
				{
					//printf("Mouse is scrolling down.\n");
				}
			}
		}

		/*
		if (mouseButtons[left].down)
			printf("Left down######################.\n");
		else
			printf("Left not down.\n");
		if (mouseButtons[left].held)
			printf("Left held.\n");
		else
			printf("Left not held.\n");
		if (mouseButtons[left].up)
			printf("Left up^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^.\n");
		else
			printf("Left not up.\n");
		printf("\n");
		*/

		//Frame of the program:
		printf("FPS: %f\n", 1.0 / elapsedTime);
		printf("Time per frame: %fms\n", deltaTime);
		printf("REAL FPS: %f\n", 1.0 / realElapsedTime);
		printf("REAL Time per frame: %fms\n\n", realDeltaTime);

		if (spacebar.down)
		{
			double radius = 1.0;
			SDL_Color color = SDL_Color(Random::get(0, 255), Random::get(0, 255), Random::get(0, 255), 255);
			Vector_2d position = mousePosition;

			PhysicsBall b(radius, color, position);
			balls.push_back(b);
		}
		if (spacebar.held)
		{
			double radius = std::sqrt(distanceSquared(balls.back().getPosition(), mousePosition));
			if (radius < 1.0)
				radius = 1.0;

			balls.back().setRadius(radius);
		}
		if (spacebar.up)
		{
			;
		}

		for (auto& ball : balls)
		{
			ball.calculateForces(balls);
		}
		for (auto& ball : balls)
		{
			if (ball.checkCollisionWithPoint(mousePosition) && (mouseButtons[left].down || mouseButtons[right].down))
				p_pickedUpBall = &ball;
			ball.update(balls, mouseButtons, mousePosition, p_pickedUpBall, realElapsedTime);
		}


		SDL_SetRenderDrawColor(g_renderer, 0xff, 0xff, 0xff, 0xff);
		SDL_RenderClear(g_renderer);

		g_background.render(0, 0);

		for (auto& ball : balls)
		{
			ball.show(g_renderer, mouseButtons, mousePosition, p_pickedUpBall);
		}

		SDL_RenderPresent(g_renderer);



		//End of the frame
		mouseButtons[left].down = false;
		mouseButtons[left].up = false;
		mouseButtons[right].down = false;
		mouseButtons[right].up = false;
		mouseButtons[middle].down = false;
		mouseButtons[middle].up = false;
		spacebar.down = false;
		spacebar.up = false;

		uint64_t endTime = SDL_GetPerformanceCounter();
		deltaTime = ((endTime - startTime) * 1000 / (double)TICKS_PER_SECOND);
		elapsedTime = deltaTime / 1000.0;

		int driftTime = (1000 / FPS - deltaTime); //time per frame (in ms) - deltaTime

		if (driftTime > 0)
		{
			SDL_Delay(driftTime);
		}
		else if (driftTime < 0)
		{
			printf("Warning: program running too slow.\n");
		}

		uint64_t realEndTime = SDL_GetPerformanceCounter();

		realDeltaTime = ((realEndTime - startTime) * 1000 / (double)TICKS_PER_SECOND);
		realElapsedTime = realDeltaTime / 1000.0;
	}


	close();
	return 0;
}