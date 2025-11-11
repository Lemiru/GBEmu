#include "Display.h"
#include <imgui_impl_sdl3.h>


Display::Display(SDL_Renderer* ren, MemoryManager* memory)
{
	renderer = ren;
    mem = memory;
    reset();
    target_frametime_ns = 16742706;
    render();
}

void Display::cycle()
{
    LCDC_flags = mem->getIOREGValue(0xFF40);
    BGP = mem->getIOREGValue(0xFF47);
    scy = mem->getIOREGValue(0xFF42);
    scx = mem->getIOREGValue(0xFF43);
    wy = mem->getIOREGValue(0xFF4A);
    wx = mem->getIOREGValue(0xFF4B);
    tick();
    tick();
    tick();
    tick();
}

void Display::reset()
{
    tick_count = 0;
    scanline = 0;
    x_pos = 0;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    screenArray = std::vector<std::vector<Uint32>>(144, std::vector<Uint32>(160, WHITE));
    screenArrayLastFrame = std::vector<std::vector<Uint32>>(144, std::vector<Uint32>(160, WHITE));
    sprite_buffer = std::vector<Sprite>();
    sprite_buffer.reserve(10);
    background_FIFO = std::queue<Pixel>();
    sprite_FIFO = std::queue<Pixel>();
    background_pixel_fetcher = Background_Fetcher(mem, &background_FIFO);
    sprite_pixel_fetcher = Sprite_Fetcher(mem, &sprite_FIFO);
    current_mode = OAM_SCAN_MODE;
    SDL_color_black = SDL_MapRGB(SDL_GetPixelFormatDetails(texture->format), NULL, 0, 0, 0);
    SDL_color_dark_gray = SDL_MapRGB(SDL_GetPixelFormatDetails(texture->format), NULL, 42, 42 * 1.30, 42);
    SDL_color_light_gray = SDL_MapRGB(SDL_GetPixelFormatDetails(texture->format), NULL, 85, 85 * 1.30, 85);
    SDL_color_white = SDL_MapRGB(SDL_GetPixelFormatDetails(texture->format), NULL, 127, 127 * 1.30, 127);
    high_sprites = false;
    sprite_fetcher_working = false;
    window = false;
    disabled = false;
    blank_frame = false;
    last_stat = false;
    line_153_to_0 = false;
    off_ticks = 0;
    last_frame_time = SDL_GetTicksNS();
}

void Display::tick()
{
    if ((LCDC_flags & 0b10000000) == 0 and !disabled)
    {
        disabled = true;
        off_ticks = (scanline * 456) + tick_count;
        scanline = 0;
        tick_count = 0;
        start_HBlank();
        mem->setIOREGValue(0xFF44, 0);
    }

    if ((LCDC_flags & 0b10000000) != 0 and disabled)
    {
        for (int x = 0; x < 160; x++)
        {
            for (int y = 0; y < 144; y++)
            {
                screenArray[y][x] = SDL_color_white;
            }
        }
        blank_frame = true;
        disabled = false;
        off_ticks = 0;
    }

    if (!disabled)
    {
        switch (current_mode)
        {
        case HBLANK_MODE:
            break;
        case VBLANK_MODE:
            break;
        case OAM_SCAN_MODE:
            break;
        case DRAWING_MODE:
            if (((LCDC_flags & 0x02) != 0) and !sprite_fetcher_working)
            {
                check_for_sprite_fetching();
            }

            if (sprite_fetcher_working)
            {
                sprite_fetcher_working = sprite_pixel_fetcher.tick(x_pos, scanline, LCDC_flags);
            }
            else
            {
                background_pixel_fetcher.tick(scx, scy, scanline, LCDC_flags, BGP);
                output_Pixel();
            }
            break;
        default:
            break;
        }
        check_for_stat_interrupt();
        tick_count++;
        if (scanline == 153 and tick_count == 4 and !mem->isInCGBMode())
        {
            scanline = 0;
            line_153_to_0 = true;
        }
    }
    if (tick_count == 456)
    {
        tick_count = 0;
        if (!line_153_to_0 == true)
        {
            scanline++;
            if (scanline == 154)
                scanline = 0;
        }
        else
        {
            line_153_to_0 = false;
        }
        mem->setIOREGValue(0xFF44, scanline);
        uint8_t stat = mem->getIOREGValue(0xFF41);
        stat = (stat & 0b11111011);
        if (scanline == mem->getIOREGValue(0xFF45))
        {
            stat += 0b00000100;
        }
        mem->setIOREGValue(0xFF41, stat);
    }
    if (!disabled)
    {
        if (x_pos == 160 and current_mode != HBLANK_MODE)
        {
            start_HBlank();
        }
        if (scanline == 144 and current_mode != VBLANK_MODE)
        {
            start_VBlank();
        }
        if (current_mode != VBLANK_MODE or (scanline == 0 and !line_153_to_0))
        {
            switch (tick_count)
            {
            case 0:
                start_OAM_Scan();
                break;
            case 80:
                start_Drawing();
                break;
            default:
                break;
            }
        }
        if (current_mode == OAM_SCAN_MODE and ((tick_count % 2) == 1))
        {
            OAM_read_tick();
        }
    }
    if (disabled and ++off_ticks == 70224)
    {
        off_ticks = 0;
        render();
    }
}

