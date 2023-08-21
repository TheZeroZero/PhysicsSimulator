#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <random> // for std::mt19937 and std::random_device
#include <vector>
#include <cmath>
#include <string>




#define DUMB_COLLISIONS
//#define ACTUAL_COLLISIONS
#define ORBIT_DEMO
//#define BLACK_HOLE
#define DRAW_VEL_LINES

#ifdef ORBIT_DEMO
#define FOLLOW_CAMERA
#endif

#ifndef ORBIT_DEMO
#define BORDER_COLLISIONS
#endif

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
const int FONT_SIZE = 19;
//const double SCALE = 57913; //how many meters is in one pixel
const double GRAV = .000000001; //gravitational constant (.00004 for mass calculated as area)
const double RESTITUTION = .95;
//onst int CIRCLES_COUNT = 15;
const double VEL_LINES_LENGTH = 30;


double dotProduct(double x1, double y1, double x2, double y2)
{
	return x1 * x2 + y1 * y2;
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

double rsqrt(double number)
{
	double y = number;
	double x2 = y * 0.5;
	std::int64_t i = *(std::int64_t*)&y;
	// The magic number is for doubles is from https://cs.uwaterloo.ca/~m32rober/rsqrt.pdf
	i = 0x5fe6eb50c7b537a9 - (i >> 1);
	y = *(double*)&i;
	y = y * (1.5 - (x2 * y * y));   // 1st iteration
	//      y  = y * ( 1.5 - ( x2 * y * y ) );   // 2nd iteration, this can be removed
	return y;
}

double distanceSquared(double x1, double y1, double x2, double y2)
{
	return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

//Code from Gumichan01 for drawing circles
int SDL_RenderDrawCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
	int offsetx, offsety, d;
	int status;

	offsetx = 0;
	offsety = radius;
	d = radius - 1;
	status = 0;

	while (offsety >= offsetx) {
		status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
		status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
		status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
		status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
		status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
		status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
		status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
		status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2 * offsetx) {
			d -= 2 * offsetx + 1;
			offsetx += 1;
		}
		else if (d < 2 * (radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	return status;
}

int SDL_RenderFillCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
	int offsetx, offsety, d;
	int status;

	offsetx = 0;
	offsety = radius;
	d = radius - 1;
	status = 0;

	while (offsety >= offsetx) {

		status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
			x + offsety, y + offsetx);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
			x + offsetx, y + offsety);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
			x + offsetx, y - offsety);
		status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
			x + offsety, y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2 * offsetx) {
			d -= 2 * offsetx + 1;
			offsetx += 1;
		}
		else if (d < 2 * (radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	return status;
}

SDL_Point operator+(const SDL_Point& p1, const SDL_Point& p2)
{
	return SDL_Point{ p1.x + p2.x, p1.y + p2.y };
}

SDL_Point operator-(const SDL_Point& p1, const SDL_Point& p2)
{
	return SDL_Point{ p1.x - p2.x, p1.y - p2.y };
}

SDL_Point operator*(int a, const SDL_Point& p)
{
	return SDL_Point{ p.x * a, p.y * a };
}
SDL_Point operator*(const SDL_Point& p, int a)
{
	return a * p;
}

bool operator==(const SDL_Point& p1, const SDL_Point& p2)
{
	return (p1.x == p2.x) && (p1.y == p2.y);
}

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
Texture g_radiusPromptTextTexture;
Texture g_InputTextTexture;







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

void Texture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
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

	g_font = TTF_OpenFont("Apple ][.ttf", FONT_SIZE);
	if (g_font == NULL)
	{
		printf("Couldn't open font! SDL_ttf Error: %s\n", TTF_GetError());
		success = false;
	}
	else
	{
		SDL_Color textColor = { 0xff, 0xff, 0xff, 0xff };
		
		if (!g_radiusPromptTextTexture.loadFromRenderedText("Wprowadz promien:", textColor))
		{
			printf("Failed to render prompt text!\n");
			success = false;
		}
	}

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








class PhysicsCircle
{
public:
	PhysicsCircle();
	PhysicsCircle(double x, double y, int radius, Uint8 r=255, Uint8 g=255, Uint8 b=255, Uint8 a=255);
	~PhysicsCircle();

	void update(std::vector<PhysicsCircle>& circles);
	void calculateForces(std::vector<PhysicsCircle>& circles);
	void render(SDL_Renderer* renderer, int camerax, int cameray);
	void renderBlackHole(SDL_Renderer* renderer, int camerax, int cameray);
	double getx() { return m_x; }
	double gety() { return m_y; }
	double getmass() { return m_mass; }
	double getradius() { return m_radius; }
	
	double m_dx, m_dy;
//private:
	double m_x, m_y;
	int m_radius;
	double m_mass;
	uint8_t m_r, m_g, m_b, m_a;
};

bool checkCollision(PhysicsCircle& a, PhysicsCircle& b)
{
	//Calculate total radius squared
	double totalRadiusSquared = a.getradius() + b.getradius();
	totalRadiusSquared = totalRadiusSquared * totalRadiusSquared;

	if (distanceSquared(a.getx(), a.gety(), b.getx(), b.gety()) < (totalRadiusSquared))
	{
		//The circles have collided
		return true;
	}

	//If not
	return false;
}












PhysicsCircle::PhysicsCircle()
{
	m_x = 0;
	m_y = 200;
	m_radius = 100;
	m_mass = 31415,9265;
	m_dx = 0;
	m_dy = 0;
	m_r = 255;
	m_g = 255;
	m_b = 255;
	m_a = 255;
}

PhysicsCircle::PhysicsCircle(double x, double y, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	m_x = x;
	m_y = y;
	m_radius = radius;
	m_mass = 4 / 3 * std::_Pi * m_radius * m_radius * m_radius;
	m_dx = 0;
	m_dy = 0;
	m_r = r;
	m_g = g;
	m_b = b;
	m_a = a;
}

PhysicsCircle::~PhysicsCircle()
{
	;
}

void PhysicsCircle::update(std::vector<PhysicsCircle>& circles)
{	
	m_x += m_dx;
	m_y += m_dy;

	//Collisions with others
	for (int i{ 0 }; i < circles.size(); ++i)
	{
		if (&circles[i] == this)
		{
			;
		}
		else
		{
#ifdef ACTUAL_COLLISIONS
			if (checkCollision(*this, circles[i]))
			{
				//Collision of 2 circles
				//Move circle back
				m_x -= m_dx;
				m_y -= m_dy;
				//Change velocity
				double velocityX = m_dx;
				double velocityY = m_dy;
				
				double distanceX = circles[i].getx() - this->getx();
				double distanceY = circles[i].gety() - this->gety();
				double distanceInv = rsqrt(distanceX * distanceX + distanceY * distanceY);

				double perpendicularDistanceX = -distanceY;
				double perpendicularDistanceY = distanceX;

				double perpendicularVelocity = dotProduct(perpendicularDistanceX, perpendicularDistanceY, velocityX, velocityY) * distanceInv;
				double parallelVelocity = dotProduct(distanceX, distanceY, velocityX, velocityY) * distanceInv;

				parallelVelocity = -parallelVelocity;
				
				//Normalize the distance vector and scale by velocity
				double perpendicularVelocityX = perpendicularDistanceX * distanceInv * perpendicularVelocity;
				double perpendicularVelocityY = perpendicularDistanceY * distanceInv * perpendicularVelocity;

				double parallelVelocityX = distanceX * distanceInv * parallelVelocity;
				double parallelVelocityY = distanceY * distanceInv * parallelVelocity;

				velocityX = perpendicularVelocityX + parallelVelocityX;
				velocityY = perpendicularVelocityY + parallelVelocityY;

				m_dx = RESTITUTION * velocityX;
				m_dy = RESTITUTION * velocityY;
			}
#endif
		}
	}

	

	//Collisions with border
#ifdef BORDER_COLLISIONS
	if (m_x - m_radius < 0 || m_x + m_radius > SCREEN_WIDTH)
	{
		m_x -= m_dx;
		m_dx = -m_dx;
	}
	if (m_y - m_radius < 0 || m_y + m_radius > SCREEN_HEIGHT)
	{
		m_y -= m_dy;
		m_dy = -m_dy;
	}
#endif
}

void PhysicsCircle::calculateForces(std::vector<PhysicsCircle>& circles)
{
	for (int i{ 0 }; i < circles.size(); ++i)
	{
		if (&circles[i] == this)
		{
			;
		}
		else
		{
			double distSqr = distanceSquared(m_x, m_y, circles[i].getx(), circles[i].gety());
			double force = GRAV * m_mass * circles[i].getmass() / distSqr;
			
			double forcex = force * rsqrt(distSqr) * (circles[i].getx() - m_x);
			double forcey = force * rsqrt(distSqr) * (circles[i].gety() - m_y);
//If DUMB_COLLISIONS check for collisions and invert forces:			
#ifdef DUMB_COLLISIONS
			if (checkCollision(*this, circles[i]))
			{
				m_dx -= RESTITUTION * forcex;
				m_dy -= RESTITUTION * forcey;
			}
			else
			{
				m_dx += forcex;
				m_dy += forcey;
			}
#endif
//Else just add forces
#ifndef DUMB_COLLISIONS
			m_dx += forcex;
			m_dy += forcey;
#endif
		}
	}
}

void PhysicsCircle::render(SDL_Renderer* renderer, int camerax, int cameray)
{
	SDL_SetRenderDrawColor(renderer, m_r, m_g, m_b, m_a);
	SDL_RenderFillCircle(renderer, m_x - camerax, m_y - cameray, m_radius);

	//Draw velocity line
#ifdef DRAW_VEL_LINES
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, m_a);
	SDL_RenderDrawLine(renderer, m_x, m_y, m_x + VEL_LINES_LENGTH * m_dx, m_y + VEL_LINES_LENGTH * m_dy);
#endif
}

