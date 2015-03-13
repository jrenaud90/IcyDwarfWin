/*
 * ParamExploration_plot.h
 *
 *  Created on: Mar 10, 2015
 *      Author: Marc Neveu (mneveu@asu.edu)
 */

#ifndef PARAMEXPLORATION_PLOT_H_
#define PARAMEXPLORATION_PLOT_H_

#include "../Graphics/Plot.h"
#include <math.h>

int ParamExploration_plot (char path[1024],	int warnings, int msgout, SDL_Renderer* renderer, int* view, int* quit, char* FontFile,
		SDL_Color axisTextColor, double Tmin, double Tmax, double Tstep, double Pmin, double Pmax, double Pstep,
		double pHmin, double pHmax, double pHstep, double pemin, double pemax, double pestep, double WRmin, double WRmax, double WRstep);

int handleClickParamExploration(SDL_Event e, int *itemp, int *ipressure, int *itopic, int ntemp, int npressure, SDL_Surface **pies);

int UpdateDisplaysParamExploration (SDL_Renderer* renderer, SDL_Texture* background_tex, SDL_Texture* pies_tex,
		SDL_Texture* legend1_tex, SDL_Texture* legend2_tex, SDL_Texture* legend3_tex, SDL_Texture* legend4_tex,
		SDL_Texture* legend5_tex, SDL_Texture* legend6_tex, SDL_Texture* legend7_tex, SDL_Texture* legend8_tex,
		char* FontFile, int nspecies);

int Angles (int itopic, SDL_Surface **pies, char *FontFile, int npH, int npe, int nWR, int ntemp, int itemp, int ipressure,
		double pie_radius, double **simdata, int *nspecies,
		SDL_Texture **legend1_tex, SDL_Texture **legend2_tex, SDL_Texture **legend3_tex, SDL_Texture **legend4_tex,
		SDL_Texture **legend5_tex, SDL_Texture **legend6_tex, SDL_Texture **legend7_tex, SDL_Texture **legend8_tex);

int Pie(double angle, double angle_start, int iWR, int ipH, int ipe, double pie_radius, SDL_Surface **pies, SDL_Color color);

