/*
 * Thermal_plot.h
 *
 *  Created on: Mar 6, 2014
 *      Author: Marc Neveu (mneveu@asu.edu)
 */

#ifndef THERMAL_PLOT_H_
#define THERMAL_PLOT_H_

#include "../Graphics/Plot.h"

int Thermal_plot (char path[1024], int Tmax_input, int NR, int NT_output, double r_p, thermalout **thoutput,
		int warnings, int msgout, SDL_Renderer* renderer, int* view, int* quit, char* FontFile, SDL_Color axisTextColor);

int StructurePlot (SDL_Renderer* renderer, thermalout **thoutput, int t, int NR,
		SDL_Texture* DryRock_tex, SDL_Texture* Liquid_tex, SDL_Texture* Ice_tex, SDL_Texture* Crust_tex,
		SDL_Rect temp_time_dilation);

int TwoAxisPlot (SDL_Renderer* renderer, char *FontFile, double **Values, int grid, int hold_tracks, int t, int NR, int irmax,
		int itempk_step, int itempk_max, double itempk_fold_min, double itempk_fold_max, double itempk_fold_step, int thickness,
		double Tmax, SDL_Surface* temp_time, SDL_Rect temp_time_clip, SDL_Rect* temp_time_dilation,
		SDL_Color axisTextColor, Uint32 alpha);

int UpdateDisplays (SDL_Renderer* renderer, SDL_Texture* background_tex, char* FontFile, thermalout **thoutput,
		double **TempK, double **Hydr, double **Kappa, int structure, int grid, int hold_tracks, int plot_switch,
		int t, int NR, int NT_output, int ircore, double Tmax, double kappamax, SDL_Surface* value_time,
		SDL_Color axisTextColor, Uint32 alpha,
		SDL_Texture* DryRock_tex, SDL_Texture* Liquid_tex, SDL_Texture* Ice_tex, SDL_Texture* Crust_tex,
		SDL_Texture* progress_bar_tex, int irad,
		SDL_Texture* xnumber0_tex, SDL_Texture* xnumber1_tex, SDL_Texture* xnumber2_tex, SDL_Texture* xnumber3_tex,
		SDL_Texture* xnumber4_tex, SDL_Texture* xnumber5_tex, SDL_Texture* xnumber6_tex, SDL_Texture* xnumber7_tex,
		SDL_Texture* xnumber8_tex, SDL_Texture* xnumber9_tex, SDL_Texture* ynumber0_tex,
		SDL_Texture* temp_tex, SDL_Texture* hydr_tex, SDL_Texture* k_tex,
		SDL_Texture* grid_tex, SDL_Texture* structure_tex, SDL_Texture* hold_tracks_tex,
		SDL_Texture* hydr_title_tex, SDL_Texture* k_title_tex, SDL_Texture* legend_tex);

int handleClickThermal(SDL_Event e, int *t, int *t_init, int *grid, int *structure, int *hold_tracks, int *plot_switch,
		int *t_memory, int NT_output);