void Display::check_for_stat_interrupt()
{
    bool should_be_high = false;
    uint8_t stat = (mem->getIOREGValue(0xFF41) & 0b11111011);

    if (scanline == mem->getIOREGValue(0xFF45))
    {
        stat += 0b00000100;
        if (stat & 0b01000000)
        {
            should_be_high = true;
        }
    }
    mem->setIOREGValue(0xFF41, stat);

    switch (current_mode)
    {
        case HBLANK_MODE:
            if ((stat & 0b00001000) and !disabled)
            {
                should_be_high = true;
            }
            break;
        case VBLANK_MODE:
            if (stat & 0b00010000)
            {
                should_be_high = true;
            }
            break;
        case OAM_SCAN_MODE:
            if (stat & 0b00100000)
            {
                should_be_high = true;
            }
            break;
    }
    if (should_be_high and !last_stat)
    {
        mem->requestInterrupt(STAT_INTERRUPT);
    }
    last_stat = should_be_high;
}

void Display::OAM_read_tick()
{
    if (sprite_buffer.size() < 10)
    {
        uint16_t OAM_address = 0xFE00 + (tick_count/2) * 4;

        uint8_t sprite_y = mem->getMemValue(OAM_address);
        uint8_t sprite_x = mem->getMemValue(++OAM_address);
        uint8_t sprite_tile = mem->getMemValue(++OAM_address);
        uint8_t sprite_flags = mem->getMemValue(++OAM_address);

        if (sprite_x > 0)
        {
            uint8_t current_y = scanline + 16;
            if (current_y >= sprite_y)
            {
                bool high = ((LCDC_flags & 0x04) != 0);
                if (current_y < (sprite_y + 8 + (high * 8)))
                {
                    bool flip = ((sprite_flags & 0x40) != 0);
                    if (high)
                    {
                        if (flip)
                        {
                            sprite_tile = (sprite_tile & 0xFE) + 1;
                            if (current_y > (sprite_y + 7))
                            {
                                sprite_tile--;
                            }
                        }
                        else 
                        {
                            sprite_tile = (sprite_tile & 0xFE);
                            if (current_y > (sprite_y + 7))
                            {
                                sprite_tile++;
                            }
                        }
                    }
                    Sprite spr = {};
                    spr.x = sprite_x;
                    spr.y = sprite_y;
                    spr.tile_number = sprite_tile;
                    spr.vertical_flip = flip;
                    spr.horizontal_flip = ((sprite_flags & 0x20) != 0);
                    spr.oam_pos = tick_count / 2;
                    if (mem->isInCGBMode())
                    {
                        spr.palette = (sprite_flags & 0b00000111);
                        spr.vram_bank = (sprite_flags & 0b00001000) != 0;
                    }
                    else
                    {
                        spr.palette = ((sprite_flags & 0x10) != 0);
                        spr.vram_bank = 0;
                    }
                    spr.background_priority = ((sprite_flags & 0x80) != 0);
                    spr.readed = false;
                    spr.vram_bank = (sprite_flags & 0x08) and mem->isInCGBMode();
                    sprite_buffer.push_back(spr);
                }
            }
        }
    }
}