int ParamExploration_plot (char path[1024],	int warnings, int msgout, SDL_Renderer* renderer, int* view, int* quit, char* FontFile,
		SDL_Color axisTextColor, double Tmin, double Tmax, double Tstep, double Pmin, double Pmax, double Pstep,
		double pHmin, double pHmax, double pHstep, double pemin, double pemax, double pestep, double WRmin, double WRmax, double WRstep) {

	int i = 0;
	int j = 0;
	int nspecies = 0;
	int nvar = 1000;
	int nsim = 0;
	int ntemp = 0;                                               // Number of different temperatures in output file
	int npressure = 0;                                           // Number of different pressures in output file
	int npH = 0;                                                 // Number of different pH in output file
	int npe = 0;                                                 // Number of different pe in output file
	int nWR = 0;                                                 // Number of different water:rock ratios in output file
	int itemp = 0;                                           	 // Rank of temperature in output file
	int ipressure = 0;                                     	     // Rank of pressure in output file
	int itopic = 0;                                              // Topic addressed (radionuclides, antifreezes, etc.)
	SDL_Texture* background_tex = NULL;
	SDL_Texture* pies_tex = NULL;        // Pies
	SDL_Surface* pies = NULL;
	SDL_Texture* legend1_tex = NULL;
	SDL_Texture* legend2_tex = NULL;
	SDL_Texture* legend3_tex = NULL;
	SDL_Texture* legend4_tex = NULL;
	SDL_Texture* legend5_tex = NULL;
	SDL_Texture* legend6_tex = NULL;
	SDL_Texture* legend7_tex = NULL;
	SDL_Texture* legend8_tex = NULL;
	double pie_radius = 0.0;

	ntemp = floor((Tmax-Tmin)/Tstep) + 1;
	npressure = floor((Pmax-Pmin)/Pstep) + 1;
	npH = floor((pHmax-pHmin)/pHstep) + 1;
	npe = floor((pemax-pemin)/pestep) + 1;
	nWR = ceil((log(WRmax)-log(WRmin))/log(WRstep)) + 1;

	nsim = ntemp*npressure*npH*npe*nWR;

	double **simdata = (double**) malloc(nsim*sizeof(double*));  // Compilation of data generated by multiple PHREEQC simulations
	if (simdata == NULL) printf("ParamExploration: Not enough memory to create simdata[npHiter]\n");

	for (i=0;i<nsim;i++) {
		simdata[i] = (double*) malloc(nvar*sizeof(double));
		if (simdata[i] == NULL) printf("Thermal: Not enough memory to create simdata[npHiter][nvar]\n");
	}
	for (i=0;i<nsim;i++) {
		for (j=0;j<nvar;j++) {
			simdata[i][j] = 0.0;
		}
	}

	// Load ParamExploration.txt
	read_input (nvar, nsim, &simdata, path, "Outputs/ParamExploration.txt");

	//-------------------------------------------------------------------
	//                     Initialize display elements
	//-------------------------------------------------------------------

	File2tex("Graphics/BG/BG.005.png", &background_tex, path);
	File2surf("Graphics/Transparent.png", &pies, path);

	pie_radius = 13.0;

	//-------------------------------------------------------------------
	//                         Interactive display
	//-------------------------------------------------------------------

	SDL_Event e;

	while (!(*quit) && (*view) == 3){
		while (SDL_PollEvent(&e)){

			if (e.type == SDL_QUIT) (*quit) = 1; // Close window
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				// Handle click: switch temperature, pressure, or species
				handleClickParamExploration(e, &itemp, &ipressure, &itopic, ntemp, npressure, &pies);

				if (itopic > 2) itopic = 2;
				Angles (itopic, &pies, FontFile, npH, npe, nWR, ntemp, itemp, ipressure, pie_radius, simdata, &nspecies,
						&legend1_tex, &legend2_tex, &legend3_tex, &legend4_tex,
						&legend5_tex, &legend6_tex, &legend7_tex, &legend8_tex);

				pies_tex = SDL_CreateTextureFromSurface(renderer, pies);

				// Switch view
				if (e.button.x >= 19 && e.button.x <= 69 && e.button.y >= 575 && e.button.y <= 599) {
					(*view) = 1;
					return 1;
				}
				if (e.button.x >= 70 && e.button.x <= 119 && e.button.y >= 575 && e.button.y <= 599) {
					(*view) = 2;
					return 1;
				}
			}
		}
		// Update displays
		UpdateDisplaysParamExploration(renderer, background_tex, pies_tex, legend1_tex, legend2_tex, legend3_tex, legend4_tex,
				legend5_tex, legend6_tex, legend7_tex, legend8_tex, FontFile, nspecies);
	}

	//-------------------------------------------------------------------
	//                      Free remaining mallocs
	//-------------------------------------------------------------------

	SDL_DestroyTexture(background_tex);
	SDL_FreeSurface(pies);
	SDL_DestroyTexture(pies_tex);
	SDL_DestroyTexture(legend1_tex);
	SDL_DestroyTexture(legend2_tex);
	SDL_DestroyTexture(legend3_tex);
	SDL_DestroyTexture(legend4_tex);
	SDL_DestroyTexture(legend5_tex);
	SDL_DestroyTexture(legend6_tex);
	SDL_DestroyTexture(legend7_tex);
	SDL_DestroyTexture(legend8_tex);
	for (i=0;i<nsim;i++) free (simdata[i]);
	free (simdata);

	return 0;
}

//-------------------------------------------------------------------
//                      Display updating subroutine
//-------------------------------------------------------------------