int Thermal_plot (char path[1024], int Tmax_input, int NR, int NT_output, double r_p, thermalout **thoutput,
		int warnings, int msgout, SDL_Renderer* renderer, int* view, int* quit, char* FontFile, SDL_Color axisTextColor) {

	int r = 0;
	int t = 0;
	int irad = 0;
	int ircore = 0;
	int grid = 0;
	int structure = 0;
	int hold_tracks = 0;
	int plot_switch = 0;
	double Tmax = 0.0;     // Max temperature ever encountered in any layer
	double kappamax = 0.0; // Max thermal conductivity ever encountered in any layer

	Uint32 *pixmem32;
	Uint32 white;
	Uint32 alpha;

	double **TempK = (double**) malloc(NT_output*sizeof(double*));    // Temperature
	if (TempK == NULL) printf("Thermal_plot: Not enough memory to create TempK[NT_output]\n");
	for (t=0;t<NT_output;t++) {
		TempK[t] = (double*) malloc(NR*sizeof(double));
		if (TempK[t] == NULL) printf("Thermal_plot: Not enough memory to create TempK[NT_output][NR]\n");
	}

	double **Hydr = (double**) malloc(NT_output*sizeof(double*));     // Degree of hydration
	if (Hydr == NULL) printf("Thermal_plot: Not enough memory to create Hydr[NT_output]\n");
	for (t=0;t<NT_output;t++) {
		Hydr[t] = (double*) malloc(NR*sizeof(double));
		if (Hydr[t] == NULL) printf("Thermal_plot: Not enough memory to create Hydr[NT_output][NR]\n");
	}

	double **Kappa = (double**) malloc(NT_output*sizeof(double*));    // Thermal conductivity
	if (Kappa == NULL) printf("Thermal_plot: Not enough memory to create Kappa[NT_output]\n");
	for (t=0;t<NT_output;t++) {
		Kappa[t] = (double*) malloc(NR*sizeof(double));
		if (Kappa[t] == NULL) printf("Thermal_plot: Not enough memory to create Kappa[NT_output][NR]\n");
	}

//-------------------------------------------------------------------
//                     Initialize display elements
//-------------------------------------------------------------------

	SDL_Texture* background_tex = NULL;
	SDL_Texture* progress_bar_tex = NULL;                   // Progress bar
	SDL_Surface* progress_bar = NULL;

	SDL_Surface* value_time = NULL;                         // Value vs. time plot

	SDL_Texture* ynumber0_tex = NULL;                       // Axis numbers
	SDL_Texture* xnumber0_tex = NULL;
	SDL_Texture* xnumber1_tex = NULL;
	SDL_Texture* xnumber2_tex = NULL;
	SDL_Texture* xnumber3_tex = NULL;
	SDL_Texture* xnumber4_tex = NULL;
	SDL_Texture* xnumber5_tex = NULL;
	SDL_Texture* xnumber6_tex = NULL;
	SDL_Texture* xnumber7_tex = NULL;
	SDL_Texture* xnumber8_tex = NULL;
	SDL_Texture* xnumber9_tex = NULL;

	SDL_Texture* DryRock_tex = NULL;
	SDL_Texture* Liquid_tex = NULL;
	SDL_Texture* Ice_tex = NULL;
	SDL_Texture* Crust_tex = NULL;

	SDL_Texture* temp_tex = NULL;
	SDL_Texture* hydr_tex = NULL;
	SDL_Texture* k_tex = NULL;
	SDL_Texture* grid_tex = NULL;
	SDL_Texture* structure_tex = NULL;
	SDL_Texture* hold_tracks_tex = NULL;
	SDL_Texture* hydr_title_tex = NULL;
	SDL_Texture* k_title_tex = NULL;
	SDL_Texture* play_tex = NULL;
	SDL_Texture* stop_tex = NULL;
	SDL_Texture* prev_tex = NULL;
	SDL_Texture* next_tex = NULL;
	SDL_Texture* legend_tex = NULL;

	File2tex("Graphics/BG/BG.001.png", &background_tex, path);
	File2surf("Graphics/Transparent.png", &progress_bar, path);
	File2surf("Graphics/Transparent520.png", &value_time, path);
	File2tex("Graphics/Thermal/DryRock.png", &DryRock_tex, path);
	File2tex("Graphics/Thermal/Liquid.png", &Liquid_tex, path);
	File2tex("Graphics/Thermal/Ice.png", &Ice_tex, path);
	File2tex("Graphics/Thermal/Crust.png", &Crust_tex, path);
	File2tex("Graphics/Thermal/temp.png", &temp_tex, path);
	File2tex("Graphics/Thermal/hydr.png", &hydr_tex, path);
	File2tex("Graphics/Thermal/k.png", &k_tex, path);
	File2tex("Graphics/Thermal/grid.png", &grid_tex, path);
	File2tex("Graphics/Thermal/structure.png", &structure_tex, path);
	File2tex("Graphics/Thermal/hold_tracks.png", &hold_tracks_tex, path);
	File2tex("Graphics/Thermal/hydr_title.png", &hydr_title_tex, path);
	File2tex("Graphics/Thermal/k_title.png", &k_title_tex, path);
	File2tex("Graphics/Thermal/legend.png", &legend_tex, path);

	// Don't forget to destroy all window, renderers, and textures at the end.

	for (t=0;t<NT_output;t++) {
		for (r=0;r<NR;r++) {
			TempK[t][r] = thoutput[r][t].tempk;
			Hydr[t][r] = thoutput[r][t].famor*100.0;
			Kappa[t][r] = thoutput[r][t].nu;
		}
	}

	white = SDL_MapRGBA(value_time->format, 255, 255, 255, 255);
	alpha = SDL_MapRGBA(value_time->format, 255, 255, 255, 0);   // r,g,b,alpha 0 to 255. Alpha of 0 is transparent

//-------------------------------------------------------------------
//                Set static elements using crack output
//-------------------------------------------------------------------

	if (Tmax_input == 0) {
		for (t=0;t<NT_output;t++) {
			for (r=0;r<NR;r++) {
				if (thoutput[r][t].tempk > Tmax) Tmax = thoutput[r][t].tempk;
			}
		}
	}
	else Tmax = Tmax_input;

	for (t=0;t<NT_output;t++) {
		for (r=0;r<NR;r++) {
			if (thoutput[r][t].nu > kappamax) kappamax = thoutput[r][t].nu;
		}
	}

	// Axis numbers (x-axis only; y-axis is dynamic)
	ynumber0_tex = renderText("      0",FontFile, axisTextColor, 16, renderer);
	char nb[10];

	xnumber0_tex = renderText("0",FontFile, axisTextColor, 16, renderer);
	for (irad=50;irad<2000;irad=irad+50) {
		if (thoutput[NR-1][0].radius > (double) irad*5.0 && thoutput[NR-1][0].radius <= (double) irad*5.0+250.0) {
			sprintf(nb, "%d", irad);         // No need to right-justify, center-justified by default
			xnumber1_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			sprintf(nb, "%d", irad*2);
			xnumber2_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			sprintf(nb, "%d", irad*3);
			xnumber3_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			sprintf(nb, "%d", irad*4);
			xnumber4_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			sprintf(nb, "%d", irad*5);
			xnumber5_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			sprintf(nb, "%d", irad*6);
			if ((double) irad*6.0 < thoutput[NR-1][0].radius) xnumber6_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			sprintf(nb, "%d", irad*7);
			if ((double) irad*7.0 < thoutput[NR-1][0].radius) xnumber7_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			sprintf(nb, "%d", irad*8);
			if ((double) irad*8.0 < thoutput[NR-1][0].radius) xnumber8_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			sprintf(nb, "%d", irad*9);
			if ((double) irad*9.0 < thoutput[NR-1][0].radius) xnumber9_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			break;
		}
	}

	// Progress bar
	for (t=21;t<780;t++) {
		for (r=551;r<566;r++) {
			pixmem32 = (Uint32*) progress_bar->pixels + r*progress_bar->w + t;
			*pixmem32 = SDL_MapRGBA(progress_bar->format, 255, floor(200*(t-21)/(780-21))+55, 55, 255);
		}
	}
	progress_bar_tex = SDL_CreateTextureFromSurface(renderer, progress_bar);
	SDL_FreeSurface(progress_bar);

//-------------------------------------------------------------------
//                      Interactive display
//-------------------------------------------------------------------

	SDL_Event e;
	t = NT_output-1;          // Initialize at the end of the simulation
	int t_memory = t;
	int t_init = 0;
	int stop_clicked = 0;

	while (!(*quit) && (*view) == 1){
		while (SDL_PollEvent(&e)){

			if (e.type == SDL_QUIT) (*quit) = 1; // Close window
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				// Handle click (except play, switch view, and % bar)
				handleClickThermal(e, &t, &t_init, &grid, &structure, &hold_tracks, &plot_switch,
						&t_memory, NT_output);

				// Switch view
				if (e.button.x >= 70 && e.button.x <= 119 && e.button.y >= 575 && e.button.y <= 599) {
					(*view) = 2;
					return 1;
				}

				// Play - Stop
				if (e.button.x >= 20 && e.button.x <= 68 && e.button.y >= 511 && e.button.y <= 539) {
					for (t=t_init;t<NT_output;t++) {

						stop_clicked = 0;

						SDL_PollEvent(&e);
						if (e.type == SDL_MOUSEBUTTONDOWN) {
							// If press stop
							if (e.button.x >= 76 && e.button.x <= 124 && e.button.y >= 511 && e.button.y <= 539) {
								t_memory = t; // Memorize where we stopped
								t_init = t;   // To start where we left off if we play again
								t = NT_output;       // Exit for loop
								stop_clicked = 1;
								break;
							}
							// If switch view
							else if (e.button.x >= 70 && e.button.x <= 119 && e.button.y >= 575 && e.button.y <= 599) {
								(*view) = 2;
								return 1;
							}
							// Otherwise
							handleClickThermal(e, &t, &t_init, &grid, &structure, &hold_tracks, &plot_switch,
									&t_memory, NT_output);
						}
						// Update displays
						UpdateDisplays(renderer, background_tex, FontFile, thoutput, TempK, Hydr, Kappa,
								structure, grid, hold_tracks, plot_switch, t, NR, NT_output, ircore, Tmax, kappamax,
								value_time, axisTextColor, alpha,
								DryRock_tex, Liquid_tex, Ice_tex, Crust_tex,
								progress_bar_tex, irad,
								xnumber0_tex, xnumber1_tex, xnumber2_tex, xnumber3_tex,
								xnumber4_tex, xnumber5_tex, xnumber6_tex, xnumber7_tex,
								xnumber8_tex, xnumber9_tex, ynumber0_tex,
								temp_tex, hydr_tex, k_tex, grid_tex, structure_tex, hold_tracks_tex,
								hydr_title_tex, k_title_tex, legend_tex);
					}
					if (stop_clicked == 1) t = t_memory;
					else t = NT_output-1, t_init = 0;
				}

				// Click on % bar to adjust time or scroll
				if (e.button.x >= 20 && e.button.x <= 780 && e.button.y >= 550 && e.button.y <= 567) {
					t = floor(((double) e.button.x - 20.0)/(780.0-20.0)*500.0);

					// While mouse button is down, scroll
					while (e.type != SDL_MOUSEBUTTONUP) {
						SDL_PollEvent(&e);

						// Do not change t past the x edges of the bar. The y limits are to avoid the program
						// crashing because we're out of the window.
						if ((double) e.button.x + e.motion.xrel >= 20 && (double) e.button.x + e.motion.xrel < 780
								&& (double) e.button.y + e.motion.yrel > 0 && (double) e.button.y + e.motion.yrel < SCREEN_HEIGHT) {

							// Adjust displays
							t = floor(((double) e.button.x + e.motion.xrel - 20.0)/(780.0-20.0)*NT_output);

							// Update displays
							UpdateDisplays(renderer, background_tex, FontFile, thoutput, TempK, Hydr, Kappa,
									structure, grid, hold_tracks, plot_switch, t, NR, NT_output, ircore, Tmax, kappamax,
									value_time, axisTextColor, alpha,
									DryRock_tex, Liquid_tex, Ice_tex, Crust_tex,
									progress_bar_tex, irad,
									xnumber0_tex, xnumber1_tex, xnumber2_tex, xnumber3_tex,
									xnumber4_tex, xnumber5_tex, xnumber6_tex, xnumber7_tex,
									xnumber8_tex, xnumber9_tex, ynumber0_tex,
									temp_tex, hydr_tex, k_tex, grid_tex, structure_tex, hold_tracks_tex,
									hydr_title_tex, k_title_tex, legend_tex);
						}
					}
					t_init = t;  // To pick up the animation back where we're leaving off
				}
			}
		}

		// Update displays
		UpdateDisplays(renderer, background_tex, FontFile, thoutput, TempK, Hydr, Kappa,
				structure, grid, hold_tracks, plot_switch, t, NR, NT_output, ircore, Tmax, kappamax,
				value_time, axisTextColor, alpha,
				DryRock_tex, Liquid_tex, Ice_tex, Crust_tex,
				progress_bar_tex, irad,
				xnumber0_tex, xnumber1_tex, xnumber2_tex, xnumber3_tex,
				xnumber4_tex, xnumber5_tex, xnumber6_tex, xnumber7_tex,
				xnumber8_tex, xnumber9_tex, ynumber0_tex,
				temp_tex, hydr_tex, k_tex, grid_tex, structure_tex, hold_tracks_tex,
				hydr_title_tex, k_title_tex, legend_tex);
	}

	//-------------------------------------------------------------------
	//                      Free remaining mallocs
	//-------------------------------------------------------------------

	SDL_DestroyTexture(background_tex);
	SDL_DestroyTexture(progress_bar_tex);
	SDL_FreeSurface(value_time);
	SDL_DestroyTexture(ynumber0_tex);
	SDL_DestroyTexture(xnumber0_tex);
	SDL_DestroyTexture(xnumber1_tex);
	SDL_DestroyTexture(xnumber2_tex);
	SDL_DestroyTexture(xnumber3_tex);
	SDL_DestroyTexture(xnumber4_tex);
	SDL_DestroyTexture(xnumber5_tex);
	SDL_DestroyTexture(xnumber6_tex);
	SDL_DestroyTexture(xnumber7_tex);
	SDL_DestroyTexture(xnumber8_tex);
	SDL_DestroyTexture(xnumber9_tex);
	SDL_DestroyTexture(DryRock_tex);
	SDL_DestroyTexture(Liquid_tex);
	SDL_DestroyTexture(Ice_tex);
	SDL_DestroyTexture(Crust_tex);
	SDL_DestroyTexture(temp_tex);
	SDL_DestroyTexture(hydr_tex);
	SDL_DestroyTexture(k_tex);
	SDL_DestroyTexture(grid_tex);
	SDL_DestroyTexture(structure_tex);
	SDL_DestroyTexture(hold_tracks_tex);
	SDL_DestroyTexture(hydr_title_tex);
	SDL_DestroyTexture(k_title_tex);
	SDL_DestroyTexture(play_tex);
	SDL_DestroyTexture(stop_tex);
	SDL_DestroyTexture(prev_tex);
	SDL_DestroyTexture(next_tex);

	for (t=0;t<NT_output;t++) {
		free (TempK[t]);
		free (Hydr[t]);
		free (Kappa[t]);
	}
	free (TempK);
	free (Hydr);
	free(Kappa);

	return 0;
}

