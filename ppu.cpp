#include "Bus.h"
#include "ppu.h"

uint32_t pallete[64] = {
	0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400, 0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
	0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10, 0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
	0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044, 0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
	0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8, 0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
};

NesPPU::NesPPU()
{
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("BlueNes", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 256, 240);
	pixels = new Uint32[240 * 256];
	ptrToBus = NULL;
	ppu_cycles = 0;
	ppu_scanline = 0;
	v = 0;
	t = 0;
	w = true;
	oam_addr = 0;
	sprite_hit_cycle = 10000;
}

NesPPU::~NesPPU()
{
	delete[] pixels;
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyTexture(texture);
	window = NULL;
	renderer = NULL;
	texture = NULL;
	SDL_Quit();
}

void NesPPU::titleUpdate(const char* title)
{
	SDL_SetWindowTitle(window, title);
}

void NesPPU::copy_x()
{
	if (mask_background || mask_sprites)
	{
		v &= 0xffe0;
		v |= (t & 0x1F);//Coarse X
		v &= 0xfbff;
		v |= ((t >> 10) & 0x1) << 10;//Nametable X
	}
}

void NesPPU::ppu_write_4014(uint8_t data)
{
	int dma_page = data;
	int oam_addr_cpy = oam_addr;
	for (int dma_addr = 0; dma_addr < 256; dma_addr++)
	{
		int addr = dma_addr + oam_addr_cpy;
		if (addr > 256)
			oam_addr_cpy = 0;
		//need to implement cpu wait
		int data = busPtr->ask_cpu(dma_page << 8 | dma_addr);
		primary_oam[dma_addr] = data;
	}
}

void NesPPU::ppu_write(int addr, uint8_t data)
{
	if (addr >= 0x2000 && addr <= 0x3000)
	{
		if (true)
		{
			ppu_ram[addr] = data;
			ppu_ram[addr + 0x400] = data;
			ppu_ram[addr + 0x800] = data;
			ppu_ram[addr + 0xC00] = data;
		}
		else if (horizontal_mirroring)
		{
			ppu_ram[addr] = data;
			ppu_ram[addr + 0x400] = data;
		}
		else //vertical mirroring
		{
			ppu_ram[addr] = data;
			ppu_ram[addr + 0x800] = data;
		}

	}
	else if (addr >= 0x3f00 && addr <= 0x3F1F)
	{
		ppu_ram[addr] = data;
		switch (addr)
		{
		case 0x3f00:
			ppu_ram[0x3f10] = data;
			break;
		case 0x3f04:
			ppu_ram[0x3f14] = data;
			break;
		case 0x3f08:
			ppu_ram[0x3f18] = data;
			break;
		case 0x3f0c:
			ppu_ram[0x3f1c] = data;
			break;
		}
		switch (addr)
		{
		case 0x3f10:
			ppu_ram[0x3f00] = data;
			break;
		case 0x3f14:
			ppu_ram[0x3f04] = data;
			break;
		case 0x3f18:
			ppu_ram[0x3f08] = data;
			break;
		case 0x3f1c:
			ppu_ram[0x3f0c] = data;
			break;
		}
	}
	else
		ppu_ram[addr & 0x3fff] = data;
}

uint16_t NesPPU::ppu_read(int addr)
{
	if ((addr < 0x2000) && (busPtr->chrFilled()))
	{
		uint8_t t = busPtr->bus_read_ppu(addr);
		return busPtr->bus_read_ppu(addr);
	}
	else
		return ppu_ram[addr];
}

void NesPPU::increment_x()
{
	if (mask_background || mask_sprites)
	{
		if ((v & 0x001F) == 31)
		{
			v &= ~0x001F;
			v ^= 0x0400;
		}
		else
			v += 1;
	}
}

void NesPPU::increment_y()
{
	if (mask_background || mask_sprites)
	{
		if ((v & 0x7000) != 0x7000)
		{
			v += 0x1000;
		}
		else
		{
			v &= ~0x7000;
			int y = (v & 0x03E0) >> 5;
			if (y == 29)
			{
				y = 0;
				v ^= 0x0800;
			}
			else if (y == 31)
			{
				y = 0;
			}
			else
			{
				y += 1;
			}
			v = (v & ~0x03E0) | (y << 5);
		}
	}
}