int UpdateDisplaysParamExploration (SDL_Renderer* renderer, SDL_Texture* background_tex, SDL_Texture* pies_tex,
		SDL_Texture* legend1_tex, SDL_Texture* legend2_tex, SDL_Texture* legend3_tex, SDL_Texture* legend4_tex,
		SDL_Texture* legend5_tex, SDL_Texture* legend6_tex, SDL_Texture* legend7_tex, SDL_Texture* legend8_tex,
		char* FontFile, int nspecies) {

	double theta_legend = 0.0;
	int i = 0;

	SDL_RenderClear(renderer);
	ApplySurface(0, 0, background_tex, renderer, NULL);
	ApplySurface(0, 0, pies_tex, renderer, NULL);

	if (nspecies > 7+1) printf("ParamExploration_plot: UpdateDisplays: too many species for good display of legend\n");
	for (i=0;i<8;i++) {
		theta_legend = 2.0*M_PI/(double)nspecies*((double)i+0.5);
		if (i==0) ApplySurface(405 + 50*cos(theta_legend), 502 + 50*sin(theta_legend), legend1_tex, renderer, NULL);
		if (i==1) ApplySurface(405 + 50*cos(theta_legend), 502 + 50*sin(theta_legend), legend2_tex, renderer, NULL);
		if (i==2) ApplySurface(405 + 50*cos(theta_legend), 502 + 50*sin(theta_legend), legend3_tex, renderer, NULL);
		if (i==3) ApplySurface(405 + 50*cos(theta_legend), 502 + 50*sin(theta_legend), legend4_tex, renderer, NULL);
		if (i==4) ApplySurface(405 + 50*cos(theta_legend), 502 + 50*sin(theta_legend), legend5_tex, renderer, NULL);
		if (i==5) ApplySurface(405 + 50*cos(theta_legend), 502 + 50*sin(theta_legend), legend6_tex, renderer, NULL);
		if (i==6) ApplySurface(405 + 50*cos(theta_legend), 502 + 50*sin(theta_legend), legend7_tex, renderer, NULL);
		if (i==7) ApplySurface(405 + 50*cos(theta_legend), 502 + 50*sin(theta_legend), legend8_tex, renderer, NULL);
	}

	SDL_RenderPresent(renderer);
	SDL_Delay(16);

	return 0;
}

//-------------------------------------------------------------------
//                      Click handling subroutine
//-------------------------------------------------------------------