int StructurePlot (SDL_Renderer* renderer, thermalout **thoutput, int t, int NR,
		SDL_Texture* DryRock_tex, SDL_Texture* Liquid_tex, SDL_Texture* Ice_tex, SDL_Texture* Crust_tex,
		SDL_Rect temp_time_dilation) {
	int r = 0;

	SDL_Rect DryRock_clip;
	SDL_Rect DryRock_dest;
	int DryRock_x = 0;
	int DryRock_w = 0;

	SDL_Rect Liquid_clip;
	SDL_Rect Liquid_dest;
	int Liquid_x = 0;
	int Liquid_w = 0;

	SDL_Rect Ice_clip;
	SDL_Rect Ice_dest;
	int Ice_x = 0;
	int Ice_w = 0;

	SDL_Rect Crust_clip;
	SDL_Rect Crust_dest;
	int Crust_x = 0;
	int Crust_w = 0;

	// Dry rock
	for (r=0;r<NR-2;r++) {
		if (thoutput[r][t].mrock <= 0.0 && thoutput[r+1][t].mrock > 0.0 && thoutput[r+2][t].mh2os <= 0.0
				&& thoutput[r+2][t].madhs <= 0.0 && thoutput[r+2][t].mh2ol <= 0.0 && thoutput[r+2][t].mnh3l <= 0.0) {
			DryRock_x = r;
			break;
		}
	}
	for (r=0;r<NR-2;r++) {
		if (thoutput[r][t].mrock > 0.0 && thoutput[r+2][t].mrock <= 0.0 && thoutput[r][t].mh2os <= 0.0
				&& thoutput[r][t].madhs <= 0.0 && thoutput[r][t].mh2ol <= 0.0 && thoutput[r][t].mnh3l <= 0.0) {
			DryRock_w = r - DryRock_x;
			break;
		}
	}
	SDL_QueryTexture(DryRock_tex, NULL, NULL, &DryRock_clip.w, &DryRock_clip.h);
	DryRock_clip.x = 0, DryRock_clip.y = 0;
	DryRock_dest.x = temp_time_dilation.x + floor((double) DryRock_x/(double) NR*(double) temp_time_dilation.w), DryRock_dest.y = temp_time_dilation.y;
	DryRock_dest.w = floor((double) DryRock_w/(double) NR*(double) temp_time_dilation.w), DryRock_dest.h = temp_time_dilation.h;
	SDL_RenderCopy(renderer, DryRock_tex, &DryRock_clip, &DryRock_dest);

	// Liquid
	for (r=0;r<NR-1;r++) {
		if (thoutput[r][t].mh2ol <= 0.0 && thoutput[r+1][t].mh2ol > 0.0) {
			Liquid_x = r;
			break;
		}
	}
	for (r=0;r<NR-1;r++) {
		if (thoutput[r][t].mh2ol > 0.0 && thoutput[r+1][t].mh2ol <= 0.0) {
			Liquid_w = r - Liquid_x;
			break;
		}
	}
	SDL_QueryTexture(Liquid_tex, NULL, NULL, &Liquid_clip.w, &Liquid_clip.h);
	Liquid_clip.x = 0, Liquid_clip.y = 0;
	Liquid_dest.x = temp_time_dilation.x + floor((double) Liquid_x/(double) NR*(double) temp_time_dilation.w), Liquid_dest.y = temp_time_dilation.y;
	Liquid_dest.w = floor((double) Liquid_w/(double) NR*(double) temp_time_dilation.w), Liquid_dest.h = temp_time_dilation.h;
	SDL_RenderCopy(renderer, Liquid_tex, &Liquid_clip, &Liquid_dest);

	// Ice
	for (r=0;r<NR-2;r++) {
		if (thoutput[r][t].mh2os <= 0.0 && thoutput[r+1][t].mh2os > 0.0 && thoutput[r+2][t].mrock <= 0.0
				&& thoutput[r+2][t].madhs <= 0.0 && thoutput[r+2][t].mh2ol <= 0.0 && thoutput[r+2][t].mnh3l <= 0.0) {
			Ice_x = r + 1;
			break;
		}
	}
	for (r=0;r<NR-1;r++) {
		if (thoutput[r][t].mh2os > 0.0 && thoutput[r+1][t].mh2os <= thoutput[r][t].mh2os) {
			Ice_w = r - Ice_x + 1;
			break;
		}
	}
	SDL_QueryTexture(Ice_tex, NULL, NULL, &Ice_clip.w, &Ice_clip.h);
	Ice_clip.x = 0, Ice_clip.y = 0;
	Ice_dest.x = temp_time_dilation.x + floor((double) Ice_x/(double) NR*(double) temp_time_dilation.w), Ice_dest.y = temp_time_dilation.y;
	Ice_dest.w = floor((double) Ice_w/(double) NR*(double) temp_time_dilation.w), Ice_dest.h = temp_time_dilation.h;
	SDL_RenderCopy(renderer, Ice_tex, &Ice_clip, &Ice_dest);

	// Crust
	for (r=0;r<NR;r++) {
		if (thoutput[r][t].mrock > 0.0 && thoutput[r][t].mh2os > 0.0 && thoutput[r][t].madhs > 0.0
				&& thoutput[r][t].mh2ol <= 0.0 && thoutput[r][t].mnh3l <= 0.0) {
			Crust_x = r;
			break;
		}
	}
	Crust_w = NR - r;
	SDL_QueryTexture(Crust_tex, NULL, NULL, &Crust_clip.w, &Crust_clip.h);
	Crust_clip.x = 0, Crust_clip.y = 0;
	Crust_dest.x = temp_time_dilation.x + floor((double) Crust_x/(double) NR*(double) temp_time_dilation.w), Crust_dest.y = temp_time_dilation.y;
	Crust_dest.w = floor((double) Crust_w/(double) NR*(double) temp_time_dilation.w), Crust_dest.h = temp_time_dilation.h;
	SDL_RenderCopy(renderer, Crust_tex, &Crust_clip, &Crust_dest);

	return 0;
}

