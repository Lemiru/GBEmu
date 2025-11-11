#pragma once

#include <stdint.h>

#include "../Memory/MemoryManager.h"
#include "../Utils/ImGui.h"
#include "../Utils/Config.h"

#include <vector>
#include <queue>
#include <iostream>
#include <stdio.h>
#include <SDL3/SDL.h>

enum Shade
{
	WHITE,
	LIGHT_GRAY,
	DARK_GRAY,
	BLACK
};

struct Pixel
{
	Shade color;
	uint8_t palette;
	uint8_t oam_pos;
	bool sprite_priority;
	bool background_priority;
};

struct Sprite
{
	uint8_t x;
	uint8_t y;
	uint8_t tile_number;
	uint8_t palette;
	uint8_t oam_pos;
	bool vertical_flip;
	bool horizontal_flip;
	bool background_priority;
	bool vram_bank;
	bool readed;
};

enum Modes
{
	HBLANK_MODE,
	VBLANK_MODE,
	OAM_SCAN_MODE,
	DRAWING_MODE
};

class Background_Fetcher
{
private:
	MemoryManager* mem;
	std::queue<Pixel>* background_fifo;
	bool lcdc0;
	bool lcdc3;
	bool lcdc4;
	bool lcdc6;
	bool initial_offset;
	bool window;
	uint8_t x_counter;
	uint8_t tile_offset;
	uint8_t window_y_counter;
	uint8_t ticks;
	uint8_t tile;
	uint8_t attributes;
	uint8_t byte_low;
	uint8_t byte_high;

public:
	Background_Fetcher() = default;
	Background_Fetcher(MemoryManager* memory, std::queue<Pixel>* fifo);
	void tick(uint8_t scx, uint8_t scy, uint8_t scanline, uint8_t lcdc, uint8_t bgp);
	void reset();
	void reset_ticks();
	void start_window(bool initial_scanline);

private:

};

class Sprite_Fetcher
{
private:
	MemoryManager* mem;
	std::queue<Pixel>* sprite_fifo;
	uint8_t x_counter;
	uint8_t ticks;
	uint8_t tile;
	uint8_t byte_low;
	uint8_t byte_high;
	Sprite spr;

public:
	Sprite_Fetcher() = default;
	Sprite_Fetcher(MemoryManager* memory, std::queue<Pixel>* fifo);
	bool tick(uint8_t x_pos, uint8_t scanline, uint8_t lcdc);
	void set_sprite(Sprite sprite);
	void reset();

private:

};

class Display
{
private:
	std::vector<std::vector<Uint32>> screenArray;
	std::vector<std::vector<Uint32>> screenArrayLastFrame;
	std::vector<Sprite> sprite_buffer;
	std::queue<Pixel> background_FIFO;
	std::queue<Pixel> sprite_FIFO;
	Background_Fetcher background_pixel_fetcher;
	Sprite_Fetcher sprite_pixel_fetcher;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	MemoryManager* mem;
	uint16_t tick_count;
	uint8_t scanline;
	uint8_t x_pos;
	uint8_t scx;
	uint8_t scy;
	uint8_t wx;
	uint8_t wy;
	uint8_t LCDC_flags;
	uint8_t BGP;
	uint8_t window;
	Modes current_mode;
	Uint32 SDL_color_black;
	Uint32 SDL_color_dark_gray;
	Uint32 SDL_color_light_gray;
	Uint32 SDL_color_white;
	bool high_sprites;
	bool sprite_fetcher_working;
	bool disabled;
	bool blank_frame;
	bool last_stat;
	bool line_153_to_0;
	uint32_t off_ticks;
	uint64_t last_frame_time;
	uint32_t target_frametime_ns;

public:
	Display(SDL_Renderer* ren, MemoryManager* memory);
	void cycle();
	void reset();

private:
	void tick();
	void OAM_read_tick();
	void start_Drawing();
	void start_VBlank();
	void start_HBlank();
	void start_OAM_Scan();
	void check_for_sprite_fetching();
	void check_for_stat_interrupt();
	void output_Pixel();
	Uint32 mix_Pixels();
	Uint32 shade_to_color(Shade shade);
	void disable();
	void enable();
	void render();
};