int handleClickParamExploration(SDL_Event e, int *itemp, int *ipressure, int *itopic, int ntemp, int npressure, SDL_Surface **pies) {

	int xstart = 0; int xend = 0; int ystart = 0; int yend = 0;
	int x = 0; int y = 0;
	Uint32 *pixmem32;

	// Reset screen
	for (x=0;x<(*pies)->w;x++) {
		for (y=0;y<=(*pies)->h;y++) {
			pixmem32 = (Uint32*) (*pies)->pixels + y*(*pies)->w + x;
			*pixmem32 = SDL_MapRGBA((*pies)->format, 0, 0, 0, 0);
		}
	}

	// Change temperature/pressure
	if (e.button.x >= 601 && e.button.x <= 788 && e.button.y >= 438 && e.button.y <= 546) {
		if (e.button.x >= 601 && e.button.x <= 625) {		        // x
			(*itemp) = 0; xstart = 601; xend = 625;
		}
		else if (e.button.x >= 628 && e.button.x <= 652) {
			(*itemp) = 1; xstart = 628; xend = 652;
		}
		else if (e.button.x >= 655 && e.button.x <= 679) {
			(*itemp) = 2; xstart = 655; xend = 679;
		}
		else if (e.button.x >= 682 && e.button.x <= 706) {
			(*itemp) = 3; xstart = 682; xend = 706;
		}
		else if (e.button.x >= 709 && e.button.x <= 733) {
			(*itemp) = 4; xstart = 709; xend = 733;
		}
		else if (e.button.x >= 736 && e.button.x <= 760) {
			(*itemp) = 5; xstart = 736; xend = 760;
		}
		else if (e.button.x >= 763 && e.button.x <= 788) {
			(*itemp) = 6; xstart = 763; xend = 788;
		}

		if (e.button.y >= 438 && e.button.y <= 450) {        		// y
			(*ipressure) = 6; ystart = 438; yend = 450;
		}
		else if (e.button.y >= 455 && e.button.y <= 467) {
			(*ipressure) = 5; ystart = 455; yend = 467;
		}
		else if (e.button.y >= 471 && e.button.y <= 483) {
			(*ipressure) = 4; ystart = 471; yend = 483;
		}
		else if (e.button.y >= 487 && e.button.y <= 499) {
			(*ipressure) = 3; ystart = 487; yend = 499;
		}
		else if (e.button.y >= 503 && e.button.y <= 515) {
			(*ipressure) = 2; ystart = 503; yend = 515;
		}
		else if (e.button.y >= 519 && e.button.y <= 531) {
			(*ipressure) = 1; ystart = 519; yend = 531;
		}
		else if (e.button.y >= 535 && e.button.y <= 546) {
			(*ipressure) = 0; ystart = 535; yend = 546;
		}

		if (*itemp > ntemp-1) *itemp = ntemp - 1;
		if (*ipressure > npressure-1) *ipressure = npressure - 1;

		if (xstart > 0 && ystart > 0) {
			for (x=xstart+1;x<xend;x++) {
				for (y=ystart+1;y<yend;y++) {
					pixmem32 = (Uint32*) (*pies)->pixels + y*(*pies)->w + x;
					*pixmem32 = SDL_MapRGBA((*pies)->format, (230*(1-abs(y-ystart)/12) + 4*230)/5, 150, 0, 255);
				}
			}
		}
	}

	// Change topic
	if (e.button.x >= 4 && e.button.x <= 104 && e.button.y >= 444 && e.button.y <= 491) (*itopic) = 1;       // Potassium
	else if (e.button.x >= 12 && e.button.x <= 105 && e.button.y >= 506 && e.button.y <= 532) (*itopic) = 2;  // NH3
	else if (e.button.x >= 12 && e.button.x <= 105 && e.button.y >= 536 && e.button.y <= 562) (*itopic) = 3;  // Salts
	else if (e.button.x >= 223 && e.button.x <= 321 && e.button.y >= 437 && e.button.y <= 463) (*itopic) = 4; // Total gas
	else if (e.button.x >= 223 && e.button.x <= 321 && e.button.y >= 468 && e.button.y <= 496) (*itopic) = 5; // Gas makeup
	else if (e.button.x >= 255 && e.button.x <= 321 && e.button.y >= 502 && e.button.y <= 528) (*itopic) = 6; // Brucite
	else if (e.button.x >= 255 && e.button.x <= 321 && e.button.y >= 532 && e.button.y <= 558) (*itopic) = 7; // Magnesite
	else if (e.button.x >= 255 && e.button.x <= 321 && e.button.y >= 561 && e.button.y <= 587) (*itopic) = 8; // Mineral makeup
	else if (e.button.x >= 115 && e.button.x <= 210 && e.button.y >= 436 && e.button.y <= 489) (*itopic) = 9; // Solution makeup

	return 0;
}

//-------------------------------------------------------------------
//                    Angle calculation subroutine
//-------------------------------------------------------------------

