/*
 * Crack_plot.h
 *
 *  Created on: Jun 26, 2013
 *      Author: Marc Neveu (mneveu@asu.edu)
 */

#ifndef CRACK_PLOT_H_
#define CRACK_PLOT_H_

#include "../Graphics/Plot.h"

int Crack_plot (char path[1024], int NR, int NT, float timestep, float r_p, thermalout **thoutput, int warnings, int msgout);

int Crack_plot (char path[1024], int NR, int NT, float timestep, float r_p, thermalout **thoutput, int warnings, int msgout) {

	float total_time = NT*timestep;
	int r = 0;
	int t = 0;

//-------------------------------------------------------------------
//                       Read output from Crack
//-------------------------------------------------------------------

	// Read the Crack file
	float **Crack = (float**) malloc(NR*sizeof(float*));       // Crack[NR][NT], cracked zone
	if (Crack == NULL) printf("Crack: Not enough memory to create Crack[NR][NT]\n");
	for (r=0;r<NR;r++) {
		Crack[r] = (float*) malloc(NT*sizeof(float));
		if (Crack[r] == NULL) printf("Crack: Not enough memory to create Crack[NR][NT]\n");
	}
	Crack = read_input (NT, NR, Crack, path, "Crack/Crack.txt");

	// Read the W/R file
	float **WRratio = (float**) malloc(NT*sizeof(float*));             // WRratio[NT][2]
	if (WRratio == NULL) printf("Crack: Not enough memory to create WRratio[NT][2]\n");
	for (t=0;t<NT;t++) {
		WRratio[t] = (float*) malloc(2*sizeof(float));
		if (WRratio[t] == NULL) printf("Crack: Not enough memory to create WRratio[NT][2]\n");
	}
	WRratio = read_input (2, NT, WRratio, path, "Crack/WR_ratio.txt");

//-------------------------------------------------------------------
//           Launch Sample DirectMedia Layer (SDL) display
//-------------------------------------------------------------------

	int quit = 0;

	if (SDL_Init(SDL_INIT_EVERYTHING) == -1){
		printf("Crack_plot: Error: SDL not initialized.");
	}
	window = SDL_CreateWindow("IcyDwarf", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == NULL){
		printf("Crack_plot: Error: Window not created.");
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED
		| SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL){
		printf("Crack_plot: Error: Renderer not created.");
	}

//-------------------------------------------------------------------
//                           Window icon
//-------------------------------------------------------------------

	char *IcyDwarfIcon_icns = (char*)malloc(1024);           // Don't forget to free!
	IcyDwarfIcon_icns[0] = '\0';
	if (release == 1) strncat(IcyDwarfIcon_icns,path,strlen(path)-16);
	strcat(IcyDwarfIcon_icns,"Graphics/CeresDanWiersemaAtIconbug.icns");
	SDL_Surface* IcyDwarfIcon = IMG_Load(IcyDwarfIcon_icns);
	if (IcyDwarfIcon == NULL) printf("IcyDwarf: Plot: Window icon not loaded.\n");
	free(IcyDwarfIcon_icns);
	SDL_SetWindowIcon(window, IcyDwarfIcon);

//-------------------------------------------------------------------
//                     Initialize display elements
//-------------------------------------------------------------------

	SDL_Texture* background_tex = NULL;
	SDL_Texture* crack_time_tex = NULL;
	SDL_Surface* crack_time = NULL;
	SDL_Texture* WR_tex = NULL;
	SDL_Surface* WR = NULL;
	SDL_Texture* WR_bar_tex = NULL;
	SDL_Surface* WR_bar = NULL;
	SDL_Texture* progress_bar_tex = NULL;
	SDL_Surface* progress_bar = NULL;
	SDL_Texture* cracked_rock_tex = NULL;
	SDL_Texture* numbers_tex_1 = NULL;
	SDL_Texture* numbers_tex_2 = NULL;
	SDL_Texture* numbers_tex_3 = NULL;
	SDL_Texture* numbers_tex_4 = NULL;
	SDL_Texture* elapsed_digit_1 = NULL;
	SDL_Texture* elapsed_digit_2 = NULL;
	SDL_Texture* elapsed_digit_3 = NULL;
	SDL_Texture* elapsed_percent_1 = NULL;
	SDL_Texture* elapsed_percent_2 = NULL;
	SDL_Texture* elapsed_percent_3 = NULL;
	SDL_Texture* surface_digit_1 = NULL;
	SDL_Texture* surface_digit_2 = NULL;
	SDL_Texture* surface_digit_3 = NULL;
	SDL_Texture* surface_digit_4 = NULL;
	SDL_Texture* seafloor_digit_1 = NULL;
	SDL_Texture* seafloor_digit_2 = NULL;
	SDL_Texture* seafloor_digit_3 = NULL;
	SDL_Texture* cracked_depth_digit_1 = NULL;
	SDL_Texture* cracked_depth_digit_2 = NULL;
	SDL_Texture* cracked_depth_digit_3 = NULL;
	SDL_Texture* max_ratio_digit_1 = NULL;
	SDL_Texture* max_ratio_digit_2 = NULL;
	SDL_Texture* max_ratio_digit_3 = NULL;
	SDL_Texture* max_depth_digit_1 = NULL;
	SDL_Texture* max_depth_digit_2 = NULL;
	SDL_Texture* max_depth_digit_3 = NULL;
	SDL_Surface* numbers = NULL;

	char *TextureBackground_png = (char*)malloc(1024);           // Don't forget to free!
	TextureBackground_png[0] = '\0';
	if (release == 1) strncat(TextureBackground_png,path,strlen(path)-16);
	strcat(TextureBackground_png,"Graphics/TextureBackground.png");
	background_tex = LoadImage(TextureBackground_png);
	if (background_tex == NULL) printf("IcyDwarf: Plot: Background image not loaded.\n");
	free(TextureBackground_png);

	char *Transparent_png = (char*)malloc(1024);           // Don't forget to free!
	Transparent_png[0] = '\0';
	if (release == 1) strncat(Transparent_png,path,strlen(path)-16);
	strcat(Transparent_png,"Graphics/Transparent.png");
	crack_time = IMG_Load(Transparent_png);
	if (crack_time == NULL) printf("IcyDwarf: Plot: crack_time layer not loaded.\n");
	WR = IMG_Load(Transparent_png);
	if (WR == NULL) printf("IcyDwarf: Plot: WR layer not loaded.\n");
	WR_bar = IMG_Load(Transparent_png);
	if (WR_bar == NULL) printf("IcyDwarf: Plot: WR bar layer not loaded.\n");
	progress_bar = IMG_Load(Transparent_png);
	if (progress_bar == NULL) printf("IcyDwarf: Plot: Progress bar layer not loaded.\n");
	free(Transparent_png);

	char *Numbers_png = (char*)malloc(1024);           // Don't forget to free!
	Numbers_png[0] = '\0';
	if (release == 1) strncat(Numbers_png,path,strlen(path)-16);
	strcat(Numbers_png,"Graphics/Numbers.png");
	numbers = IMG_Load(Numbers_png);
	if (crack_time == NULL) printf("IcyDwarf: Plot: numbers layer not loaded.\n");
	free(Numbers_png);

	char *TextureCracked_png = (char*)malloc(1024);           // Don't forget to free!
	TextureCracked_png[0] = '\0';
	if (release == 1) strncat(TextureCracked_png,path,strlen(path)-16);
	strcat(TextureCracked_png,"Graphics/TextureCracked.png");
	cracked_rock_tex = LoadImage(TextureCracked_png);
	if (cracked_rock_tex == NULL) printf("IcyDwarf: Plot: Cracked texture not loaded.\n");
	free(TextureCracked_png);

	// Don't forget to destroy all window, renderers, and textures at the end.

//-------------------------------------------------------------------
//                Set static elements using crack output
//-------------------------------------------------------------------

	// DEPTH VS. TIME PLOT

	Uint32 white_alpha;
	Uint32 red_alpha;
	Uint32 blue_alpha;
	Uint32 orange_alpha;
	Uint32 purple_alpha;
	Uint32 yellow_alpha;
	Uint32 light_green_alpha;
	Uint32 green_alpha;
	white_alpha = SDL_MapRGBA(crack_time->format, 255, 255, 255, 200); // r,g,b,alpha 0 to 255. Alpha of 0 is transparent
	red_alpha = SDL_MapRGBA(crack_time->format, 255, 200, 200, 200);
	blue_alpha = SDL_MapRGBA(crack_time->format, 100, 100, 255, 200);
	orange_alpha = SDL_MapRGBA(crack_time->format, 255, 195, 0, 200);
	purple_alpha = SDL_MapRGBA(crack_time->format, 128, 0, 200, 200);
	yellow_alpha = SDL_MapRGBA(crack_time->format, 255, 255, 100, 200);
	light_green_alpha = SDL_MapRGBA(crack_time->format, 100, 200, 100, 100);
	green_alpha = SDL_MapRGBA(crack_time->format, 100, 200, 100, 200);

	Uint32 *pixmem32;

	// Plot the cracking depth, flipped upside down (depth->h - r instead of r)
	// Memorize the min and max depth to correctly scale the plot

	int min_depth = 0;
	int max_depth = NR;

	float **Crack_depth = (float**) malloc(NT*sizeof(float*));         // Crack_depth[NT][2]
	                                                                   // Used for the subseafloor zoom graphics
	if (Crack_depth == NULL) printf("Crack: Not enough memory to create Crack_depth[NT][2]\n");
	for (t=0;t<NT;t++) {
		Crack_depth[t] = (float*) malloc(2*sizeof(float));
		if (Crack_depth[t] == NULL) printf("Crack: Not enough memory to create Crack_depth[NT][2]\n");
	}

	for (t=0;t<NT;t++) {
		Crack_depth[t][1] = 0.0, Crack_depth[t][2] = 0.0;
	}

	for (t=0;t<NT;t++) {
		for (r=0;r<NR;r++) {
			// Cooling cracks in white
			if (Crack[r][t] == 1.0) {
				pixmem32 = (Uint32*) crack_time->pixels + (crack_time->h - r)*crack_time->w + t;
				*pixmem32 = white_alpha;
			}
			// Heating cracks in red
			if (Crack[r][t] == 2.0) {
				pixmem32 = (Uint32*) crack_time->pixels + (crack_time->h - r)*crack_time->w + t;
				*pixmem32 = red_alpha;
			}
			// Hydration cracks in blue
			if (Crack[r][t] == 3.0) {
				pixmem32 = (Uint32*) crack_time->pixels + (crack_time->h - r)*crack_time->w + t;
				*pixmem32 = blue_alpha;
			}
			// Dehydration cracks in yellow
			if (Crack[r][t] == 4.0) {
				pixmem32 = (Uint32*) crack_time->pixels + (crack_time->h - r)*crack_time->w + t;
				*pixmem32 = orange_alpha;
			}
			// Pore dilation cracks in purple
			if (Crack[r][t] == 5.0) {
				pixmem32 = (Uint32*) crack_time->pixels + (crack_time->h - r)*crack_time->w + t;
				*pixmem32 = purple_alpha;
			}
			// Cracks widened by dissolution in yellow
			if (Crack[r][t] == 6.0) {
				pixmem32 = (Uint32*) crack_time->pixels + (crack_time->h - r)*crack_time->w + t;
				*pixmem32 = yellow_alpha;
			}
			// Cracks shrunk by precipitation in light green
			if (Crack[r][t] == 7.0) {
				pixmem32 = (Uint32*) crack_time->pixels + (crack_time->h - r)*crack_time->w + t;
				*pixmem32 = light_green_alpha;
			}
			// Cracks clogged by precipitation in green
			if (Crack[r][t] == -1.0) {
				pixmem32 = (Uint32*) crack_time->pixels + (crack_time->h - r)*crack_time->w + t;
				*pixmem32 = green_alpha;
			}
			if (Crack[r][t] > 0.0 && r>min_depth) {
				min_depth = r;
			}
			if (Crack[r][t] > 0.0 && r<max_depth) {
				max_depth = r;
			}
			if (Crack[r][t] > 0.0) {
				Crack_depth[t][2] = Crack_depth[t][2] + 1.0*r_p/NR;
			}
		}
	}

	// The cracked depth goes artificially to zero at t=NT-1, so let's set it equal to that at t=NT-2
	Crack_depth[NT-1][2] = Crack_depth[NT-2][2];

	crack_time_tex = SDL_CreateTextureFromSurface(renderer, crack_time);

	// Add numbers on the axes

	// x-axis
	numbers_tex_1 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect numbers_clip_1;
	SDL_Rect numbers_dest_1;
	numbers_clip_1 = ClipNumber(0,14);
	numbers_dest_1.x = crack_time->w - 240 - 56, numbers_dest_1.y = 63, numbers_dest_1.w = 12, numbers_dest_1.h = 20;

	numbers_tex_2 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect numbers_clip_2;
	SDL_Rect numbers_dest_2;
	numbers_clip_2 = ClipNumber(floor(total_time),14);
	if (total_time - (float) floor(total_time) != 0)      // Scale final time on x-axis
		numbers_dest_2.x = (int) (crack_time->w - 240 - 56 + 240*(float) floor(total_time)/total_time);
	else
		numbers_dest_2.x = crack_time->w - 240 - 56 + 240;
	numbers_dest_2.y = 63, numbers_dest_2.w = 12, numbers_dest_2.h = 20;

	// y-axis: max depth assumes a 2 digit number.

	int max_depth_100 = floor((min_depth-max_depth)*r_p/NR/100.0);                                   // 1st digit
	int max_depth_10 = floor(((min_depth-max_depth)*r_p/NR - max_depth_100*100.0)/10.0);             // 2nd digit
	int max_depth_1 = floor((min_depth-max_depth)*r_p/NR - max_depth_100*100.0 - max_depth_10*10.0); // 3rd digit

	max_depth_digit_1 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect max_depth_digit_clip_1;
	SDL_Rect max_depth_digit_dest_1;
	max_depth_digit_clip_1 = ClipNumber(max_depth_100,14);
	max_depth_digit_dest_1.x = 475, max_depth_digit_dest_1.y = 195;
	max_depth_digit_dest_1.w = 12, max_depth_digit_dest_1.h = 20;

	max_depth_digit_2 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect max_depth_digit_clip_2;
	SDL_Rect max_depth_digit_dest_2;
	max_depth_digit_clip_2 = ClipNumber(max_depth_10,14);
	max_depth_digit_dest_2.x = 483, max_depth_digit_dest_2.y = 195;
	max_depth_digit_dest_2.w = 12, max_depth_digit_dest_2.h = 20;

	max_depth_digit_3 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect max_depth_digit_clip_3;
	SDL_Rect max_depth_digit_dest_3;
	max_depth_digit_clip_3 = ClipNumber(max_depth_1,14);
	max_depth_digit_dest_3.x = 491, max_depth_digit_dest_3.y = 195;
	max_depth_digit_dest_3.w = 12, max_depth_digit_dest_3.h = 20;

	// WATER-ROCK RATIO VS. TIME PLOT

	// Plot the water-rock ratio
	// Memorize the min and max ratio to correctly scale the plot

	float max_ratio = 0.0;

	for (t=0;t<NT;t++) {
		if (WRratio[t][1]>max_ratio) {
			max_ratio = WRratio[t][1];
		}
	}

	Uint32 WR_ratio_color;

	for (t=0;t<NT;t++) {
		if (WRratio[t][1] != 0) {
			for (r=0;r<(int) floor(WRratio[t][1]/max_ratio*WR->h);r++) {
				WR_ratio_color = SDL_MapRGBA(WR->format, (Uint8) (200.0*(1.0-0.35*WRratio[t][1]/max_ratio)), (Uint8) (100.0*(1.0+0.5*WRratio[t][1]/max_ratio)), (Uint8) (255.0*WRratio[t][1]/max_ratio), 200);
				pixmem32 = (Uint32*) WR->pixels + (WR->h-r-1)*WR->w + t;
				*pixmem32 = WR_ratio_color;
			}
		}
	}

	WR_tex = SDL_CreateTextureFromSurface(renderer, WR);

	// Add numbers on the axes

	numbers_tex_3 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect numbers_clip_3;
	SDL_Rect numbers_dest_3;
	numbers_clip_3 = ClipNumber(0,14);
	numbers_dest_3.x = crack_time->w - 240 - 56, numbers_dest_3.y = 448, numbers_dest_3.w = 12, numbers_dest_3.h = 20;

	numbers_tex_4 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect numbers_clip_4;
	SDL_Rect numbers_dest_4;
	numbers_clip_4 = ClipNumber(floor(total_time),14);
	if (total_time - (float) floor(total_time) != 0)      // Scale final time on x-axis
		numbers_dest_4.x = (int) (crack_time->w - 240 - 56 + 240*(float) floor(total_time)/total_time);
	else
		numbers_dest_4.x = crack_time->w - 240 - 56 + 240;
	numbers_dest_4.y = 448, numbers_dest_4.w = 12, numbers_dest_4.h = 20;

	// y-axis bar

	for (t=473;t<493;t++) {
		for (r=340;r<448;r++) {
			int red = (Uint8) floor(200.0*(1.0-0.35*(448.0-r)/(448.0-340.0)));
			int green = (Uint8) floor(100.0*(1.0+0.5*(448.0-r)/(448.0-340.0)));
			int blue = (Uint8) floor(255.0*(448.0-r)/(448.0-340.0));
			WR_ratio_color = SDL_MapRGBA(WR_bar->format, red, green, blue, 200);
			pixmem32 = (Uint32*) WR_bar->pixels + r*WR_bar->w + t;
			*pixmem32 = WR_ratio_color;
		}
	}

	WR_bar_tex = SDL_CreateTextureFromSurface(renderer, WR_bar);

	// y-axis max ratio (goes to the 2nd decimal place).

	int max_ratio_100 = floor(max_ratio);                                        // 1st digit
	int max_ratio_10 = floor((max_ratio-max_ratio_100)*10.0);                    // 2nd digit
	int max_ratio_1 = floor((max_ratio-max_ratio_100)*100.0-max_ratio_10*10.0);  // 3rd digit

	max_ratio_digit_1 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect max_ratio_digit_clip_1;
	SDL_Rect max_ratio_digit_dest_1;
	max_ratio_digit_clip_1 = ClipNumber(max_ratio_100,14);
	max_ratio_digit_dest_1.x = 468, max_ratio_digit_dest_1.y = 321;
	max_ratio_digit_dest_1.w = 12, max_ratio_digit_dest_1.h = 20;

	max_ratio_digit_2 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect max_ratio_digit_clip_2;
	SDL_Rect max_ratio_digit_dest_2;
	max_ratio_digit_clip_2 = ClipNumber(max_ratio_10,14);
	max_ratio_digit_dest_2.x = 478, max_ratio_digit_dest_2.y = 321;
	max_ratio_digit_dest_2.w = 12, max_ratio_digit_dest_2.h = 20;

	max_ratio_digit_3 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect max_ratio_digit_clip_3;
	SDL_Rect max_ratio_digit_dest_3;
	max_ratio_digit_clip_3 = ClipNumber(max_ratio_1,14);
	max_ratio_digit_dest_3.x = 486, max_ratio_digit_dest_3.y = 321;
	max_ratio_digit_dest_3.w = 12, max_ratio_digit_dest_3.h = 20;

	// ZOOM ON SUBSEAFLOOR NUMBERS

	// Surface radius: assumes a 3 to 4 digit number.

	int r_p_1000 = floor(r_p/1000.0);                                   // 1st digit
	int r_p_100 = floor((r_p-r_p_1000*1000.0)/100.0);                   // 2nd digit
	int r_p_10 = floor((r_p-r_p_1000*1000.0-r_p_100*100.0)/10.0);       // 3rd digit
	int r_p_1 = floor(r_p-r_p_1000*1000.0-r_p_100*100.0-r_p_10*10.0);   // 4th digit

	surface_digit_1 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect surface_digit_clip_1;
	SDL_Rect surface_digit_dest_1;
	surface_digit_clip_1 = ClipNumber(r_p_1000,12);
	surface_digit_dest_1.x = 67, surface_digit_dest_1.y = 33;
	surface_digit_dest_1.w = 12, surface_digit_dest_1.h = 20;

	surface_digit_2 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect surface_digit_clip_2;
	SDL_Rect surface_digit_dest_2;
	surface_digit_clip_2 = ClipNumber(r_p_100,12);
	surface_digit_dest_2.x = 73, surface_digit_dest_2.y = 33;
	surface_digit_dest_2.w = 12, surface_digit_dest_2.h = 20;

	surface_digit_3 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect surface_digit_clip_3;
	SDL_Rect surface_digit_dest_3;
	surface_digit_clip_3 = ClipNumber(r_p_10,12);
	surface_digit_dest_3.x = 79, surface_digit_dest_3.y = 33;
	surface_digit_dest_3.w = 12, surface_digit_dest_3.h = 20;

	surface_digit_4 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect surface_digit_clip_4;
	SDL_Rect surface_digit_dest_4;
	surface_digit_clip_4 = ClipNumber(r_p_1,12);
	surface_digit_dest_4.x = 85, surface_digit_dest_4.y = 33;
	surface_digit_dest_4.w = 12, surface_digit_dest_4.h = 20;

	// Seafloor/core radius: assumes a 3-digit number

	float r_core = 0.0;
	r=0;

	while (thoutput[r][500].mrock > 0) {
		r++;
	}
	r_core = r*r_p/NR;
	r = 0;

	int seafloor_100 = floor(r_core/100);                                  // 1st digit
	int seafloor_10 = floor((r_core-seafloor_100*100.0)/10.0);             // 2nd digit
	int seafloor_1 = floor(r_core-seafloor_100*100.0-seafloor_10*10.0);    // 3rd digit

	seafloor_digit_1 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect seafloor_digit_clip_1;
	SDL_Rect seafloor_digit_dest_1;
	seafloor_digit_clip_1 = ClipNumber(seafloor_100,12);
	seafloor_digit_dest_1.x = 73, seafloor_digit_dest_1.y = 51;
	seafloor_digit_dest_1.w = 12, seafloor_digit_dest_1.h = 20;

	seafloor_digit_2 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect seafloor_digit_clip_2;
	SDL_Rect seafloor_digit_dest_2;
	seafloor_digit_clip_2 = ClipNumber(seafloor_10,12);
	seafloor_digit_dest_2.x = 79, seafloor_digit_dest_2.y = 51;
	seafloor_digit_dest_2.w = 12, seafloor_digit_dest_2.h = 20;

	seafloor_digit_3 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect seafloor_digit_clip_3;
	SDL_Rect seafloor_digit_dest_3;
	seafloor_digit_clip_3 = ClipNumber(seafloor_1,12);
	seafloor_digit_dest_3.x = 85, seafloor_digit_dest_3.y = 51;
	seafloor_digit_dest_3.w = 12, seafloor_digit_dest_3.h = 20;

	// Seafloor/cracked depth: assumes a 3 digit number.

	float r_depth = r_core-(min_depth-max_depth)*r_p/NR;

	int r_depth_100 = floor(r_depth/100.0);                              // 1st digit
	int r_depth_10 = floor((r_depth-r_depth_100*100.0)/10.0);            // 2nd digit
	int r_depth_1 = floor(r_depth-r_depth_100*100.0-r_depth_10*10.0);    // 3rd digit

	cracked_depth_digit_1 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect cracked_depth_digit_clip_1;
	SDL_Rect cracked_depth_digit_dest_1;
	cracked_depth_digit_clip_1 = ClipNumber(r_depth_100,12);
	cracked_depth_digit_dest_1.x = 73, cracked_depth_digit_dest_1.y = 176;
	cracked_depth_digit_dest_1.w = 12, cracked_depth_digit_dest_1.h = 20;

	cracked_depth_digit_2 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect cracked_depth_digit_clip_2;
	SDL_Rect cracked_depth_digit_dest_2;
	cracked_depth_digit_clip_2 = ClipNumber(r_depth_10,12);
	cracked_depth_digit_dest_2.x = 79, cracked_depth_digit_dest_2.y = 176;
	cracked_depth_digit_dest_2.w = 12, cracked_depth_digit_dest_2.h = 20;

	cracked_depth_digit_3 = SDL_CreateTextureFromSurface(renderer, numbers);
	SDL_Rect cracked_depth_digit_clip_3;
	SDL_Rect cracked_depth_digit_dest_3;
	cracked_depth_digit_clip_3 = ClipNumber(r_depth_1,12);
	cracked_depth_digit_dest_3.x = 85, cracked_depth_digit_dest_3.y = 176;
	cracked_depth_digit_dest_3.w = 12, cracked_depth_digit_dest_3.h = 20;

	// PROGRESS BAR

	int percent;                  // % of history, 4.56 Gyr = 100%
	int percent_10;               // 2nd digit
	int percent_100;              // 3rd digit

	Uint32 progress_bar_color;
	progress_bar_color = SDL_MapRGBA(progress_bar->format, 100, 100, 255, 220);

	for (t=21;t<780;t++) {
		for (r=551;r<566;r++) {
			pixmem32 = (Uint32*) progress_bar->pixels + r*progress_bar->w + t;
			*pixmem32 = progress_bar_color;
		}
	}

	progress_bar_tex = SDL_CreateTextureFromSurface(renderer, progress_bar);

//-------------------------------------------------------------------
//                      Interactive display
//-------------------------------------------------------------------

	SDL_Rect crack_time_clip;          // Section of the image to clip
	SDL_Rect crack_time_dilation;      // Resized and repositioned clip

	SDL_Rect WR_time_clip;             // Section of the image to clip
	SDL_Rect WR_time_dilation;         // Resized and repositioned clip

	SDL_Rect progress_bar_clip;        // Section of the image to clip
	SDL_Rect progress_bar_dilation;    // Resized and repositioned clip

	SDL_Rect cracked_rock_clip;        // Section of the image to clip
	SDL_Rect cracked_rock_dilation;    // Resized and repositioned clip

	SDL_Rect elapsed_digit_clip_1;
	SDL_Rect elapsed_digit_dest_1;

	SDL_Rect elapsed_digit_clip_2;
	SDL_Rect elapsed_digit_dest_2;

	SDL_Rect elapsed_digit_clip_3;
	SDL_Rect elapsed_digit_dest_3;

	SDL_Rect elapsed_percent_clip_1;
	SDL_Rect elapsed_percent_dest_1;

	SDL_Rect elapsed_percent_clip_2;
	SDL_Rect elapsed_percent_dest_2;

	SDL_Rect elapsed_percent_clip_3;
	SDL_Rect elapsed_percent_dest_3;

	SDL_Event e;
	t = NT-1;          // Initialize at the end of the simulation
	int t_memory = t;
	int t_init = 0;
	int stop_clicked = 0;

	while (!quit){
		//Event polling
		while (SDL_PollEvent(&e)){
			//If user closes the window
			if (e.type == SDL_QUIT) quit = 1;
			if (e.type == SDL_MOUSEBUTTONDOWN) {

				// Play - Stop

				if (e.button.x >= 20 && e.button.x <= 68 && e.button.y >= 511 && e.button.y <= 539) {
					for (t=t_init;t<NT;t++) {

						stop_clicked = 0;

						SDL_RenderClear(renderer);
						ApplySurface(0, 0, background_tex, renderer, NULL);

						// Resize, position, and unveil the cracking depth plot
						crack_time_clip.x = 0, crack_time_clip.y = crack_time->h - min_depth;
						crack_time_clip.w = t, crack_time_clip.h = min_depth - max_depth;
						crack_time_dilation.x = crack_time->w - 240 - 50, crack_time_dilation.y = 87;
						crack_time_dilation.w = floor(240.0*t/NT), crack_time_dilation.h = 120;
						SDL_RenderCopy(renderer, crack_time_tex, &crack_time_clip, &crack_time_dilation);

						// Resize, position, and unveil the water-rock ratio plot
						WR_time_clip.x = 0, WR_time_clip.y = 0;
						WR_time_clip.w = t, WR_time_clip.h = WR->h;
						WR_time_dilation.x = WR->w - 240 - 50, WR_time_dilation.y = 341;
						WR_time_dilation.w = floor(240.0*t/NT), WR_time_dilation.h = 105;
						SDL_RenderCopy(renderer, WR_tex, &WR_time_clip, &WR_time_dilation);

						// Unveil the progress bar
						progress_bar_clip.x = 21, progress_bar_clip.y = 551;
						progress_bar_clip.w = floor((780.0-21.0)*t/NT), progress_bar_clip.h = 15;
						progress_bar_dilation.x = 21, progress_bar_dilation.y = 551;
						progress_bar_dilation.w = floor((780.0-21.0)*t/NT), progress_bar_dilation.h = 15;
						SDL_RenderCopy(renderer, progress_bar_tex, &progress_bar_clip, &progress_bar_dilation);

						// Zoom on the subseafloor
						cracked_rock_clip.x = 0, cracked_rock_clip.y = 0;
						cracked_rock_clip.w = SCREEN_WIDTH, cracked_rock_clip.h = 2*floor(Crack_depth[t][2]/((min_depth-max_depth)*r_p/NR)*110.0);
						cracked_rock_dilation.x = 118, cracked_rock_dilation.y = 64;
						cracked_rock_dilation.w = 319, cracked_rock_dilation.h = floor(Crack_depth[t][2]/((min_depth-max_depth)*r_p/NR)*110.0);
						SDL_RenderCopy(renderer, cracked_rock_tex, &cracked_rock_clip, &cracked_rock_dilation);

						// Time elapsed

						elapsed_digit_1 = SDL_CreateTextureFromSurface(renderer, numbers);
						elapsed_digit_clip_1 = ClipNumber(floor(t/100.0),18);
						elapsed_digit_dest_1.x = 625, elapsed_digit_dest_1.y = 502;
						elapsed_digit_dest_1.w = 12, elapsed_digit_dest_1.h = 20;
						SDL_RenderCopy(renderer, elapsed_digit_1, &elapsed_digit_clip_1, &elapsed_digit_dest_1);

						elapsed_digit_2 = SDL_CreateTextureFromSurface(renderer, numbers);
						int t_10 = floor((t-floor(t/100.0)*100.0)/10.0);
						elapsed_digit_clip_2 = ClipNumber(t_10,18);
						elapsed_digit_dest_2.x = 640, elapsed_digit_dest_2.y = elapsed_digit_dest_1.y;
						elapsed_digit_dest_2.w = 12, elapsed_digit_dest_2.h = 20;
						SDL_RenderCopy(renderer, elapsed_digit_2, &elapsed_digit_clip_2, &elapsed_digit_dest_2);

						elapsed_digit_3 = SDL_CreateTextureFromSurface(renderer, numbers);
						int t_100 = floor(t-floor(t/100.0)*100.0-floor(t_10)*10.0);
						elapsed_digit_clip_3 = ClipNumber(t_100,18);
						elapsed_digit_dest_3.x = 650, elapsed_digit_dest_3.y = elapsed_digit_dest_1.y;
						elapsed_digit_dest_3.w = 12, elapsed_digit_dest_3.h = 20;
						SDL_RenderCopy(renderer, elapsed_digit_3, &elapsed_digit_clip_3, &elapsed_digit_dest_3);

						// % history elapsed

						percent = t/4.56;

						elapsed_percent_1 = SDL_CreateTextureFromSurface(renderer, numbers);
						elapsed_percent_clip_1 = ClipNumber(floor(percent/100.0),18);
						elapsed_percent_dest_1.x = 630, elapsed_percent_dest_1.y = 526;
						elapsed_percent_dest_1.w = 12, elapsed_percent_dest_1.h = 20;
						if (floor(percent/100.0) > 0.0)                      // Don't display the first number if it is 0
							SDL_RenderCopy(renderer, elapsed_percent_1, &elapsed_percent_clip_1, &elapsed_percent_dest_1);

						elapsed_percent_2 = SDL_CreateTextureFromSurface(renderer, numbers);
						percent_10 = floor((percent-floor(percent/100.0)*100.0)/10.0);
						elapsed_percent_clip_2 = ClipNumber(percent_10,18);
						elapsed_percent_dest_2.x = 640, elapsed_percent_dest_2.y = elapsed_percent_dest_1.y;
						elapsed_percent_dest_2.w = 12, elapsed_percent_dest_2.h = 20;
						if (floor(percent/100.0) > 0.0 || percent_10 > 0.0)    // Don't display the first numbers if they are both 0
							SDL_RenderCopy(renderer, elapsed_percent_2, &elapsed_percent_clip_2, &elapsed_percent_dest_2);

						elapsed_percent_3 = SDL_CreateTextureFromSurface(renderer, numbers);
						percent_100 = floor(percent-floor(percent/100.0)*100.0-floor(percent_10)*10.0);
						elapsed_percent_clip_3 = ClipNumber(percent_100,18);
						elapsed_percent_dest_3.x = 650, elapsed_percent_dest_3.y = elapsed_percent_dest_1.y;
						elapsed_percent_dest_3.w = 12, elapsed_percent_dest_3.h = 20;
						SDL_RenderCopy(renderer, elapsed_percent_3, &elapsed_percent_clip_3, &elapsed_percent_dest_3);

						// Other renderings

						SDL_RenderCopy(renderer, WR_bar_tex, NULL, NULL);
						SDL_RenderCopy(renderer, numbers_tex_1, &numbers_clip_1, &numbers_dest_1);
						SDL_RenderCopy(renderer, numbers_tex_2, &numbers_clip_2, &numbers_dest_2);
						SDL_RenderCopy(renderer, numbers_tex_3, &numbers_clip_3, &numbers_dest_3);
						SDL_RenderCopy(renderer, numbers_tex_4, &numbers_clip_4, &numbers_dest_4);
						if (r_p_1000 > 0.0)
						SDL_RenderCopy(renderer, surface_digit_1, &surface_digit_clip_1, &surface_digit_dest_1);
						SDL_RenderCopy(renderer, surface_digit_2, &surface_digit_clip_2, &surface_digit_dest_2);
						SDL_RenderCopy(renderer, surface_digit_3, &surface_digit_clip_3, &surface_digit_dest_3);
						SDL_RenderCopy(renderer, surface_digit_4, &surface_digit_clip_4, &surface_digit_dest_4);
						SDL_RenderCopy(renderer, seafloor_digit_1, &seafloor_digit_clip_1, &seafloor_digit_dest_1);
						SDL_RenderCopy(renderer, seafloor_digit_2, &seafloor_digit_clip_2, &seafloor_digit_dest_2);
						SDL_RenderCopy(renderer, seafloor_digit_3, &seafloor_digit_clip_3, &seafloor_digit_dest_3);
						if (r_depth > 100.0)
						SDL_RenderCopy(renderer, cracked_depth_digit_1, &cracked_depth_digit_clip_1, &cracked_depth_digit_dest_1);
						if (r_depth > 10.0)
						SDL_RenderCopy(renderer, cracked_depth_digit_2, &cracked_depth_digit_clip_2, &cracked_depth_digit_dest_2);
						SDL_RenderCopy(renderer, cracked_depth_digit_3, &cracked_depth_digit_clip_3, &cracked_depth_digit_dest_3);
						SDL_RenderCopy(renderer, max_ratio_digit_1, &max_ratio_digit_clip_1, &max_ratio_digit_dest_1);
						SDL_RenderCopy(renderer, max_ratio_digit_2, &max_ratio_digit_clip_2, &max_ratio_digit_dest_2);
						SDL_RenderCopy(renderer, max_ratio_digit_3, &max_ratio_digit_clip_3, &max_ratio_digit_dest_3);
						if ((min_depth-max_depth)*r_p/NR > 100.0)
						SDL_RenderCopy(renderer, max_depth_digit_1, &max_depth_digit_clip_1, &max_depth_digit_dest_1);
						if ((min_depth-max_depth)*r_p/NR > 10.0)
						SDL_RenderCopy(renderer, max_depth_digit_2, &max_depth_digit_clip_2, &max_depth_digit_dest_2);
						SDL_RenderCopy(renderer, max_depth_digit_3, &max_depth_digit_clip_3, &max_depth_digit_dest_3);
						SDL_RenderPresent(renderer);
						SDL_Delay(16);

						SDL_PollEvent(&e);
						if (e.type == SDL_MOUSEBUTTONDOWN) {
							// If press stop
							if (e.button.x >= 76 && e.button.x <= 124 && e.button.y >= 511 && e.button.y <= 539) {
								t_memory = t; // Memorize where we stopped
								t_init = t;   // To start where we left off if we play again
								t = NT;       // Exit for loop
								stop_clicked = 1;
							}
							// If click on the bar
							else if (e.button.x >= 20 && e.button.x <= 780 && e.button.y >= 550 && e.button.y <= 567) {
								t = floor(((float) e.button.x - 20.0)/(780.0-20.0)*500.0);
							}
						}
					}
					if (stop_clicked == 1) t = t_memory;
					else t = NT-1, t_init = 0;

				}

				// Click on % bar to adjust time or scroll
				if (e.button.x >= 20 && e.button.x <= 780 && e.button.y >= 550 && e.button.y <= 567) {
					t = floor(((float) e.button.x - 20.0)/(780.0-20.0)*500.0);

					// While mouse button is down, scroll
					while (e.type != SDL_MOUSEBUTTONUP) {
						SDL_PollEvent(&e);

						// Do not change t past the x edges of the bar. The y limits are to avoid the program
						// crashing because we're out of the window.
						if ((float) e.button.x + e.motion.xrel >= 20 && (float) e.button.x + e.motion.xrel <= 780
								&& (float) e.button.y + e.motion.yrel > 0 && (float) e.button.y + e.motion.yrel < SCREEN_HEIGHT) {

							// Adjust displays
							t = floor(((float) e.button.x + e.motion.xrel - 20.0)/(780.0-20.0)*500.0);

							// Update displays
							SDL_RenderClear(renderer);
							ApplySurface(0, 0, background_tex, renderer, NULL);

							// Resize, position, and unveil the cracking depth plot
							crack_time_clip.x = 0, crack_time_clip.y = crack_time->h - min_depth;
							crack_time_clip.w = t, crack_time_clip.h = min_depth - max_depth;
							crack_time_dilation.x = crack_time->w - 240 - 50, crack_time_dilation.y = 87;
							crack_time_dilation.w = floor(240.0*t/NT), crack_time_dilation.h = 120;
							SDL_RenderCopy(renderer, crack_time_tex, &crack_time_clip, &crack_time_dilation);

							// Resize, position, and unveil the water-rock ratio plot
							WR_time_clip.x = 0, WR_time_clip.y = 0;
							WR_time_clip.w = t, WR_time_clip.h = WR->h;
							WR_time_dilation.x = WR->w - 240 - 50, WR_time_dilation.y = 341;
							WR_time_dilation.w = floor(240.0*t/NT), WR_time_dilation.h = 105;
							SDL_RenderCopy(renderer, WR_tex, &WR_time_clip, &WR_time_dilation);

							// Unveil the progress bar
							progress_bar_clip.x = 21, progress_bar_clip.y = 551;
							progress_bar_clip.w = floor((780.0-21.0)*t/NT), progress_bar_clip.h = 15;
							progress_bar_dilation.x = 21, progress_bar_dilation.y = 551;
							progress_bar_dilation.w = floor((780.0-21.0)*t/NT), progress_bar_dilation.h = 15;
							SDL_RenderCopy(renderer, progress_bar_tex, &progress_bar_clip, &progress_bar_dilation);

							// Zoom on the subseafloor
							cracked_rock_clip.x = 0, cracked_rock_clip.y = 0;
							cracked_rock_clip.w = SCREEN_WIDTH, cracked_rock_clip.h = 2*floor(Crack_depth[t][2]/((min_depth-max_depth)*r_p/NR)*110.0);
							cracked_rock_dilation.x = 118, cracked_rock_dilation.y = 64;
							cracked_rock_dilation.w = 319, cracked_rock_dilation.h = floor(Crack_depth[t][2]/((min_depth-max_depth)*r_p/NR)*110.0);
							SDL_RenderCopy(renderer, cracked_rock_tex, &cracked_rock_clip, &cracked_rock_dilation);

							// Time elapsed

							elapsed_digit_1 = SDL_CreateTextureFromSurface(renderer, numbers);
							elapsed_digit_clip_1 = ClipNumber(floor(t/100.0),18);
							elapsed_digit_dest_1.x = 625, elapsed_digit_dest_1.y = 502;
							elapsed_digit_dest_1.w = 12, elapsed_digit_dest_1.h = 20;
							SDL_RenderCopy(renderer, elapsed_digit_1, &elapsed_digit_clip_1, &elapsed_digit_dest_1);

							elapsed_digit_2 = SDL_CreateTextureFromSurface(renderer, numbers);
							int t_10 = floor((t-floor(t/100.0)*100.0)/10.0);
							elapsed_digit_clip_2 = ClipNumber(t_10,18);
							elapsed_digit_dest_2.x = 640, elapsed_digit_dest_2.y = elapsed_digit_dest_1.y;
							elapsed_digit_dest_2.w = 12, elapsed_digit_dest_2.h = 20;
							SDL_RenderCopy(renderer, elapsed_digit_2, &elapsed_digit_clip_2, &elapsed_digit_dest_2);

							elapsed_digit_3 = SDL_CreateTextureFromSurface(renderer, numbers);
							int t_100 = floor(t-floor(t/100.0)*100.0-floor(t_10)*10.0);
							elapsed_digit_clip_3 = ClipNumber(t_100,18);
							elapsed_digit_dest_3.x = 650, elapsed_digit_dest_3.y = elapsed_digit_dest_1.y;
							elapsed_digit_dest_3.w = 12, elapsed_digit_dest_3.h = 20;
							SDL_RenderCopy(renderer, elapsed_digit_3, &elapsed_digit_clip_3, &elapsed_digit_dest_3);

							// % history elapsed

							percent = t/4.56;

							elapsed_percent_1 = SDL_CreateTextureFromSurface(renderer, numbers);
							elapsed_percent_clip_1 = ClipNumber(floor(percent/100.0),18);
							elapsed_percent_dest_1.x = 630, elapsed_percent_dest_1.y = 526;
							elapsed_percent_dest_1.w = 12, elapsed_percent_dest_1.h = 20;
							if (floor(percent/100.0) > 0.0)                      // Don't display the first number if it is 0
								SDL_RenderCopy(renderer, elapsed_percent_1, &elapsed_percent_clip_1, &elapsed_percent_dest_1);

							elapsed_percent_2 = SDL_CreateTextureFromSurface(renderer, numbers);
							percent_10 = floor((percent-floor(percent/100.0)*100.0)/10.0);
							elapsed_percent_clip_2 = ClipNumber(percent_10,18);
							elapsed_percent_dest_2.x = 640, elapsed_percent_dest_2.y = elapsed_percent_dest_1.y;
							elapsed_percent_dest_2.w = 12, elapsed_percent_dest_2.h = 20;
							if (floor(percent/100.0) > 0.0 || percent_10 > 0.0)    // Don't display the first numbers if they are both 0
								SDL_RenderCopy(renderer, elapsed_percent_2, &elapsed_percent_clip_2, &elapsed_percent_dest_2);

							elapsed_percent_3 = SDL_CreateTextureFromSurface(renderer, numbers);
							percent_100 = floor(percent-floor(percent/100.0)*100.0-floor(percent_10)*10.0);
							elapsed_percent_clip_3 = ClipNumber(percent_100,18);
							elapsed_percent_dest_3.x = 650, elapsed_percent_dest_3.y = elapsed_percent_dest_1.y;
							elapsed_percent_dest_3.w = 12, elapsed_percent_dest_3.h = 20;
							SDL_RenderCopy(renderer, elapsed_percent_3, &elapsed_percent_clip_3, &elapsed_percent_dest_3);

							// Other renderings

							SDL_RenderCopy(renderer, WR_bar_tex, NULL, NULL);
							SDL_RenderCopy(renderer, numbers_tex_1, &numbers_clip_1, &numbers_dest_1);
							SDL_RenderCopy(renderer, numbers_tex_2, &numbers_clip_2, &numbers_dest_2);
							SDL_RenderCopy(renderer, numbers_tex_3, &numbers_clip_3, &numbers_dest_3);
							SDL_RenderCopy(renderer, numbers_tex_4, &numbers_clip_4, &numbers_dest_4);
							if (r_p_1000 > 0.0)
							SDL_RenderCopy(renderer, surface_digit_1, &surface_digit_clip_1, &surface_digit_dest_1);
							SDL_RenderCopy(renderer, surface_digit_2, &surface_digit_clip_2, &surface_digit_dest_2);
							SDL_RenderCopy(renderer, surface_digit_3, &surface_digit_clip_3, &surface_digit_dest_3);
							SDL_RenderCopy(renderer, surface_digit_4, &surface_digit_clip_4, &surface_digit_dest_4);
							SDL_RenderCopy(renderer, seafloor_digit_1, &seafloor_digit_clip_1, &seafloor_digit_dest_1);
							SDL_RenderCopy(renderer, seafloor_digit_2, &seafloor_digit_clip_2, &seafloor_digit_dest_2);
							SDL_RenderCopy(renderer, seafloor_digit_3, &seafloor_digit_clip_3, &seafloor_digit_dest_3);
							if (r_depth > 100.0)
							SDL_RenderCopy(renderer, cracked_depth_digit_1, &cracked_depth_digit_clip_1, &cracked_depth_digit_dest_1);
							if (r_depth > 10.0)
							SDL_RenderCopy(renderer, cracked_depth_digit_1, &cracked_depth_digit_clip_2, &cracked_depth_digit_dest_2);
							SDL_RenderCopy(renderer, cracked_depth_digit_1, &cracked_depth_digit_clip_3, &cracked_depth_digit_dest_3);
							SDL_RenderCopy(renderer, max_ratio_digit_1, &max_ratio_digit_clip_1, &max_ratio_digit_dest_1);
							SDL_RenderCopy(renderer, max_ratio_digit_2, &max_ratio_digit_clip_2, &max_ratio_digit_dest_2);
							SDL_RenderCopy(renderer, max_ratio_digit_3, &max_ratio_digit_clip_3, &max_ratio_digit_dest_3);
							if ((min_depth-max_depth)*r_p/NR > 100.0)
							SDL_RenderCopy(renderer, max_depth_digit_1, &max_depth_digit_clip_1, &max_depth_digit_dest_1);
							if ((min_depth-max_depth)*r_p/NR > 10.0)
							SDL_RenderCopy(renderer, max_depth_digit_2, &max_depth_digit_clip_2, &max_depth_digit_dest_2);
							SDL_RenderCopy(renderer, max_depth_digit_3, &max_depth_digit_clip_3, &max_depth_digit_dest_3);
							SDL_RenderPresent(renderer);
							SDL_Delay(16);
						}
					}
					t_init = t;  // To pick up the animation back where we're leaving off
				}
			}
		}
		SDL_RenderClear(renderer);
		ApplySurface(0, 0, background_tex, renderer, NULL);

		// Resize, position, and unveil the cracking depth plot
		crack_time_clip.x = 0, crack_time_clip.y = crack_time->h - min_depth;
		crack_time_clip.w = t, crack_time_clip.h = min_depth - max_depth;
		crack_time_dilation.x = crack_time->w - 240 - 50, crack_time_dilation.y = 87;
		crack_time_dilation.w = floor(240.0*t/NT), crack_time_dilation.h = 120;
		SDL_RenderCopy(renderer, crack_time_tex, &crack_time_clip, &crack_time_dilation);

		// Resize, position, and unveil the water-rock ratio plot
		WR_time_clip.x = 0, WR_time_clip.y = 0;
		WR_time_clip.w = t, WR_time_clip.h = WR->h;
		WR_time_dilation.x = WR->w - 240 - 50, WR_time_dilation.y = 341;
		WR_time_dilation.w = floor(240.0*t/NT), WR_time_dilation.h = 105;
		SDL_RenderCopy(renderer, WR_tex, &WR_time_clip, &WR_time_dilation);

		// Unveil the progress bar
		progress_bar_clip.x = 21, progress_bar_clip.y = 551;
		progress_bar_clip.w = floor((780.0-21.0)*t/NT), progress_bar_clip.h = 15;
		progress_bar_dilation.x = 21, progress_bar_dilation.y = 551;
		progress_bar_dilation.w = floor((780.0-21.0)*t/NT), progress_bar_dilation.h = 15;
		SDL_RenderCopy(renderer, progress_bar_tex, &progress_bar_clip, &progress_bar_dilation);

		// Zoom on the subseafloor
		cracked_rock_clip.x = 0, cracked_rock_clip.y = 0;
		cracked_rock_clip.w = SCREEN_WIDTH, cracked_rock_clip.h = 2*floor(Crack_depth[t][2]/((min_depth-max_depth)*r_p/NR)*110.0);
		cracked_rock_dilation.x = 118, cracked_rock_dilation.y = 64;
		cracked_rock_dilation.w = 319, cracked_rock_dilation.h = floor(Crack_depth[t][2]/((min_depth-max_depth)*r_p/NR)*110.0);
		SDL_RenderCopy(renderer, cracked_rock_tex, &cracked_rock_clip, &cracked_rock_dilation);

		// Time elapsed

		elapsed_digit_1 = SDL_CreateTextureFromSurface(renderer, numbers);
		elapsed_digit_clip_1 = ClipNumber(floor(t/100.0),18);
		elapsed_digit_dest_1.x = 625, elapsed_digit_dest_1.y = 502;
		elapsed_digit_dest_1.w = 12, elapsed_digit_dest_1.h = 20;
		SDL_RenderCopy(renderer, elapsed_digit_1, &elapsed_digit_clip_1, &elapsed_digit_dest_1);

		elapsed_digit_2 = SDL_CreateTextureFromSurface(renderer, numbers);
		int t_10 = floor((t-floor(t/100.0)*100.0)/10.0);
		elapsed_digit_clip_2 = ClipNumber(t_10,18);
		elapsed_digit_dest_2.x = 640, elapsed_digit_dest_2.y = elapsed_digit_dest_1.y;
		elapsed_digit_dest_2.w = 12, elapsed_digit_dest_2.h = 20;
		SDL_RenderCopy(renderer, elapsed_digit_2, &elapsed_digit_clip_2, &elapsed_digit_dest_2);

		elapsed_digit_3 = SDL_CreateTextureFromSurface(renderer, numbers);
		int t_100 = floor(t-floor(t/100.0)*100.0-floor(t_10)*10.0);
		elapsed_digit_clip_3 = ClipNumber(t_100,18);
		elapsed_digit_dest_3.x = 650, elapsed_digit_dest_3.y = elapsed_digit_dest_1.y;
		elapsed_digit_dest_3.w = 12, elapsed_digit_dest_3.h = 20;
		SDL_RenderCopy(renderer, elapsed_digit_3, &elapsed_digit_clip_3, &elapsed_digit_dest_3);

		// % history elapsed

		percent = t/4.56;

		elapsed_percent_1 = SDL_CreateTextureFromSurface(renderer, numbers);
		elapsed_percent_clip_1 = ClipNumber(floor(percent/100.0),18);
		elapsed_percent_dest_1.x = 630, elapsed_percent_dest_1.y = 526;
		elapsed_percent_dest_1.w = 12, elapsed_percent_dest_1.h = 20;
		if (floor(percent/100.0) > 0.0)                        // Don't display the first number if it is 0
			SDL_RenderCopy(renderer, elapsed_percent_1, &elapsed_percent_clip_1, &elapsed_percent_dest_1);

		elapsed_percent_2 = SDL_CreateTextureFromSurface(renderer, numbers);
		percent_10 = floor((percent-floor(percent/100.0)*100.0)/10.0);
		elapsed_percent_clip_2 = ClipNumber(percent_10,18);
		elapsed_percent_dest_2.x = 640, elapsed_percent_dest_2.y = elapsed_percent_dest_1.y;
		elapsed_percent_dest_2.w = 12, elapsed_percent_dest_2.h = 20;
		if (floor(percent/100.0) > 0.0 || percent_10 > 0.0)    // Don't display the first numbers if they are both 0
			SDL_RenderCopy(renderer, elapsed_percent_2, &elapsed_percent_clip_2, &elapsed_percent_dest_2);

		elapsed_percent_3 = SDL_CreateTextureFromSurface(renderer, numbers);
		percent_100 = floor(percent-floor(percent/100.0)*100.0-floor(percent_10)*10.0);
		elapsed_percent_clip_3 = ClipNumber(percent_100,18);
		elapsed_percent_dest_3.x = 650, elapsed_percent_dest_3.y = elapsed_percent_dest_1.y;
		elapsed_percent_dest_3.w = 12, elapsed_percent_dest_3.h = 20;
		SDL_RenderCopy(renderer, elapsed_percent_3, &elapsed_percent_clip_3, &elapsed_percent_dest_3);

		// Other renderings

		SDL_RenderCopy(renderer, WR_bar_tex, NULL, NULL);
		SDL_RenderCopy(renderer, numbers_tex_1, &numbers_clip_1, &numbers_dest_1);
		SDL_RenderCopy(renderer, numbers_tex_2, &numbers_clip_2, &numbers_dest_2);
		SDL_RenderCopy(renderer, numbers_tex_3, &numbers_clip_3, &numbers_dest_3);
		SDL_RenderCopy(renderer, numbers_tex_4, &numbers_clip_4, &numbers_dest_4);
		if (r_p_1000 > 0.0)
		SDL_RenderCopy(renderer, surface_digit_1, &surface_digit_clip_1, &surface_digit_dest_1);
		SDL_RenderCopy(renderer, surface_digit_2, &surface_digit_clip_2, &surface_digit_dest_2);
		SDL_RenderCopy(renderer, surface_digit_3, &surface_digit_clip_3, &surface_digit_dest_3);
		SDL_RenderCopy(renderer, surface_digit_4, &surface_digit_clip_4, &surface_digit_dest_4);
		SDL_RenderCopy(renderer, seafloor_digit_1, &seafloor_digit_clip_1, &seafloor_digit_dest_1);
		SDL_RenderCopy(renderer, seafloor_digit_2, &seafloor_digit_clip_2, &seafloor_digit_dest_2);
		SDL_RenderCopy(renderer, seafloor_digit_3, &seafloor_digit_clip_3, &seafloor_digit_dest_3);
		if (r_depth > 100.0)
		SDL_RenderCopy(renderer, cracked_depth_digit_1, &cracked_depth_digit_clip_1, &cracked_depth_digit_dest_1);
		if (r_depth > 10.0)
		SDL_RenderCopy(renderer, cracked_depth_digit_1, &cracked_depth_digit_clip_2, &cracked_depth_digit_dest_2);
		SDL_RenderCopy(renderer, cracked_depth_digit_1, &cracked_depth_digit_clip_3, &cracked_depth_digit_dest_3);
		SDL_RenderCopy(renderer, max_ratio_digit_1, &max_ratio_digit_clip_1, &max_ratio_digit_dest_1);
		SDL_RenderCopy(renderer, max_ratio_digit_2, &max_ratio_digit_clip_2, &max_ratio_digit_dest_2);
		SDL_RenderCopy(renderer, max_ratio_digit_3, &max_ratio_digit_clip_3, &max_ratio_digit_dest_3);
		if ((min_depth-max_depth)*r_p/NR > 100.0)
		SDL_RenderCopy(renderer, max_depth_digit_1, &max_depth_digit_clip_1, &max_depth_digit_dest_1);
		if ((min_depth-max_depth)*r_p/NR > 10.0)
		SDL_RenderCopy(renderer, max_depth_digit_2, &max_depth_digit_clip_2, &max_depth_digit_dest_2);
		SDL_RenderCopy(renderer, max_depth_digit_3, &max_depth_digit_clip_3, &max_depth_digit_dest_3);
		SDL_RenderPresent(renderer);
		SDL_Delay(16);
	}

//-------------------------------------------------------------------
//                      Free remaining mallocs
//-------------------------------------------------------------------

	SDL_DestroyTexture(background_tex);
	SDL_DestroyTexture(crack_time_tex);
	SDL_FreeSurface(crack_time);
	SDL_DestroyTexture(WR_tex);
	SDL_FreeSurface(WR);
	SDL_DestroyTexture(WR_bar_tex);
	SDL_FreeSurface(WR_bar);
	SDL_DestroyTexture(progress_bar_tex);
	SDL_FreeSurface(progress_bar);
	SDL_DestroyTexture(cracked_rock_tex);
	SDL_DestroyTexture(numbers_tex_1);
	SDL_DestroyTexture(numbers_tex_2);
	SDL_DestroyTexture(numbers_tex_3);
	SDL_DestroyTexture(numbers_tex_4);
	SDL_DestroyTexture(elapsed_digit_1);
	SDL_DestroyTexture(elapsed_digit_2);
	SDL_DestroyTexture(elapsed_digit_3);
	SDL_DestroyTexture(elapsed_percent_1);
	SDL_DestroyTexture(elapsed_percent_2);
	SDL_DestroyTexture(elapsed_percent_3);
	SDL_DestroyTexture(surface_digit_1);
	SDL_DestroyTexture(surface_digit_2);
	SDL_DestroyTexture(surface_digit_3);
	SDL_DestroyTexture(surface_digit_4);
	SDL_DestroyTexture(seafloor_digit_1);
	SDL_DestroyTexture(seafloor_digit_2);
	SDL_DestroyTexture(seafloor_digit_3);
	SDL_DestroyTexture(cracked_depth_digit_1);
	SDL_DestroyTexture(cracked_depth_digit_2);
	SDL_DestroyTexture(cracked_depth_digit_3);
	SDL_DestroyTexture(max_ratio_digit_1);
	SDL_DestroyTexture(max_ratio_digit_2);
	SDL_DestroyTexture(max_ratio_digit_3);
	SDL_DestroyTexture(max_depth_digit_1);
	SDL_DestroyTexture(max_depth_digit_2);
	SDL_DestroyTexture(max_depth_digit_3);
	SDL_FreeSurface(numbers);

	SDL_FreeSurface(IcyDwarfIcon);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	for (r=0;r<NR;r++) {
		free (Crack[r]);
	}
	for (t=0;t<NT;t++) {
		free (Crack_depth[t]);
		free (WRratio[t]);
	}
	free (Crack);
	free (Crack_depth);
	free (WRratio);

	return 0;
}

#endif /* CRACK_PLOT_H_ */