void NesPPU::copy_y()
{
	if (mask_background || mask_sprites)
	{
		v &= 0xfc1f;
		v |= ((t >> 5) & 0x1F) << 5; //coarse Y
		v &= 0xf7ff;
		v |= ((t >> 11) & 0x1) << 11;//Nametable y
		v &= 0x0fff;
		v |= ((t >> 12) & 0x7) << 12;//Fine Y
	}
}

uint16_t NesPPU::interleave(uint16_t byte_0, uint16_t byte_1)
{
	int final = 0;
	for (int i = 0; i < 8; i++)
	{
		int x_masked_i = (byte_0 & (1 << i));
		int y_masked_i = (byte_1 & (1 << i));

		final |= (x_masked_i << i);
		final |= (y_masked_i << (i + 1));
	}
	return final;
}

void NesPPU::step_ppu()
{
	if (ppu_scanline >= 0 && ppu_scanline < 240)
	{
		if (ppu_cycles == 0) {
			if (mask_background) { drawBackgroundLines(ppu_scanline); }
			if (ppu_scanline > 0)
			{
				evaluateSprites(ppu_scanline - 1);
				if (mask_sprites) { drawSpriteLines(ppu_scanline - 1); }
			}
		}

		if (ppu_cycles == sprite_hit_cycle) { sprite_0_hit = true; }

		if (ppu_cycles == 256) { increment_y(); }

		if (ppu_cycles == 257) { copy_x(); }
	}
	else if (ppu_scanline == 241 && ppu_cycles == 1)
	{
		vblank = true;
		nmi_occured = true;
		updateWindow();
	}
	else if (ppu_scanline == 261)
	{
		vblank = false;
		sprite_0_hit = false;
		nmi_occured = false;
		sprite_hit_cycle = 1000;
		copy_y();
	}

	ppu_cycles++;

	if (ppu_cycles > 340)
	{
		ppu_cycles = 0;
		ppu_scanline++;
		if (ppu_scanline == 262)
			ppu_scanline = 0;
	}
}

void NesPPU::ppu_powerup()
{
	ppu_scanline = 0;
	ppu_cycles = 0;
}

void NesPPU::updateWindow()
{
	SDL_UpdateTexture(texture, 0, pixels, 256 * sizeof(Uint32));
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, 0, 0);
	SDL_RenderPresent(renderer);
}

uint8_t NesPPU::ppu_read_registers(uint16_t addr)
{
	uint8_t data = 0;

	switch (addr & 0x2007)
	{
	case 0x2002:
		//clear nmi_occured
		w = true;
		nmi_occured = false;
		data = vblank ? data += 0x80 : data &= 0x7F;
		data = sprite_0_hit ? data += 0x40 : data &= 0xBF;
		break;
	case 0x2004:
		if (vblank)
		{
			data = primary_oam[oam_addr];
		}
		break;
	case 0x2007:
	{
		data = ppu_buffer;
		ppu_buffer = (uint8_t)ppu_read(v);
		if (v > 0x3f00)
			data = ppu_buffer;
		//increment vram addr per read/write of ppudata
		v += v_inc;
		break;
	}
	}
	return data;
}

void NesPPU::evaluateSprites(int scanline)
{
	sprite_0_present = false;
	sprite_count = 0;
	for (int n = 0; n < 64; n++)
	{
		int r = primary_oam[n * 4 + 0];
		if (sprite_count < 8)
		{
			r = scanline - r;
			if (r >= 0 && r < sprite_size)
			{
				if (n == 0)
				{
					sprite_0_present = true;
				}
				secondary_oam[sprite_count * 4 + 0] = primary_oam[n * 4 + 0];
				secondary_oam[sprite_count * 4 + 1] = primary_oam[n * 4 + 1];
				secondary_oam[sprite_count * 4 + 2] = primary_oam[n * 4 + 2];
				secondary_oam[sprite_count * 4 + 3] = primary_oam[n * 4 + 3];
				sprite_count++;
			}
		}
	}
}