void Display::start_Drawing()
{
    window = false;
    uint8_t stat = mem->getIOREGValue(0xFF41);
    mem->setIOREGValue(0xFF41, (stat & 0b11111100) + 0b11);
    current_mode = DRAWING_MODE;
    mem->setDisplayMode(DRAWING_MODE);
}

void Display::start_VBlank()
{
    uint8_t stat = mem->getIOREGValue(0xFF41);
    
    mem->setIOREGValue(0xFF41, (stat & 0b11111100) + 0b01);
    current_mode = VBLANK_MODE;
    mem->requestInterrupt(VBLANK_INTERRUPT);
    mem->setDisplayMode(VBLANK_MODE);
    render();
}

void Display::start_HBlank()
{
    uint8_t stat = mem->getIOREGValue(0xFF41);
    mem->setIOREGValue(0xFF41, (stat & 0b11111100));
    mem->cycleHDMA();
    current_mode = HBLANK_MODE;
    mem->setDisplayMode(HBLANK_MODE);
    x_pos = 0;
    background_pixel_fetcher.reset();
    sprite_pixel_fetcher.reset();
    sprite_buffer.clear();
}

void Display::start_OAM_Scan()
{
    uint8_t stat = mem->getIOREGValue(0xFF41);
    mem->setIOREGValue(0xFF41, (stat & 0b11111100) + 0b10);
    current_mode = OAM_SCAN_MODE;
    mem->setDisplayMode(OAM_SCAN_MODE);
}

void Display::output_Pixel()
{
    if ((LCDC_flags & 0x20) != 0)
    {
        uint8_t wy = mem->getIOREGValue(0xFF4A);
        if (scanline >= wy and !window)
        {
            if (x_pos == (mem->getIOREGValue(0xFF4B) - 7) or mem->getIOREGValue(0xFF4B) < 7)
            {
                while (!background_FIFO.empty())
                {
                    background_FIFO.pop();
                }
                background_pixel_fetcher.start_window(scanline == wy);
                window = true;
            }
        }
    }
    if (!background_FIFO.empty() and x_pos < 160 and scanline < 144)
    {
        screenArray[scanline][x_pos] = mix_Pixels();
        x_pos++;
    }
}

void Display::check_for_sprite_fetching() {
    for (int x = 0; x < sprite_buffer.size(); x++)
    {
        if (!sprite_buffer[x].readed)
        {
            if (sprite_buffer[x].x <= (x_pos + 8)) {
                sprite_buffer[x].readed = true;
                sprite_pixel_fetcher.set_sprite(sprite_buffer[x]);
                background_pixel_fetcher.reset_ticks();
                sprite_fetcher_working = true;
                return;
            }
        }
    }
}

Uint32 Display::mix_Pixels()
{
    Pixel bg = background_FIFO.front();
    background_FIFO.pop();

    if (sprite_FIFO.empty())
    {
        if (mem->isInCGBMode())
        {
            uint16_t color_palette = mem->getBCRAMValue((bg.palette * 8) + (bg.color * 2)) + (mem->getBCRAMValue((bg.palette * 8) + (bg.color * 2) + 1) << 8);
            return SDL_MapRGB(SDL_GetPixelFormatDetails(texture->format), NULL, (color_palette & 0b00011111) * 5, ((color_palette >> 5) & 0b00011111) * 5, ((color_palette >> 10) & 0b00011111) * 5);
        }
        else
        {
            return shade_to_color((Shade)((bg.palette >> ((uint8_t)bg.color * 2)) & 0x03));
        }
    }
    else
    {
        Pixel spr = sprite_FIFO.front();
        sprite_FIFO.pop();
        if (spr.color == 0 or (bg.color != 0 and !bg.sprite_priority and (spr.background_priority or bg.background_priority)))
        {
            if (mem->isInCGBMode())
            {
                uint16_t color_palette = mem->getBCRAMValue((bg.palette * 8) + (bg.color * 2)) + (mem->getBCRAMValue((bg.palette * 8) + (bg.color * 2) + 1) << 8);
                return SDL_MapRGB(SDL_GetPixelFormatDetails(texture->format), NULL, (color_palette & 0b00011111) * 5, ((color_palette >> 5) & 0b00011111) * 5, ((color_palette >> 10) & 0b00011111) * 5);
            }
            else
            {
                return shade_to_color((Shade)((bg.palette >> ((uint8_t)bg.color * 2)) & 0x03));
            }
        }
        else
        {
            if (mem->isInCGBMode())
            {
                uint16_t color_palette = mem->getOCRAMValue((spr.palette * 8) + (spr.color * 2)) + (mem->getOCRAMValue((spr.palette * 8) + (spr.color * 2) + 1) << 8);
                return SDL_MapRGB(SDL_GetPixelFormatDetails(texture->format), NULL, (color_palette & 0b00011111) * 5, ((color_palette >> 5) & 0b00011111) * 5, ((color_palette >> 10) & 0b00011111) * 5);
            }
            else
            {
                return shade_to_color((Shade)((spr.palette >> ((uint8_t)spr.color * 2)) & 0x03));
            }
        }
    }
}