void PhysicsCircle::renderBlackHole(SDL_Renderer* renderer, int camerax, int cameray)
{
	SDL_SetRenderDrawColor(renderer, m_r, m_g, m_b, m_a);
	SDL_RenderDrawCircle(renderer, m_x - camerax, m_y - cameray, m_radius);
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

			bool renderText = false;
			std::string inputText = "";
			SDL_Color textColor = { 0xff, 0xff, 0xff, 0xff };

			int x, y;
			int radius;
			Uint8 h, s, v, a;
			SDL_Color color;

			double camerax = 0, cameray = 0;
			double cameraVelx = 0, cameraVely = 0;

			std::vector<PhysicsCircle> circles;
			//Create random circles
			/*
			for (int i{ 0 }; i < CIRCLES_COUNT; ++i)
			{
				int x, y;
				int radius;
				Uint8 h, s, v, a;

				radius = Random::get(5, 20);


				x = Random::get(0 + radius, SCREEN_WIDTH - radius);
				y = Random::get(0 + radius, SCREEN_HEIGHT - radius);
				h = Random::get(0, 360);
				s = 85;//Random::get(50, 100);
				v = 95;//Random::get(50, 100);
				a = Random::get(128, 255);

				SDL_Color color = HSVtoRGB(h, s, v);

				circles.push_back(PhysicsCircle(x ,y, radius, color.r, color.g, color.b, a));
			}*/
			//Orbitting but hits wall after a while:
#ifdef FOLLOW_CAMERA
			circles.push_back(PhysicsCircle(400, 400, 110));
#endif
#ifdef ORBIT_DEMO
			circles.push_back(PhysicsCircle(475, 400, 30));
			circles.back().m_dy = -16,67;
#endif
			while (!quit)
			{
				camerax += cameraVelx;
				cameray += cameraVely;

				while (SDL_PollEvent(&e))
				{
					if (e.type == SDL_QUIT)
					{
						quit = true;
					}
					//Special key input
					if (e.type == SDL_KEYDOWN)
					{
						//Handle backspace
						if (e.key.keysym.sym == SDLK_BACKSPACE && inputText.length() > 0)
						{
							//lop off character
							inputText.pop_back();
							renderText = true;
						}
						//Handle copy
						else if (e.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL)
						{
							SDL_SetClipboardText(inputText.c_str());
						}
						//Handle paste
						else if (e.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL)
						{
							inputText = SDL_GetClipboardText();
							renderText = true;
						}
					}
					//Special text input event
					else if (e.type == SDL_TEXTINPUT)
					{
						//Not copy or pasting or inputting not numbers
						if (!(SDL_GetModState() & KMOD_CTRL && (e.text.text[0] == 'c' || e.text.text[0] == 'C' || e.text.text[0] == 'v' || e.text.text[0] == 'V')))
						{
							//Append character
							if (e.text.text[0] == '1' || e.text.text[0] == '2' || e.text.text[0] == '3' || e.text.text[0] == '4' || e.text.text[0] == '5' || e.text.text[0] == '6' || e.text.text[0] == '7' || e.text.text[0] == '8' || e.text.text[0] == '9' || e.text.text[0] == '0')
							{
								inputText += e.text.text;
								renderText = true;
							}
						}
					}
					if (e.type == SDL_KEYDOWN && e.key.repeat == 0)
					{
						if (e.key.keysym.sym == SDLK_UP)
							cameraVely -= 1;
						if (e.key.keysym.sym == SDLK_DOWN)
							cameraVely += 1; //move down
						if (e.key.keysym.sym == SDLK_RIGHT)
							cameraVelx += 1;
						if (e.key.keysym.sym == SDLK_LEFT)
							cameraVelx -= 1; //move down
					}
					if (e.type == SDL_KEYUP && e.key.repeat == 0)
					{
						if (e.key.keysym.sym == SDLK_UP)
							cameraVely -= -1;
						if (e.key.keysym.sym == SDLK_DOWN)
							cameraVely += -1; //move down
						if (e.key.keysym.sym == SDLK_RIGHT)
							cameraVelx += -1;
						if (e.key.keysym.sym == SDLK_LEFT)
							cameraVelx -= -1; //move down
					}
					if (e.type == SDL_MOUSEBUTTONDOWN)
					{
						static bool first = true;
						//Spawn a random ball
						renderText = true;
						


						SDL_GetMouseState(&x, &y);

						radius = Random::get(20, 65);
						h = Random::get(0, 360);
						s = 85;//Random::get(50, 100);
						v = 95;//Random::get(50, 100);
						a = Random::get(128, 255);

						color = HSVtoRGB(h, s, v);

						
						//Create a black hole
#ifdef BLACK_HOLE
						if (first)
						{
							circles.back().m_r = 255;
							circles.back().m_g = 0;
							circles.back().m_b = 0;
							circles.back().m_mass = 1000'000;
							first = false;
						}
#endif
					}
					if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN)
					{
						radius = std::stoi(inputText);
						circles.push_back(PhysicsCircle(x + camerax, y + cameray, radius, color.r, color.g, color.b, a));
						inputText = "";
						renderText = false;
					}
				}

				//Rerender text if needed
				if (renderText)
				{
					//Text is not empty
					if (inputText != "")
					{
						//Render new text
						g_InputTextTexture.loadFromRenderedText(inputText.c_str(), textColor);
					}
					//Text is empty
					else
					{
						//Render space texture
						g_InputTextTexture.loadFromRenderedText(" ", textColor);
					}
				}

				SDL_SetRenderDrawColor(g_renderer, 0xff, 0xff, 0xff, 0xff);
				SDL_RenderClear(g_renderer);

				//g_background.render(0 - camerax, 0 - cameray, NULL);
				g_background.render(0, 0, NULL);
				for (PhysicsCircle& c : circles)
				{
					c.update(circles);
				}

				for (PhysicsCircle& c : circles)
				{
					c.calculateForces(circles);
				}

#ifdef FOLLOW_CAMERA
				camerax = circles[0].getx() - SCREEN_WIDTH / 2;
				cameray = circles[0].gety() - SCREEN_HEIGHT / 2;
#endif


#ifdef BLACK_HOLE
				if(0 < circles.size())
					circles[0].renderBlackHole(g_renderer, camerax, cameray);
#endif
#ifndef BLACK_HOLE
				if (0 < circles.size())
					circles[0].render(g_renderer, camerax, cameray);
#endif
				for (int i{ 1 }; i < circles.size(); ++i)
				{

					circles[i].render(g_renderer, camerax, cameray);
				}


				if (renderText == true)
				{
					g_radiusPromptTextTexture.render(x, y, NULL);
					g_InputTextTexture.render((x + g_radiusPromptTextTexture.getWidth()), y, NULL);
				}

				SDL_RenderPresent(g_renderer);
			}

		}
	}

	close();
	return 0;
}