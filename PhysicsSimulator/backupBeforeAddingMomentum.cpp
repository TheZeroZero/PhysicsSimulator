#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <random> // for std::mt19937 and std::random_device
#include <vector>
#include <cmath>
#include <string>

#include "CircleDrawing.h"
#include "Constants.h"
#include "Vector_2d.h"


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
			g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

	void calculateForces(const std::vector<PhysicsBall>& otherBalls);
	void update(std::vector<PhysicsBall>& otherBalls);
	void show(SDL_Renderer* renderer, Vector_2d cameraPosition, double zoom);
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
	static double fourOverThree = 4 / 3;
	m_mass = std::_Pi * fourOverThree * m_radius * m_radius * m_radius;
	m_color = color;
}

bool PhysicsBall::checkCollision(const PhysicsBall& otherBall)
{
	return distanceSquared(otherBall.m_position, m_position) <= (otherBall.m_radius + m_radius) * (otherBall.m_radius + m_radius);
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

void PhysicsBall::update(std::vector<PhysicsBall>& otherBalls)
{
	m_velocity += m_force;
	m_position += m_velocity;

	//ball collisions
	for(PhysicsBall& otherBall : otherBalls)
	{
		if (&otherBall != this && checkCollision(otherBall))
		{
			Vector_2d momentum = m_mass * m_velocity + otherBall.m_mass * otherBall.m_velocity;
			printf("momentum: %f\n", distanceSquared(momentum));


			//This ball
			
			//aproximate the position of our ball, when the collision occurs
			Vector_2d collisionPosition = m_position - m_velocity / 2;
			Vector_2d perpendicularFlippingAxis = getVectorFromPositions(otherBall.m_position, collisionPosition);
			Vector_2d flippingAxis = getPerpendicularVector(perpendicularFlippingAxis);

			//put the ball out of collision
			m_position -= m_velocity;


			Vector_2d newVelocity = projectVector(m_velocity, flippingAxis) * normalizeVector(flippingAxis);	//act on this ball
			Vector_2d otherNewVelocity = m_velocity - newVelocity;	//act on other ball
			
			Vector_2d newVelocityFromOther = projectVector(otherBall.m_velocity, flippingAxis) * normalizeVector(flippingAxis);	//act on other ball
			Vector_2d otherNewVelocityFromOther = otherBall.m_velocity - newVelocityFromOther;	//act on this ball


			//add all acting velocities
			m_velocity = RESTITUTION * (newVelocity + otherNewVelocityFromOther);
			Vector_2d thisMomentum = m_mass * (newVelocity + otherNewVelocityFromOther);
			otherBall.m_velocity = RESTITUTION * (otherNewVelocity + newVelocityFromOther);
			Vector_2d otherMomentum = otherBall.m_mass * (otherNewVelocity + newVelocityFromOther);
			printf("momentum after coll: %f\n\n", distanceSquared(thisMomentum + otherMomentum));
		}
	}

	//border collisions
	if (m_position.x - m_radius < 0 || m_position.x + m_radius > SCREEN_WIDTH)
	{
		m_position.x -= m_velocity.x;
		m_velocity.x = -m_velocity.x;
	}
	if (m_position.y - m_radius < 0 || m_position.y + m_radius > SCREEN_HEIGHT)
	{
		m_position.y -= m_velocity.y;
		m_velocity.y = -m_velocity.y;
	}
}

void PhysicsBall::show(SDL_Renderer* renderer, Vector_2d cameraPosition, double zoom = 1.0)
{
	SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, m_color.a);
	SDL_RenderFillCircle(renderer, m_position.x * zoom, m_position.y * zoom, m_radius / zoom);
	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
	SDL_RenderDrawLine(renderer, m_position.x, m_position.y, m_position.x + LINE_SCALE_FORCE * m_force.x, m_position.y + LINE_SCALE_FORCE * m_force.y);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawLine(renderer, m_position.x, m_position.y, m_position.x + LINE_SCALE_VELOCITY * m_velocity.x, m_position.y + LINE_SCALE_VELOCITY * m_velocity.y);
}








