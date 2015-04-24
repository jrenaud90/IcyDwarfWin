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

#define nleg 15

int ParamExploration_plot (char path[1024],	int warnings, int msgout, SDL_Renderer* renderer, int* view, int* quit, char* FontFile,
		SDL_Color axisTextColor, double Tmin, double Tmax, double Tstep, double Pmin, double Pmax, double Pstep,
		double pHmin, double pHmax, double pHstep, double pemin, double pemax, double pestep, double WRmin, double WRmax, double WRstep,
		int chondrite, int comet);

int handleClickParamExploration(SDL_Event e, int *itemp, int *ipressure, int *itopic, int ntemp, int npressure, SDL_Surface **pies,
		int *xstart, int *xend, int *ystart, int *yend);

int UpdateDisplaysParamExploration (SDL_Renderer* renderer, SDL_Texture* background_tex, SDL_Texture* pies_tex, SDL_Texture* leg_tex[nleg],
		char* FontFile, int nspecies, int itopic);

int Angles (int itopic, SDL_Surface **pies, char *FontFile, int npH, int npe, int nWR, int ntemp, int itemp, int ipressure,
		double pie_radius, double **simdata, int *nspecies, SDL_Texture *(*leg_tex)[nleg], int chondrite, int comet);

int Pie(double angle, double angle_start, int iWR, int ipH, int ipe, double pie_radius, SDL_Surface **pies, SDL_Color color);

int ParamExploration_plot (char path[1024],	int warnings, int msgout, SDL_Renderer* renderer, int* view, int* quit, char* FontFile,
		SDL_Color axisTextColor, double Tmin, double Tmax, double Tstep, double Pmin, double Pmax, double Pstep,
		double pHmin, double pHmax, double pHstep, double pemin, double pemax, double pestep, double WRmin, double WRmax, double WRstep,
		int chondrite, int comet) {

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
	int xstart = 0; int xend = 0; int ystart = 0; int yend = 0;
	SDL_Texture* background_tex = NULL;
	SDL_Texture* pies_tex = NULL;        // Pies
	SDL_Surface* pies = NULL;
	SDL_Texture* leg_tex[nleg];
	double pie_radius = 0.0;

	for (i=0;i<nleg;i++) leg_tex[i] = NULL;

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
				handleClickParamExploration(e, &itemp, &ipressure, &itopic, ntemp, npressure, &pies, &xstart, &xend, &ystart, &yend);

				Angles (itopic, &pies, FontFile, npH, npe, nWR, ntemp, itemp, ipressure, pie_radius, simdata, &nspecies, &leg_tex,
						chondrite, comet);

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
		UpdateDisplaysParamExploration(renderer, background_tex, pies_tex, leg_tex, FontFile, nspecies, itopic);
	}

	//-------------------------------------------------------------------
	//                      Free remaining mallocs
	//-------------------------------------------------------------------

	SDL_DestroyTexture(background_tex);
	SDL_FreeSurface(pies);
	SDL_DestroyTexture(pies_tex);
	for (i=0;i<nleg;i++) SDL_DestroyTexture(leg_tex[i]);
	for (i=0;i<nsim;i++) free (simdata[i]);
	free (simdata);

	return 0;
}

//-------------------------------------------------------------------
//                      Display updating subroutine
//-------------------------------------------------------------------

int UpdateDisplaysParamExploration (SDL_Renderer* renderer, SDL_Texture* background_tex, SDL_Texture* pies_tex, SDL_Texture* leg_tex[nleg],
		char* FontFile, int nspecies, int itopic) {

	double theta_legend = 0.0;
	int i = 0;
	int x = 405; int y = 502; int R = 50;

	SDL_RenderClear(renderer);
	ApplySurface(0, 0, background_tex, renderer, NULL);
	ApplySurface(0, 0, pies_tex, renderer, NULL);
	ApplySurface(358, 433, leg_tex[0], renderer, NULL);

	if (itopic == 1) R = 40;
	if (itopic == 4 || itopic == 10) {
		x = 418; y = 530;
	}
	if (itopic == 5) {
		x = 415; R = 40;
	}
	if (itopic == 8) x = 413;
	if (itopic == 9) x = 425;

	for (i=0;i<nleg-1;i++) {
		theta_legend = 2.0*M_PI/(double)nspecies*((double)i+0.5);
		ApplySurface(x + R*cos(theta_legend), y + R*sin(theta_legend), leg_tex[i+1], renderer, NULL);
	}

	SDL_RenderPresent(renderer);
	SDL_Delay(16);

	return 0;
}