int TwoAxisPlot (SDL_Renderer* renderer, char *FontFile, double **Values, int grid, int hold_tracks, int t, int NR, int irmax,
		int itempk_step, int itempk_max, double itempk_fold_min, double itempk_fold_max, double itempk_fold_step, int thickness,
		double Tmax, SDL_Surface* value_time, SDL_Rect value_time_clip, SDL_Rect* value_time_dilation,
		SDL_Color axisTextColor, Uint32 alpha) {

	SDL_Texture* value_time_tex;
	SDL_Texture* ynumber1_tex = NULL;
	SDL_Texture* ynumber2_tex = NULL;
	SDL_Texture* ynumber3_tex = NULL;
	SDL_Texture* ynumber4_tex = NULL;
	SDL_Texture* ynumber5_tex = NULL;
	SDL_Texture* ynumber6_tex = NULL;
	SDL_Texture* ynumber7_tex = NULL;
	SDL_Texture* ynumber8_tex = NULL;
	SDL_Texture* ynumber9_tex = NULL;
	SDL_Texture* ynumber10_tex = NULL;
	SDL_Texture* ynumber11_tex = NULL;

	int T = 0;
	int r = 0;
	int itempk = 0;
	char nb[10];
	Uint32 *pixmem32;
	int Tmax_int = 0;
	int xoffset = 0;
	int yoffset = 0;

	Tmax_int = (int) Tmax + 1;

	// y-axis numbers
	for (itempk=itempk_step;itempk<itempk_max;itempk=itempk+itempk_step) {
		if (Tmax > (double) itempk*itempk_fold_min && Tmax <= (double) itempk*itempk_fold_max+itempk_fold_step) {
			scanNumber(&nb, itempk);          // Right-justified
			ynumber1_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*2);
			ynumber2_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*3);
			ynumber3_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*4);
			ynumber4_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*5);
			ynumber5_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*6);
			ynumber6_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*7);
			if ((double) itempk*7.0 < Tmax) ynumber7_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*8);
			if ((double) itempk*8.0 < Tmax) ynumber8_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*9);
			if ((double) itempk*9.0 < Tmax) ynumber9_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*10);
			if ((double) itempk*10.0 < Tmax) ynumber10_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			scanNumber(&nb, itempk*11);
			if ((double) itempk*11.0 < Tmax) ynumber11_tex = renderText(nb,FontFile, axisTextColor, 16, renderer);
			break;
		}
	}

	// Temperature-time plot
	if (!hold_tracks) {
		for (T=0;T<value_time->h;T++) {
			for (r=0;r<value_time->w;r++) {
				pixmem32 = (Uint32*) value_time->pixels + (value_time->h - T-1)*value_time->w + r+1;
				*pixmem32 = alpha;
			}
		}
	}
	for (T=0;T<=Tmax_int;T++) {
		for (r=0;r<irmax;r++) {
			// Temperature grid every i/2.0 K
			if (grid) {
				if ((double) T/((double) itempk/2.0) - (double) floor((double) T/((double) itempk/2.0)) <= 0.1) {
					xoffset = floor((double)r/(double)NR*value_time->w);
					yoffset = floor((double)T/(double)Tmax_int*value_time->h);
					pixmem32 = (Uint32*) value_time->pixels + (value_time->h - yoffset)*value_time->w + xoffset;
					*pixmem32 = SDL_MapRGBA(value_time->format, 255, 255, 255, 20);
				}
			}
			// Temperature profile yellower when more recent, redder when more ancient
			if (T >= (int) Values[t][r] - thickness && T <= (int) Values[t][r] + thickness) {
				xoffset = floor((double)r/(double)NR*value_time->w);
				yoffset = floor((double)T/(double)Tmax_int*value_time->h);
				pixmem32 = (Uint32*) value_time->pixels + (value_time->h - yoffset)*value_time->w + xoffset;
				*pixmem32 = SDL_MapRGBA(value_time->format, 255, floor(200*(t-21)/(780-21))+55, 55, 255);
			}
		}
	}
	value_time_tex = SDL_CreateTextureFromSurface(renderer, value_time);

	value_time_clip.x = 0, value_time_clip.y = 0;
	value_time_clip.w = value_time->w, value_time_clip.h = value_time->h;
	(*value_time_dilation).x = 90, (*value_time_dilation).y = 87;
	(*value_time_dilation).w = value_time->w, (*value_time_dilation).h = value_time->h;
	SDL_RenderCopy(renderer, value_time_tex, &value_time_clip, &(*value_time_dilation));

	renderTexture(ynumber1_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk / Tmax*(double) (*value_time_dilation).h)));
	renderTexture(ynumber2_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*2.0 / Tmax*(double) (*value_time_dilation).h)));
	renderTexture(ynumber3_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*3.0 / Tmax*(double) (*value_time_dilation).h)));
	renderTexture(ynumber4_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*4.0 / Tmax*(double) (*value_time_dilation).h)));
	renderTexture(ynumber5_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*5.0 / Tmax*(double) (*value_time_dilation).h)));
	renderTexture(ynumber6_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*6.0 / Tmax*(double) (*value_time_dilation).h)));
	if ((double) itempk*7.0 < Tmax)
		renderTexture(ynumber7_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*7.0 / Tmax*(double) (*value_time_dilation).h)));
	if ((double) itempk*8.0 < Tmax)
		renderTexture(ynumber8_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*8.0 / Tmax*(double) (*value_time_dilation).h)));
	if ((double) itempk*9.0 < Tmax)
		renderTexture(ynumber9_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*9.0 / Tmax*(double) (*value_time_dilation).h)));
	if ((double) itempk*10.0 < Tmax)
		renderTexture(ynumber10_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*10.0 / Tmax*(double) (*value_time_dilation).h)));
	if ((double) itempk*11.0 < Tmax)
		renderTexture(ynumber11_tex, renderer, 50, -8 + (*value_time_dilation).y + floor((double) (*value_time_dilation).h-((double) itempk*11.0 / Tmax*(double) (*value_time_dilation).h)));

	SDL_DestroyTexture(value_time_tex);

	SDL_DestroyTexture(ynumber1_tex);
	SDL_DestroyTexture(ynumber2_tex);
	SDL_DestroyTexture(ynumber3_tex);
	SDL_DestroyTexture(ynumber4_tex);
	SDL_DestroyTexture(ynumber5_tex);
	SDL_DestroyTexture(ynumber6_tex);
	SDL_DestroyTexture(ynumber7_tex);
	SDL_DestroyTexture(ynumber8_tex);
	SDL_DestroyTexture(ynumber9_tex);
	SDL_DestroyTexture(ynumber10_tex);
	SDL_DestroyTexture(ynumber11_tex);

	return 0;
}