int main(int argc, char** argv)
{
	if (!init())
	{
		printf("Couldn't initialize!\n");
	}
	else
	{
		if (!loadMedia())
		{
			printf("Couldn't load media!\n");
		}
		else
		{
			bool quit = false;
			SDL_Event e;

			Vector_2d cameraPosition{ 0, 0 };

			std::vector<PhysicsBall> balls;
			/*for (int i{ 0 }; i < CIRCLES_COUNT; ++i)
			{
				Vector_2d position;
				int radius;
				Uint8 h, s, v, a;

				radius = Random::get(5, 20);


				position.x = Random::get(0 + radius, SCREEN_WIDTH - radius);
				position.y = Random::get(0 + radius, SCREEN_HEIGHT - radius);
				h = Random::get(0, 360);
				s = 85;//Random::get(50, 100);
				v = 95;//Random::get(50, 100);
				a = Random::get(128, 255);

				SDL_Color color = HSVtoRGB(h, s, v);

				balls.push_back(PhysicsBall(radius, color, position));
			}*/
			while (!quit)
			{
				//Event loop
				while (SDL_PollEvent(&e))
				{
					if (e.type == SDL_QUIT)
					{
						quit = true;
					}
					if (e.type == SDL_MOUSEBUTTONDOWN)
					{
						int mouseX, mouseY;
						/* Used as a mask when testing buttons in buttonstate   Button 1:	Left mouse button   Button 2:	Middle mouse button   Button 3:	Right mouse button   Button 4:	Mouse wheel up	 (may also be a real button)   Button 5:	Mouse wheel down (may also be a real button) #define SDL_BUTTON(X)		(SDL_PRESSED<<(X-1))#define SDL_BUTTON_LEFT		1#define SDL_BUTTON_MIDDLE	2#define SDL_BUTTON_RIGHT	3#define SDL_BUTTON_WHEELUP	4#define SDL_BUTTON_WHEELDOWN	5#define SDL_BUTTON_LMASK	SDL_BUTTON(SDL_BUTTON_LEFT)#define SDL_BUTTON_MMASK	SDL_BUTTON(SDL_BUTTON_MIDDLE)#define SDL_BUTTON_RMASK	SDL_BUTTON(SDL_BUTTON_RIGHT)*/
						if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(1))
							printf("Mouse Button 1(left) is pressed.\n");
						if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(2))
							printf("Mouse Button 2(mid) is pressed.\n");
						if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(3))
							printf("Mouse Button 3(right) is pressed.\n");
						if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(4))
							printf("Mouse Button 4(up) is pressed.\n");
						if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(5))
							printf("Mouse Button 5(down) is pressed.\n");
						if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(6))
							printf("Mouse Button 6(???) is pressed.\n");
						if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(7))
							printf("Mouse Button 7(???) is pressed.\n");
						if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(8))
							printf("Mouse Button 8(???) is pressed.\n");
						if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(9))
							printf("Mouse Button 9(???) is pressed.\n");
						
						Vector_2d position;
						int radius;
						Uint8 h, s, v, a;

						radius = Random::get(25, 50);

						int x, y;
						SDL_GetMouseState(&x, &y);
						position.x = x;
						position.y = y;

						h = Random::get(0, 360);
						s = 85;//Random::get(50, 100);
						v = 95;//Random::get(50, 100);
						a = Random::get(128, 255);

						SDL_Color color = HSVtoRGB(h, s, v);

						balls.push_back(PhysicsBall(radius, color, position));
						
					}
				}

				SDL_SetRenderDrawColor(g_renderer, 0xff, 0xff, 0xff, 0xff);
				SDL_RenderClear(g_renderer);

				g_background.render(0, 0);

				for (PhysicsBall& ball : balls)
				{
					ball.calculateForces(balls);
				}
				for (PhysicsBall& ball : balls)
				{
					ball.update(balls);
				}
				for (PhysicsBall& ball : balls)
				{
					ball.show(g_renderer, cameraPosition);
				}

				SDL_RenderPresent(g_renderer);
			}

		}
	}

	close();
	return 0;
}