//-------------------------------------------------------------------
//                      Click handling subroutine
//-------------------------------------------------------------------

int handleClickParamExploration(SDL_Event e, int *itemp, int *ipressure, int *itopic, int ntemp, int npressure, SDL_Surface **pies,
		int *xstart, int *xend, int *ystart, int *yend) {

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
			(*itemp) = 0; (*xstart) = 601; (*xend) = 625;
		}
		else if (e.button.x >= 628 && e.button.x <= 652) {
			(*itemp) = 1; (*xstart) = 628; (*xend) = 652;
		}
		else if (e.button.x >= 655 && e.button.x <= 679) {
			(*itemp) = 2; (*xstart) = 655; (*xend) = 679;
		}
		else if (e.button.x >= 682 && e.button.x <= 706) {
			(*itemp) = 3; (*xstart) = 682; (*xend) = 706;
		}
		else if (e.button.x >= 709 && e.button.x <= 733) {
			(*itemp) = 4; (*xstart) = 709; (*xend) = 733;
		}
		else if (e.button.x >= 736 && e.button.x <= 760) {
			(*itemp) = 5; (*xstart) = 736; (*xend) = 760;
		}
		else if (e.button.x >= 763 && e.button.x <= 788) {
			(*itemp) = 6; (*xstart) = 763; (*xend) = 788;
		}

		if (e.button.y >= 438 && e.button.y <= 450) {        		// y
			(*ipressure) = 6; (*ystart) = 438; (*yend) = 450;
		}
		else if (e.button.y >= 455 && e.button.y <= 467) {
			(*ipressure) = 5; (*ystart) = 455; (*yend) = 467;
		}
		else if (e.button.y >= 471 && e.button.y <= 483) {
			(*ipressure) = 4; (*ystart) = 471; (*yend) = 483;
		}
		else if (e.button.y >= 487 && e.button.y <= 499) {
			(*ipressure) = 3; (*ystart) = 487; (*yend) = 499;
		}
		else if (e.button.y >= 503 && e.button.y <= 515) {
			(*ipressure) = 2; (*ystart) = 503; (*yend) = 515;
		}
		else if (e.button.y >= 519 && e.button.y <= 531) {
			(*ipressure) = 1; (*ystart) = 519; (*yend) = 531;
		}
		else if (e.button.y >= 535 && e.button.y <= 546) {
			(*ipressure) = 0; (*ystart) = 535; (*yend) = 546;
		}

		if (*itemp > ntemp-1) *itemp = ntemp - 1;
		if (*ipressure > npressure-1) *ipressure = npressure - 1;
	}
	if ((*xstart) > 0 && (*ystart) > 0) {
		for (x=(*xstart)+1;x<(*xend);x++) {
			for (y=(*ystart)+1;y<(*yend);y++) {
				pixmem32 = (Uint32*) (*pies)->pixels + y*(*pies)->w + x;
				*pixmem32 = SDL_MapRGBA((*pies)->format, (230*(1-abs(y-(*ystart))/12) + 4*230)/5, 150, 0, 255);
			}
		}
	}

	// Change topic
	if (e.button.x >= 4 && e.button.x <= 104 && e.button.y >= 444 && e.button.y <= 491) (*itopic) = 1;       // Potassium
	else if (e.button.x >= 12 && e.button.x <= 105 && e.button.y >= 505 && e.button.y <= 562) (*itopic) = 2;  // NH3
	else if (e.button.x >= 223 && e.button.x <= 321 && e.button.y >= 437 && e.button.y <= 463) (*itopic) = 4; // Total gas
	else if (e.button.x >= 223 && e.button.x <= 321 && e.button.y >= 468 && e.button.y <= 496) (*itopic) = 5; // Gas makeup
	else if (e.button.x >= 218 && e.button.x <= 321 && e.button.y >= 506 && e.button.y <= 532) (*itopic) = 6; // Brucite / carbonates
	else if (e.button.x >= 218 && e.button.x <= 321 && e.button.y >= 537 && e.button.y <= 563) (*itopic) = 8; // Mineral makeup
	else if (e.button.x >= 115 && e.button.x <= 210 && e.button.y >= 437 && e.button.y <= 463) (*itopic) = 9; // Solution makeup
	else if (e.button.x >= 115 && e.button.x <= 210 && e.button.y >= 468 && e.button.y <= 494) (*itopic) = 10; // Solution ionic strength

	return 0;
}