int UpdateDisplays (SDL_Renderer* renderer, SDL_Texture* background_tex, char* FontFile, thermalout **thoutput,
		double **TempK, double **Hydr, double **Kappa, int structure, int grid, int hold_tracks, int plot_switch,
		int t, int NR, int NT_output, int ircore, double Tmax, double kappamax, SDL_Surface* value_time,
		SDL_Color axisTextColor, Uint32 alpha,
		SDL_Texture* DryRock_tex, SDL_Texture* Liquid_tex, SDL_Texture* Ice_tex, SDL_Texture* Crust_tex,
		SDL_Texture* progress_bar_tex, int irad,
		SDL_Texture* xnumber0_tex, SDL_Texture* xnumber1_tex, SDL_Texture* xnumber2_tex, SDL_Texture* xnumber3_tex,
		SDL_Texture* xnumber4_tex, SDL_Texture* xnumber5_tex, SDL_Texture* xnumber6_tex, SDL_Texture* xnumber7_tex,
		SDL_Texture* xnumber8_tex, SDL_Texture* xnumber9_tex, SDL_Texture* ynumber0_tex,
		SDL_Texture* temp_tex, SDL_Texture* hydr_tex, SDL_Texture* k_tex,
		SDL_Texture* grid_tex, SDL_Texture* structure_tex, SDL_Texture* hold_tracks_tex,
		SDL_Texture* hydr_title_tex, SDL_Texture* k_title_tex, SDL_Texture* legend_tex) {

	int r = 0;
	double percent = 0.0; // % of history, 4.56 Gyr = 100%
	char nb[10];

	SDL_Texture* elapsed_time = NULL;                    // Time elapsed
	SDL_Texture* elapsed_percent = NULL;                  // % history elapsed

	SDL_Rect progress_bar_clip;        // Section of the image to clip
	SDL_Rect progress_bar_dilation;    // Resized and repositioned clip
	SDL_Rect value_time_clip;
	SDL_Rect value_time_dilation;

	SDL_RenderClear(renderer);
	ApplySurface(0, 0, background_tex, renderer, NULL);

	// Main plot
	switch (plot_switch) {
	case 1:  // Hydration-time plot
		for (r=0;r<NR-1;r++) {
			if (thoutput[r][t].mrock > 0.0 && thoutput[r+1][t].mrock == 0.0) ircore = r;
		}
		TwoAxisPlot(renderer, FontFile, Hydr, grid, hold_tracks, t, NR, ircore,
				10, 11, 9.0, 9.0, 20.0, 1,
				101, value_time, value_time_clip, &value_time_dilation,
				axisTextColor, alpha);
		break;
	case 2:  // Thermal conductivity-time plot
		TwoAxisPlot(renderer, FontFile, Kappa, grid, hold_tracks, t, NR, NR,
				1, 100, 1.0, 6.0, 6.0, 0,
				kappamax, value_time, value_time_clip, &value_time_dilation,
				axisTextColor, alpha);
		break;
	default: // Temperature-time plot
		TwoAxisPlot(renderer, FontFile, TempK, grid, hold_tracks, t, NR, NR,
				50, 1500, 6.0, 6.0, 300.0, 4,
				Tmax, value_time, value_time_clip, &value_time_dilation,
				axisTextColor, alpha);
	}

	// Structure plot
	if (structure == 1) {
		StructurePlot(renderer, thoutput, t, NR, DryRock_tex, Liquid_tex, Ice_tex, Crust_tex, value_time_dilation);
	}

	// Unveil the progress bar
	progress_bar_clip.x = 21, progress_bar_clip.y = 551;
	progress_bar_clip.w = floor((780.0-21.0)*t/NT_output), progress_bar_clip.h = 15;
	progress_bar_dilation.x = 21, progress_bar_dilation.y = 551;
	progress_bar_dilation.w = floor((780.0-21.0)*t/NT_output), progress_bar_dilation.h = 15;
	SDL_RenderCopy(renderer, progress_bar_tex, &progress_bar_clip, &progress_bar_dilation);

	// Time elapsed
	sprintf(nb, "%.2f", t/100.0);
	elapsed_time = renderText(nb,FontFile, axisTextColor, 18, renderer);
	renderTexture(elapsed_time, renderer, 629, 502);

	// % history elapsed
	percent = t/4.56;
	sprintf(nb, "%.0f", percent);
	elapsed_percent = renderText(nb,FontFile, axisTextColor, 18, renderer);
	renderTexture(elapsed_percent, renderer, 636, 527);

	// Transient items
	switch (plot_switch) {
	case 1:
		ApplySurface(691, 162, hydr_tex, renderer, NULL);
		ApplySurface(22, 150, hydr_title_tex, renderer, NULL);
		break;
	case 2:
		ApplySurface(738, 157, k_tex, renderer, NULL);
		ApplySurface(22, 135, k_title_tex, renderer, NULL);
		break;
	default:
		ApplySurface(648, 162, temp_tex, renderer, NULL);
	}
	if (grid == 1) ApplySurface(648, 262, grid_tex, renderer, NULL);
	if (structure == 1) {
		ApplySurface(648, 338, structure_tex, renderer, NULL);
		ApplySurface(644, 65, legend_tex, renderer, NULL);
	}
	if (hold_tracks == 1) ApplySurface(648, 411, hold_tracks_tex, renderer, NULL);

	// Other renderings
	renderTexture(ynumber0_tex, renderer, 50, -8 + value_time_dilation.y + value_time_dilation.h);

	renderTexture(xnumber0_tex, renderer, -5 + value_time_dilation.x, 445);
	renderTexture(xnumber1_tex, renderer, -5 + value_time_dilation.x + floor((double) irad / thoutput[NR-1][0].radius*(double) value_time_dilation.w), 445);
	renderTexture(xnumber2_tex, renderer, -5 + value_time_dilation.x + floor((double) irad*2.0 / thoutput[NR-1][0].radius*(double) value_time_dilation.w), 445);
	renderTexture(xnumber3_tex, renderer, -5 + value_time_dilation.x + floor((double) irad*3.0 / thoutput[NR-1][0].radius*(double) value_time_dilation.w), 445);
	renderTexture(xnumber4_tex, renderer, -5 + value_time_dilation.x + floor((double) irad*4.0 / thoutput[NR-1][0].radius*(double) value_time_dilation.w), 445);
	renderTexture(xnumber5_tex, renderer, -5 + value_time_dilation.x + floor((double) irad*5.0 / thoutput[NR-1][0].radius*(double) value_time_dilation.w), 445);
	renderTexture(xnumber6_tex, renderer, -5 + value_time_dilation.x + floor((double) irad*6.0 / thoutput[NR-1][0].radius*(double) value_time_dilation.w), 445);
	renderTexture(xnumber7_tex, renderer, -5 + value_time_dilation.x + floor((double) irad*7.0 / thoutput[NR-1][0].radius*(double) value_time_dilation.w), 445);
	renderTexture(xnumber8_tex, renderer, -5 + value_time_dilation.x + floor((double) irad*8.0 / thoutput[NR-1][0].radius*(double) value_time_dilation.w), 445);
	renderTexture(xnumber9_tex, renderer, -5 + value_time_dilation.x + floor((double) irad*9.0 / thoutput[NR-1][0].radius*(double) value_time_dilation.w), 445);

	SDL_RenderPresent(renderer);

	SDL_DestroyTexture(elapsed_time);
	SDL_DestroyTexture(elapsed_percent);

	SDL_Delay(16);

	return 0;
}

