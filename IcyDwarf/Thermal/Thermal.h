/*
 * Thermal.h
 *
 *  Created on: Jan 31, 2014
 *      Author: Marc Neveu (mneveu@asu.edu)
 *
 *      Started off as a C copy of the FORTRAN code developed by Steve Desch (Desch et al. 2009).
 *      !! IN FORTRAN, ARRAYS START AT 1 UNLESS OTHERWISE SPECIFIED. IN C THEY START AT 0 !!
 *
 *      Inputs: Planet density, radius, surface temperature, initial temperature, NH3 content wrt H2O (Xp), output step,
 *      number of grid zones, duration of the sim, initial time of sim (for 26Al).
 *      Outputs temperature and structure profiles (ice, rock, liquid water, liquid NH3, ADH ice), as well as Nu
 *      numbers (for parameterized ice convection).
 *
 *      Includes parameterized convection in ice, otherwise heat transfer is by conduction.
 *
 *      Reference:
 *      Desch et al. (2009) Thermal evolution of Kuiper belt objects, with implications for cryovolcanism.
 *      Icarus 202, 694-714.
 */

#ifndef THERMAL_H_
#define THERMAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../IcyDwarf.h"

int Thermal (int argc, char *argv[], char path[1024], int NR, double r_p, double rho_p,
		int warnings, int msgout, double Xp, double *Xhydr, double tzero, double Tsurf, double Tinit, double fulltime, double dtoutput,
		int *crack_input, int *crack_species);

int state (char path[1024], int itime, int ir, double E, double *frock, double *fh2os, double *fadhs, double *fh2ol, double *fnh3l, double *T);

double heatRock (double T);

int heatIce (double T, double X, double *E, double *gh2os, double *gadhs, double *gh2ol, double *gnh3l);

int kapcond(double T, double frock, double fh2os, double fadhs, double fh2ol, double dnh3l, double *kap, double Xhydr);

int decay(double *t, double *tzero, double *S);

int separate(int NR, int *irdiff, int *ircore, int *irice, double *dVol, double **dM, double **dE, double **Mrock, double **Mh2os, double **Madhs,
		double **Mh2ol, double **Mnh3l, double **Vrock, double **Vh2os, double **Vadhs, double **Vh2ol, double **Vnh3l, double **Erock,
		double **Eh2os, double **Eslush, double rhoH2olth, double rhoNh3lth);

int dehydrate(double T, double dM, double dVol, double *Mrock, double *Mh2ol, double *Vrock,
		double *Vh2ol, double rhoRockth, double rhoHydrth, double rhoH2olth, double *Xhydr);

int hydrate(double T, double **dM, double *dVol, double **Mrock, double *Mh2os, double *Madhs, double **Mh2ol, double *Mnh3l, double **Vrock,
		double **Vh2ol, double rhoRockth, double rhoHydrth, double rhoH2olth, double **Xhydr, int ir, int ircore, int irice, int NR);

double viscosity(double T, double Mh2ol, double Mnh3l);

