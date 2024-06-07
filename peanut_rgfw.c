#define ENABLE_SOUND 0
#define ENABLE_LCD 1

/* Import emulator library. */
#include "peanut_gb.h"

#define RGFW_IMPLEMENTATION
#define RGFW_BUFFER
#include "RGFW.h"

struct priv_t
{
	/* Pointer to allocated memory holding GB file. */
	uint8_t *rom;
	/* Pointer to allocated memory holding save file. */
	uint8_t *cart_ram;

	/* RGFW window*/
	RGFW_window* win;
	/* width of the screen (and width of draw buffer)*/
	u32 screenWidth;
};

/**
 * Returns a byte from the ROM file at the given address.
 */
uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_t * const p = gb->direct.priv;
	return p->rom[addr];
}

/**
 * Returns a byte from the cartridge RAM at the given address.
 */
uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_t * const p = gb->direct.priv;
	return p->cart_ram[addr];
}

/**
 * Writes a given byte to the cartridge RAM at the given address.
 */
void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr,
		       const uint8_t val)
{
	const struct priv_t * const p = gb->direct.priv;
	p->cart_ram[addr] = val;
}

/**
 * Returns a pointer to the allocated space containing the ROM. Must be freed.
 */
uint8_t *read_rom_to_ram(const char *file_name)
{
	FILE *rom_file = fopen(file_name, "rb");
	size_t rom_size;
	uint8_t *rom = NULL;

	if(rom_file == NULL)
		return NULL;

	fseek(rom_file, 0, SEEK_END);
	rom_size = ftell(rom_file);
	rewind(rom_file);
	rom = malloc(rom_size);

	if(fread(rom, sizeof(uint8_t), rom_size, rom_file) != rom_size)
	{
		free(rom);
		fclose(rom_file);
		return NULL;
	}

	fclose(rom_file);
	return rom;
}

/**
 * Ignore all errors.
 */
void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t val)
{
	const char* gb_err_str[GB_INVALID_MAX] = {
		"UNKNOWN",
		"INVALID OPCODE",
		"INVALID READ",
		"INVALID WRITE",
		"HALT FOREVER"
	};
	struct priv_t *priv = gb->direct.priv;

	fprintf(stderr, "Error %d occurred: %s at %04X\n. Exiting.\n",
			gb_err, gb_err_str[gb_err], val);

	/* Free memory and then exit. */
	free(priv->cart_ram);
	free(priv->rom);
	exit(EXIT_FAILURE);
}

#if ENABLE_LCD
/**
 * Draws scanline into framebuffer.
 */
void lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160],
		   const uint_fast8_t line)
{
	struct priv_t *priv = gb->direct.priv;
	const uint32_t palette[] = { 0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000 };

	for(unsigned int x = 0; x < priv->win->r.w; x++) {
		((uint32_t*)priv->win->buffer)[ (priv->screenWidth * line) + x] = palette[pixels[x] & 3];
	}
}
#endif

int main(int argc, char **argv)
{
	/* Must be freed */
	char *rom_file_name = NULL;
	static struct gb_s gb;
	static struct priv_t priv;
	enum gb_init_error_e ret;

	priv.win = RGFW_createWindow("RGFW Peanut-gb", RGFW_RECT(0, 0, LCD_WIDTH, LCD_HEIGHT), RGFW_CENTER | RGFW_NO_RESIZE);

	priv.screenWidth = RGFW_getScreenSize().w;


	switch(argc)
	{
	case 2:
		rom_file_name = argv[1];
		break;

	default:
		fprintf(stderr, "%s ROM\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Copy input ROM file to allocated memory. */
	if((priv.rom = read_rom_to_ram(rom_file_name)) == NULL)
	{
		printf("%d: %s\n", __LINE__, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Initialise context. */
	ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read,
		      &gb_cart_ram_write, &gb_error, &priv);

	if(ret != GB_INIT_NO_ERROR)
	{
		printf("Error: %d\n", ret);
		exit(EXIT_FAILURE);
	}

	priv.cart_ram = malloc(gb_get_save_size(&gb));

#if ENABLE_LCD
	gb_init_lcd(&gb, &lcd_draw_line);
	// gb.direct.interlace = 1;
#endif

	priv.win->fpsCap = 60;

	while(RGFW_window_shouldClose(priv.win) == 0) {
		if (RGFW_window_checkEvent(priv.win)) {
			switch(priv.win->event.type) {
				case RGFW_keyReleased:
				case RGFW_keyPressed:

					gb.direct.joypad_bits.a	= !RGFW_isPressedI(priv.win, RGFW_z);
					gb.direct.joypad_bits.b	= !RGFW_isPressedI(priv.win, RGFW_x);
					gb.direct.joypad_bits.select = !RGFW_isPressedI(priv.win, RGFW_BackSpace);
					gb.direct.joypad_bits.start	= !RGFW_isPressedI(priv.win, RGFW_Return);
					gb.direct.joypad_bits.right	= !RGFW_isPressedI(priv.win, RGFW_Right);
					gb.direct.joypad_bits.left	= !RGFW_isPressedI(priv.win, RGFW_Left);
					gb.direct.joypad_bits.up	= !RGFW_isPressedI(priv.win, RGFW_Up);
					gb.direct.joypad_bits.down	= !RGFW_isPressedI(priv.win, RGFW_Down);

					switch(priv.win->event.keyCode) {
							case RGFW_r:
								gb_reset(&gb);
								break;
		#if ENABLE_LCD

						case RGFW_i:
							gb.direct.interlace = ~gb.direct.interlace;
							break;

						case RGFW_o:
							gb.direct.frame_skip = ~gb.direct.frame_skip;
							break;
		#endif

						break;
				}
			}
		}

		/* Execute CPU cycles until the screen has to be redrawn. */
		gb_run_frame(&gb);

		RGFW_window_swapBuffers(priv.win);
	}

	RGFW_window_close(priv.win);
	free(priv.cart_ram);
	free(priv.rom);

	return EXIT_SUCCESS;
}
