/*
 * IcyDwarfPlot.c
 *
 *  Created on: Mar 4, 2014
 *      Author: Marc Neveu (mneveu@asu.edu)
 *
 *		Plotting program for IcyDwarf: shows graphically the outputs of IcyDwarf.
 */

#include <unistd.h>    // To check current working directory
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "modifdyld.h" // Like mach-o/dyld.h but without the boolean DYLD_BOOL typedef
                       // that conflicts with the R_boolean typedef in IcyDwarf

#include "IcyDwarfPlot.h"
#include "Thermal/Thermal_plot.h"
#include "Crack/Crack_plot.h"
#include "Graphics/Plot.h"

int main(int argc, char *argv[]){

	// Housekeeping inputs
	int warnings = 0;                  // Display warnings
	int msgout = 0;                    // Display messages
    int view = 0;  				       // Plot view
    int NT_output = 0;                 // Timestep for writing output
    int quit = 0;                      // Close window

	// Planet inputs
    float rho_p = 0.0;                 // Planetary density
    float r_p = 0.0;                   // Planetary radius
    float nh3 = 0.0;                   // Ammonia w.r.t. water
    float Tsurf = 0.0;				   // Surface temperature
    float Tinit = 0.0;                 // Initial temperature
    float tzero = 0.0;                 // Time zero of the sim (Myr)

    // Grid inputs
	int NR = 0;                        // Number of grid zones
	int NT = 0;                        // Number of time intervals
    float timestep = 0.0;              // Time step of the sim (Gyr)

    // Call specific subroutines
    int t_cryolava = 0;                // Time at which to calculate gas exsolution

	int r = 0;

	double *input = (double*) malloc(25*sizeof(double));
	if (input == NULL) printf("IcyDwarf: Not enough memory to create input[25]\n");

	//-------------------------------------------------------------------
	// Startup
	//-------------------------------------------------------------------

	printf("\n");
	printf("-------------------------------------------------------------------\n");
	printf("IcyDwarfPlot v.14.3\n");
	if (release == 1) printf("Release mode\n");
	else if (cmdline == 1) printf("Command line mode\n");
	printf("-------------------------------------------------------------------\n");

	// Get current directory. Works for Mac only! To switch between platforms, see:
	// http://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe

	char path[1024];
	unsigned int size = sizeof(path);
	path[0] = '\0';

	if (_NSGetExecutablePath(path, &size) == 0)
		printf("\n");
	else
	    printf("IcyDwarf: Couldn't retrieve executable directory. Buffer too small; need size %u\n", size);

	input = icy_dwarf_input (input, path);

	warnings = (int) input[0];
	msgout = (int) input[1];
	rho_p = input[3];
	r_p = input[4];
	nh3 = input[5];
	tzero = 1.5;     // Myr
	Tsurf = input[6];
	Tinit = Tsurf;
	NR = input[7];
	// timestep = (float) input[9]/1000.0;
	timestep = 10.0/1000.0; // Change
	NT = floor(input[8]/(timestep*1000.0))+1;
	NT_output = floor(input[8]/input[9])+1;

	t_cryolava = (int) input[16]/input[9];

	//-------------------------------------------------------------------
	// Read thermal output (currently kbo.dat, need to read Thermal.txt)
	//-------------------------------------------------------------------

	thermalout **thoutput = malloc(NR*sizeof(thermalout*));        // Thermal model output
	if (thoutput == NULL) printf("IcyDwarf: Not enough memory to create the thoutput structure\n");
	for (r=0;r<NR;r++) {
		thoutput[r] = malloc(NT*sizeof(thermalout));
		if (thoutput[r] == NULL) printf("IcyDwarf: Not enough memory to create the thoutput structure\n");
	}
	thoutput = read_thermal_output (thoutput, NR, NT, path);

	//-------------------------------------------------------------------
	// Launch Sample DirectMedia Layer (SDL) display
	//-------------------------------------------------------------------

	if (SDL_Init(SDL_INIT_EVERYTHING) == -1){
		printf("IcyDwarfPlot: Error: SDL not initialized.");
	}
	if (TTF_Init() != 0){
		printf("IcyDwarfPlot: Error: TTF not initialized.");
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
	// Window icon
	//-------------------------------------------------------------------

	char *IcyDwarfIcon_icns = (char*)malloc(1024);           // Don't forget to free!
	IcyDwarfIcon_icns[0] = '\0';
	if (release == 1) strncat(IcyDwarfIcon_icns,path,strlen(path)-24);
	else if (cmdline == 1) strncat(IcyDwarfIcon_icns,path,strlen(path)-26);
	strcat(IcyDwarfIcon_icns,"Graphics/CeresDanWiersemaAtIconbug.icns");
	SDL_Surface* IcyDwarfIcon = IMG_Load(IcyDwarfIcon_icns);
	if (IcyDwarfIcon == NULL) printf("IcyDwarf: Plot: Window icon not loaded.\n");
	free(IcyDwarfIcon_icns);
	SDL_SetWindowIcon(window, IcyDwarfIcon);

	//-------------------------------------------------------------------
	// Display interaction
	//-------------------------------------------------------------------

	view = 1;

	while (!quit) {
		if (view == 1) // Display thermal tab
			Thermal_plot (path, NR, NT, timestep, NT_output, r_p, thoutput, warnings, msgout, window, renderer, &view, &quit);
		if (view == 2) // Display crack tab
			Crack_plot (path, NR, NT, timestep, NT_output, r_p, thoutput, warnings, msgout, window, renderer, &view, &quit);
	}

	//-------------------------------------------------------------------
	// Exit
	//-------------------------------------------------------------------

	SDL_FreeSurface(IcyDwarfIcon);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	for (r=0;r<NR;r++) {
		free (thoutput[r]);
	}
	free (thoutput);
	free (input);

	printf("Exiting IcyDwarfPlot...\n");
	return 0;
}