//-------------------------------------------------------------------
//                    Angle calculation subroutine
//-------------------------------------------------------------------

int Angles (int itopic, SDL_Surface **pies, char *FontFile, int npH, int npe, int nWR, int ntemp, int itemp, int ipressure,
		double pie_radius, double **simdata, int *nspecies, SDL_Texture *(*leg_tex)[nleg], int chondrite, int comet) {

	int i = 0;
	int ipH = 0; // Rank of pH in output file
	int ipe = 0; // Rank of pe in output file
	int iWR = 0; // Rank of water:rock ratio in output file
	int isim = 0;
	double mass_water = 0.0; // Final mass of water
	double total_Gas = 0.0;  // Final moles of gases
	double total_Min = 0.0;  // Final moles of solids
	SDL_Color black;
	SDL_Color white;
	SDL_Color red;
	SDL_Color green;
	SDL_Color aqua;
	SDL_Color purple;
	SDL_Color gray;
	SDL_Color yellow;
	SDL_Color pale_yellow;
	SDL_Color orange;
	SDL_Color pink;
	SDL_Color cyan;
	SDL_Color light_green;
	SDL_Color maroon;
	SDL_Color key;
	black.r = 30; black.g = 30; black.b = 30;
	white.r = 250; white.g = 250; white.b = 250;
	red.r = 250; red.g = 20; red.b = 20;
	green.r = 39; green.g = 145; green.b = 39;
	aqua.r = 0; aqua.g = 128; aqua.b = 255;
	purple.r = 168; purple.g = 50; purple.b = 208;
	gray.r = 174; gray.g = 174; gray.b = 174;
	yellow.r = 245; yellow.g = 217; yellow.b = 33;
	pale_yellow.r = 255; pale_yellow.g = 255; pale_yellow.b = 158;
	orange.r = 238; orange.g = 124; orange.b = 22;
	pink.r = 255; pink.g = 47; pink.b = 146;
	cyan.r = 138; cyan.g = 240; cyan.b = 255;
	light_green.r = 204; light_green.g = 255; light_green.b = 102;
	maroon.r = 128; maroon.g = 0; maroon.b = 64;

	if (itopic == 1) (*nspecies) = 3;       // Potassium
	else if (itopic == 2) (*nspecies) = 8;  // NH3
	else if (itopic == 4 || itopic == 10) (*nspecies) = 1;  // Total gases
	else if (itopic == 5) (*nspecies) = 5;  // Gases
	else if (itopic == 6) (*nspecies) = 5;  // Brucite / carbonates
	else if (itopic == 8) (*nspecies) = 14; // Mineral makeup
	else if (itopic == 9) (*nspecies) = 11; // Solution

	double angle[(*nspecies)+1];
	SDL_Color color[(*nspecies)+1];
	for (i=0;i<(*nspecies);i++) angle[i] = 0.0;
	color[0] = black;

	for (i=0;i<nleg;i++) (*leg_tex)[i] = NULL;

	if (itopic == 1) {
		color[1] = gray; color[2] = purple; color[3] = yellow;
		(*leg_tex)[0] = renderText("per mol K",FontFile, black, 16, renderer);
		(*leg_tex)[1] = renderText("Leached",FontFile, black, 16, renderer);
		(*leg_tex)[2] = renderText("Clays",FontFile, black, 16, renderer);
		(*leg_tex)[3] = renderText("K-feldspar",FontFile, black, 16, renderer);
	}
	else if (itopic == 2) {
		color[1] = green; color[2] = red; color[3] = orange; color[4] = yellow; color[5] = white; color[6] = aqua; color[7] = purple; color[8] = gray;
		(*leg_tex)[0] = renderText("per mol N",FontFile, black, 16, renderer);
		(*leg_tex)[1] = renderText("NH3(aq)",FontFile, black, 16, renderer);
		(*leg_tex)[2] = renderText("musc/feldspar",FontFile, black, 16, renderer);
		(*leg_tex)[3] = renderText("NH4-",FontFile, black, 16, renderer);
		(*leg_tex)[4] = renderText("N2(g)",FontFile, black, 16, renderer);
		(*leg_tex)[5] = renderText("NH3(g)",FontFile, black, 16, renderer);
		(*leg_tex)[6] = renderText("N2(aq)",FontFile, black, 16, renderer);
		(*leg_tex)[7] = renderText("NH4+(aq)",FontFile, black, 16, renderer);
		(*leg_tex)[8] = renderText("Other N(aq)",FontFile, black, 16, renderer);
	}
	else if (itopic == 4) {
		(*leg_tex)[0] = renderText("gas per kg rock",FontFile, black, 16, renderer);
		(*leg_tex)[1] = renderText("1         10         100",FontFile, black, 16, renderer);
	}
	else if (itopic == 5) {
		color[1] = black; color[2] = aqua; color[3] = yellow; color[4] = red; color[5] = white;
		(*leg_tex)[0] = renderText("per mol gas",FontFile, black, 16, renderer);
		(*leg_tex)[1] = renderText("C2H6",FontFile, white, 16, renderer);
		(*leg_tex)[2] = renderText("CO2",FontFile, black, 16, renderer);
		(*leg_tex)[3] = renderText("N2",FontFile, black, 16, renderer);
		(*leg_tex)[4] = renderText("H2",FontFile, black, 16, renderer);
		(*leg_tex)[5] = renderText("H2O",FontFile, black, 16, renderer);
	}
	else if (itopic == 6) {
		color[1] = pink; color[2] = aqua; color[3] = purple; color[4] = white; color[5] = green;
		(*leg_tex)[0] = renderText("per mol solids",FontFile, black, 16, renderer);
		(*leg_tex)[1] = renderText("Brucite",FontFile, black, 16, renderer);
		(*leg_tex)[2] = renderText("Magnesite",FontFile, black, 16, renderer);
		(*leg_tex)[3] = renderText("Hydromagnesite",FontFile, black, 16, renderer);
		(*leg_tex)[4] = renderText("Huntite",FontFile, black, 16, renderer);
		(*leg_tex)[5] = renderText("Dolomite",FontFile, black, 16, renderer);
	}
	else if (itopic == 8) {
		color[1] = gray; color[2] = light_green; color[3] = black; color[4] = cyan; color[5] = green; color[6] = purple;
		color[7] = pink; color[8] = orange; color[9] = red; color[10] = white; color[11] = maroon; color[12] = aqua;
		color[13] = yellow; color[14] = pale_yellow;
		(*leg_tex)[0] = renderText("per mol solids",FontFile, black, 16, renderer);
		(*leg_tex)[1] = renderText("Andr",FontFile, black, 16, renderer);
		(*leg_tex)[2] = renderText("Atg",FontFile, black, 16, renderer);
		(*leg_tex)[3] = renderText("C(s)",FontFile, white, 16, renderer);
		(*leg_tex)[4] = renderText("Cronst",FontFile, black, 16, renderer);
		(*leg_tex)[5] = renderText("Green",FontFile, black, 16, renderer);
		(*leg_tex)[6] = renderText("H2SO4-salt",FontFile, black, 16, renderer);
		(*leg_tex)[7] = renderText("Hem",FontFile, black, 16, renderer);
		(*leg_tex)[8] = renderText("Mgt",FontFile, black, 16, renderer);
		(*leg_tex)[9] = renderText("NH4-",FontFile, black, 16, renderer);
		(*leg_tex)[10] = renderText("Oliv",FontFile, black, 16, renderer);
		(*leg_tex)[11] = renderText("Px",FontFile, white, 16, renderer);
		(*leg_tex)[12] = renderText("Sap",FontFile, black, 16, renderer);
		(*leg_tex)[13] = renderText("Troi",FontFile, black, 16, renderer);
		(*leg_tex)[14] = renderText("Pyr",FontFile, black, 16, renderer);
	}
	else if (itopic == 9) {
		color[1] = gray; color[2] = black; color[3] = cyan; color[4] = red; color[5] = pink; color[6] = orange; color[7] = green;
		color[8] = white; color[9] = purple; color[10] = yellow; color[11] = aqua;
		(*leg_tex)[0] = renderText("per mol solutes",FontFile, black, 16, renderer);
		(*leg_tex)[1] = renderText("Al",FontFile, black, 16, renderer);
		(*leg_tex)[2] = renderText("C",FontFile, white, 16, renderer);
		(*leg_tex)[3] = renderText("Ca",FontFile, black, 16, renderer);
		(*leg_tex)[4] = renderText("P",FontFile, black, 16, renderer);
		(*leg_tex)[5] = renderText("K",FontFile, black, 16, renderer);
		(*leg_tex)[6] = renderText("Mg",FontFile, black, 16, renderer);
		(*leg_tex)[7] = renderText("N",FontFile, black, 16, renderer);
		(*leg_tex)[8] = renderText("Na",FontFile, black, 16, renderer);
		(*leg_tex)[9] = renderText("Ni",FontFile, black, 16, renderer);
		(*leg_tex)[10] = renderText("S",FontFile, black, 16, renderer);
		(*leg_tex)[11] = renderText("Si",FontFile, black, 16, renderer);
	}
	else if (itopic == 10) {
		(*leg_tex)[0] = renderText("per kg H2O",FontFile, black, 16, renderer);
		(*leg_tex)[1] = renderText("0.1         1         10",FontFile, black, 16, renderer);
	}

	if (itopic == 4) {
		key.r = 255; key.b = 0;
		key.g = (int) ((1.0-1.0/200.0)*255.0);
		Pie(2.0*M_PI, 0.0, -2, 0, 0, 2.0*log(1.0e1), &(*pies), key);
		key.g = (int) ((1.0-10.0/200.0)*255.0);
		Pie(2.0*M_PI, 0.0, -2, 2, 0, 2.0*log(10.0e1), &(*pies), key);
		key.g = (int) ((1.0-100.0/200.0)*255.0);
		Pie(2.0*M_PI, 0.0, -2, 4, 0, 2.0*log(100.0e1), &(*pies), key);
	}
	else if (itopic == 10) {
		key.r = 255; key.b = 0;
		key.g = (int) ((1.0-1.0/200.0)*255.0);
		Pie(2.0*M_PI, 0.0, -2, 0, 0, 2.0*log(1.0e1), &(*pies), key);
		key.g = (int) ((1.0-10.0/200.0)*255.0);
		Pie(2.0*M_PI, 0.0, -2, 2, 0, 2.0*log(10.0e1), &(*pies), key);
		key.g = (int) ((1.0-100.0/200.0)*255.0);
		Pie(2.0*M_PI, 0.0, -2, 4, 0, 2.0*log(100.0e1), &(*pies), key);
	}
	else {
		for (i=0;i<(*nspecies);i++) { // Legend pie
			Pie(2.0*M_PI/(double)(*nspecies), 2.0*M_PI/(double)(*nspecies)*(double)i, -1, 0, 0, 60, &(*pies), color[i+1]);
		}
	}

	for (iWR=0;iWR<nWR;iWR++) {
		for (ipe=0;ipe<npe;ipe++) {
			for (ipH=0;ipH<npH;ipH++) {
				isim = ipH + ipe*npH + iWR*npH*npe + itemp*npH*npe*nWR + ipressure*npH*npe*nWR*ntemp;
				mass_water = 0.0; total_Gas = 0.0; total_Min = 0.0;
				mass_water = simdata[isim][11];
				total_Gas = simdata[isim][198] + simdata[isim][200] + simdata[isim][232] + simdata[isim][282]
						  + simdata[isim][284] + simdata[isim][420] + simdata[isim][422] + simdata[isim][424]
						  + simdata[isim][426] + simdata[isim][440] + simdata[isim][460] + simdata[isim][654]
						  + simdata[isim][710] + simdata[isim][754] + simdata[isim][756] + simdata[isim][826]
						  + simdata[isim][860];
				for (i=46;i<995;i=i+2) total_Min = total_Min + simdata[isim][i];
				total_Min = total_Min - total_Gas;
				for (i=0;i<(*nspecies)+1;i++) angle[i] = 0.0;

				if (mass_water > 0.0) { // Otherwise the simulation crashed and we're not plotting
					if (itopic == 1) {
						double total_K = 0.0;
						if (chondrite == 0) // ordinary chondrite (H/L/LL), K present as K-feldspar initially
							total_K = simdata[isim][52]-simdata[isim][53]; // Initial K-feldspar
						else                // carbonaceous chondrite (CI/CM), K present as clays
							total_K = (simdata[isim][854]-simdata[isim][855])*0.2    // Smectite-high-Fe-Mg
							        + (simdata[isim][762]-simdata[isim][763])*0.33   // Nontronite-K
							        + (simdata[isim][640]-simdata[isim][641])*0.33   // Montmor-K
							        + (simdata[isim][274]-simdata[isim][275])*3.467; // Clinoptilolite-K
						angle[1] = 0.999*2.0*M_PI*simdata[isim][19]*mass_water/total_K;             // Dissolved potassium
						angle[2] = 0.999*2.0*M_PI*(simdata[isim][778]+simdata[isim][134]+simdata[isim][762]*0.33+simdata[isim][834]*0.33+simdata[isim][652]+0.2*simdata[isim][856])/total_K; // Phlogopite + Annite + Nontronite-K + Saponite-K + Muscovite + Smectite-low-Fe-Mg
						angle[3] = 0.999*2.0*M_PI*simdata[isim][52]/total_K;                       // K-feldspar
					}
					else if (itopic == 2) {
						// Initial dissolved N + pyridine. Dissolved N, if specified in ppm in the input, depends on the mass of C, N, S.
						// That's too complicated to figure out analytically, just copy-paste from any PHREEQC speciation run of the template input.
						double total_N = 0.0;
						if (comet == 1) total_N = 1.879e+00*simdata[isim][6] + simdata[isim][84]-simdata[isim][85];
						else total_N = simdata[isim][84]-simdata[isim][85];
						angle[1] = 0.999*2.0*M_PI*simdata[isim][33]*mass_water/total_N; // NH3(aq)
						angle[2] = 0.999*2.0*M_PI*simdata[isim][712]/total_N;           // NH4-feldspar
						angle[3] = 0.999*2.0*M_PI*simdata[isim][714]/total_N;           // NH4-muscovite
						angle[4] = 0.999*2.0*M_PI*2.0*simdata[isim][654]/total_N; 		// N2(g)
						angle[5] = 0.999*2.0*M_PI*simdata[isim][710]/total_N; 			// NH3(g)
						angle[6] = 0.999*2.0*M_PI*2.0*simdata[isim][34]*mass_water/total_N; // N2(aq)
						angle[7] = 0.999*2.0*M_PI*simdata[isim][32]*mass_water/total_N; // NH4+(aq)
						angle[8] = 0.999*2.0*M_PI*(simdata[isim][23]-simdata[isim][32]-2.0*simdata[isim][34]-simdata[isim][33])*mass_water/total_N; // NH4+(aq)
					}
					else if (itopic == 5) { // Final moles of gases
						angle[1] = 0.999*2.0*M_PI*simdata[isim][200]/total_Gas; // C2H6
						angle[2] = 0.999*2.0*M_PI*simdata[isim][284]/total_Gas; // CO2
						angle[3] = 0.999*2.0*M_PI*simdata[isim][654]/total_Gas; // N2
						angle[4] = 0.999*2.0*M_PI*simdata[isim][420]/total_Gas; // H2
						angle[5] = 0.999*2.0*M_PI*simdata[isim][422]/total_Gas; // H2O
					}
					else if (itopic == 6) {
						// double gfw_Mg = 24.305; double gfw_C = 12.0110; double gfw_O = 15.994; double gfw_H = 1.0079; double gfw_Ca = 40.078;
						angle[1] = 0.999*2.0*M_PI*simdata[isim][188]/total_Min; // Brucite
						angle[2] = 0.999*2.0*M_PI*simdata[isim][556]/total_Min; // Magnesite
						angle[3] = 0.999*2.0*M_PI*simdata[isim][468]/total_Min; // Hydromagnesite
						angle[4] = 0.999*2.0*M_PI*simdata[isim][464]/total_Min; // Huntite
						angle[5] = 0.999*2.0*M_PI*(simdata[isim][346]+simdata[isim][348]+simdata[isim][350])/total_Min; // Dolomite x 10
					}
					else if (itopic == 8) {
						angle[1] = 0.999*2.0*M_PI*simdata[isim][130]/total_Min; // Andr
						angle[2] = 0.999*2.0*M_PI*simdata[isim][140]/total_Min; // Atg
						angle[3] = 0.999*2.0*M_PI*simdata[isim][196]/total_Min; // C
						angle[4] = 0.999*2.0*M_PI*simdata[isim][312]/total_Min; // Cronst
						angle[5] = 0.999*2.0*M_PI*simdata[isim][410]/total_Min; // Green
						angle[6] = 0.999*2.0*M_PI*(simdata[isim][428]+simdata[isim][430])/total_Min; // H2SO4:4H2O and H2SO4:6.5H2O
						angle[7] = 0.999*2.0*M_PI*simdata[isim][450]/total_Min; // Hem
						angle[8] = 0.999*2.0*M_PI*simdata[isim][558]/total_Min; // Mgt
						angle[9] = 0.999*2.0*M_PI*(simdata[isim][712]+simdata[isim][714])/total_Min; // NH4-feldspar + NH4-muscovite
						angle[10] = 0.999*2.0*M_PI*(simdata[isim][64]+simdata[isim][66])/total_Min; // Oliv: forsterite + fayalite
						angle[11] = 0.999*2.0*M_PI*(simdata[isim][60]+simdata[isim][62])/total_Min; // Px: enstatite + ferrosilite
						angle[12] = 0.999*2.0*M_PI*(simdata[isim][830]+simdata[isim][832]+simdata[isim][834]+simdata[isim][836]+simdata[isim][838])/total_Min; // Sap
						angle[13] = 0.999*2.0*M_PI*simdata[isim][76]/total_Min; // Troi
						angle[14] = 0.999*2.0*M_PI*simdata[isim][796]/total_Min; // Pyrite
					}
					else if (itopic == 9) {
						double total_Sol = 0.0; // Approx. final mass of solids and gases
						for (i=12;i<31;i++) total_Sol = total_Sol + simdata[isim][i];
						angle[1] = 0.999*2.0*M_PI*simdata[isim][12]/total_Sol; // Al
						angle[2] = 0.999*2.0*M_PI*simdata[isim][13]/total_Sol; // C
						angle[3] = 0.999*2.0*M_PI*simdata[isim][14]/total_Sol; // Ca
						angle[4] = 0.999*2.0*M_PI*simdata[isim][26]/total_Sol; // P
						angle[5] = 0.999*2.0*M_PI*simdata[isim][19]/total_Sol; // K
						angle[6] = 0.999*2.0*M_PI*simdata[isim][20]/total_Sol; // Mg
						angle[7] = 0.999*2.0*M_PI*simdata[isim][23]/total_Sol; // N
						angle[8] = 0.999*2.0*M_PI*simdata[isim][24]/total_Sol; // Na
						angle[9] = 0.999*2.0*M_PI*simdata[isim][25]/total_Sol; // Ni
						angle[10] = 0.999*2.0*M_PI*simdata[isim][27]/total_Sol; // S
						angle[11] = 0.999*2.0*M_PI*simdata[isim][28]/total_Sol; // Si
					}

					if (itopic == 4) {
						key.r = 255; key.b = 0;
						if (total_Gas/200.0 > 1.0) key.g = 0;
						else key.g = (int) ((1.0-total_Gas/200.0)*255.0);
						Pie(2.0*M_PI, 0.0, iWR, ipH, ipe, 2.0*log(10.0*total_Gas), &(*pies), key); // log = natural logarithm ln
					}
					else if (itopic == 10) {
						key.r = 255; key.b = 0;
						if (simdata[isim][10]/200.0 > 1.0) key.g = 0;
						else key.g = (int) ((1.0-simdata[isim][10]/200.0)*255.0);
						Pie(2.0*M_PI, 0.0, iWR, ipH, ipe, 2.0*log(10.0*simdata[isim][10]), &(*pies), key); // log = natural logarithm ln
					}
					else {
						for (i=0;i<(*nspecies);i++) {
							if (angle[i+1] < 0.0 && angle[i+1] > -1.0e-4) angle[i+1] = 0.0;
							if (angle[i+1] < 0.0 || angle[i+1] > 2.0*M_PI) printf("ParamExplorationPlot: angle %d out of bounds: %g at ipH %d, ipe %d, iWR %d, itemp %d, ipressure %d\n",i+1,angle[i+1],ipH,ipe,iWR,itemp,ipressure);
							else if (angle[i+1] > 0.0) Pie(angle[i+1], angle[i], iWR, ipH, ipe, pie_radius, &(*pies), color[i+1]);
							angle[i+1] = angle[i+1] + angle[i]; // To change the starting angle at the next iteration
						}
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
	if (iWR == -2) {
		x = x + 370; y = y + 510; // Legend pie, totals
	}
	else if (iWR == -1) {
		x = x + 430; y = y + 510; // Legend pie, species
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
	x = x + 2.02*13.0*ipH;
	y = y - 2.02*13.0*ipe; // Bottom right

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