Uint32 Display::shade_to_color(Shade shade)
{
    switch (shade)
    {
    case BLACK:
        return SDL_color_black;
    case DARK_GRAY:
        return SDL_color_dark_gray;
    case LIGHT_GRAY:
        return SDL_color_light_gray;
    case WHITE:
        return SDL_color_white;
    default:
        return 0;
    }
}

void Display::render()
{
    Uint32* pixels = nullptr;
    int pitch;
    if (SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch)) {
        if (!disabled and !blank_frame)
        {
            for (int x = 0; x < 160; x++)
            {
                for (int y = 0; y < 144; y++)
                {
                    pixels[y * 160 + x] = screenArray[y][x];
                }
            }
        }
        else if(!mem->isInCGBMode())
        {
            for (int x = 0; x < 160; x++)
            {
                for (int y = 0; y < 144; y++)
                {
                    pixels[y * 160 + x] = SDL_color_white;
                }
            }
        }
        if (blank_frame)
        {
            blank_frame = false;
        }
        SDL_UnlockTexture(texture);
    }
    
    SDL_FRect rect = SDL_FRect{ 0.0f, (float)getMenuHeight(), 160.0f * Config::getResolutionScale(), 144.0f * Config::getResolutionScale() };
    SDL_RenderTexture(renderer, texture, NULL, &rect);
    drawImGuiMenu(renderer);
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);

    uint64_t new_frame_time = SDL_GetTicksNS();
    uint64_t current_frametime = (new_frame_time - last_frame_time);
    uint32_t target_frametime_ns_adjusted = (target_frametime_ns / Config::getSpeedScale());
    if (current_frametime < target_frametime_ns_adjusted)
    {
        SDL_DelayPrecise(target_frametime_ns_adjusted - current_frametime);
        new_frame_time = SDL_GetTicksNS();
    }
    last_frame_time = new_frame_time;
}

Background_Fetcher::Background_Fetcher(MemoryManager* memory, std::queue<Pixel>* fifo)
{
    mem = memory;
    lcdc0 = false;
    lcdc3 = false;
    lcdc4 = false;
    lcdc6 = false;
    initial_offset = true;
    window = false;
    x_counter = 0;
    tile_offset = 0;
    window_y_counter = 0;
    ticks = 0;
    background_fifo = fifo;
    tile = 0;
    byte_low = 0;
    byte_high = 0;
}