void NesPPU::ppu_write_registers(uint16_t addr, uint8_t data)
{
	switch (addr & 0x2007)
	{
	case 0x2000:
		v_inc = (data & 0x4) ? 32 : 1;
		sprite_table = (data & 0x8) ? 0x1000 : 0x000;
		background_table = (data & 0x10) ? 0x1000 : 0x000;
		sprite_size = (data & 0x20) ? 16 : 8;
		nmi_output = (data & 0x80) ? true : false;
		t &= 0xF3FF;
		t |= (data & 0x3) << 10;
		break;
	case 0x2001:
		mask_background = data & 0x8;
		mask_sprites = data & 0x10;
		break;
	case 0x2002:
		break;
	case 0x2003:
		oam_addr = data;
		break;
	case 0x2004:
		primary_oam[oam_addr] = data;
		oam_addr++;
		break;
	case 0x2005:
		if (w)
		{
			fine_x = data & 0x07;
			t &= 0xffe0;
			t |= ((uint16_t)data >> 3);
			w = false;
		}
		else
		{
			t &= 0xfc1f;
			t |= ((uint16_t)data >> 3) << 5;
			t &= 0x0fff;
			t |= ((uint16_t)data & 0x7) << 12;
			w = true;
		}
		break;
	case 0x2006://ppuaddr
	{
		//$2006 holds no memory
		if (w)
		{
			//high byte
			t &= 0x00ff;
			t |= (data & 0x3F) << 8;
			w = false;
		}
		else
		{
			//low byte
			t &= 0xff00;
			t |= data;
			w = true;
			v = t;
		}
		break;
	}
	case 0x2007://ppudata
	{
		ppu_write(v, data);
		// increment vram addr per read/write of ppudata
		v += v_inc;
		break;
	}
	}
}

void NesPPU::drawBackgroundLines(int y)
{
	uint8_t pixel;
	uint16_t pixel_help;
	int pos, bkg_color, c, location;
	bool next_tile = true;
	int m = fine_x;
	int index;
	for (int x = 0; x < 32; x++)
	{
		//nametable byte
		int addr = 0x2000 + (v & 0x0fff);
		nametable_byte = ppu_read(0x2000 | (v & 0x0FFF));
		low_bg_tile = ppu_read(background_table + (nametable_byte * 16) + ((v >> 12) & 0x7));
		hi_bg_tile = ppu_read(background_table + (nametable_byte * 16) + ((v >> 12) & 0x7) + 8);
		attribute_byte = ppu_read(0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07));
		pixel_help = interleave(low_bg_tile, hi_bg_tile);
		for (int l = 0; l < 8; l++)
		{
			if ((m + l) == 8)
				next_tile = false;
			if (next_tile)
			{
				pos = 7 - (m + l);
				pixel = (pixel_help & (0x3 << (pos * 2))) >> (pos * 2);
				//atrib
				location = ((v >> 4) & 4) | (v & 2);
				c = (pixel == 0) ? 0 : ((attribute_byte >> location) & 0b11) * 4;
				//store
				bkg_color = (pallete[ppu_read(0x3f00 + c + pixel)]);
				(c + pixel) != 0 ? (bkg_color += (0xf << 24)) : bkg_color;
				index = y * 256 + x * 8 + l - fine_x;
				if(index >= 0)
					pixels[index] = bkg_color;
			}
		}
		next_tile = true;
		m = 0;
		increment_x();
	}
}