int Angles (int itopic, SDL_Surface **pies, char *FontFile, int npH, int npe, int nWR, int ntemp, int itemp, int ipressure,
		double pie_radius, double **simdata, int *nspecies,
		SDL_Texture **legend1_tex, SDL_Texture **legend2_tex, SDL_Texture **legend3_tex, SDL_Texture **legend4_tex,
		SDL_Texture **legend5_tex, SDL_Texture **legend6_tex, SDL_Texture **legend7_tex, SDL_Texture **legend8_tex) {

	int i = 0;
	int ipH = 0; // Rank of pH in output file
	int ipe = 0; // Rank of pe in output file
	int iWR = 0; // Rank of water:rock ratio in output file
	int isim = 0;
	double mass_water = 0.0;
	SDL_Color black;
	SDL_Color white;
	SDL_Color red;
	SDL_Color green;
	SDL_Color aqua;
	SDL_Color purple;
	SDL_Color gray;
	SDL_Color yellow;
	SDL_Color orange;
	black.r = 0; black.g = 0; black.b = 0;
	white.r = 255; white.g = 255; white.b = 255;
	red.r = 250; red.g = 20; red.b = 20;
	green.r = 39; green.g = 145; green.b = 39;
	aqua.r = 0; aqua.g = 128; aqua.b = 255;
	purple.r = 168; purple.g = 50; purple.b = 208;
	gray.r = 174; gray.g = 174; gray.b = 174;
	yellow.r = 245; yellow.g = 217; yellow.b = 33;
	orange.r = 238; orange.g = 124; orange.b = 22;

	if (itopic == 1) (*nspecies) = 3;      // Potassium
	else if (itopic == 2) (*nspecies) = 8; // NH3

	double angle[(*nspecies)+1];
	SDL_Color color[(*nspecies)+1];
	for (i=0;i<(*nspecies);i++) angle[i] = 0.0;
	color[0] = black;

	if (itopic == 1) {
		color[1] = gray; color[2] = purple; color[3] = yellow;
		(*legend1_tex) = renderText("Leached",FontFile, black, 16, renderer);
		(*legend2_tex) = renderText("Clays",FontFile, black, 16, renderer);
		(*legend3_tex) = renderText("K-feldspar",FontFile, black, 16, renderer);
		(*legend4_tex) = NULL;
		(*legend5_tex) = NULL;
		(*legend6_tex) = NULL;
		(*legend7_tex) = NULL;
		(*legend8_tex) = NULL;
	}
	else if (itopic == 2) {
		color[1] = green; color[2] = red; color[3] = orange; color[4] = yellow; color[5] = white; color[6] = aqua; color[7] = purple; color[8] = gray;
		(*legend1_tex) = renderText("NH3(aq)",FontFile, black, 16, renderer);
		(*legend2_tex) = renderText("musc/feldspar",FontFile, black, 16, renderer);
		(*legend3_tex) = renderText("NH4-",FontFile, black, 16, renderer);
		(*legend4_tex) = renderText("N2(g)",FontFile, black, 16, renderer);
		(*legend5_tex) = renderText("NH3(g)",FontFile, black, 16, renderer);
		(*legend6_tex) = renderText("N2(aq)",FontFile, black, 16, renderer);
		(*legend7_tex) = renderText("NH4+(aq)",FontFile, black, 16, renderer);
		(*legend8_tex) = renderText("Other N(aq)",FontFile, black, 16, renderer);
	}

	for (i=0;i<(*nspecies);i++) { // Legend pie
		Pie(2.0*M_PI/(double)(*nspecies), 2.0*M_PI/(double)(*nspecies)*(double)i, -1, 0, 0, 60, &(*pies), color[i+1]);
	}

	for (iWR=0;iWR<nWR;iWR++) {
		for (ipe=0;ipe<npe;ipe++) {
			for (ipH=0;ipH<npH;ipH++) {
				isim = ipH + ipe*npH + iWR*npH*npe + itemp*npH*npe*nWR + ipressure*npH*npe*nWR*ntemp;
				mass_water = simdata[isim][11];
				for (i=0;i<(*nspecies)+1;i++) angle[i] = 0.0;

				if (mass_water > 0.0) { // Otherwise the simulation crashed and we're not plotting
					if (itopic == 1) {
						double total_K = 0.0; total_K = simdata[isim][52]-simdata[isim][53]; // Initial K-feldspar
						angle[1] = 0.999*2.0*M_PI*simdata[isim][19]*mass_water/total_K;             // Dissolved potassium
						angle[2] = 0.999*2.0*M_PI*(simdata[isim][778]+simdata[isim][134]+simdata[isim][762]*0.33+simdata[isim][834]*0.33+simdata[isim][652])/total_K; // Phlogopite + Annite + Nontronite-K + Saponite-K + Muscovite
						angle[3] = 0.999*2.0*M_PI*simdata[isim][52]/total_K;                       // K-feldspar
					}
					else if (itopic == 2) {
						// Initial dissolved N + pyridine. Dissolved N, if specified in ppm in the input, depends on the mass of C, N, S.
						// That's too complicated to figure out analytically, just copy-paste from any PHREEQC speciation run of the template input.
						double total_N = 0.0; total_N = 1.879e+00*simdata[isim][6] + simdata[isim][84]-simdata[isim][85];
						angle[1] = 0.999*2.0*M_PI*simdata[isim][33]*mass_water/total_N; // NH3(aq)
						angle[2] = 0.999*2.0*M_PI*simdata[isim][712]/total_N;           // NH4-feldspar
						angle[3] = 0.999*2.0*M_PI*simdata[isim][714]/total_N;           // NH4-muscovite
						angle[4] = 0.999*2.0*M_PI*2.0*simdata[isim][654]/total_N; 		// N2(g)
						angle[5] = 0.999*2.0*M_PI*simdata[isim][710]/total_N; 			// NH3(g)
						angle[6] = 0.999*2.0*M_PI*2.0*simdata[isim][34]*mass_water/total_N; // N2(aq)
						angle[7] = 0.999*2.0*M_PI*simdata[isim][32]*mass_water/total_N; // NH4+(aq)
						angle[8] = 0.999*2.0*M_PI*(simdata[isim][23]-simdata[isim][32]-2.0*simdata[isim][34]-simdata[isim][33])*mass_water/total_N; // NH4+(aq)
					}

					for (i=0;i<(*nspecies);i++) {
						if (angle[i+1] < 0.0 && angle[i+1] > -1.0e-4) angle[i+1] = 0.0;
						if (angle[i+1] < 0.0 || angle[i+1] > 2.0*M_PI) printf("ParamExplorationPlot: angle %d out of bounds: %g at ipH %d, ipe %d, iWR %d\n",i+1,angle[i+1],ipH,ipe,iWR);
						else if (angle[i+1] > 0.0) Pie(angle[i+1], angle[i], iWR, ipH, ipe, pie_radius, &(*pies), color[i+1]);
						angle[i+1] = angle[i+1] + angle[i]; // To change the starting angle at the next iteration
					}
				}
			}
		}
	}

	return 0;
}

