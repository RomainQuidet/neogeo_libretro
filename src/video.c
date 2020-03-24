#include "cartridge.h"
#include "video.h"
#include "endian.h"
#include "log.h"
#include "memory_mapping.h"
#include "neogeo.h"
#include "timers_group.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>


const uint8_t X_SHRINK_TABLE[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0,
	0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0,
	0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
	1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
	1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0,
	1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0,
	1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1,
	1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,
	1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const size_t PALETTES_COLORS_SIZE = (8 * 1024);	// 8KB palettes RAM bank equivalent with converted RGB colors
const size_t VRAM_SIZE = (68*1024);
static const size_t PALETTE_COLOR_NBR =	16;
static const size_t PALETTES_PER_BANK = 256;
static const size_t VRAM_SPRITES_LIST_SIZE = 0x80;

// VRAM MAPPING - https://wiki.neogeodev.org/index.php?title=Sprites
//
#define VRAM_SCB1_START			0x0000		// Tilemaps
#define VRAM_SCB1_END			0x6FFF
#define VRAM_FIXMAP_START		0x7000
#define VRAM_FIXMAP_END			0x74FF
#define VRAM_EXTENSION_START	0x7500
#define VRAM_EXTENSION_END		0x7FFF
#define VRAM_SCB2_START			0x8000		// Shrinking coefficients
#define VRAM_SCB2_END			0x81FF
#define VRAM_SCB3_START			0x8200		// Vertical position
#define VRAM_SCB3_END			0x83FF
#define VRAM_SCB4_START			0x8400		// Horizontal position
#define VRAM_SCB4_END			0x85FF
#define VRAM_SPRITES_EVEN_START	0x8600
#define VRAM_SPRITES_EVEN_END	0x867F
#define VRAM_SPRITES_ODD_START	0x8680
#define VRAM_SPRITES_ODD_END	0x86FF
#define VRAM_UNUSED_START		0x8700
#define VRAM_UNUSED_END			0x87FF

#define SCB3_VERTICAL_SPRITE_SIZE_MASK		0x003F
#define SCB3_STICKY_BIT_MASK				0x0040
#define SCB3_Y_POSITION_MASK				0xFF80
#define SCB2_VERTICAL_SHRINK_MASK			0x00FF
#define SCB2_HORIZONTAL_SHRINK_MASK			0x0F00

video_t video;
uint16_t vram_address;
uint16_t vram_modulo;
uint16_t timer_reg_low;
uint16_t timer_reg_high;

static uint16_t read_vram(void);
static void write_vram(uint16_t data);

uint32_t sprite_x = 0;
uint32_t sprite_y = 0;
uint32_t sprite_zoomX = 15;
uint32_t sprite_zoomY = 255;
uint32_t sprite_clipping = 0x20;

uint16_t *_vram_data;

bool cartrigde_plugged_in = false;

static uint8_t debug_log_vram = 0;

#pragma mark - 68k Video Registers access

static uint16_t vram_read_word(uint32_t offset) {
	uint32_t address = IO_PORTS_START + offset;
	uint16_t res = 0;
	switch (address) {
		case REG_VRAMADDR:
		case REG_VRAMRW:
			res = read_vram();
//			LOG(LOG_DEBUG, "vram_read_word REG_VRAMRW from 0x%08X - 0x%04X\n", vram_address, res);
			break;
		case REG_VRAMMOD:
			res = vram_modulo;
//			LOG(LOG_DEBUG, "vram_read_word REG_VRAMMOD %u\n", vram_modulo);
			break;
		case REG_LSPCMODE:
		{
			res = 0;
			//$00F8-$00FF : Vertical sync - 8 px
			//$0100-$010F : Top border - 16 px
			//$0110-$01EF : Active display - 224 px
			//$01F0-$01FF : Bottom border - 16 px
			//TODO: update scanline to get real vertical sync line
			uint32_t line_counter = timer_group_get_current_y_scanline() + 0x100;
			uint32_t screen_freq = 1; // 1: 50Hz, 0: 60Hz
			res = line_counter << 7 | screen_freq << 3 | (video.auto_animation_counter & 0x07);
			LOG(LOG_DEBUG, "vram_read_word REG_LSPCMODE 0x%04X\n", res);
		}
			break;
		case REG_TIMERHIGH:
			res = (uint16_t)(video.timer_counter >> 16);
			break;
		case REG_TIMERLOW:
			res = (uint16_t)video.timer_counter;
			break;
		case REG_IRQACK:
			LOG(LOG_DEBUG, "vram_read_word needs to implement REG_IRQACK read\n");
			break;
		case REG_TIMERSTOP:
			//TODO Like lspc mode ?
			break;
		default:
			LOG(LOG_DEBUG, "vram_read_word unkown offset 0x%08X\n", address);
			break;
	}
	LOG(LOG_DEBUG, "vram_read_word at 0x%08X - 0x%04X\n", address, res);
	return res;
}

static uint8_t vram_read_byte(uint32_t offset) {
	return (uint8_t)vram_read_word(offset);
}

static uint32_t vram_read_dword(uint32_t offset) {
	LOG(LOG_ERROR, "vram_read_dword call ... on a 16bit bus?\n");
	return 0;
}

static void vram_write_word(uint32_t offset, uint16_t data) {
	uint32_t address = IO_PORTS_START + offset;
	switch (address) {
		case REG_VRAMADDR:
			assert(data <= VRAM_UNUSED_END);
			vram_address = data;
//			LOG(LOG_DEBUG, "vram_write_word REG_VRAMADDR - 0x%04X\n", data);
			break;
		case REG_VRAMRW:
			write_vram(data);
			break;
		case REG_VRAMMOD:
			vram_modulo = data;
//			LOG(LOG_DEBUG, "vram_write_word REG_VRAMMOD - 0x%04X\n", data);
			break;
		case REG_LSPCMODE:
			video.auto_animation_speed = data >> 8;
			video.auto_animation_disabled = (data & 0x0008) != 0;
			video.timer_control = (uint8_t)(data & 0x00F0);
			if (video.timer_control & TIMER_CTRL_IRQ_ENABLED_MASK) {
				LOG(LOG_DEBUG, "vram_write_word TIMER_CTRL_IRQ_ENABLED_MASK\n");
				timer_arm(timers.video_timer, pixelToMaster(video.timer_counter));
			}
			LOG(LOG_DEBUG, "vram_write_word REG_LSPCMODE - 0x%04X\n", data);
			break;
		case REG_TIMERHIGH:
			timer_reg_high = data;
			break;
		case REG_TIMERLOW:
			timer_reg_low = data;
			if (video.timer_control & TIMER_CTRL_RELOAD_LOW_WRITE_MASK) {
				video_reload_timer();
			}
			break;
		case REG_IRQACK:
			LOG(LOG_DEBUG, "REG_IRQACK %u\n", data);
			cpu_68k_ack_interrupt(data & 0x07);
			break;
		case REG_TIMERSTOP:
			//TODO
			break;
		default:
			LOG(LOG_ERROR, "vram_write_word unknown address 0x%08X - 0x%04X from offset 0x%08X\n", address, data, offset);
			break;
	}
}

static void vram_write_byte(uint32_t offset, uint8_t data) {
	LOG(LOG_DEBUG, "vram_write_byte call ... on a 16bit bus?\n");
	vram_write_word(offset, data);
}

static void vram_write_dword(uint32_t offset, uint32_t data) {
	vram_write_word(offset, data >> 16);
	vram_write_word(offset + 2, data);
}

#pragma mark - Public
#pragma mark Lifecycle

void video_init(void) {
	video.palettes_colors = malloc(PALETTES_COLORS_SIZE);
	size_t frame_buffer_size = FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * sizeof(uint16_t);
	video.frameBuffer = malloc(frame_buffer_size);
	memset(video.frameBuffer, 0, frame_buffer_size);
	memset(&(video.vram), 0, sizeof(memory_region_t));
	video.vram.data = malloc(VRAM_SIZE);
	_vram_data = (uint16_t *)video.vram.data;
	video.vram.size = VRAM_SIZE;
	video.vram.handlers.read_byte = &vram_read_byte;
	video.vram.handlers.read_word = &vram_read_word;
	video.vram.handlers.read_dword = &vram_read_dword;
	video.vram.handlers.write_byte = &vram_write_byte;
	video.vram.handlers.write_word = &vram_write_word;
	video.vram.handlers.write_dword = &vram_write_dword;
}

void video_reset(void) {
	video.auto_animation_speed = 0;
	video.auto_animation_counter = 0;
	video.auto_animation_disabled = false;
	
	memset(_vram_data, 0, VRAM_SIZE);
	vram_address = 0;
	vram_modulo = 0;
	
	sprite_x = 0;
	sprite_y = 0;
	sprite_zoomX = 0x0F;
	sprite_zoomY = 0xFF;
	sprite_clipping = 32;
	
	cartrigde_plugged_in = cartridge_plugged_in();
}

uint32_t video_reload_timer(void) {
	video.timer_counter = ((uint32_t)timer_reg_high << 16) + (uint32_t)timer_reg_low;
	return video.timer_counter;
}

#pragma mark Palette converter

void video_convert_current_palette_bank(void)
{
    for (uint32_t index = 0; index < PALETTE_COLOR_NBR * PALETTES_PER_BANK; index++) {
        video_convert_current_palette_color(index);
    }
}

/*
 Plalettes colors :
 Bit 	15 			14 	13 	12 	11 	10 	9 	8 	7 	6 	5 	4 	3 	2 	1 	0
 Def 	Dark bit 	R0	G0	B0	R4	R3	R2	R1	G4	G3	G2	G1	B4	B3	B2	B1
 
 retro colors : RGB565
 Bit 	15 	14 	13 	12 	11 	10 	9 	8 	7 	6 	5 	4 	3 	2 	1 	0
 Def 	R4 	R3	R2	R1	R0	G5	G4	G3	G2	G1	G0	B4	B3	B2	B1	B0
 RGB565 G0 will always be 0 as we have less definition
 */
void video_convert_current_palette_color(uint32_t index) {
	uint16_t c = current_palette_ram->handlers.read_word(index*2);
	video.palettes_colors[index] = ((c & 0x0F00) << 4) | ((c & 0x4000) >> 3) |
									((c & 0x00F0) << 3) | ((c & 0x2000) >> 7) |
									((c & 0x000F) << 1) | ((c & 0x1000) >> 12);
    //TODO: b15 as dark bit
//	LOG(LOG_DEBUG, "video_convert_current_palette_color #%i 0x%04X - 0x%04X\n", index, c, video.palettes_colors[index]);
}

#pragma mark Sprites

static inline bool isSpriteOnScanline(uint32_t scanline, uint32_t y, uint32_t clipping)
{
	return (clipping != 0) && ((clipping >= 0x20) || ((scanline - y) & 0x1ff) < (clipping * 0x10));
}

void video_create_sprites_list(uint32_t scanline) {
	uint16_t* vertical_attributes_p = _vram_data + VRAM_SCB3_START;
	uint16_t activeCount = 0;
	uint16_t attributes;
	uint32_t y = 0;
	uint32_t clipping = 32;
	bool spriteIsOnScanline = false;
	
	uint16_t *spriteList;
	if (scanline & 1) {
		spriteList = _vram_data + VRAM_SPRITES_ODD_START;
	}
	else {
		spriteList = _vram_data + VRAM_SPRITES_EVEN_START;
	}
	memset(spriteList, 0, sizeof(uint16_t) * VRAM_SPRITES_LIST_SIZE);
	
	for (uint16_t spriteNumber = 0; spriteNumber < MAX_SPRITES_PER_SCREEN; ++spriteNumber)
	{
		attributes = *vertical_attributes_p++;
		
		if (!(attributes & SCB3_STICKY_BIT_MASK))
		{
			y = (496 - (attributes >> 7)) + 16;
			clipping = attributes & 0x3F;
			spriteIsOnScanline = isSpriteOnScanline(scanline, y, clipping);
		}
		
		if (!spriteIsOnScanline)
		continue;
		
		*spriteList++ = spriteNumber;
		activeCount++;
		
		if (activeCount >= MAX_SPRITES_PER_LINE)
		break;
	}
	
	if (scanline == 112 && debug_log_vram) {
		LOG(LOG_DEBUG, "video_create_sprites_list: %d sprites on scanline %d - ", activeCount, scanline);
		spriteList -= activeCount;
		for (int i = 0; i < activeCount; i++) {
			LOG(LOG_DEBUG, "#%u ", spriteList[i]);
		}
		LOG(LOG_DEBUG, "\n");
	}
}

static inline void draw_sprite_line(uint32_t zoomX, int increment, uint8_t *pixels_base,
									const uint16_t* paletteBase, uint16_t* frameBuffer_p)
{
	uint64_t pixels_pair = *(uint64_t *)pixels_base;
	uint8_t color_index = 0;
	uint16_t shrinkX_table_index = zoomX * 16;
	for (int i = 0; i < 16; ++i)
	{
		if (X_SHRINK_TABLE[shrinkX_table_index + i])
		{
			color_index = (pixels_pair >> (4 * i)) & 0x0F;
			if (color_index)
			{
				*frameBuffer_p = paletteBase[color_index];
			}
			frameBuffer_p += increment;
		}
	}
}

static inline void draw_sprite_line_clipped(uint32_t zoomX, int increment, uint8_t *pixels_base, const uint16_t* paletteBase,
											uint16_t* frameBuffer_p, const uint16_t* low, const uint16_t* high)
{
	uint64_t pixels_pair = *(uint64_t *)pixels_base;
	uint8_t color_index = 0;
	uint16_t shrinkX_table_index = zoomX * 16;
	for (int i = 0; i < 16; ++i)
	{
		if (X_SHRINK_TABLE[shrinkX_table_index + i])
		{
			color_index = (pixels_pair >> (4 * i)) & 0x0F;
			if (color_index && (frameBuffer_p >= low) && (frameBuffer_p < high))
			{
				*frameBuffer_p = paletteBase[color_index];
			}
			frameBuffer_p += increment;
		}
	}
}

void video_draw_sprite(uint32_t spriteNumber, uint32_t x, uint32_t y, uint32_t zoomX, uint32_t zoomY, uint32_t scanline, uint32_t clipping)
{
	uint32_t spriteLine = (scanline - y) & 0x1FF;
	uint32_t zoomLine = spriteLine & 0xFF;
	bool invert = (spriteLine & 0x100) != 0;
	bool clipped = false;
	
	uint32_t x_right = (x + zoomX + 1) & 0x1FF;
	uint32_t x_left = (x & 0x1FF);
	
	if (!(x_left < FRAMEBUFFER_WIDTH) && !(x_right < FRAMEBUFFER_WIDTH))
		return;
	
	if (!(x_left < FRAMEBUFFER_WIDTH) || !(x_right < FRAMEBUFFER_WIDTH))
		clipped = true;
	
	if (invert)
		zoomLine ^= 0xFF;
	
	if (clipping > 0x20)
	{
		zoomLine = zoomLine % ((zoomY + 1) << 1);
		
		if (zoomLine > zoomY)
		{
			zoomLine = ((zoomY + 1) << 1) - 1 - zoomLine;
			invert = !invert;
		}
	}
	
	uint32_t tileNumber = system_y_zoom_rom.data[zoomY * 256 + zoomLine];
	uint32_t tileLine = tileNumber & 0xF;
	tileNumber >>= 4;
	
	if (invert)
	{
		tileLine ^= 0x0f;
		tileNumber ^= 0x1f;
	}
	
	uint32_t tileIndex = _vram_data[spriteNumber * 64 + tileNumber * 2];
	uint32_t tileControl = _vram_data[spriteNumber * 64 + tileNumber * 2 + 1];
	tileIndex += (tileControl & 0x00F0) << 12;

	if (tileControl & 2)
	tileLine ^= 0x0F;

	if (video.auto_animation_disabled == false)
	{
		if (tileControl & 0x0008)
			tileIndex = (tileIndex & ~0x07) | (video.auto_animation_counter & 0x07);
		else if (tileControl & 0x0004)
			tileIndex = (tileIndex & ~0x03) | (video.auto_animation_counter & 0x03);
	}
	
//	if (spriteNumber == 253) {
//		LOG(LOG_DEBUG, "video_draw_sprite scanline %u, spriteLine %u, tileNumber %u, tileLine %u, tileIndex %04X, tileControl %04X\n", scanline, spriteLine, tileNumber, tileLine, tileIndex, tileControl);
//	}

	uint16_t* frameBufferPtr = video.frameBuffer;

	frameBufferPtr += x;

	if (x > 0x1F0)
	frameBufferPtr -= 0x200;

	frameBufferPtr -= 0;//Video::LEFT_BORDER;

	frameBufferPtr += (scanline - 16) * FRAMEBUFFER_WIDTH;

	int increment = 1;

	if (tileControl & 1)
	{
		frameBufferPtr += zoomX;
		increment = -1;
	}

	const uint16_t* paletteBase = video.palettes_colors + ((tileControl >> 8) * PALETTE_COLOR_NBR);
	uint32_t pixels_offset = (tileIndex * CHARACTER_TILE_BYTES) + (tileLine * 8);
	assert(pixels_offset < serialized_c_roms.size);
	uint8_t *pixels_base = serialized_c_roms.data + pixels_offset;

	if (clipped)
	{
		draw_sprite_line_clipped(
							  zoomX,
							  increment,
							  pixels_base,
							  paletteBase,
							  frameBufferPtr,
							  video.frameBuffer + ((scanline - 16) * FRAMEBUFFER_WIDTH),
							  video.frameBuffer + ((scanline - 15) * FRAMEBUFFER_WIDTH));
	}
	else
		draw_sprite_line(zoomX, increment, pixels_base, paletteBase, frameBufferPtr);
}

void video_draw_sprites(uint32_t scanline)
{
	if (cartrigde_plugged_in == false) {
		return;
	}
	
	uint16_t *spriteList;
	if (scanline & 1) {
		spriteList = _vram_data + VRAM_SPRITES_ODD_START;
	}
	else {
		spriteList = _vram_data + VRAM_SPRITES_EVEN_START;
	}
	
	for (uint16_t currentSprite = 0; currentSprite < VRAM_SPRITES_LIST_SIZE; currentSprite++)
	{
		uint16_t spriteNumber = *spriteList++;
		if (!spriteNumber)
			break;
		
		uint16_t sprite_shrink_coefs = _vram_data[VRAM_SCB2_START + spriteNumber];
		uint16_t sprite_vertical_pos = _vram_data[VRAM_SCB3_START + spriteNumber];
		
		if (sprite_vertical_pos & SCB3_STICKY_BIT_MASK)
		{
			sprite_x = (sprite_x + sprite_zoomX + 1) & 0x1FF;
			sprite_zoomX = (sprite_shrink_coefs >> 8) & 0xF;
		}
		else
		{
			uint16_t sprite_horizontal_pos = _vram_data[VRAM_SCB4_START + spriteNumber];
			sprite_zoomY = sprite_shrink_coefs & SCB2_VERTICAL_SHRINK_MASK;
			sprite_zoomX = (sprite_shrink_coefs & SCB2_HORIZONTAL_SHRINK_MASK) >> 8;
			sprite_clipping = sprite_vertical_pos & SCB3_VERTICAL_SPRITE_SIZE_MASK;
			sprite_y = 496 - (sprite_vertical_pos >> 7) + 16;
			sprite_x = sprite_horizontal_pos >> 7;
		}
		
//		if (scanline == 100) {
//			LOG(LOG_DEBUG, "video_draw_sprite: #%u - x %u, y %u, zx %01X, zy %02X, %u tiles %s\n", spriteNumber, sprite_x, sprite_y, sprite_zoomX, sprite_zoomY, sprite_clipping, (sprite_vertical_pos & SCB3_STICKY_BIT_MASK) ? "(sticky)" : "");
//		}
		
		video_draw_sprite(spriteNumber, sprite_x, sprite_y, sprite_zoomX, sprite_zoomY, scanline, sprite_clipping);
	}
}

#pragma mark - Fix layer

//static const uint16_t FIX_TILE_PIXELS_WIDTH = 8;
static const uint16_t FIX_TILE_PIXELS_HEIGHT = 8;
static const uint16_t FIX_TILES_PER_LINE = 40;
static const uint16_t FIX_TILES_PER_COLUMN = 32;
static const uint16_t FIX_ROM_BYTES_PER_TILE = 32;		// 8*8 pixels * 4 bits = 32 bytes per tile

/********* FIX IN ROM FORMAT **********
 Pixels are coded by pairs as bytes, in columns from top to bottom.
 4 bits = 0..15 palette color index
 Left pixel is bits 0~3, right pixel is bits 4~7
 Bytes are in oddly mixed columns format (Graph shows byte index in hex)
 
 		10 - 18 - 00 - 08
 		11 - 19 - 01 - 09
 		12 - 1A - 02 - 0A
  		13 - 1B - 03 - 0B
  		14 - 1C - 04 - 0C
  		15 - 1D - 05 - 0D
  		16 - 1E - 06 - 0E
  		17 - 1F - 07 - 0F
 
 So to find our scanline to draw, we need to advance for tile address (scanline % 8) bytes
 */
static const uint8_t fix_framebuffer_offset_mapping[4] = {0x10, 0x18, 0x00, 0x08};

// Note: scanline between 16 and 240!
void video_draw_fix(uint32_t scanline) {
	uint16_t* videoRamPtr = _vram_data + VRAM_FIXMAP_START;
	videoRamPtr += (scanline / FIX_TILE_PIXELS_HEIGHT);
	uint16_t* frameBufferPtr = video.frameBuffer + ((scanline - 16) * FRAMEBUFFER_WIDTH);
	
	for (uint8_t fix_column_index = 0; fix_column_index < FIX_TILES_PER_LINE; fix_column_index++)
	{
		uint16_t fix = *videoRamPtr;
		uint16_t palette_number = (fix & 0xF000) >> 12;
		uint16_t tile_number = fix & 0x0FFF;
		assert((tile_number + 1) * FIX_ROM_BYTES_PER_TILE <= current_fix_rom->size);
		
//		if (0) {	//TODO
//			// Do not display transparent tiles
//			frameBufferPtr += FIX_TILE_PIXELS_WIDTH;
//			continue;
//		}
		
		uint8_t* fixBase = current_fix_rom->data + ((tile_number * FIX_ROM_BYTES_PER_TILE) + (scanline % FIX_TILE_PIXELS_HEIGHT));
		uint16_t* colorsBase = video.palettes_colors + (palette_number * PALETTE_COLOR_NBR);
		
		for (uint8_t index = 0; index < 4; index++) {
			uint8_t offset = fix_framebuffer_offset_mapping[index];
			uint8_t pixel_pair = fixBase[offset];
			uint8_t pixel_left_color_index = pixel_pair & 0x0F;
			uint8_t pixel_right_color_index = pixel_pair >> 4;
			
			if (pixel_left_color_index) {
				*frameBufferPtr = colorsBase[pixel_left_color_index];
			}
			frameBufferPtr++;
			
			if (pixel_right_color_index) {
				*frameBufferPtr = colorsBase[pixel_right_color_index];
			}
			frameBufferPtr++;
		}
		
		videoRamPtr += FIX_TILES_PER_COLUMN;
	}
}

#pragma mark - Background layer

static const uint16_t BACK_DROP_COLOR_INDEX = 256 * PALETTE_COLOR_NBR - 1;

void video_draw_empty_line(uint32_t scanline) {
	uint16_t* ptr = video.frameBuffer + ((scanline - 16) * FRAMEBUFFER_WIDTH);
	uint16_t color = video.palettes_colors[BACK_DROP_COLOR_INDEX];
	
//	LOG(LOG_DEBUG, "video_draw_empty_line %d - color 0x%04X - index %d\n", scanline, color, BACK_DROP_COLOR_INDEX);
	
	for (uint16_t pixel = 0; pixel < FRAMEBUFFER_WIDTH; pixel++) {
		ptr[pixel] = color;
	}
}

#pragma mark - Private

static uint16_t read_vram() {
	assert(vram_address <= VRAM_UNUSED_END);
	return _vram_data[vram_address];
}

static void write_vram(uint16_t data) {
	
	if (debug_log_vram) {
//		LOG(LOG_DEBUG, "write_vram at 0x%08X - 0x%04X\n", vram_address, data);
		//FIX
		if (vram_address >= VRAM_FIXMAP_START && vram_address <= VRAM_FIXMAP_END) {
			LOG(LOG_DEBUG, "write_vram VRAM_FIXMAP %08X - %04X ", vram_address, data);
			if ((data & 0x00FF) >= 0x20 && (data & 0x00FF) <= 0x7D) {
				LOG(LOG_DEBUG, "\t%c\n", (data & 0x00FF));
			}
			else {
				LOG(LOG_DEBUG, "\n", (data & 0x00FF));
			}
		}
		if (vram_address >= VRAM_EXTENSION_START && vram_address <= VRAM_EXTENSION_START) {
			LOG(LOG_DEBUG, "write_vram VRAM_EXTENSION 0x%08X - 0x%04X\n", vram_address, data);
		}
		if (vram_address >= VRAM_SCB1_START && vram_address <= VRAM_SCB1_END) {
			LOG(LOG_DEBUG, "write_vram VRAM_SCB1 sprite #%d - 0x%04X ", (vram_address - VRAM_SCB1_START) / 64, data);
			if (vram_address & 1) {
				uint32_t tileNumber = _vram_data[vram_address-1] + ((data & 0x00F0) << 12);
				LOG(LOG_DEBUG, " (tile #%u)\n", tileNumber);
			}
			else {
				LOG(LOG_DEBUG, " \n");
			}
		}
		if (vram_address >= VRAM_SCB2_START && vram_address <= VRAM_SCB2_END) {
			LOG(LOG_DEBUG, "write_vram VRAM_SCB2 sprite #%d - 0x%04X\n", vram_address - VRAM_SCB2_START, data);
		}
		if (vram_address >= VRAM_SCB3_START && vram_address <= VRAM_SCB3_END) {
			uint32_t y = 496 - ((data & SCB3_Y_POSITION_MASK) >> 7);
			uint32_t size = data & 0x003F;
			LOG(LOG_DEBUG, "write_vram VRAM_SCB3 sprite #%d - 0x%04X (y %d, size %u)\n", (vram_address - VRAM_SCB3_START) / 2, data, y, size);
		}
		if (vram_address >= VRAM_SCB4_START && vram_address <= VRAM_SCB4_END) {
			LOG(LOG_DEBUG, "write_vram VRAM_SCB4 sprite #%d - 0x%04X (x %u)\n", vram_address - VRAM_SCB4_START, data, data >> 7);
		}
		if (vram_address >= VRAM_SPRITES_EVEN_START && vram_address <= VRAM_SPRITES_EVEN_END) {
			LOG(LOG_DEBUG, "write_vram VRAM_SPRITES_EVEN 0x%08X - 0x%04X\n", vram_address, data);
		}
		if (vram_address >= VRAM_SPRITES_ODD_START && vram_address <= VRAM_SPRITES_ODD_END) {
			LOG(LOG_DEBUG, "write_vram VRAM_SPRITES_ODD 0x%08X - 0x%04X\n", vram_address, data);
		}
		if (vram_address > VRAM_UNUSED_START) {
			LOG(LOG_DEBUG, "write_vram VRAM_UNUSED_START 0x%08X - 0x%04X\n", vram_address, data);
		}
	}
	if ((vram_address >= VRAM_SPRITES_EVEN_START && vram_address <= VRAM_SPRITES_ODD_END)
		|| vram_address > VRAM_UNUSED_END) {
		return;
	}
	_vram_data[vram_address] = data;
	vram_address += vram_modulo;
}