int Thermal (int argc, char *argv[], char path[1024], int NR, double r_p, double rho_p,
		int warnings, int msgout, double Xp, double *Xhydr, double tzero, double Tsurf, double Tinit, double fulltime, double dtoutput,
		int *crack_input, int *crack_species) {

	//-------------------------------------------------------------------
	//                 Declarations and initializations
	//-------------------------------------------------------------------

	int itime = 0;    // Time counter
	int ntime = 0;    // Total number of iterations
	int isteps = 0;   // Output step counter
	int nsteps = 0;   // Total number of output steps

	double dtime = 0.0; // Time step (s)
	double time = 0.0;  // Time elapsed (s)

	double frockp = 0.0;// Fraction of rock in the planet by mass

	int ir = 0;       // Grid counter
	int jr = 0;       // Secondary grid counter
	int i = 0;

	int irdiff = 0;   // Radius of differentiation
	int irice = 0;    // Radius of the top of the slush
	int ircore = 0;   // Radius of the core
	int ircrack = 0;  // Inner radius of the continuously cracked rock layer in contact with the ocean

	double Thermal[9];										  // Thermal[9], output
	for (ir=0;ir<9;ir++)
		Thermal[ir] = 0.0;

	double *r = (double*) malloc((NR+1)*sizeof(double));      // Radius (cm)
	if (r == NULL) printf("Thermal: Not enough memory to create r[NR+1]\n");

	double *dVol = (double*) malloc((NR)*sizeof(double));     // Total volume of a layer (cm^3)
	if (dVol == NULL) printf("Thermal: Not enough memory to create dVol[NR]\n");

	double *dM = (double*) malloc((NR)*sizeof(double));       // Mass of a layer (g)
	if (dM == NULL) printf("Thermal: Not enough memory to create dM[NR]\n");

	double *dM_old = (double*) malloc((NR)*sizeof(double));   // Old mass of a layer (g)
	if (dM_old == NULL) printf("Thermal: Not enough memory to create dM_old[NR]\n");

	double *M = (double*) malloc((NR)*sizeof(double));        // Mass under a layer (g)
	if (M == NULL) printf("Thermal: Not enough memory to create M[NR]\n");

	double *dE = (double*) malloc((NR)*sizeof(double));       // Energy of a layer (erg)
	if (dE == NULL) printf("Thermal: Not enough memory to create dE[NR]\n");

	double *Mrock = (double*) malloc((NR)*sizeof(double));    // Mass of rock (g)
	if (Mrock == NULL) printf("Thermal: Not enough memory to create Mrock[NR]\n");

	double *Mh2os = (double*) malloc((NR)*sizeof(double));    // Mass of water ice (g)
	if (Mh2os == NULL) printf("Thermal: Not enough memory to create Mh2os[NR]\n");

	double *Madhs = (double*) malloc((NR)*sizeof(double));    // Mass of ammonia dihydrate ice (g)
	if (Madhs == NULL) printf("Thermal: Not enough memory to create Madhs[NR]\n");

	double *Mh2ol = (double*) malloc((NR)*sizeof(double));    // Mass of liquid water (g)
	if (Mh2ol == NULL) printf("Thermal: Not enough memory to create Mh2ol[NR]\n");

	double *Mnh3l = (double*) malloc((NR)*sizeof(double));    // Mass of liquid ammonia (g)
	if (Mnh3l == NULL) printf("Thermal: Not enough memory to create Mnh3l[NR]\n");

	double *Erock = (double*) malloc((NR)*sizeof(double));    // Energy of rock (erg)
	if (Erock == NULL) printf("Thermal: Not enough memory to create Erock[NR]\n");

	double *Eh2os = (double*) malloc((NR)*sizeof(double));    // Energy of water ice (erg)
	if (Eh2os == NULL) printf("Thermal: Not enough memory to create Eh2os[NR]\n");

	double *Eslush = (double*) malloc((NR)*sizeof(double));   // Energy of slush/ocean (erg)
	if (Eslush == NULL) printf("Thermal: Not enough memory to create Eslush[NR]\n");

	double *Vrock = (double*) malloc((NR)*sizeof(double));    // Volume of rock (cm^3)
	if (Vrock == NULL) printf("Thermal: Not enough memory to create Vrock[NR]\n");

	double *Vh2os = (double*) malloc((NR)*sizeof(double));    // Volume of water ice (cm^3)
	if (Vh2os == NULL) printf("Thermal: Not enough memory to create Vh2os[NR]\n");

	double *Vadhs = (double*) malloc((NR)*sizeof(double));    // Volume of ammonia dihydrate ice (cm^3)
	if (Vadhs == NULL) printf("Thermal: Not enough memory to create Vadhs[NR]\n");

	double *Vh2ol = (double*) malloc((NR)*sizeof(double));    // Volume of liquid water (cm^3)
	if (Vh2ol == NULL) printf("Thermal: Not enough memory to create Vh2ol[NR]\n");

	double *Vnh3l = (double*) malloc((NR)*sizeof(double));    // Volume of liquid ammonia (cm^3)
	if (Vnh3l == NULL) printf("Thermal: Not enough memory to create Vnh3l[NR]\n");

	double *T = (double*) malloc((NR)*sizeof(double));        // Temperature (K)
	if (T == NULL) printf("Thermal: Not enough memory to create T[NR]\n");

	double *T_old = (double*) malloc((NR)*sizeof(double));    // Temperature (K)
	if (T == NULL) printf("Thermal: Not enough memory to create T_old[NR]\n");

	double *kappa = (double*) malloc((NR)*sizeof(double));    // Thermal conductivity (erg/s/cm/K)
	if (kappa == NULL) printf("Thermal: Not enough memory to create kappa[NR]\n");

	double *RRflux = (double*) malloc((NR+1)*sizeof(double)); // Thermal flux (erg/s/cm2)
	if (RRflux == NULL) printf("Thermal: Not enough memory to create RRflux[NR+1]\n");

	double *Qth = (double*) malloc((NR)*sizeof(double));      // Heating power (erg/s)
	if (Qth == NULL) printf("Thermal: Not enough memory to create Qth[NR]\n");

	double *Xhydr_old = (double*) malloc((NR)*sizeof(double));// Old degree of hydration, 0=dry, 1=hydrated
	if (Xhydr_old == NULL) printf("Thermal: Not enough memory to create Xhydr_old[NR]\n");

	double *Pressure = (double*) malloc(NR*sizeof(double));      // Pressure in Pa
	if (Pressure == NULL) printf("Thermal: Not enough memory to create Pressure[NR]\n");

	double *Crack = (double*) malloc(NR*sizeof(double));         // Crack[NR], type of cracked zone, output
	if (Crack == NULL) printf("Thermal: Not enough memory to create Crack[NR]\n");

	double *Crack_old = (double*) malloc(NR*sizeof(double));     // Crack_old[NR], previously cracked zone
	if (Crack_old == NULL) printf("Thermal: Not enough memory to create Crack_old[NR]\n");

	double *Crack_size = (double*) malloc(NR*sizeof(double));    // Crack_size[NR], subgrid crack width in m (width in 1-D, diameter in cylindrical 2-D)
	if (Crack_size == NULL) printf("Thermal: Not enough memory to create Crack_size[NR]\n");

	double *Crack_size_old = (double*) malloc(NR*sizeof(double));// Crack_size_old[NR], previous crack width (m)
	if (Crack_size_old == NULL) printf("Thermal: Not enough memory to create Crack_size_old[NR]\n");

	double **Act = (double**) malloc(NR*sizeof(double*));        // Activity of chemical products in cracks, dimensionless or (mol m-3) if << salinity
	if (Act == NULL) printf("Thermal: Not enough memory to create Act[NR][n_species_crack]\n");
	for (ir=0;ir<NR;ir++) {
		Act[ir] = (double*) malloc(n_species_crack*sizeof(double));
		if (Act[ir] == NULL) printf("Thermal: Not enough memory to create Act[NR][n_species_crack]\n");
	}

	double **Act_old = (double**) malloc(NR*sizeof(double*));    // Activity of chemical products at the previous step
	if (Act_old == NULL) printf("Thermal: Not enough memory to create Act_old[NR]\n");
	for (ir=0;ir<NR;ir++) {
		Act_old[ir] = (double*) malloc(n_species_crack*sizeof(double));
		if (Act_old[ir] == NULL) printf("Thermal: Not enough memory to create Act_old[NR][n_species_crack]\n");
	}

	double Crack_depth[2];										 // Crack_depth[2] in km, output
	Crack_depth[0] = 0.0, Crack_depth[1] = 0.0;

	double WRratio[2];											 // WRratio[2], output
	WRratio[0] = 0.0, WRratio[1] = 0.0;

	double e1 = 0.0;    // Temporary specific energy (erg/g)
	double e2 = 0.0;    // Temporary specific energy (erg/g)
	double frock = 0.0; // Rock mass fraction
	double fh2os = 0.0; // Water ice mass fraction
	double fadhs = 0.0; // Ammonia dihydrate ice mass fraction
	double fh2ol = 0.0; // Liquid water mass fraction
	double fnh3l = 0.0; // Liquid ammonia mass fraction
	double temp1 = 0.0; // Temporary temperature (K)
	double Xhydr_temp = 0.0; // Temporary hydration index

	double S = 0.0;     // Radiogenic power, specific (erg/s/g)
	double Tdiff = 0.0; // Temperature at which differentiation proceeds (K)
	double kap1 = 0.0;  // Temporary thermal conductivity (erg/s/cm/K)
	double dr = 0.0;    // Physical thickness of a shell (cm)

	double Phi = 0.0;   // Gravitational potential energy (erg)
	double Phiold = 0.0;
	double ravg = 0.0;  // Average radius of a layer (cm)
	double Volume1 = 0.0; // Differentiated volume (cm3)

	double Ra = 0.0;    // Rayleigh number
	double dT = 0.0;    // Temperature difference across convective region (K)
	double alf1 = 0.0;  // Thermal expansion coefficient of H2O ice (K-1)
	double mu1 = 0.0;   // Water ice viscosity (cgs)
	double cp1 = 0.0;   // Heat capacity of H2O ice (erg g-1 K-1)
	double g1 = 0.0;    // Gravitational acceleration for calculation of Ra in ice (cgs)

	double *Nu = (double*) malloc((NR)*sizeof(double));      // Nusselt number
	if (Nu == NULL) printf("Thermal: Not enough memory to create Nu[NR]\n");

	int *dont_dehydrate = (int*) malloc((NR)*sizeof(int));      // Don't dehydrate a layer that just got hydrated
	if (dont_dehydrate == NULL) printf("Thermal: Not enough memory to create dont_dehydrate[NR]\n");

	double Nu0 = 0.0;   // Critical Nusselt number = Ra_c^0.25

	double rhoRockth = rhoRock/1000.0;  // Density of dry rock, just for this thermal routine (g/cm3)
	double rhoHydrth = rhoHydr/1000.0;  // Density of hydrated rock, just for this thermal routine (g/cm3)
	double rhoH2olth = 0.0;             // Density of liquid water, just for this thermal routine (g/cm3)
	double rhoH2osth = 0.935;           // Density of ice, just for this thermal routine (g/cm3) TODO appropriate for KBOs, but 0.918 for Ceres
	double rhoNh3lth = 0.0;             // Density of liquid ammonia, just for this thermal routine (g/cm3)
	double rhoAdhsth = 0.985;           // Density of ammonia dihydrate, just for this thermal routine (g/cm3)
	double rhoIce = 0.0;                // Density of the bulk ice (g/cm3)

	double *Mrock_init = (double*) malloc((NR)*sizeof(double));      // Initial rock mass
	if (Mrock_init == NULL) printf("Thermal: Not enough memory to create Mrock_init[NR]\n");

	double *time_hydr = (double*) malloc((NR)*sizeof(double));       // Time since last hydration/dehydration
	if (time_hydr == NULL) printf("Thermal: Not enough memory to create time_hydr[NR]\n");

	double Mliq = 0.0;                   // Mass of liquid in the planet in g
	double Mcracked_rock = 0.0;          // Mass of cracked rock in the planet in g

	// Crack options
    int thermal_mismatch = 0;                                    // Grain thermal expansion/contraction mismatch effects
	int pore_water_expansion = 0;                                // Pore water expansion effects
	int hydration_dehydration = 0;                               // Rock hydration/dehydration effects
	int dissolution_precipitation = 0;                           // Rock dissolution/precipitation effects

	// Crack data tables
	float **aTP = (float**) malloc((sizeaTP)*sizeof(float*));    // a[sizeaTP][sizeaTP], table of flaw sizes a that maximize the stress K_I
	if (aTP == NULL) printf("aTP: Not enough memory to create a[sizeaTP][sizeaTP]\n");
	for (i=0;i<sizeaTP;i++) {
		aTP[i] = (float*) malloc((sizeaTP)*sizeof(float));
		if (aTP[i] == NULL) printf("Crack: Not enough memory to create a[sizeaTP][sizeaTP]\n");
	}

	float **integral = (float**) malloc(int_size*sizeof(float*)); // integral[int_size][2], used for K_I calculation
	if (integral == NULL) printf("Crack: Not enough memory to create integral[int_size][2]\n");
	for (i=0;i<int_size;i++) {
		integral[i] = (float*) malloc(2*sizeof(float));
		if (integral[i] == NULL) printf("Crack: Not enough memory to create integral[int_size][2]\n");
	}

	float **alpha = (float**) malloc(sizeaTP*sizeof(float*));    // Thermal expansivity of water (T,P) in K-1
	if (alpha == NULL) printf("Crack: Not enough memory to create alpha[sizeaTP][sizeaTP]\n");
	for (i=0;i<sizeaTP;i++) {
		alpha[i] = (float*) malloc(sizeaTP*sizeof(float));
		if (alpha[i] == NULL) printf("Crack: Not enough memory to create alpha[sizeaTP][sizeaTP]\n");
	}

	float **beta = (float**) malloc(sizeaTP*sizeof(float*));     // Compressibility of water (T,P) in bar-1
	if (beta == NULL) printf("Crack: Not enough memory to create beta[sizeaTP][sizeaTP]\n");
	for (i=0;i<sizeaTP;i++) {
		beta[i] = (float*) malloc(sizeaTP*sizeof(float));
		if (beta[i] == NULL) printf("Crack: Not enough memory to create beta[sizeaTP][sizeaTP]\n");
	}

	float **silica = (float**) malloc(sizeaTP*sizeof(float*));   // log K of silica dissolution
	if (silica == NULL) printf("Crack: Not enough memory to create silica[sizeaTP][sizeaTP]\n");
	for (i=0;i<sizeaTP;i++) {
		silica[i] = (float*) malloc(sizeaTP*sizeof(float));
		if (silica[i] == NULL) printf("Crack: Not enough memory to create silica[sizeaTP][sizeaTP]\n");
	}

	float **chrysotile = (float**) malloc(sizeaTP*sizeof(float*)); // log K of chrysotile dissolution
	if (chrysotile == NULL) printf("Crack: Not enough memory to create chrysotile[sizeaTP][sizeaTP]\n");
	for (i=0;i<sizeaTP;i++) {
		chrysotile[i] = (float*) malloc(sizeaTP*sizeof(float));
		if (chrysotile[i] == NULL) printf("Crack: Not enough memory to create chrysotile[sizeaTP][sizeaTP]\n");
	}

	float **magnesite = (float**) malloc(sizeaTP*sizeof(float*)); // log K of magnesite dissolution
	if (magnesite == NULL) printf("Crack: Not enough memory to create magnesite[sizeaTP][sizeaTP]\n");
	for (i=0;i<sizeaTP;i++) {
		magnesite[i] = (float*) malloc(sizeaTP*sizeof(float));
		if (magnesite[i] == NULL) printf("Crack: Not enough memory to create magnesite[sizeaTP][sizeaTP]\n");
	}

	int seafloor_link = 0;
	double Vcracked = 0.0;                                       // Volume of cracked rock
	double Vliq = 0.0;                                           // Volume of liquid

	// Zero all the arrays
    for (ir=0;ir<NR;ir++) {
    	dVol[ir] = 0.0;
    	dM[ir] = 0.0;
    	M[ir] = 0.0;
    	dE[ir] = 0.0;
    	Mrock[ir] = 0.0;
    	Mh2os[ir] = 0.0;
    	Madhs[ir] = 0.0;
    	Mh2ol[ir] = 0.0;
    	Mnh3l[ir] = 0.0;
    	Erock[ir] = 0.0;
    	Eh2os[ir] = 0.0;
    	Eslush[ir] = 0.0;
    	Vrock[ir] = 0.0;
    	Vh2os[ir] = 0.0;
    	Vadhs[ir] = 0.0;
    	Vh2ol[ir] = 0.0;
    	Vnh3l[ir] = 0.0;
    	T[ir] = 0.0;
    	T_old[ir] = 0.0;
    	kappa[ir] = 0.0;
    	Qth[ir] = 0.0;
    	Xhydr_old[ir] = 0.0;
    	Pressure[ir] = 0.0;
    	Crack[ir] = 0.0;
    	Crack_old[ir] = 0.0;
    	Crack_size[ir] = 0.0;
    	Crack_size_old[ir] = 0.0;
    	Nu[ir] = 1.0;
    	Mrock_init[ir] = 0.0;
    	time_hydr[ir] = 0.0;
    	for (i=0;i<n_species_crack;i++) {
			Act[ir][i] = 0.0;
			Act_old[ir][i] = 0.0;
    	}
    }

    for (ir=0;ir<NR+1;ir++) {
    	r[ir] = 0.0;
    	RRflux[ir] = 0.0;
    }

	for (i=0;i<int_size;i++) {
		integral[i][0] = 0.0;
		integral[i][1] = 0.0;
	}
	for (i=0;i<sizeaTP;i++) {
		for (ir=0;ir<sizeaTP;ir++) {
			aTP[i][ir] = 0.0;
			alpha[i][ir] = 0.0;
			beta[i][ir] = 0.0;
			silica[i][ir] = 0.0;
			chrysotile[i][ir] = 0.0;
			magnesite[i][ir] = 0.0;
		}
	}

	//-------------------------------------------------------------------
	//                              Setup
	//-------------------------------------------------------------------

	thermal_mismatch = crack_input[0];
	pore_water_expansion = crack_input[1];
	hydration_dehydration = crack_input[2];
	dissolution_precipitation = crack_input[3];

    //-------------------------------------------------------------------
    //                     Initialize physical tables
    //-------------------------------------------------------------------

	// Read the a(T,P) and integral input files.
	// - aTP: table of a(deltaT,P) in the model of
	// Vance et al. (2007) so we don't have to calculate a(deltaT,P)
	// each time the routine is called. Use aTP() to generate this file.
	// - integral: Geometry part of the integral in eqs. (3) and (4) of
	// Vance et al. (2007) for various a, to calculate the stress intensity K_I.
	if (thermal_mismatch == 1) {
		aTP = read_input (sizeaTP, sizeaTP, aTP, path, "Data/Crack_aTP.txt");
		if (aTP[0][0] == 0) printf("Generate a table of a(T,P) using the aTP routine.\n");
		integral = read_input (2, int_size, integral, path, "Data/Crack_integral.txt");
		if (integral[0][0] == 0) printf("Generate a table of integral results using the aTP routine.\n");
	}

	if (pore_water_expansion == 1) {
		// Open alpha and beta files
		alpha = read_input (sizeaTP, sizeaTP, alpha, path, "Data/Crack_alpha.txt");
		if (alpha[0][0] == 0) printf("Generate a table of parameter alpha for water using the Crack_water_CHNOSZ routine.\n");
		beta = read_input (sizeaTP, sizeaTP, beta, path, "Data/Crack_beta.txt");
		if (beta[0][0] == 0) printf("Generate a table of parameter beta for water using the Crack_water_CHNOSZ routine.\n");
	}

	if (dissolution_precipitation == 1) {
		silica = read_input (sizeaTP, sizeaTP, silica, path, "Data/Crack_silica.txt");
		if (silica[0][0] == 0) printf("Generate a table of silica log K using the Crack_species_CHNOSZ routine.\n");
		chrysotile = read_input (sizeaTP, sizeaTP, chrysotile, path, "Data/Crack_chrysotile.txt");
		if (silica[0][0] == 0) printf("Generate a table of chrysotile log K using the Crack_species_CHNOSZ routine.\n");
		magnesite = read_input (sizeaTP, sizeaTP, magnesite, path, "Data/Crack_magnesite.txt");
		if (silica[0][0] == 0) printf("Generate a table of magnesite log K using the Crack_species_CHNOSZ routine.\n");
	}

	create_output(path, "Outputs/Thermal.txt");
	create_output(path, "Outputs/Crack.txt");
	create_output(path, "Outputs/Crack_depth.txt");
	create_output(path, "Outputs/Crack_WRratio.txt");

    r_p = r_p*km2cm;                                                     // Convert planet radius to cm
    tzero = tzero*Myr2sec;                                               // Convert tzero to s
    fulltime = fulltime*Myr2sec;                                         // Convert fulltime to s
    dtoutput = dtoutput*Myr2sec;                                         // Convert increment between outputs in s

    // Determine the core vs. ice shell content from bulk density.
	  // Densities of liquid water and ammonia are chosen to conserve mass and volume,
      // actual densities are 1.00 g/cm-3 and about 0.74 g/cm-3
    rhoH2olth = rhoH2osth;
    rhoNh3lth = (1.0/rhoH2olth) + (1.0/rhoAdhsth - 1.0/rhoH2osth) / Xc;  // Slush mass balance
    rhoNh3lth = 1.0/rhoNh3lth;
    rhoIce = 1.0 / ((Xp/Xc)/rhoAdhsth + (1.0-Xp/Xc)/rhoH2osth);          // Bulk ice density
    frockp = (1.0-rhoIce/rho_p) / (1.0-rhoIce/(Xhydr[0]*rhoHydrth+(1.0-Xhydr[0])*rhoRockth));

    dr = r_p/((double) NR);

    for (ir=0;ir<NR;ir++) {
    	r[ir+1] = r[ir] + dr;
    	dVol[ir] = 4.0/3.0*PI_greek*(r[ir+1]*r[ir+1]*r[ir+1] - r[ir]*r[ir]*r[ir]);
    	dM[ir] = dVol[ir]*rho_p;
    	Mrock[ir] = dM[ir]*frockp;
    	Mrock_init[ir] = Mrock[ir];
    	Mh2os[ir] = dM[ir]*(1.0-frockp)*(1.0-Xp/Xc);
    	Madhs[ir] = dM[ir]*(1.0-frockp)*(Xp/Xc);

    	// Init of the energies, prop to Cp(T) * deltaT. Because often Cp(T) prop to T, energies prop to T*deltaT.
    	e1 = heatRock(Tinit);
    	Erock[ir] = Mrock[ir]*e1;
    	e1 = qh2o*Tinit*Tinit/2.0;
    	e2 = qadh*Tinit*Tinit/2.0;
    	Eh2os[ir] = Mh2os[ir]*e1;
    	Eslush[ir] = Madhs[ir]*e2;
    	dE[ir] = Erock[ir] + Eh2os[ir] + Eslush[ir];
    	Vrock[ir] = Mrock[ir] / (Xhydr[ir]*rhoHydrth+(1.0-Xhydr[ir])*rhoRockth);
    	Vh2os[ir] = Mh2os[ir] / rhoH2osth;
    	Vadhs[ir] = Madhs[ir] / rhoAdhsth;
    	T[ir] = Tinit;
    	Nu[ir] = 1.0;
    }

    // Gravitational potential energy
    Phi = 0.6*Gcgs*dM[0]*dM[0]/r[1];
    M[0] = dM[0];
    for (ir=1;ir<NR;ir++) {
    	ravg = (r[ir+1]+r[ir])/2.0;
    	Phi = Phi + Gcgs*M[ir-1]*dM[ir] / ravg;
    	M[ir] = M[ir-1] + dM[ir];
    }

	//-------------------------------------------------------------------
	//                  Allow for chemical equilibrium
	//-------------------------------------------------------------------

    for (ir=0;ir<NR;ir++) {
    	e1 = dE[ir] / dM[ir];
    	frock = Mrock[ir] / dM[ir];
    	fh2os = Mh2os[ir] / dM[ir];
    	fadhs = Madhs[ir] / dM[ir];
    	fh2ol = Mh2ol[ir] / dM[ir];
    	fnh3l = Mnh3l[ir] / dM[ir];
    	state (path, itime, ir, e1, &frock, &fh2os, &fadhs, &fh2ol, &fnh3l, &temp1);
    	T[ir] = temp1;
    	Mrock[ir] = dM[ir]*frock;
    	Mh2os[ir] = dM[ir]*fh2os;
    	Madhs[ir] = dM[ir]*fadhs;
    	Mh2ol[ir] = dM[ir]*fh2ol;
    	Mnh3l[ir] = dM[ir]*fnh3l;
    }

    for (ir=0;ir<NR;ir++) dM_old[ir] = dM[ir];

	//-------------------------------------------------------------------
	//                       Initialize time loop
	//-------------------------------------------------------------------

    dtime = 0.00005*Myr2sec;  // Static time step. Make it dynamic, CFL-compliant?
    // dtime = 0.0010*Myr2sec / ((double) NR / 100.0) / ((double) NR / 100.0);
    time = -dtime;
    ntime = (int) (fulltime / dtime + 1.0e-3);
    nsteps = (int) (dtoutput / dtime + 1.0e-3);
    isteps = nsteps-1;        // Write first output at first time step

//    // DEBUG of crack routine
//    int NT_output = floor(fulltime/dtoutput)+1;
//    ntime = NT_output;
//    nsteps = 1;
//    isteps = nsteps-1;
//    dtime = dtoutput;
//
//	thermalout **thoutput = malloc(NR*sizeof(thermalout*));        // Thermal model output
//	if (thoutput == NULL) printf("IcyDwarf: Not enough memory to create the thoutput structure\n");
//	for (ir=0;ir<NR;ir++) {
//		thoutput[ir] = malloc(ntime*sizeof(thermalout));
//		if (thoutput[ir] == NULL) printf("IcyDwarf: Not enough memory to create the thoutput structure\n");
//	}
//	thoutput = read_thermal_output (thoutput, NR, ntime, path);
//	// END DEBUG

    for (itime=0;itime<ntime;itime++) {

    	time = time + dtime;

    	for (ir=0;ir<NR;ir++) time_hydr[ir] = time_hydr[ir] + dtime;

//    	// DEBUG of crack routine
//    	for (ir=0;ir<NR;ir++) {
//    		r[ir+1] = thoutput[ir][itime].radius*km2cm;
//    		T[ir] = thoutput[ir][itime].tempk;
//    		Mrock[ir] = thoutput[ir][itime].mrock;
//    		Mh2os[ir] = thoutput[ir][itime].mh2os;
//    		Madhs[ir] = thoutput[ir][itime].madhs;
//    		Mh2ol[ir] = thoutput[ir][itime].mh2ol;
//    		Mnh3l[ir] = thoutput[ir][itime].mnh3l;
//    		dM[ir] = Mrock[ir] + Mh2os[ir] + Madhs[ir] + Mh2ol[ir] + Mnh3l[ir];
//    	}
//    	// END DEBUG

    	//-------------------------------------------------------------------
    	//     Calculate pressure everywhere. This takes time, so do that
    	//          only if the structure has changed significantly
    	//-------------------------------------------------------------------

    	i = 0;
    	for (ir=0;ir<NR;ir++) {
    		Xhydr_old[ir] = Xhydr[ir];
    		dont_dehydrate[ir] = 0;
			if (fabs(dM[ir] - dM_old[ir])/dM_old[ir] > 0.05) {
				i = 1;
				break;
			}
    	}
    	for (ir=0;ir<NR;ir++) dM_old[ir] = dM[ir];
    	if (i == 1) {
    		Pressure = calculate_pressure(Pressure, NR, dM, Mrock, Mh2os, Madhs, Mh2ol, Mnh3l, r);     // Pressure
    	}

    	//-------------------------------------------------------------------
    	//               Rock hydration & dehydration, cracking
    	//-------------------------------------------------------------------

    	// DEBUG of crack routine for (ir=0;ir<NR;ir++) {
    	for (ir=0;ir<ircore;ir++) {
    		Crack_old[ir] = Crack[ir];
    		Crack_size_old[ir] = Crack_size[ir];
    		Crack[ir] = 0.0;
    		Crack_size[ir] = 0.0;
    		if (T[ir]<Tdehydr_max) {
    			crack(argc, argv, path, ir, T[ir], T_old[ir], Pressure, &Crack[ir], Crack_old[ir], &Crack_size[ir], Crack_size_old[ir],
						Xhydr[ir], Xhydr_old[ir], dtime, Mrock[ir], Mrock_init[ir], &Act[ir], &Act_old[ir],
						warnings, msgout, crack_input, crack_species, aTP, integral, alpha, beta, silica, chrysotile, magnesite);
    		}
    	}
		// Find the depth of the continuous cracked layer in contact with the ocean
    	ircrack = 0;
    	seafloor_link = 0;
		for (ir=0;ir<ircore;ir++) {
			if (Crack[ir] > 0.0) {
				seafloor_link = 1;
				for (jr=ir;jr<ircore;jr++) {
    				if (Crack[jr] <= 0.0) {                  // No crack or crack shut
    					seafloor_link = 0;
    					break;
    				}
				}
				if (seafloor_link == 1) {
					ircrack = ir;
					break;
				}
			}
		}
    	for (ir=ircrack;ir<ircore;ir++) {
    		if (T[ir] < Tdehydr_max && Mrock[ir] == dM[ir] && Xhydr[ir] <= 0.99 && time_hydr[ir] > dr/hydration_rate*Gyr2sec) {
    			time_hydr[ir] = 0.0;
    			Xhydr_temp = Xhydr[ir];
				hydrate(T[ir], &dM, dVol, &Mrock, Mh2os, Madhs, &Mh2ol, Mnh3l, &Vrock, &Vh2ol,
					rhoRockth, rhoHydrth, rhoH2olth, &Xhydr, ir, ircore, irice, NR);
				if (Xhydr[ir] >= Xhydr_temp) dont_dehydrate[ir] = 1;
    		}
    		if (T[ir] > Tdehydr_min && Xhydr[ir] >= 0.01 && dont_dehydrate[ir] == 0 && time_hydr[ir] > dr/hydration_rate*Gyr2sec) {
    			time_hydr[ir] = 0.0;
    			dehydrate(T[ir], dM[ir], dVol[ir], &Mrock[ir], &Mh2ol[ir], &Vrock[ir], &Vh2ol[ir],
    					rhoRockth, rhoHydrth, rhoH2olth, &Xhydr[ir]);
    		}
    	}

		//-------------------------------------------------------------------
		//               Allow for chemical equilibrium again
		//-------------------------------------------------------------------

		for (ir=0;ir<NR;ir++) {
			e1 = dE[ir] / dM[ir];
			frock = Mrock[ir] / dM[ir];
			fh2os = Mh2os[ir] / dM[ir];
			fadhs = Madhs[ir] / dM[ir];
			fh2ol = Mh2ol[ir] / dM[ir];
			fnh3l = Mnh3l[ir] / dM[ir];
	    	state (path, itime, ir, e1, &frock, &fh2os, &fadhs, &fh2ol, &fnh3l, &temp1);
			T[ir] = temp1;
			Mrock[ir] = dM[ir]*frock;
			Mh2os[ir] = dM[ir]*fh2os;
			Madhs[ir] = dM[ir]*fadhs;
			Mh2ol[ir] = dM[ir]*fh2ol;
			Mnh3l[ir] = dM[ir]*fnh3l;
		}

    	//-------------------------------------------------------------------
    	//           Differentiate the rock, liquids, and H2O ice
    	//-------------------------------------------------------------------

    	Tdiff = 140.0;

    	for (ir=0;ir<NR-1;ir++) {
    		if (ir > irdiff && T[ir] > Tdiff) {
    			irdiff = ir;
    		}
    	}

    	if (irdiff > 0) {
    		separate(NR, &irdiff, &ircore, &irice, dVol, &dM, &dE, &Mrock, &Mh2os, &Madhs, &Mh2ol, &Mnh3l,
    				 &Vrock, &Vh2os, &Vadhs, &Vh2ol, &Vnh3l, &Erock, &Eh2os, &Eslush, rhoH2olth, rhoNh3lth);
    	}

    	// Update Xhydr. Xhydr is defined only in layers full of rock.
    	// But setting Xhydr to 0 elsewhere craches the code, because it upsets separate().
    	for (ir=0;ir<ircore;ir++) {
    		if (Mrock[ir] > Mrock_init[ir]) { // ircore is the first layer not full of rock, we stop one layer before that
				Xhydr[ir] = (Mrock[ir]/Vrock[ir] - rhoRockth) / (rhoHydrth - rhoRockth);
				if (Xhydr[ir] < 1.0e-10) Xhydr[ir] = 0.0;   // Avoid numerical residuals
				if (Xhydr[ir] > 1.0-1.0e-10) Xhydr[ir] = 1.0;
    		}
    	}

		//-------------------------------------------------------------------
		//               Allow for chemical equilibrium again
		//-------------------------------------------------------------------

		for (ir=0;ir<NR;ir++) {
			e1 = dE[ir] / dM[ir];
			frock = Mrock[ir] / dM[ir];
			fh2os = Mh2os[ir] / dM[ir];
			fadhs = Madhs[ir] / dM[ir];
			fh2ol = Mh2ol[ir] / dM[ir];
			fnh3l = Mnh3l[ir] / dM[ir];
	    	state (path, itime, ir, e1, &frock, &fh2os, &fadhs, &fh2ol, &fnh3l, &temp1);
			T[ir] = temp1;
			Mrock[ir] = dM[ir]*frock;
			Mh2os[ir] = dM[ir]*fh2os;
			Madhs[ir] = dM[ir]*fadhs;
			Mh2ol[ir] = dM[ir]*fh2ol;
			Mnh3l[ir] = dM[ir]*fnh3l;
		}

		//-------------------------------------------------------------------
		//                    Find gravitational energy
		//-------------------------------------------------------------------

		Phiold = Phi;
		Phi = 0.6*Gcgs*dM[0]*dM[0]/r[1];
		M[0] = dM[0];

		for (ir=1;ir<NR;ir++) {
			ravg = (r[ir+1]+r[ir]) / 2.0;
			Phi = Phi + Gcgs*M[ir-1]*dM[ir] / ravg;
			M[ir] = M[ir-1] + dM[ir];
		}

		if (fabs(Phi-Phiold) < 1.0e-5*Phiold)
			Phi = Phiold;

		//-------------------------------------------------------------------
		// Calculate heating from:
		// - radioactive decay in rocky layers
		// - gravitational potential energy release in differentiated layers
		// - hydration / dehydration (cooling)
		//-------------------------------------------------------------------

		decay(&time, &tzero, &S);
		for (ir=0;ir<NR;ir++) {
			Qth[ir] = Mrock[ir]*S;
		}

		if (irdiff > 0) {
			Volume1 = 0.0;
			for (ir=0;ir<=irdiff;ir++) {
				Volume1 = Volume1 + dVol[ir];
			}
			for (ir=0;ir<=irdiff;ir++) {
				Qth[ir] = Qth[ir] + (Phi-Phiold)/dtime * (dVol[ir]/Volume1);
			}
		}

		for (ir=0;ir<ircore;ir++) {
			if (fabs(Xhydr_old[ir] - Xhydr[ir]) > 0.01) {
				Qth[ir] = Qth[ir] + (Xhydr[ir] - Xhydr_old[ir])*Mrock[ir]*Hhydr/dtime;
			}
		}
		// Compare the different energy sources
		// ir = 150;
		// printf("%d %g %g %g\n",itime, Mrock[ir]*S, (Phi-Phiold)/dtime * (dVol[ir]/Volume1), (Xhydr[ir] - Xhydr_old[ir])*Mrock[ir]*Hhydr/dtime);

		//-------------------------------------------------------------------
		// Calculate fluxes.
		// Fluxes are assumed to be conductive everywhere except where the
		// liquid fraction exceeds a small amount (2%), in which case the
		// conductivity is set to a large but reasonable value, 400 W/m/K.
		//-------------------------------------------------------------------

		for (ir=0;ir<NR;ir++) {
			frock = fabs(Vrock[ir] / dVol[ir]);
			fh2os = fabs(Vh2os[ir] / dVol[ir]);
			fadhs = fabs(Vadhs[ir] / dVol[ir]);
			fh2ol = fabs(Vh2ol[ir] / dVol[ir]);
			fnh3l = fabs(Vnh3l[ir] / dVol[ir]);
			kap1 = S / (4.0*PI_greek*Gcgs);

			kapcond(T[ir], frock, fh2os, fadhs, fh2ol, fnh3l, &kap1, Xhydr[ir]);

			if (fh2ol + fnh3l >= 0.02)
				kappa[ir] = 400.0*1.0e5;  // cgs
			else
				kappa[ir] = kap1;
		}

		//-------------------------------------------------------------------
		//       Convection in cracked layer (hydrothermal circulation)
		//-------------------------------------------------------------------

		Vcracked = 0.0;
		Vliq = 0.0;

		if (ircrack > 0 && ircrack < ircore && Mh2ol[ircore] > 0.0) {
			// Calculate Rayleigh number
			mu1 = 10.0*viscosity(T[ircore],Mh2ol[ircore],Mnh3l[ircore]);
			kap1 = rhoH2olth*ch2ol/porosity*permeability/cm/cm/mu1*(Pressure[ircrack]-Pressure[ircore]);
			alf1 = 1.0e-3;
			cp1 = ch2ol;                   // cgs
			dT = T[ircrack] - T[ircore];
			dr = r[ircore+1] - r[ircrack+1];
			jr = floor(((double)ircrack + (double)ircore)/2.0);
			g1 = Gcgs*M[jr]/(r[jr+1]*r[jr+1]);
			Ra = alf1*g1*dT*dr*dr*dr*cp1*rhoH2olth*rhoH2olth / (kap1*mu1);

			if (Ra > Ra_cr) {
				// Calculate volumes of liquid water and fractured rock
				for (ir=ircrack;ir<ircore;ir++) {
					Vcracked = Vcracked + dVol[ir];
				}
				for (ir=ircore;ir<irice;ir++) { // Check if there is enough liquid to circulate
					Vliq = Vliq + Vh2ol[ir];
				}

				if (Vliq >= porosity*Vcracked) {
					for (ir=ircrack;ir<=ircore;ir++) {  // Capped at kap_hydro for numerical stability
						if (kap1 < kap_hydro) kappa[ir] = kap1;
						else kappa[ir] = kap_hydro;
					}
				}
			}
		}

		//-------------------------------------------------------------------
		//                    Convection in H2O(s) layer
		//-------------------------------------------------------------------

		// Reset Nu at each iteration
		for (ir=0;ir<NR;ir++) Nu[ir] = 1.0;

		if (irdiff >= irice+2) {
			jr = irice + 1;
			alf1 = -0.5 + 6.0*(T[jr]-50.0)/200.0; // Not as in D09!
			alf1 = alf1 * 1.0e-5;
			cp1 = 7.73e4*T[jr];                   // cgs
			kap1 = 5.67e7/T[jr];                  // cgs
			mu1 = (1.0e15)*exp(25.0*(273.0/T[jr]-1.0)); // 1.0e14 in SI
			dT = T[irice] - T[irdiff];
			dr = r[irdiff+1] - r[irice+1];
			jr = (int) (irice+irdiff)/2;
			g1 = Gcgs*M[jr]/(r[jr+1]*r[jr+1]);
			Ra = alf1*g1*dT*dr*dr*dr*cp1*rhoH2osth*rhoH2osth / (kap1*mu1);
			Nu0 = pow((Ra/1100.0),0.25);

			if (Nu0 > 1.0) {
				for (jr=irice+1;jr<irdiff;jr++) {
					Nu[jr] = Nu0;
				}
			}
		}

		for (ir=irice;ir<=irdiff;ir++) {
			kappa[ir] = kappa[ir]*Nu[ir];
		}

		//-------------------------------------------------------------------
		//              Calculate conductive fluxes everywhere
		//-------------------------------------------------------------------

		for (ir=1;ir<NR;ir++) {
			RRflux[ir] = -r[ir]*r[ir]*(kappa[ir]+kappa[ir-1]) * (T[ir]-T[ir-1]) / (r[ir+1]-r[ir-1]);
		}

		//-------------------------------------------------------------------
		//                   Solve heat diffusion equation
		//-------------------------------------------------------------------

    	for (ir=0;ir<NR;ir++) {
    		T_old[ir] = T[ir];  // Memorize temperature
    	}

		// Heat equation
		for (ir=0;ir<NR-1;ir++) {
			dE[ir] = dE[ir] + dtime*Qth[ir] + 4*PI_greek*dtime*(RRflux[ir]-RRflux[ir+1]);
		}

		// Chemical equilibrium
		for (ir=0;ir<NR;ir++) {
			e1 = dE[ir] / dM[ir];
			frock = Mrock[ir] / dM[ir];
			fh2os = Mh2os[ir] / dM[ir];
			fadhs = Madhs[ir] / dM[ir];
			fh2ol = Mh2ol[ir] / dM[ir];
			fnh3l = Mnh3l[ir] / dM[ir];
	    	state (path, itime, ir, e1, &frock, &fh2os, &fadhs, &fh2ol, &fnh3l, &temp1);
	    	T[ir] = temp1;
			Mrock[ir] = dM[ir]*frock;
			Mh2os[ir] = dM[ir]*fh2os;
			Madhs[ir] = dM[ir]*fadhs;
			Mh2ol[ir] = dM[ir]*fh2ol;
			Mnh3l[ir] = dM[ir]*fnh3l;
		}

		// Update energies
		for (ir=0;ir<NR-1;ir++) {
			Erock[ir] = heatRock(T[ir])*Mrock[ir];
			e1 = qh2o*T[ir]*T[ir]/2.0;
			e2 = qadh*T[ir]*T[ir]/2.0;
			Eh2os[ir] = e1*Mh2os[ir];
			if (dM[ir] == Mrock[ir]+Mh2os[ir])
				Eslush[ir] = 0.0;
			else
				Eslush[ir] = dE[ir] - Eh2os[ir] - Erock[ir];
		}

		// Surface boundary condition (applied when phases found)
		// Unnecessary since all parameters are already set to the values specified? The boundary condition is
    	// really given by looking for Tdiff and updating the energies only up to NR-2, so NR-1 is always left unchanged.
    	Mrock[NR-1] = dM[NR-1]*frockp;
		Mh2os[NR-1] = dM[NR-1]*(1.0-frockp)*(1.0-Xp/Xc);
		Madhs[NR-1] = dM[NR-1]*(1.0-frockp)*Xp/Xc;
		Mh2ol[NR-1] = 0.0;
		Mnh3l[NR-1] = 0.0;
		Erock[NR-1] = heatRock(Tsurf)*Mrock[NR-1];
		e1 = qh2o*Tsurf*Tsurf/2.0;
		e2 = qadh*Tsurf*Tsurf/2.0;
		Eh2os[NR-1] = e1*Mh2os[NR-1];
		Eslush[NR-1] = e2*Madhs[NR-1];
		dE[NR-1] = Erock[NR-1] + Eh2os[NR-1] + Eslush[NR-1];

		// Update volumes
		for (ir=0;ir<NR;ir++) {
			Vrock[ir] = Mrock[ir] / (Xhydr[ir]*rhoHydrth+(1.0-Xhydr[ir])*rhoRockth); // /rhoRockth where there is no rock (Xhydr = 0)
			Vh2os[ir] = Mh2os[ir] / rhoH2osth;
			Vadhs[ir] = Madhs[ir] / rhoAdhsth;
			Vh2ol[ir] = Mh2ol[ir] / rhoH2olth;
			Vnh3l[ir] = Mnh3l[ir] / rhoNh3lth;
		}

		//-------------------------------------------------------------------
		//                           Write output
		//-------------------------------------------------------------------

		isteps++;
		if (isteps == nsteps) {
			isteps = 0;
			// Thermal outputs
			for (ir=0;ir<NR;ir++) {
				Thermal[0] = r[ir+1]/km2cm;
				Thermal[1] = T[ir];
				Thermal[2] = Mrock[ir];
				Thermal[3] = Mh2os[ir];
				Thermal[4] = Madhs[ir];
				Thermal[5] = Mh2ol[ir];
				Thermal[6] = Mnh3l[ir];
				Thermal[7] = Nu[ir];
				Thermal[8] = Xhydr[ir];
				append_output(9, Thermal, path, "Outputs/Thermal.txt");
			}
			// Crack outputs
			append_output(NR, Crack, path, "Outputs/Crack.txt");        // Crack type

			// Crack depth (km)
			Crack_depth[0] = (double) itime*dtime/Gyr2sec;              // t in Myr
			for (ir=0;ir<NR;ir++) {
				if (Crack[ir] > 0.0) break;
			}

			Crack_depth[1] = (double) (ircore-ir)/(double)NR*r_p/km2cm;
			if (Crack_depth[1] < 0.0) Crack_depth[1] = 0.0;
			append_output(2, Crack_depth, path, "Outputs/Crack_depth.txt");

			// Water:rock ratio by mass in cracked layer
			// Depends entirely on porosity! The W/R by volume is porosity. Here, we say W/R = Mliq/Mcracked_rock.
			WRratio[0] = (double) itime*dtime/Gyr2sec;                   // t in Gyr
			Mliq = 0.0;
			for (ir=0;ir<NR;ir++) {
				Mliq = Mliq + Mh2ol[ir] + Mnh3l[ir];
			}
			Mcracked_rock = 0.0;
			for (ir=0;ir<NR;ir++) {
				if (Crack[ir] > 0.0) {
					Mcracked_rock = Mcracked_rock + Mrock[ir];
				}
			}
			if (Mcracked_rock < 0.000001) WRratio[1] = 0.0;              // If Mcracked_rock is essentially 0, to avoid infinities
			else WRratio[1] = Mliq/Mcracked_rock;
			append_output(2, WRratio, path, "Outputs/Crack_WRratio.txt");
		}

		//-------------------------------------------------------------------
		//                          End time loop
		//-------------------------------------------------------------------

    }

	//-------------------------------------------------------------------
	//                           Free mallocs
	//-------------------------------------------------------------------

	for (i=0;i<int_size;i++) {
		free (integral[i]);
	}
	for (i=0;i<sizeaTP;i++) {
		free (aTP[i]);
		free (alpha[i]);
		free (beta[i]);
		free(silica[i]);
		free(chrysotile[i]);
		free(magnesite[i]);
	}
	free (r);
	free (dVol);
	free (dM);
	free (dM_old);
	free (M);
	free (dE);
	free (Mrock);
	free (Mh2os);
	free (Madhs);
	free (Mh2ol);
	free (Mnh3l);
	free (Erock);
	free (Eh2os);
	free (Eslush);
	free (Vrock);
	free (Vh2os);
	free (Vadhs);
	free (Vh2ol);
	free (Vnh3l);
	free (T);
	free (T_old);
	free (kappa);
	free (RRflux);
	free (Qth);
	free (Xhydr_old);
	free (Pressure);
	free (Crack);
	free (Crack_old);
	free (Crack_size);
	free (Crack_size_old);
	free (Nu);
	free (dont_dehydrate);
	free (Mrock_init);
	free (time_hydr);
	free (aTP);             // Thermal mismatch-specific
	free (integral);
	free (alpha);           // Pore water expansion-specific
	free (beta);
	free (silica);          // Dissolution/precipitation-specific
	free (chrysotile);
	free (magnesite);

	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine state
 *
 * Inputs: Total Energy E, rock fraction frock, and current ice phase
 *  compositions.
 * Outputs: Temperature T and ice phase compositions consistent with
 *  total energy E (passed as pointers to return several outputs
 *  as in the original FORTRAN code).
 *
 *--------------------------------------------------------------------*/

int state (char path[1024], int itime, int ir, double E, double *frock, double *fh2os, double *fadhs, double *fh2ol, double *fnh3l, double *T) {

	int iter = 0;

	double X = 0.0;         // Total solid (dihydrate) + liquid ammonia fraction

	double Tlo = 0.0;       // Low temperature limit (K) for equation of state of material
	double Thi = 0.0;       // High temperature limit (K)
	double Tmd = 0.0;
	double Tp = 0.0;
	double Elo = 0.0;       // Low energy limit (J) for equation of state of material
	double Ehi = 0.0;       // High energy limit (J)
	double Emd = 0.0;

	double Erock = 0.0;     // Energy of the rock
	double Eice = 0.0;      // Energy of the ice

	double gh2os = 0.0;     // Mass fraction of water ice in ice shell or crust
	double gadhs = 0.0;     // Mass fraction of ammonia dihydrate ice in ice shell or crust
	double gh2ol = 0.0;     // Mass fraction of liquid water in ice shell or crust
	double gnh3l = 0.0;     // Mass fraction of liquid ammonia in ice shell or crust

	if ((*frock) < 1.0) {
		gh2os = (*fh2os) / (1.0-(*frock));
		gadhs = (*fadhs) / (1.0-(*frock));
		gh2ol = (*fh2ol) / (1.0-(*frock));
		gnh3l = (*fnh3l) / (1.0-(*frock));
		X = gnh3l + Xc*gadhs;
	}

	// Bisect to find the temperature from the energy
	Tlo = 20.0;
	Thi = 3000.0;
	Tmd = (Tlo+Thi)/2.0;

	for (iter=0;iter<30;iter++) {
		// Calculate Elo
		Tp = Tlo;
    	Erock = heatRock(Tp);
    	heatIce (Tp, X, &Eice, &gh2os, &gadhs, &gh2ol, &gnh3l);
    	Elo = (*frock)*Erock + (1.0-(*frock))*Eice;

    	// Calculate Emd
		Tp = Tmd;
		Erock = heatRock(Tp);
		heatIce (Tp, X, &Eice, &gh2os, &gadhs, &gh2ol, &gnh3l);
    	Emd = (*frock)*Erock + (1.0-(*frock))*Eice;

    	// Calculate Ehi
		Tp = Thi;
		Erock = heatRock(Tp);
    	heatIce (Tp, X, &Eice, &gh2os, &gadhs, &gh2ol, &gnh3l);
    	Ehi = (*frock)*Erock + (1.0-(*frock))*Eice;

    	if (E >= Elo && E <= Ehi && Elo > 0.0 && Ehi > 0.0 && Emd > 0.0) {
    		if (E <= Emd) {
    			Thi = Tmd;
    		}
    		else {
    			Tlo = Tmd;
    		}
    		Tmd = (Tlo + Thi)/2.0;
    	}
    	else {
    		printf("Thermal: Could not compute temperature\n");
    		printf("Thermal: itime=%d, ir=%d, iter=%d\n",itime, ir, iter);
    		printf("Thermal: Tlo=%g K, Thi=%g K, Tmd=%g K\n", Tlo, Thi, Tmd);
    		printf("Thermal: Elo=%g, Ehi=%g, Emd=%g, E=%g\n", Elo, Ehi, Emd, E);
    		printf("Thermal: frock=%g, gh2os=%g, gadhs=%g, gh2ol=%g, gnh3l=%g, X=%g\n", (*frock), gh2os, gadhs, gh2ol, gnh3l, X);

    		FILE *fout;

    		// Turn working directory into full file path by moving up two directories
    		// to IcyDwarf (e.g., removing "Release/IcyDwarf" characters) and specifying
    		// the right path end.

    		char *title = (char*)malloc(1024*sizeof(char));       // Don't forget to free!
    		title[0] = '\0';
    		if (release == 1) strncat(title,path,strlen(path)-16);
    		else if (cmdline == 1) strncat(title,path,strlen(path)-18);
    		strcat(title,"Outputs/Thermal.txt");

    		fout = fopen(title,"a");
    		if (fout == NULL) {
    			printf("IcyDwarf: Error opening %s output file.\n",title);
    		}
    		else {
    	  		fprintf(fout,"Thermal: Could not compute temperature\n");
				fprintf(fout,"Thermal: itime=%d, ir=%d\n",itime, ir);
				fprintf(fout,"Thermal: Tlo=%g K, Thi=%g K, Tmd=%g K\n", Tlo, Thi, Tmd);
				fprintf(fout,"Thermal: Elo=%g, Ehi=%g, Emd=%g, E=%g\n", Elo, Ehi, Emd, E);
				fprintf(fout,"Thermal: frock=%g, gh2os=%g, gadhs=%g, gh2ol=%g, gnh3l=%g, X=%g\n", (*frock), gh2os, gadhs, gh2ol, gnh3l, X);
    		}
    		fclose (fout);
    		free (title);
    		exit(0);
    	}
	}

	(*T) = Tmd;

	if ((*frock) == 1.0) { // Unnecessary?
		(*fh2os) = 0.0;
		(*fadhs) = 0.0;
		(*fh2ol) = 0.0;
		(*fnh3l) = 0.0;
	}
	else {
		(*fh2os) = (1.0-(*frock))*gh2os;
		(*fadhs) = (1.0-(*frock))*gadhs;
		(*fh2ol) = (1.0-(*frock))*gh2ol;
		(*fnh3l) = (1.0-(*frock))*gnh3l;
	}

	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine heatRock
 *
 * Calculates the energy of the rock using an equation of state
 * Input: T, Output: erock.
 *
 *--------------------------------------------------------------------*/

double heatRock (double T) {

	double erock = 0.0;              // Output

	erock = ErockA*T*T;
	if (T > 275.0) {
		erock = ErockA*275.0*275.0 + (T-275.0)*(ErockC+ErockD*T);
		if (T > 1000.0) {
			erock = ErockA*275.0*275.0 + (1000.0-275.0)*(ErockC+ErockD*1000.0) + (ErockF)*(T-1000.0);
		}
	}

	return erock;
}

/*--------------------------------------------------------------------
 *
 * Subroutine heatIce
 *
 * Calculates the energy of the ice using an equation of state
 * Inputs: T, X_NH3.
 * Outputs: Eice, mass fractions w.r.t. bulk ice (passed as pointers
 * to return several outputs as in the original FORTRAN code).
 *
 *--------------------------------------------------------------------*/

int heatIce (double T, double X, double *E, double *gh2os, double *gadhs, double *gh2ol, double *gnh3l){

	double Xb = 0.0;        // Specific point on simplified, analytical phase diagram with quadratic equation
	double Xliq = 0.0;      // Ammonia fraction of the liquid
	double Tliq = 0.0;      // Temperature of the liquid (K)
	double T2 = 0.0;        // Temporary temperature (K)
	double r = 0.0;         // Square root

	Xb = Xc*sqrt(2.0/95.0); // Artificial point of simplified phase diagram of H2O-NH3 system

	(*E) = 0.0;

	// The H2O-NH3 phase diagram is divided into 9 regions and simplified to be analytically tractable.

	// Low-ammonia case
	if (X <= Xb) {

		// Low NH3 - Region 1
		if (T <= 174.0) {
			T2 = T;
			(*E) = 0.5*qh2o*T2*T2 + (X/Xc)*0.5*(qadh-qh2o)*T2*T2;
			(*gh2os) = 1.0 - X/Xc;
			(*gadhs) = X/Xc;
			(*gh2ol) = 0.0;
			(*gnh3l) = 0.0;
			return 1;
		}
		T2 = 174.0;
		(*E) = 0.5*qh2o*T2*T2 + (X/Xc)*0.5*(qadh-qh2o)*T2*T2;

		// Low NH3 - Region 2
		if (T > 174.0 && T <= 178.0) {
			T2 = T;
			(*E) = (*E) + (1.0-X/Xc)*0.5*qh2o*(T2*T2 - 174.0*174.0)
					    + X/Xc*(T2-174.0)/4.0* ( ladh
							                   + (182.0 - T2)/2.0*qadh*174.0
							                   + Xc*(T2-174.0/2.0*cnh3l)
							                   + (1.0-Xc)*(T2-174.0)/2.0*ch2ol);
			(*gh2os) = 1.0-X/Xc;
			(*gadhs) = X/Xc*(178.0-T)/4.0;
			(*gh2ol) = X/Xc*(T-174.0)/4.0*(1.0-Xc);
			(*gnh3l) = X/Xc*(T-174.0)/4.0*Xc;
			return 2;
		}
		T2 = 178.0;
		(*E) = (*E) + (1.0-X/Xc)*0.5*qh2o*(T2*T2 - 174.0*174.0)
				    + X/Xc*(T2-174.0)/4.0* ( ladh
					                   	   + (182.0 - T2)/2.0*qadh*174.0
						                   + Xc*(T2-174.0/2.0*cnh3l)
						                   + (1.0-Xc)*(T2-174.0)/2.0*ch2ol);

		// Low NH3 - Region 3
		if (T > 178.0 && T <= 271.0) {
			T2 = T;
			r = sqrt((273.0-T2)/95.0);
			Xliq = Xc*r;
			(*E) = (*E) + 0.5*qh2o*(T2*T2 - 178.0*178.0)
					    + X*(cnh3l-ch2ol)*(T2-178.0)
					    + X/Xc*(1.0-r)* (  lh2o/r
							             + 2.0*95.0*ch2ol
							             - 2.0*95.0*qh2o*273.0
							             + 2.0*qh2o*95.0*95.0/3.0*(1.0+r+r*r) );
			(*gh2os) = 1.0 - X/Xliq;
			(*gadhs) = 0.0;
			(*gh2ol) = X/Xliq - X;
			(*gnh3l) = X;
			return 3;
		}
		T2 = 271.0;
		r = sqrt((273.0-T2)/95.0);
		(*E) = (*E) + 0.5*qh2o*(T2*T2 - 178.0*178.0)
				    + X*(cnh3l-ch2ol)*(T2-178.0)
				    + X/Xc*(1.0-r)* (  lh2o/r
						             + 2.0*95.0*ch2ol
						             - 2.0*95.0*qh2o*273.0
						             + 2.0*qh2o*95.0*95.0/3.0*(1.0+r+r*r) );

		// Low NH3 - Region 4
		if (T > 271.0 && T <= 275.0) {
			T2 = T;
			(*E) = (*E) + (T2-271.0)*X*(cnh3l + ch2ol*(1.0/Xb - 1.0))
					    + (1.0-X/Xb)*(T2-271.0)/4.0 * (  lh2o
							                           + 0.5*ch2ol*(T2-271.0)
							                           + 0.5*qh2o*271.0*(279.0-T2));
			(*gh2os) = (1.0-X/Xb)*(275.0-T)/4.0;
			(*gadhs) = 0.0;
			(*gh2ol) = (1.0-X/Xb)*(T-271.0)/4.0 + (X/Xb - X);
			(*gnh3l) = X;
			return 4;
		}
		T2 = 275.0;
		(*E) = (*E) + (T2-271.0)*X*(cnh3l + ch2ol*(1.0/Xb - 1.0))
				    + (1.0-X/Xb)*(T2-271.0)/4.0 * (  lh2o
						                           + 0.5*ch2ol*(T2-271.0)
						                           + 0.5*qh2o*271.0*(279.0-T2));

		// Low NH3 - Region 5
		if (T > 275.0) {
			(*E) = (*E) + (X*cnh3l + (1.0-X)*ch2ol)*(T-275.0);
			(*gh2os) = 0.0;
			(*gadhs) = 0.0;
			(*gh2ol) = 1.0-X;
			(*gnh3l) = X;
		}
	}

	// High-ammonia case
	else {
		Tliq = 273.0 - 95.0*(X/Xc)*(X/Xc);

		// High NH3 - Region 1
		if (T <= 174.0) {
			T2 = T;
			(*E) = 0.5*qh2o*T2*T2 + X/Xc*0.5*(qadh-qh2o)*T2*T2;
			(*gh2os) = 1.0 - X/Xc;
			(*gadhs) = X/Xc;
			(*gh2ol) = 0.0;
			(*gnh3l) = 0.0;
			return 6;
		}
		T2 = 174.0;
		(*E) = 0.5*qh2o*T2*T2 + X/Xc*0.5*(qadh-qh2o)*T2*T2;

		// High NH3 - Region 2
		if (T > 174.0 && T <= 178.0) {
			T2 = T;
			(*E) = (*E) + (1.0-X/Xc)*0.5*qh2o*(T2*T2 - 174.0*174.0)
					    + X/Xc*(T2-174.0)/4.0 * (  ladh
							                     + (182.0-T2)/2.0*qadh*174.0
							                     + Xc*(T2-174.0)/2.0*cnh3l
							                     + (1.0-Xc)*(T2-174.0)/2.0*ch2ol );
			(*gh2os) = 1.0 - X/Xc;
			(*gadhs) = X/Xc*(178.0-T)/4.0;
			(*gh2ol) = X/Xc*(T-174.0)/4.0*(1.0-Xc);
			(*gnh3l) = X/Xc*(T-174.0)/4.0*Xc;
			return 7;
		}
		T2 = 178.0;
		(*E) = (*E) + (1.0-X/Xc)*0.5*qh2o*(T2*T2 - 174.0*174.0)
				    + X/Xc*(T2-174.0)/4.0 * (  ladh
						                     + (182.0-T2)/2.0*qadh*174.0
						                     + Xc*(T2-174.0)/2.0*cnh3l
						                     + (1.0-Xc)*(T2-174.0)/2.0*ch2ol );

		// High NH3 - Region 3
		if (T > 178.0 && T <= Tliq) {
			T2 = T;
			r = sqrt((273.0-T2)/95.0);
			Xliq = Xc*r;
			(*E) = (*E) + 0.5*qh2o*(T2*T2 - 178.0*178.0)
					    + X*(cnh3l-ch2ol)*(T2-178.0)
					    + X/Xc*(1.0-r) * (  lh2o/r
						    	          + 2.0*95.0*ch2ol
							              - 2.0*95.0*qh2o*273.0
							              + 2.0*qh2o*95.0*95.0/3.0*(1.0+r+r*r) );
			(*gh2os) = 1.0 - X/Xliq;
			(*gadhs) = 0.0;
			(*gh2ol) = X/Xliq - X;
			(*gnh3l) = X;
			return 8;
		}
		T2 = Tliq;
		r = sqrt((273.0-T2)/95.0);
		// Xliq = Xc*r; // Unnecessary? See low NH3 region 3.
		(*E) = (*E) + 0.5*qh2o*(T2*T2 - 178.0*178.0)
				    + X*(cnh3l-ch2ol)*(T2-178.0)
				    + X/Xc*(1.0-r) * (  lh2o/r
						              + 2.0*95.0*ch2ol
						              - 2.0*95.0*qh2o*273.0
						              + 2.0*qh2o*95.0*95.0/3.0*(1.0+r+r*r) );

		// High NH3 - Region 5
		if (T > Tliq) {
			(*E) = (*E) + (X*cnh3l + (1.0-X)*ch2ol) * (T-Tliq);
			(*gh2os) = 0.0;
			(*gadhs) = 0.0;
			(*gh2ol) = 1.0-X;
			(*gnh3l) = X;
		}
	}
	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine kapcond
 *
 * Inputs: Temperature T, volume fractions, thermal cond. kap = S/4piG
 * Output: Thermal conductivity kap
 *
 *--------------------------------------------------------------------*/

int kapcond(double T, double frock, double fh2os, double fadhs, double fh2ol, double fnh3l, double *kap, double Xhydr) {

    double kaph2os = 5.67e7/T;  // Thermal conductivity of water ice (cgs) (Klinger 1980)

	double kapice = 0.0;
	double b1 = 0.0;            // Coefs of the quadratic equation of Sirono and Yamamoto (1997) to combine
	double c1 = 0.0;            // rock and ice conductivities

	// Combined conductivities (equations (7) and (8) of D09)
	if (frock == 1.0)
		(*kap) = Xhydr*kaphydr + (1-Xhydr)*kaprock;
	else {
		// Geometric mean for ice phases (equation 7)
		kapice = fh2os*log(kaph2os) + fadhs*log(kapadhs) + fh2ol*log(kaph2ol) + fnh3l*log(kapnh3l);
		kapice = kapice / (fh2os+fadhs+fh2ol+fnh3l);
		kapice = exp(kapice);
		// Using the formulation of Sirono and Yamamoto 1997 for rock-ice phases (eq. 8)
		b1 = -kaprock*(3.0*frock - 1.0) - kapice*(2.0 - 3.0*frock);
		c1 = -kaprock*kapice;
		(*kap) = (-b1 + sqrt(b1*b1 - 8.0*c1)) / 4.0;
	}

	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine decay
 *
 * Input: t, time since solar system formation
 * Output: S, rate at which heat energy is released per gram of rock
 *
 *--------------------------------------------------------------------*/

int decay(double *t, double *tzero, double *S) {

	double si = 1.0 / (1.0e6 * 1.67e-24 * 151.0); // Grams^-1 / # of Si atoms: 1e6 atoms * nucleon mass in grams * avg. molar mass of rock

	// The rate of radiogenic heating due to an isotope x with half-life t1/2, per mass of that isotope,
	// is (DeltaE)_x (ln 2/t1/2)/m_x , where m_x is the mass of an atom of x, and DeltaE_x the heat energy per decay.
	// These DeltaE_x include heating due to emission of alpha and beta particles and gamma rays, but not the emission of neutrinos, which escape planets.
	// Because of the loss of neutrino energies, the radiogenic heating rate cannot be determined from the mass deficit between the parent and daughter nuclei alone.
	// The uncertainties in the neutrino energy during radioactive decay are often on the order of 10%, and lead to uncertainties almost as large in DeltaE_x.
	// The assumed values of t1/2 and DeltaE_x for each radionuclide are:
	// Radionuclide  t1/2 (Gyr)  DeltaE (MeV)
	// ------------  ----------  ------------
	//  40 K         1.265       0.6087
	// 235 U         0.704       42.74
	// 238 U         4.47        46.07
	// 232 Th        14.0        38.96
	//  26 Al        0.000716    8.410e4

	// ln 2 = 0.6931

	// Long-lived radionuclides
	(*S) = 5.244   * 0.6087      / 1.265 * exp(-(*t)*0.6931/(1.265*Gyr2sec))  // 40 K
	     + 0.00592 * (46.74-4.0) / 0.704 * exp(-(*t)*0.6931/(0.704*Gyr2sec))  // 235 U
	     + 0.01871 * (52.07-6.0) / 4.47  * exp(-(*t)*0.6931/(4.47 *Gyr2sec))  // 238 U
         + 0.04399 * (42.96-4.0) / 14.0  * exp(-(*t)*0.6931/(14.0 *Gyr2sec)); // 232 Th

	// Short-lived radionuclides. Why one extra term?
	(*S) = (*S) + 5.0e-5 * 8.410e4 * 3.117 / 0.000716 * exp(-((*t)+(*tzero))*0.6931/(0.000716*Gyr2sec)); // 26 Al

	(*S) = (*S) * si*MeV2erg/Gyr2sec*0.6931;
	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine separate
 *
 * Moves rock, ice, and slush around as the body differentiates.
 *
 *--------------------------------------------------------------------*/

int separate(int NR, int *irdiff, int *ircore, int *irice, double *dVol, double **dM, double **dE, double **Mrock, double **Mh2os, double **Madhs,
		double **Mh2ol, double **Mnh3l, double **Vrock, double **Vh2os, double **Vadhs, double **Vh2ol, double **Vnh3l, double **Erock,
		double **Eh2os, double **Eslush, double rhoH2olth, double rhoNh3lth){

	int ir = 0;
	int jr = 0;

	double *Mrocknew = (double*) malloc((NR)*sizeof(double));      // New mass of rock
	if (Mrocknew == NULL) printf("Thermal: Not enough memory to create Mrocknew[NR]\n");

	double *Mh2osnew = (double*) malloc((NR)*sizeof(double));      // New mass of water ice
	if (Mh2osnew == NULL) printf("Thermal: Not enough memory to create Mh2osnew[NR]\n");

	double *Madhsnew = (double*) malloc((NR)*sizeof(double));      // New mass of ADH ice
	if (Madhsnew == NULL) printf("Thermal: Not enough memory to create Madhsnew[NR]\n");

	double *Mh2olnew = (double*) malloc((NR)*sizeof(double));      // New mass of liquid water
	if (Mh2olnew == NULL) printf("Thermal: Not enough memory to create Mh2olnew[NR]\n");

	double *Mnh3lnew = (double*) malloc((NR)*sizeof(double));      // New mass of liquid ammonia
	if (Mnh3lnew == NULL) printf("Thermal: Not enough memory to create Mnh3lnew[NR]\n");

	double *Vrocknew = (double*) malloc((NR)*sizeof(double));      // New volume of rock
	if (Vrocknew == NULL) printf("Thermal: Not enough memory to create Vrocknew[NR]\n");

	double *Vh2osnew = (double*) malloc((NR)*sizeof(double));      // New volume of water ice
	if (Vh2osnew == NULL) printf("Thermal: Not enough memory to create Vh2osnew[NR]\n");

	double *Vadhsnew = (double*) malloc((NR)*sizeof(double));      // New volume of ADH ice
	if (Vadhsnew == NULL) printf("Thermal: Not enough memory to create Vadhsnew[NR]\n");

	double *Vh2olnew = (double*) malloc((NR)*sizeof(double));      // New volume of liquid water
	if (Vh2olnew == NULL) printf("Thermal: Not enough memory to create Vh2olnew[NR]\n");

	double *Vnh3lnew = (double*) malloc((NR)*sizeof(double));      // New volume of liquid ammonia
	if (Vnh3lnew == NULL) printf("Thermal: Not enough memory to create Vnh3lnew[NR]\n");

	double *Erocknew = (double*) malloc((NR)*sizeof(double));      // New energy of rock
	if (Erocknew == NULL) printf("Thermal: Not enough memory to create Erocknew[NR]\n");

	double *Eh2osnew = (double*) malloc((NR)*sizeof(double));      // New energy of water ice
	if (Eh2osnew == NULL) printf("Thermal: Not enough memory to create Eh2osnew[NR]\n");

	double *Eslushnew = (double*) malloc((NR)*sizeof(double));     // New energy of slush
	if (Eslushnew == NULL) printf("Thermal: Not enough memory to create Eslushnew[NR]\n");

	double *Volcell = (double*) malloc((NR)*sizeof(double));      // Cell volume
	if (Volcell == NULL) printf("Thermal: Not enough memory to create Volcell[NR]\n");

	double q = 0.0;                                               // Unused volume fraction of cell ir, the part that does not fit into cell jr
	double Volume1 = 0.0;
	double Volume2 = 0.0;
	double Mammonia = 0.0;
	double Mwater = 0.0;
	double Vslushtot = 0.0;
	double Eslushtot = 0.0;
	int nextcell = 0;

	for (ir=0;ir<NR;ir++) {
		Mrocknew[ir] = 0.0;
		Mh2osnew[ir] = 0.0;
		Madhsnew[ir] = 0.0;
		Mh2olnew[ir] = 0.0;
		Mnh3lnew[ir] = 0.0;
		Vrocknew[ir] = 0.0;
		Vh2osnew[ir] = 0.0;
		Vadhsnew[ir] = 0.0;
		Vh2olnew[ir] = 0.0;
		Vnh3lnew[ir] = 0.0;
		Erocknew[ir] = 0.0;
		Eh2osnew[ir] = 0.0;
		Eslushnew[ir] = 0.0;
		Volcell[ir] = 0.0;
	}

	for (jr=0;jr<=(*irdiff);jr++) {
		Volcell[jr] = dVol[jr];
	}

	//-------------------------------------------------------------------
	//                         Fill up rocky core
	//-------------------------------------------------------------------

	jr = 0;
	(*ircore) = jr;
	for (ir=0;ir<=(*irdiff);ir++) {
		if (Vrocknew[jr] > Volcell[jr] && (*Vrock)[ir] > 0.0) {
			q = (Vrocknew[jr]-Volcell[jr]) / (*Vrock)[ir];
			Vrocknew[jr] = Volcell[jr];
			Mrocknew[jr] = Mrocknew[jr] - q*(*Mrock)[ir];
			Erocknew[jr] = Erocknew[jr] - q*(*Erock)[ir];
			Volcell[jr] = 0.0;
			jr++;
			(*ircore) = jr;
			Vrocknew[jr] = q*(*Vrock)[ir];
			Mrocknew[jr] = q*(*Mrock)[ir];
			Erocknew[jr] = q*(*Erock)[ir];
		}

		Vrocknew[jr] = Vrocknew[jr] + (*Vrock)[ir];
		Mrocknew[jr] = Mrocknew[jr] + (*Mrock)[ir];
		Erocknew[jr] = Erocknew[jr] + (*Erock)[ir];

		if (Vrocknew[jr] >= Volcell[jr] && (*Vrock)[ir] > 0.0) {
			q = (Vrocknew[jr]-Volcell[jr]) / (*Vrock)[ir];
			Vrocknew[jr] = Volcell[jr];
			Mrocknew[jr] = Mrocknew[jr] - q*(*Mrock)[ir];
			Erocknew[jr] = Erocknew[jr] - q*(*Erock)[ir];
			Volcell[jr] = 0.0;
			jr++;
			(*ircore) = jr;
			Vrocknew[jr] = q*(*Vrock)[ir];
			Mrocknew[jr] = q*(*Mrock)[ir];
			Erocknew[jr] = q*(*Erock)[ir];
		}
	}
	Volcell[*ircore] = Volcell[*ircore] - Vrocknew[*ircore];

	//-------------------------------------------------------------------
	//                          Fill up slush layer
	//-------------------------------------------------------------------

	for (ir=0;ir<=(*irdiff);ir++) {
		Volume1 = Vadhsnew[jr] + Vh2olnew[jr] + Vnh3lnew[jr];
		Volume2 = (*Vadhs)[ir] + (*Vh2ol)[ir] + (*Vnh3l)[ir];
		if (Volume1 >= Volcell[jr] && Volume2 > 0.0) {
			nextcell = 1;                   // Slush fills more than one layer
			q = (Volume1-Volcell[jr]) / Volume2;
			Vh2olnew[jr] = Vh2olnew[jr] - q*(*Vh2ol)[ir];
			Vnh3lnew[jr] = Vnh3lnew[jr] - q*(*Vnh3l)[ir];
			Vadhsnew[jr] = Vadhsnew[jr] - q*(*Vadhs)[ir];
			Mh2olnew[jr] = Mh2olnew[jr] - q*(*Mh2ol)[ir];
			Mnh3lnew[jr] = Mnh3lnew[jr] - q*(*Mnh3l)[ir];
			Madhsnew[jr] = Madhsnew[jr] - q*(*Madhs)[ir];
			Eslushnew[jr] = Eslushnew[jr] - q*(*Eslush)[ir];
			Volcell[jr] = 0.0;
			jr++;
			(*irice) = jr;
			Vadhsnew[jr] = q*(*Vadhs)[ir];
			Vh2olnew[jr] = q*(*Vh2ol)[ir];
			Vnh3lnew[jr] = q*(*Vnh3l)[ir];
			Madhsnew[jr] = q*(*Madhs)[ir];
			Mh2olnew[jr] = q*(*Mh2ol)[ir];
			Mnh3lnew[jr] = q*(*Mnh3l)[ir];
			Eslushnew[jr] = q*(*Eslush)[ir];
		}

		Vadhsnew[jr] = Vadhsnew[jr] + (*Vadhs)[ir];
		Vh2olnew[jr] = Vh2olnew[jr] + (*Vh2ol)[ir];
		Vnh3lnew[jr] = Vnh3lnew[jr] + (*Vnh3l)[ir];
		Madhsnew[jr] = Madhsnew[jr] + (*Madhs)[ir];
		Mh2olnew[jr] = Mh2olnew[jr] + (*Mh2ol)[ir];
		Mnh3lnew[jr] = Mnh3lnew[jr] + (*Mnh3l)[ir];
		Eslushnew[jr] = Eslushnew[jr] + (*Eslush)[ir];

		Volume1 = Vadhsnew[jr] + Vh2olnew[jr] + Vnh3lnew[jr];
		Volume2 = (*Vadhs)[ir] + (*Vh2ol)[ir] + (*Vnh3l)[ir];
		if (Volume1 >= Volcell[jr] && Volume2 > 0.0) {
			nextcell = 1;                   // Slush fills more than one layer
			q = (Volume1-Volcell[jr]) / Volume2;
			Vh2olnew[jr] = Vh2olnew[jr] - q*(*Vh2ol)[ir];
			Vnh3lnew[jr] = Vnh3lnew[jr] - q*(*Vnh3l)[ir];
			Vadhsnew[jr] = Vadhsnew[jr] - q*(*Vadhs)[ir];
			Mh2olnew[jr] = Mh2olnew[jr] - q*(*Mh2ol)[ir];
			Mnh3lnew[jr] = Mnh3lnew[jr] - q*(*Mnh3l)[ir];
			Madhsnew[jr] = Madhsnew[jr] - q*(*Madhs)[ir];
			Eslushnew[jr] = Eslushnew[jr] - q*(*Eslush)[ir];
			Volcell[jr] = 0.0;
			jr++;
			(*irice) = jr;
			Vadhsnew[jr] = q*(*Vadhs)[ir];
			Vh2olnew[jr] = q*(*Vh2ol)[ir];
			Vnh3lnew[jr] = q*(*Vnh3l)[ir];
			Madhsnew[jr] = q*(*Madhs)[ir];
			Mh2olnew[jr] = q*(*Mh2ol)[ir];
			Mnh3lnew[jr] = q*(*Mnh3l)[ir];
			Eslushnew[jr] = q*(*Eslush)[ir];
		}
	}

	if (nextcell == 0) (*irice) = jr;       // Slush fills less than one layer

	Volcell[*irice] = Volcell[*irice] - Vadhsnew[*irice] - Vh2olnew[*irice] - Vnh3lnew[*irice];

	//-------------------------------------------------------------------
	//                           Fill up ice layer
	//-------------------------------------------------------------------

	for (ir=0;ir<=(*irdiff);ir++) {
		if (Vh2osnew[jr] >= Volcell[jr] && (*Vh2os)[ir] > 0.0) {
			q = (Vh2osnew[jr] - Volcell[jr]) / (*Vh2os)[ir];
			Vh2osnew[jr] = Volcell[jr];
			Mh2osnew[jr] = Mh2osnew[jr] - q*(*Mh2os)[ir];
			Eh2osnew[jr] = Eh2osnew[jr] - q*(*Eh2os)[ir];
			Volcell[jr] = 0.0;
			jr++;
			Vh2osnew[jr] = q*(*Vh2os)[ir];
			Mh2osnew[jr] = q*(*Mh2os)[ir];
			Eh2osnew[jr] = q*(*Eh2os)[ir];
		}

		Vh2osnew[jr] = Vh2osnew[jr] + (*Vh2os)[ir];
		Mh2osnew[jr] = Mh2osnew[jr] + (*Mh2os)[ir];
		Eh2osnew[jr] = Eh2osnew[jr] + (*Eh2os)[ir];

		if (Vh2osnew[jr] >= Volcell[jr] && (*Vh2os)[ir] > 0.0) {
			q = (Vh2osnew[jr] - Volcell[jr]) / (*Vh2os)[ir];
			Vh2osnew[jr] = Volcell[jr];
			Mh2osnew[jr] = Mh2osnew[jr] - q*(*Mh2os)[ir];
			Eh2osnew[jr] = Eh2osnew[jr] - q*(*Eh2os)[ir];
			Volcell[jr] = 0.0;
			jr++;
			Vh2osnew[jr] = q*(*Vh2os)[ir];
			Mh2osnew[jr] = q*(*Mh2os)[ir];
			Eh2osnew[jr] = q*(*Eh2os)[ir];
		}
	}
	Volcell[jr] = Volcell[jr] - Vh2osnew[jr];

	//-------------------------------------------------------------------
	//                        Homogenize slush layer
	//-------------------------------------------------------------------

	for (jr=0;jr<=(*irdiff);jr++) {
		Mammonia = Mammonia + Xc*Madhsnew[jr] + Mnh3lnew[jr];
		Mwater = Mwater + (1-Xc)*Madhsnew[jr] + Mh2olnew[jr];
		Eslushtot = Eslushtot + Eslushnew[jr];
		Vslushtot = Vslushtot + Vadhsnew[jr] + Vh2olnew[jr] + Vnh3lnew[jr];
	}
	for (ir=0;ir<=(*irdiff);ir++) {
		// Rock
		(*Vrock)[ir] = Vrocknew[ir];
		(*Mrock)[ir] = Mrocknew[ir];
		(*Erock)[ir] = Erocknew[ir];

		// Slush
		Volume1 = Vadhsnew[ir] + Vh2olnew[ir] + Vnh3lnew[ir];
		(*Eslush)[ir] = (Volume1/Vslushtot) * Eslushtot;
		(*Vadhs)[ir] = 0.0;  // It's OK to set ADH = 0 because we will redo phases right away
		(*Madhs)[ir] = 0.0;
		(*Mh2ol)[ir] = Mwater*(Volume1/Vslushtot);
		(*Vh2ol)[ir] = (*Mh2ol)[ir]/rhoH2olth;
		(*Mnh3l)[ir] = Mammonia*(Volume1/Vslushtot);
		(*Vnh3l)[ir] = (*Mnh3l)[ir]/rhoNh3lth;

		// H2O ice
		(*Vh2os)[ir] = Vh2osnew[ir];
		(*Mh2os)[ir] = Mh2osnew[ir];
		(*Eh2os)[ir] = Eh2osnew[ir];

		// Totals
		(*dM)[ir] = (*Mrock)[ir] + (*Mh2os)[ir] + (*Madhs)[ir] + (*Mh2ol)[ir] + (*Mnh3l)[ir];
		(*dE)[ir] = (*Erock)[ir] + (*Eh2os)[ir] + (*Eslush)[ir];

	}

	//-------------------------------------------------------------------
	//                             Free mallocs
	//-------------------------------------------------------------------

	free (Mrocknew);
	free (Mh2osnew);
	free (Madhsnew);
	free (Mh2olnew);
	free (Mnh3lnew);
	free (Vrocknew);
	free (Vh2osnew);
	free (Vadhsnew);
	free (Vh2olnew);
	free (Vnh3lnew);
	free (Erocknew);
	free (Eh2osnew);
	free (Eslushnew);
	free (Volcell);

	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine dehydrate
 *
 * Separates hydrated rock into dry rock and liquid water in each cell
 * where T>Tdehydr_min in the core.
 *
 *--------------------------------------------------------------------*/

int dehydrate(double T, double dM, double dVol, double *Mrock, double *Mh2ol, double *Vrock,
		double *Vh2ol, double rhoRockth, double rhoHydrth, double rhoH2olth, double *Xhydr){

	// Set hydration level: 1 at Tdehydr_min, 0 at Tdehydr_max, linear in between
	if (T<Tdehydr_min) (*Xhydr) = 1.0;
	else if (T>=Tdehydr_min && T<Tdehydr_max) (*Xhydr) = 1.0 - (T-Tdehydr_min)/(Tdehydr_max-Tdehydr_min);
	else (*Xhydr) = 0.0;

	// Split cell into water and rock
	(*Vrock) = (*Mrock)/((*Xhydr)*rhoHydrth + (1.0-(*Xhydr))*rhoRockth);
	(*Vh2ol) = dVol - (*Vrock);

	(*Mh2ol) = (*Vh2ol)*rhoH2olth;
	(*Mrock) = dM - (*Mh2ol);

	// Update Xhydr: not 0 to conserve mass and volume in each shell, but has decreased
	(*Xhydr) = ((*Mrock)/(*Vrock) - rhoRockth) / (rhoHydrth - rhoRockth);
	if (fabs(*Xhydr) < 1.0e-10) (*Xhydr) = 0.0;  // Avoid numerical residuals
	if (fabs(*Xhydr) > 1.0-1.0e-10) (*Xhydr) = 1.0;

	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine hydrate
 *
 * Merges dry rock and liquid water into hydrated rock in each cell
 * that (1) has a connection with liquid water or hydrated rock and
 * (2) where T < Tdehydr_max.
 * TODO hydrate only where hydrothermal circulation can occur, and
 * take into account a slow migration of the hydration front (1 km/Myr,
 * MacDonald and Fyfe, 1985).
 *
 *--------------------------------------------------------------------*/

int hydrate(double T, double **dM, double *dVol, double **Mrock, double *Mh2os, double *Madhs, double **Mh2ol, double *Mnh3l, double **Vrock,
		double **Vh2ol, double rhoRockth, double rhoHydrth, double rhoH2olth, double **Xhydr, int ir, int ircore, int irice, int NR){

	int jr = 0;
	double Vliq = 0.0;
	double Vmoved = 0.0;
	double q = 0.0;   // Similar q as in the separate() routine
	double Xhydr_old = 0.0;

	Xhydr_old = (*Xhydr)[ir];

	// Set hydration level: 1 at Tdehydr_min, 0 at Tdehydr_max, linear in between
	if (T<Tdehydr_min) (*Xhydr)[ir] = 1.0;
	else if (T>=Tdehydr_min && T<Tdehydr_max) (*Xhydr)[ir] = 1.0 - (T-Tdehydr_min)/(Tdehydr_max-Tdehydr_min);
	else (*Xhydr)[ir] = 0.0;

	if ((*Xhydr)[ir] < Xhydr_old) {
		return 1; // We'll dehydrate instead
	}

	// Determine how much liquid there is
	for (jr=ircore;jr<irice+1;jr++) {
		Vliq = Vliq + (*Vh2ol)[jr];
	}

	// Merge water and rock into the cell: equivalently, swap rock in the core and water in the ocean
	// 1- Find out what the volume of rock becomes: dVol -> (1+x)*dVol. x*dVol = Vmoved is the volume moved
	Vmoved = (*Mrock)[ir]/((*Xhydr)[ir]*rhoHydrth + (1.0-(*Xhydr)[ir])*rhoRockth) - dVol[ir];
	// 2- This is also the volume of water displaced (no compaction). Is there enough water for that?

	if (Vmoved > Vliq) {          // If no, get out

		// Update Xhydr: not 1 to conserve mass and volume in each shell, but has increased
		(*Xhydr)[ir] = ((*Mrock)[ir]/(*Vrock)[ir] - rhoRockth) / (rhoHydrth - rhoRockth);
		if (fabs((*Xhydr)[ir]) < 1.0e-10) (*Xhydr)[ir] = 0.0;  // Avoid numerical residuals
		if (fabs((*Xhydr)[ir]) > 1.0-1.0e-10) (*Xhydr)[ir] = 1.0;

		return -1;
	}
	else {                        // If yes, swap. The mass of water moved is split half and half in rock b/w the swapping layers
		(*Vrock)[ir] = dVol[ir];
		if ((*Vrock)[ircore] + Vmoved < dVol[ircore]) {
			(*Vrock)[ircore] = (*Vrock)[ircore] + Vmoved;
			(*Vh2ol)[ircore] = (*Vh2ol)[ircore] - Vmoved;
			(*Mh2ol)[ircore] = (*Mh2ol)[ircore] - Vmoved*rhoH2olth;
			(*Mrock)[ircore] = (*Mrock)[ircore] + Vmoved*((*Xhydr)[ir]*rhoHydrth + (1.0-(*Xhydr)[ir])*rhoRockth) + Vmoved*rhoH2olth*0.5;
			(*Mrock)[ir] = (*Mrock)[ir] - Vmoved*((*Xhydr)[ir]*rhoHydrth + (1.0-(*Xhydr)[ir])*rhoRockth) + Vmoved*rhoH2olth*0.5;

			(*dM)[ir] = (*Mrock)[ir] + Mh2os[ir] + Madhs[ir] + (*Mh2ol)[ir] + Mnh3l[ir];
			(*dM)[ircore] = (*Mrock)[ircore] + Mh2os[ircore] + Madhs[ircore] + (*Mh2ol)[ircore] + Mnh3l[ircore];
		}
		else {
			q = (Vmoved - (dVol[ircore] - (*Vrock)[ircore]))/Vmoved; // Fraction of Vmoved that didn't fit
			(*Vrock)[ircore] = dVol[ircore];
			(*Vh2ol)[ircore] = 0.0; // What about Vnh3l[ircore]?
			(*Vrock)[ircore+1] = (*Vrock)[ircore+1] + q*Vmoved;
			(*Vh2ol)[ircore+1] = (*Vh2ol)[ircore+1] - q*Vmoved;
			(*Mrock)[ircore] = (*Mrock)[ircore] + (1.0-q)*Vmoved*((*Xhydr)[ir]*rhoHydrth + (1.0-(*Xhydr)[ir])*rhoRockth);
			(*Mh2ol)[ircore] = 0.0;
			(*Mh2ol)[ircore+1] = (*Mh2ol)[ircore+1] - q*Vmoved*rhoH2olth;
			(*Mrock)[ircore+1] = (*Mrock)[ircore+1] + q*Vmoved*((*Xhydr)[ir]*rhoHydrth + (1.0-(*Xhydr)[ir])*rhoRockth) + q*Vmoved*rhoH2olth*0.5;
			(*Mrock)[ir] = (*Mrock)[ir] - Vmoved*((*Xhydr)[ir]*rhoHydrth + (1.0-(*Xhydr)[ir])*rhoRockth) + q*Vmoved*rhoH2olth*0.5;

			(*dM)[ir] = (*Mrock)[ir] + Mh2os[ir] + Madhs[ir] + (*Mh2ol)[ir] + Mnh3l[ir];
			(*dM)[ircore] = (*Mrock)[ircore] + Mh2os[ircore] + Madhs[ircore] + (*Mh2ol)[ircore] + Mnh3l[ircore];
			(*dM)[ircore+1] = (*Mrock)[ircore+1] + Mh2os[ircore+1] + Madhs[ircore+1] + (*Mh2ol)[ircore+1] + Mnh3l[ircore+1];

			// Update Xhydr to reflect mass and volume conservation
			(*Xhydr)[ircore+1] = ((*Mrock)[ircore+1]/(*Vrock)[ircore+1] - rhoRockth) / (rhoHydrth - rhoRockth);
			if (fabs((*Xhydr)[ircore+1]) < 1.0e-10) (*Xhydr)[ircore+1] = 0.0;  // Avoid numerical residuals
			if (fabs((*Xhydr)[ircore+1]) > 1.0-1.0e-10) (*Xhydr)[ircore+1] = 1.0;
		}
	}

	// Update Xhydr: not 1 to conserve mass and volume in each shell, but has increased
	(*Xhydr)[ir] = ((*Mrock)[ir]/(*Vrock)[ir] - rhoRockth) / (rhoHydrth - rhoRockth);
	if (fabs((*Xhydr)[ir]) < 1.0e-10) (*Xhydr)[ir] = 0.0;  // Avoid numerical residuals
	if (fabs((*Xhydr)[ir]) > 1.0-1.0e-10) (*Xhydr)[ir] = 1.0;

	(*Xhydr)[ircore] = ((*Mrock)[ircore]/(*Vrock)[ircore] - rhoRockth) / (rhoHydrth - rhoRockth);
	if (fabs((*Xhydr)[ircore]) < 1.0e-10) (*Xhydr)[ircore] = 0.0;  // Avoid numerical residuals
	if (fabs((*Xhydr)[ircore]) > 1.0-1.0e-10) (*Xhydr)[ircore] = 1.0;

	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine viscosity
 *
 * Calculates the viscosity of a water-ammonia liquid depending on
 * temperature and ammonia mass fraction (Kargel et al. 1991)
 *
 *--------------------------------------------------------------------*/

double viscosity(double T, double Mh2ol, double Mnh3l) {
	double visc = 0.0;
	double A = 0.0;
	double B = 0.0;
	double X = 0.0;

	X = Mnh3l/Mh2ol;

	if (T<240.0) {
		A = -10.8143 + 0.711062*X - 22.4943*X*X + 41.8343*X*X*X - 18.5149*X*X*X*X;
		B = 1819.86 + 250.822*X + 6505.25*X*X - 14923.4*X*X*X + 7141.46*X*X*X*X;
	}
	else {
		A = -13.8628 - 68.7617*X + 230.083*X*X - 249.897*X*X*X;
		B = 2701.73 + 14973.3*X - 46174.5*X*X + 45967.6*X*X*X;
	}

	visc = exp(A+B/T);

	return visc;
}

#endif /* THERMAL_H_ */