int handleClickThermal(SDL_Event e, int *t, int *t_init, int *grid, int *structure, int *hold_tracks, int *plot_switch,
		int *t_memory, int NT_output) {

	// Advance forward/backward one frame at a time
	if (e.button.x >= 132 && e.button.x <= 180 && e.button.y >= 511 && e.button.y <= 539 && (*t)>0) {
		(*t)--;
		(*t_init) = (*t);
	}
	if (e.button.x >= 188 && e.button.x <= 236 && e.button.y >= 511 && e.button.y <= 539 && (*t)<NT_output-1) {
		(*t)++;
		(*t_init) = (*t);
	}

	// Make grid appear or disappear
	if (e.button.x >= 648 && e.button.x <= 764 && e.button.y >= 262 && e.button.y <= 327) {
		if ((*grid) == 1) (*grid) = 0;
		else (*grid) = 1;
	}

	// Make structure appear or disappear
	if (e.button.x >= 648 && e.button.x <= 764 && e.button.y >= 338 && e.button.y <= 400) {
		if ((*structure) == 1) (*structure) = 0;
		else (*structure) = 1;
	}

	// Hold tracks switch
	if (e.button.x >= 648 && e.button.x <= 764 && e.button.y >= 411 && e.button.y <= 458) {
		if ((*hold_tracks) == 1) (*hold_tracks) = 0;
		else (*hold_tracks) = 1;
	}

	// Main plot switch
	if (e.button.x >= 648 && e.button.x <= 679 && e.button.y >= 162 && e.button.y <= 227) {
		(*plot_switch) = 0; // Temperature
	}
	if (e.button.x >= 691 && e.button.x <= 732 && e.button.y >= 162 && e.button.y <= 227) {
		(*plot_switch) = 1; // Hydration
	}
	if (e.button.x >= 738 && e.button.x <= 778 && e.button.y >= 162 && e.button.y <= 227) {
		(*plot_switch) = 2; // Thermal conductivity
	}

	// If click on the bar
	else if (e.button.x >= 20 && e.button.x <= 780 && e.button.y >= 550 && e.button.y <= 567) {
		(*t) = floor(((double) e.button.x - 20.0)/(780.0-20.0)*500.0);
	}

	return 0;
}

#endif /* THERMAL_PLOT_H_ */