//-------------------------------------------------------------------
//                      Pie plotting subroutine
//-------------------------------------------------------------------

int Pie(double angle, double angle_start, int iWR, int ipH, int ipe, double pie_radius, SDL_Surface **pies, SDL_Color color) {

	int x = 0; int y = 0; // Pie center coordinates
	int xvar = 0; int yvar = 0;
	int i = 0; int j = 0;
	Uint32 *pixmem32;

	int red = 0; int green = 0; int blue = 0; int alpha = 0;

	red = color.r; green = color.g; blue = color.b; alpha = 255;

	if (angle + angle_start > 2.0*M_PI) {
		printf("ParamExplorationPlot: Pies: angle %g + angle_start %g > 2 pi\n",angle,angle_start);
		return 1;
	}

	x = 0; y = 0;
	// Position in the correct subwindow according to water:rock ratio
	if (iWR == -1) {
		x = x + 430; y = y + 510; // Legend pie
	}
	else if (iWR == 0) {
		x = x + 590; y = y + 380; // Bottom right
	}
	else if (iWR == 1) {
		x = x + 330; y = y + 380; // Top right
	}
	else {
		x = x + 70; y = y + 380; // Top left
	}

	// Position within the correct subwindow according to pH and pe
	x = x + 2.02*(int)pie_radius*ipH;
	y = y - 2.02*(int)pie_radius*ipe; // Bottom right

	if (x > (*pies)->w) {
		printf("ParamExploration: Pies: x out of bounds\n");
		return 1;
	}
	if (y > (*pies)->h || y < 0) {
		printf("ParamExploration: Pies: y out of bounds\n");
		return 1;
	}

	for (i=0;i<2*(int)pie_radius;i++) {
		for (j=0;j<2*(int)pie_radius;j++) {
			xvar = x - (int)pie_radius + i; yvar = y - (int)pie_radius + j;
			if (angle <= M_PI && angle + angle_start <= M_PI) {
				if (sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) < pie_radius
						&& (double)(xvar-x)/sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) <= cos(angle_start)
						&& (double)(xvar-x)/sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) >= cos(angle+angle_start) && (yvar-y) >= 0) {
					pixmem32 = (Uint32*) (*pies)->pixels + yvar*(*pies)->w + xvar;
					*pixmem32 = SDL_MapRGBA((*pies)->format, (red*(1-abs(y-yvar)/pie_radius) + 2*red)/3,
															 (green*(1-abs(y-yvar)/pie_radius) + 2*green)/3,
															 (blue*(1-abs(y-yvar)/pie_radius) + 2*blue)/3, alpha);
				}
			}
			else if (angle <= M_PI && angle_start > M_PI) {
				if (sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) < pie_radius
						&& (double)(xvar-x)/sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) >= cos(angle_start)
						&& (double)(xvar-x)/sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) <= cos(angle+angle_start) && (yvar-y) <= 0) {
					pixmem32 = (Uint32*) (*pies)->pixels + yvar*(*pies)->w + xvar;
					*pixmem32 = SDL_MapRGBA((*pies)->format, (red*abs(y-yvar)/pie_radius + 2*red)/3,
															 (green*abs(y-yvar)/pie_radius + 2*green)/3,
															 (blue*abs(y-yvar)/pie_radius + 2*blue)/3, alpha);
				}
			}
			else {
				// Equivalent to two pies of angle <= M_PI: one of angle (M_PI - angle_start) starting at angle_start:
				if (sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) < pie_radius
					&& (double)(xvar-x)/sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) <= cos(angle_start)
					&& (double)(xvar-x)/sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) >= cos(M_PI) && (yvar-y) >= 0) {

					pixmem32 = (Uint32*) (*pies)->pixels + yvar*(*pies)->w + xvar;
					*pixmem32 = SDL_MapRGBA((*pies)->format, (red*0.5*(1-abs(y-yvar)/pie_radius) + 2*red)/3,
															 (green*0.5*(1-abs(y-yvar)/pie_radius) + 2*green)/3,
															 (blue*0.5*(1-abs(y-yvar)/pie_radius) + 2*blue)/3, alpha);
				}
				// and one of angle (angle + angle_start - M_PI) starting at M_PI:
				if (sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) < pie_radius
					&& (double)(xvar-x)/sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) >= cos(M_PI)
					&& (double)(xvar-x)/sqrt((xvar-x)*(xvar-x)+(yvar-y)*(yvar-y)) <= cos(angle+angle_start) && (yvar-y) <= 0) {

					pixmem32 = (Uint32*) (*pies)->pixels + yvar*(*pies)->w + xvar;
					*pixmem32 = SDL_MapRGBA((*pies)->format, (red*0.5*(1+abs(y-yvar)/pie_radius) + 2*red)/3,
															 (green*0.5*(1+abs(y-yvar)/pie_radius) + 2*green)/3,
															 (blue*0.5*(1+abs(y-yvar)/pie_radius) + 2*blue)/3, alpha);
				}
			}
		}
	}

	return 0;
}

#endif /* PARAMEXPLORATION_PLOT_H_ */