void Background_Fetcher::tick(uint8_t scx, uint8_t scy, uint8_t scanline, uint8_t lcdc, uint8_t bgp)
{
    switch (ticks)
    {
    case 0:
        lcdc0 = (lcdc & 0x01) != 0;
        lcdc3 = (lcdc & 0x08) != 0;
        lcdc4 = (lcdc & 0x10) != 0;
        lcdc6 = (lcdc & 0x40) != 0;
        if (window)
        {
            tile_offset = window_y_counter;
        }
        else
        {
            tile_offset = (scanline + scy);
        }
        break;
    case 1:
        if ((lcdc6 and window) or (lcdc3 and !window))
        {
            tile = mem->getVRAMValue(0x9C00 + ((x_counter + (!window * (scx / 8)) & 0x1F)) + 32 * (tile_offset / 8));
            attributes = mem->getVRAMValue(0x9C00 + ((x_counter + (!window * (scx / 8)) & 0x1F)) + 32 * (tile_offset / 8), true);
        }
        else
        {
            tile = mem->getVRAMValue(0x9800 + ((x_counter + (!window * (scx / 8)) & 0x1F)) + 32 * (tile_offset / 8));
            attributes = mem->getVRAMValue(0x9800 + ((x_counter + (!window * (scx / 8)) & 0x1F)) + 32 * (tile_offset / 8), true);
        }
        break;
    case 2:
        break;
    case 3:
        if ((attributes & 0b01000000) != 0)
        {
            if (lcdc4)
            {
                byte_low = mem->getVRAMValue(0x8000 + ((int16_t)tile * 16) + (2 * (7- (tile_offset % 8))), (attributes & 0b00001000) != 0);
            }
            else
            {
                byte_low = mem->getVRAMValue(0x9000 + (((int16_t)(int8_t)tile * 16) + (2 * (7 - (tile_offset % 8)))), (attributes & 0b00001000) != 0);
            }
        }
        else
        {
            if (lcdc4)
            {
                byte_low = mem->getVRAMValue(0x8000 + ((int16_t)tile * 16) + (2 * (tile_offset % 8)), (attributes & 0b00001000) != 0);
            }
            else
            {
                byte_low = mem->getVRAMValue(0x9000 + (((int16_t)(int8_t)tile * 16) + (2 * (tile_offset % 8))), (attributes & 0b00001000) != 0);
            }
        }
        break;
    case 4:
        break;
    case 5:
        if ((attributes & 0b01000000) != 0)
        {
            if (lcdc4)
            {
                byte_high = mem->getVRAMValue(0x8000 + ((int16_t)tile * 16) + (2 * (7 - (tile_offset % 8))) + 1, (attributes & 0b00001000) != 0);
            }
            else
            {
                byte_high = mem->getVRAMValue(0x9000 + (((int16_t)(int8_t)tile * 16) + (2 *(7 - (tile_offset % 8)))) + 1, (attributes & 0b00001000) != 0);
            }
        }
        else
        {
            if (lcdc4)
            {
                byte_high = mem->getVRAMValue(0x8000 + ((int16_t)tile * 16) + (2 * (tile_offset % 8)) + 1, (attributes & 0b00001000) != 0);
            }
            else
            {
                byte_high = mem->getVRAMValue(0x9000 + (((int16_t)(int8_t)tile * 16) + (2 * (tile_offset % 8))) + 1, (attributes & 0b00001000) != 0);
            }
        }
        break;
    default:
        if (background_fifo->empty())
        {
            uint8_t x = 0;
            if (initial_offset)
            {
                initial_offset = false;
                x += (!window) * (scx % 8);
            }
            for (x; x < 8; x++)
            {
                uint8_t pixel_num = 7 - x;
                if ((attributes & 0b00100000) != 0)
                {
                    pixel_num = 7 - pixel_num;
                }
                uint8_t mask = 0x01 << (pixel_num);
                Pixel to_push = {};
                if (!lcdc0 and !mem->isInCGBMode())
                {
                    to_push.color = WHITE;
                    to_push.palette = 0b00000000;
                }
                else
                {
                    to_push.color = (Shade)(((byte_low & mask) >> pixel_num) + 2 * ((byte_high & mask) >> pixel_num));
                    if (mem->isInCGBMode())
                    {
                        to_push.palette = attributes & 0b00000111;
                    }
                    else
                    {
                        to_push.palette = bgp;
                    }
                }

                to_push.sprite_priority = !lcdc0;
                to_push.background_priority = (attributes & 0b10000000) != 0;
                background_fifo->push(to_push);
            }
            ticks = 255;
            x_counter++;
        }
        break;
    }
    ticks++;
}

void Background_Fetcher::reset()
{
    while (!background_fifo->empty())
    {
        background_fifo->pop();
    }
    x_counter = 0;
    ticks = 0;
    initial_offset = true;
    window = false;
}

void Background_Fetcher::reset_ticks()
{
    ticks = 0;
}

void Background_Fetcher::start_window(bool initial_scanline)
{
    window = true;
    x_counter = 0;
    if (initial_scanline)
    {
        window_y_counter = 0;
    }
    else 
    {
        window_y_counter++;
    }
    reset_ticks();
}

Sprite_Fetcher::Sprite_Fetcher(MemoryManager* memory, std::queue<Pixel>* fifo)
{
    mem = memory;
    x_counter = 0;
    ticks = 0;
    sprite_fifo = fifo;
    tile = 0;
    byte_low = 0;
    byte_high = 0;
}