void NesPPU::drawSpriteLines(int scanline)
{
	int tile, row, tile_x, tile_y/*, pos, opt*/;
	uint8_t y, t, a, x, flip, priority, sprite_pixel;
	uint16_t sprite_pixel_help;
	for (int i = (sprite_count - 1); i >= 0; i--)
	{
		y = secondary_oam[i * 4 + 0];
		t = secondary_oam[i * 4 + 1];
		a = secondary_oam[i * 4 + 2];
		x = secondary_oam[i * 4 + 3];

		row = scanline - y;
		bool flip_horizontal = a & 0x40 ? 1 : 0;
		bool flip_vertical = a & 0x80 ? 1 : 0;
		flip = ((a & 0x80) << 1) + (a & 0x40);
		tile = sprite_table + t * 16;
		priority = (a & 0x20);
		sprite_pixel = 0;

		if (sprite_size == 8)
		{
			tile_x = ppu_read(tile + row);
			tile_y = ppu_read(tile + row + 8);
			sprite_pixel_help = interleave(tile_x, tile_y);
			for (int l = 0; l < 8; l++)
			{
				if (flip_horizontal && flip_vertical) {
					tile_x = ppu_read(tile + 7 - row);
					tile_y = ppu_read(tile + 7 - row + 8);
					sprite_pixel = (interleave(tile_x, tile_y) & (0x3 << (((l % 8)) * 2))) >> (((l % 8)) * 2);
				}
				else if (flip_vertical) {
					tile_x = ppu_read(tile + 7 - row);
					tile_y = ppu_read(tile + 7 - row + 8);
					sprite_pixel = (interleave(tile_x, tile_y) & (0x3 << ((7 - (l % 8)) * 2))) >> ((7 - (l % 8)) * 2);
				}
				else if (flip_horizontal) {
					sprite_pixel = (sprite_pixel_help & (0x3 << (((l % 8)) * 2))) >> (((l % 8)) * 2);
				}
				else {
					sprite_pixel = (sprite_pixel_help & (0x3 << ((7 - (l % 8)) * 2))) >> ((7 - (l % 8)) * 2);
				}
				WriteToPixelArray(sprite_pixel, a, y + 1, row, x, l, i);
			}
		}
		else if (sprite_size == 16)
		{
			tile = (t & 0x1) * 0x1000 + (t & 0xFE) * 0x10;
			tile_x = ppu_read(tile + row);
			tile_y = ppu_read(tile + row + 8);
			sprite_pixel_help = interleave(tile_x, tile_y);
			if (row < 8) 
			{
				for (int l = 0; l < 8; l++)
				{
					if (flip_horizontal && flip_vertical) {
						tile += 16;
						tile_x = ppu_read(tile + 7 - row);
						tile_y = ppu_read(tile + 7 - row + 8);
						sprite_pixel = (interleave(tile_x, tile_y) & (0x3 << ((((l % 8)) * 2)))) >> ((((l % 8)) * 2));
					}
					else if (flip_vertical) {
						tile += 16;
						tile_x = ppu_read(tile + 7 - row);
						tile_y = ppu_read(tile + 7 - row + 8);
						sprite_pixel = (interleave(tile_x, tile_y) & (0x3 << (((7 - (l % 8)) * 2)))) >> (((7 - (l % 8)) * 2));
					}
					else if (flip_horizontal) {
						sprite_pixel = (sprite_pixel_help & (0x3 << (((l % 8)) * 2))) >> (((l % 8)) * 2);
					}
					else {
						sprite_pixel = (sprite_pixel_help & (0x3 << ((7 - (l % 8)) * 2))) >> ((7 - (l % 8)) * 2);
					}

					WriteToPixelArray(sprite_pixel, a, y + 1, row, x, l, i);
				}
			}
			else 
			{
				row -= 8;
				tile = (t & 0x1) * 0x1000 + (t & 0xFE) * 0x10 + 16;
				tile_x = ppu_read(tile + row );
				tile_y = ppu_read(tile + row + 8);
				sprite_pixel_help = interleave(tile_x, tile_y);
				for (int l = 0; l < 8; l++)
				{
					if (flip_horizontal && flip_vertical) {
						tile -= 16;
						tile_x = ppu_read(tile + 7 - row);
						tile_y = ppu_read(tile + 7 - row + 8);
						sprite_pixel = (interleave(tile_x, tile_y) & (0x3 << (((l % 8)) * 2))) >> (((l % 8)) * 2);
					}
					else if (flip_vertical) {
						tile -= 16;
						tile_x = ppu_read(tile + 7 - row);
						tile_y = ppu_read(tile + 7 - row + 8);
						sprite_pixel = (interleave(tile_x, tile_y) & (0x3 << ((7 - (l % 8)) * 2))) >> ((7 - (l % 8)) * 2);
					}
					else if (flip_horizontal) {
						sprite_pixel = (sprite_pixel_help & (0x3 << (((l % 8)) * 2))) >> (((l % 8)) * 2);
					}
					else {
						sprite_pixel = (sprite_pixel_help & (0x3 << ((7 - (l % 8)) * 2))) >> ((7 - (l % 8)) * 2);
					}

					WriteToPixelArray(sprite_pixel, a, y + 1, row + 8, x, l, i);
				}
			}
		}
	}
}

void NesPPU::WriteToPixelArray(uint8_t sprite_pixel, int a, int y, int row, int x, int l, int i)
{
	if (sprite_pixel != 0) {
		uint8_t background_channel = pixels[(y + row) * 256 + (x + l)] >> 24;
		if (background_channel == 0) { pixels[(y + row) * 256 + (x + l)] = pallete[ppu_read(0x3f10 + (a & 0x3) * 4 + sprite_pixel)]; }
		else if (background_channel != 0) {
			if ((!(sprite_0_hit)) && (sprite_0_present) && (i == 0) && ((x + l) != 255)) { sprite_hit_cycle = x + l; }
			if ((a & 0x20) == 0) { pixels[(y + row) * 256 + (x + l)] = pallete[ppu_read(0x3f10 + (a & 0x3) * 4 + sprite_pixel)]; }
		}
	}
}
 