bool Sprite_Fetcher::tick(uint8_t x_pos, uint8_t scanline, uint8_t lcdc)
{
    switch (ticks)
    {
    case 0:
        
        break;
    case 1:
        tile = spr.tile_number;
        break;
    case 2:
        break;
    case 3:
        if (!spr.vertical_flip)
        {
            byte_low = mem->getVRAMValue(0x8000 + ((int16_t)tile * 16) + 2 * (7 - ((spr.y - scanline - 1) % 8)), spr.vram_bank);
        }
        else
        {
            byte_low = mem->getVRAMValue(0x8000 + ((int16_t)tile * 16) + 2 * ((spr.y - scanline - 1) % 8), spr.vram_bank);
        }
        
        break;
    case 4:
        break;
    case 5:
        if (!spr.vertical_flip)
        {
            byte_high = mem->getVRAMValue(0x8000 + ((int16_t)tile * 16) + 2 * (7 - ((spr.y - scanline - 1) % 8)) + 1, spr.vram_bank);
        }
        else
        {
            byte_high = mem->getVRAMValue(0x8000 + ((int16_t)tile * 16) + 2 * ((spr.y - scanline - 1) % 8) + 1, spr.vram_bank);
        }
        break;
    default:
        uint8_t cond_offset = 0;
        if (spr.x < 8)
        {
            cond_offset = (8 - spr.x);
        }
        for (uint8_t x = 0; (x + cond_offset) < 8; x++)
        {
            if (sprite_fifo->size() <= x)
            {
                uint8_t pixel_num = 7 - (x + cond_offset);
                if (spr.horizontal_flip)
                {
                    pixel_num = 7 - pixel_num;
                }
                uint8_t mask = 0x01 << (pixel_num);
                Pixel to_push = {};
                to_push.color = (Shade)(((byte_low & mask) >> pixel_num) + 2 * ((byte_high & mask) >> pixel_num));
                if (mem->isInCGBMode())
                {
                    to_push.palette = spr.palette;
                }
                else
                {
                    to_push.palette = mem->getIOREGValue(0xFF48 + spr.palette);
                }
                to_push.sprite_priority = false;
                to_push.oam_pos = spr.oam_pos;
                to_push.background_priority = spr.background_priority;
                sprite_fifo->push(to_push);
            }
            else
            {
                std::queue<Pixel> temp;
                for (int i = 0; i < x; i++)
                {
                    temp.push(sprite_fifo->front());
                    sprite_fifo->pop();
                }
                Pixel temp_pixel = sprite_fifo->front();
                sprite_fifo->pop();
                if (temp_pixel.color == 0 or ((temp_pixel.oam_pos > spr.oam_pos) and mem->isInCGBMode()))
                {
                    uint8_t pixel_num = 7 - (x + cond_offset);
                    if (spr.horizontal_flip)
                    {
                        pixel_num = 7 - pixel_num;
                    }
                    uint8_t mask = 0x01 << (pixel_num);
                    Pixel to_push = {};
                    to_push.color = (Shade)(((byte_low & mask) >> pixel_num) + 2 * ((byte_high & mask) >> pixel_num));
                    if (to_push.color == 0)
                    {
                        temp.push(temp_pixel);
                    }
                    else
                    {
                        if (mem->isInCGBMode())
                        {
                            to_push.palette = spr.palette;
                        }
                        else
                        {
                            to_push.palette = mem->getIOREGValue(0xFF48 + spr.palette);
                        }
                        to_push.sprite_priority = false;
                        to_push.oam_pos = spr.oam_pos;
                        to_push.background_priority = spr.background_priority;
                        temp.push(to_push);
                    }
                }
                else
                {
                    temp.push(temp_pixel);
                }
                while (!sprite_fifo->empty())
                {
                    temp.push(sprite_fifo->front());
                    sprite_fifo->pop();
                }
                sprite_fifo->swap(temp);
            }
        };
        ticks = 0;
        return false;
        break;
    }
    ticks++;
    return true;
}

void Sprite_Fetcher::set_sprite(Sprite sprite)
{
    spr = sprite;
}

void Sprite_Fetcher::reset()
{
    while (!sprite_fifo->empty())
    {
        sprite_fifo->pop();
    }
    x_counter = 0;
    ticks = 0;
}