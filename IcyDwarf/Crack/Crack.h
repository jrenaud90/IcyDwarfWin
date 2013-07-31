/*
 * Crack.h
 *
 *  Created on: Apr 29, 2013
 *      Author: Marc Neveu (mneveu@asu.edu)
 *
 *-------------------------------------------------------------------
 * THIS ROUTINE CALCULATES THE DEPTH AND PROFILE OF CRACKING
 * OVER TIME, PROVIDED A GRAIN SIZE, COOLING RATE OVER TIME,
 * TEMPERATURE AND PRESSURE OVER TIME
 *
 * Marc Neveu - 20 Dec. 2012 - Adapted from Vance et al. (2007)
 * Initially coded for Scilab.
 * Ref.: Neveu et al. (2013) Cracking in Ceres' core as an oppor-
 * tunity for late hydrothermal activity. 44th LPSC, abstract 2216.
 *-------------------------------------------------------------------
 *-------------------------------------------------------------------
 * To work, this routine needs:
 *
 * 1- Temperature vs. time (T(t)) and pressute vs. time (P(t))
 *    from a thermal evolution model, imported from a "kbo.dat" file
 *    that needs to be in the same folder as this file.
 *
 * 2- A pre-built table a(T,P), that gives the flaw size in
 *    a mineral grain yielding the maximum stress intensity K_I
 *    (see Fig. 1 of Vance et al. (2007)) at a given T and P.
 *    To build such a table, use the attached file aTable.sce.
 *    This file, (crack.sce) and aTable.sce (or at least the file
 *    manually generated from an aTable.sce output, "aTP.dat") should
 *    be in the same folder.
 *-------------------------------------------------------------------
 *-------------------------------------------------------------------
 * Note (20 June 2013): the results are a tiny bit different
 * from what the Scilab code gave. This seems to be due to small
 * differences in how dTdt is calculated and the integral table is
 * looked up (1 line difference sometimes). No big deal compared to
 * uncertainties in the parameters.
 *-------------------------------------------------------------------
 */

#ifndef CRACK_H_
#define CRACK_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <Rembedded.h>

#include "Crack_parameters.h"
#include "Crack_grain_aTP.h"
#include "Crack_water_CHNOSZ.h"

int Crack(int argc, char *argv[], char path[1024], int NR, int NT, float r_p, float timestep, float rho_p, thermalout **thoutput,
		int warnings, int msgout, int pore_water_expansion, int hydration_dehydration, int dissolution_precipitation);

int Crack(int argc, char *argv[], char path[1024], int NR, int NT, float r_p, float timestep, float rho_p, thermalout **thoutput,
		int warnings, int msgout, int pore_water_expansion, int hydration_dehydration, int dissolution_precipitation) {

	//-------------------------------------------------------------------
	//                 Declarations and initializations
	//-------------------------------------------------------------------

    int r = 0;
    int t = 0;
	int i = 0;
	int j = 0;
	int r_brittle_ductile = 0;                                          // Brittle-ductile transition radius

	double **Pressure = (double**) malloc(NR*sizeof(double*));          // Pressure in Pa
	if (Pressure == NULL) printf("Crack: Not enough memory to create Pressure[NR][NT]\n");
	for (r=0;r<NR;r++) {
		Pressure[r] = (double*) malloc(NT*sizeof(double));
		if (Pressure[r] == NULL) printf("Crack: Not enough memory to create Pressure[NR][NT]\n");
	}
	Pressure = calculate_pressure(Pressure, NR, NT, thoutput);          // Calculate pressures

	float **Rock_strength = (float**) malloc(NR*sizeof(float*));        // Rock strength in Pa
	if (Rock_strength == NULL) printf("Crack: Not enough memory to create Rock_strength[NR][NT]\n");
	for (r=0;r<NR;r++) {
		Rock_strength[r] = (float*) malloc(NT*sizeof(float));
		if (Rock_strength[r] == NULL) printf("Crack: Not enough memory to create Rock_strength[NR][NT]\n");
	}

	float **dTdt = (float**) malloc(NR*sizeof(float*));                 // Heating/cooling rate in K/Gyr
	if (dTdt == NULL) printf("Crack: Not enough memory to create dTdt[NR][NT]\n");
	for (r=0;r<NR;r++) {
		dTdt[r] = (float*) malloc(NT*sizeof(float));
		if (dTdt[r] == NULL) printf("Crack: Not enough memory to create dTdt[NR][NT]\n");
	}

	double **Crack = (double**) malloc(NR*sizeof(double*));                // Crack[NR][NT], cracked zone
	if (Crack == NULL) printf("Crack: Not enough memory to create Crack[NR][NT]\n");
	for (r=0;r<NR;r++) {
		Crack[r] = (double*) malloc(NT*sizeof(double));
		if (Crack[r] == NULL) printf("Crack: Not enough memory to create Crack[NR][NT]\n");
	}

	float **Hydrated = (float**) malloc(NR*sizeof(float*));             // Hydrated[NR][NT], hydrated zone
	if (Hydrated == NULL) printf("Crack: Not enough memory to create Hydrated[NR][NT]\n");
	for (r=0;r<NR;r++) {
		Hydrated[r] = (float*) malloc(NT*sizeof(float));
		if (Hydrated[r] == NULL) printf("Crack: Not enough memory to create hydrated[NR][NT]\n");
	}

	float **Tprime = (float**) malloc(NR*sizeof(float*));   // T' is the temperature at zero stress from thermal mismatch
	if (Tprime == NULL) printf("Crack: Not enough memory to create Tprime[NR][NT]\n");
	for (r=0;r<NR;r++) {
		Tprime[r] = (float*) malloc(NT*sizeof(float));
		if (Tprime[r] == NULL) printf("Crack: Not enough memory to create Tprime[NR][NT]\n");
	}

	float **aTP = (float**) malloc((sizeaTP)*sizeof(float*));    // a[sizeaTP][sizeaTP], table of flaw sizes a
	                                                             // that maximize the stress K_I
	if (aTP == NULL) printf("aTP: Not enough memory to create a[sizeaTP][sizeaTP]\n");
	for (t=0;t<sizeaTP;t++) {
		aTP[t] = (float*) malloc((sizeaTP)*sizeof(float));
		if (aTP[t] == NULL) printf("Crack: Not enough memory to create a[sizeaTP][sizeaTP]\n");
	}

	float **integralCrack = (float**) malloc(2*int_size*sizeof(float)); // Initialize integralCrack[int_size+1][2]
	if (integralCrack == NULL) printf("Crack: Not enough memory to create integralCrack[int_size+1][2]\n");
	for (i=0;i<2*int_size;i++) {
		integralCrack[i] = (float*) malloc(sizeof(float));              // Shouldn't it be loop over int_size, then alloc 2*sizeof(float)? But the code crashes in that case.
		if (integralCrack[i] == NULL) printf("Crack: Not enough memory to create integralCrack[int_size+1][2]\n");
	}

	// Zero all the matrices
	for (t=0;t<NR;t++) {
		for (r=0;r<NR;r++) {
			Rock_strength[r][t] = 0.0;
			dTdt[r][t] = 0.0;
			Crack[r][t] = 0.0;
			Hydrated[r][t] = 0.0;
			Tprime[r][t] = 0.0;
		}
	}
	for (j=0;j<int_size;j++) {
		integralCrack[j][0] = 0.0;
		integralCrack[j][1] = 0.0;
	}
	for (i=0;i<sizeaTP;i++) {
		for (j=0;j<sizeaTP;j++) {
			aTP[i][j] = 0;
		}
	}

    //-------------------------------------------------------------------
    //                          Read input files
    //-------------------------------------------------------------------

	// Read the a(T,P) input file: table of a(deltaT,P) in the model of
	// Vance et al. (2007) so we don't have to calculate a(deltaT,P)
	// each time the routine is called. Use aTP() to generate this file.
	aTP = read_input (sizeaTP, sizeaTP, aTP, path, "Crack/aTP.dat");
	if (aTP[0][0] == 0) printf("Generate a table of a(T,P) using the aTP routine.\n");

	// Read the integral input file:
	// Geometry part of the integral in eqs. (3) and (4) of
	// Vance et al. (2007) for various a, to calculate the stress intensity K_I.
	integralCrack = read_input (2, int_size, integralCrack, path, "Crack/integral.dat");

	//-------------------------------------------------------------------
	//                         Initial conditions
    //-------------------------------------------------------------------

	for (r=0;r<NR;r++) {
		Crack[r][0] = 0.0;                                              // No cracking
		Hydrated[r][0] = 1.0;                                           // Fully hydrated
	}

	//-------------------------------------------------------------------
	//                       Begin main loop over time
	//-------------------------------------------------------------------

	for (t=1;t<NT;t++) {

		//-------------------------------------------------------------------
		//      Calculate rock strength in Pa in each layer over time
		//-------------------------------------------------------------------

		// Rock strength depends on pressure, temperature, and porosity (Wong and Baud 2012). Here, we consider only
		// the dependence on pressure. Mogi (1966) suggests the following dependence for porous silicates based on
		// experiments: compressive rock strength C � 1+2*P^0.4 with P and C in kbar in the brittle regime, and
		// C � 3.4 P in kbar in the ductile regime.
		// This suggests compressive rock strengths C on the order of 1 to 10 kbar (100 to 1000 MPa).
		// Here, we take 1/5th of that because the rocks could fail in tensile strength (�0.1*compressive strength,
		// see UConn lecture slides on rock strength) or in shear strength (as the empirical dependence on pressure
		// of Mogi 1966 suggests), with tensile<shear<compressive strength (UConn slides).

		r_brittle_ductile = 0;

		// Find the brittle/ductile transition using the criterion of Mogi (1966) / 10
		for (r=0;r<NR-1;r++) {
			if (C0_Mogi + alpha_Mogi*pow(Pressure[r][t],n_Mogi) <= ductile_Mogi*Pressure[r][t]
			        && C0_Mogi + alpha_Mogi*pow(Pressure[r+1][t],n_Mogi) > ductile_Mogi*Pressure[r+1][t]) {
				r_brittle_ductile = r;
			}
		}

		for (r=0;r<NR-1;r++) {
			if (thoutput[r][t].mrock > rho_p/rhoH2os*thoutput[r][t].mh2os) {          // Calculate rock strength only if there is enough rock
				if (r < r_brittle_ductile) {                            // Ductile zone
					Rock_strength[r][t] = ductile_Mogi*Pressure[r_brittle_ductile][t];
					// Alternative 7/11/2013: weak dependence on pressure (0.01*(P-P_brittle_ductile)) to reflect pore compaction
					// Rock_strength[r][t] = ductile_Mogi*Pressure[r_brittle_ductile][t] + 0.01*(Pressure[r][t]-Pressure[r_brittle_ductile][t]);
				}
				else {                                                  // Brittle zone
					Rock_strength[r][t] = C0_Mogi + alpha_Mogi*pow(Pressure[r][t],n_Mogi);
				}
			}
			// Debug
			// if (t==456) printf ("r=%d, r_brittle_ductile=%d, Rock strength=%g MPa, Brittle strength=%g MPa, Ductile strength=%g MPa\n",r,r_brittle_ductile,Rock_strength[r][t]/1.0e6,(C0_Mogi + alpha_Mogi*pow(Pressure[r][t],n_Mogi))/1.0e6,ductile_Mogi*Pressure[r+1][t]/1.0e6);
		}

		//-------------------------------------------------------------------
		//  Calculate heating/cooling rate in K/Gyr in each layer over time
		//-------------------------------------------------------------------

		for (r=0;r<NR;r++) {
			dTdt[r][t] = (thoutput[r][t+1].tempk - thoutput[r][t].tempk)/timestep;
			//dTdt[r][t] = -1.0e9;  // Arbitrary cooling rate of Vance et al. (2007)
		}

		//-------------------------------------------------------------------
		//      Initialize crack and hydration in each layer over time
		//-------------------------------------------------------------------

		for (r=0;r<NR;r++) {
			Hydrated[r][t] = Hydrated[r][t-1];                    // Start at final state of t-1
			Crack[r][t] = Crack[r][t-1];
			if (Crack[r][t] > 0.0) Hydrated[r][t] = 1.0;          // Hydration where cracks
			if (thoutput[r][t].tempk >= tempk_dehydration) Hydrated[r][t] = 0.0; // Dehydration above a threshold tempk
		}

		//-------------------------------------------------------------------
		// Cracks open from thermal expansion / contraction mismatch
		// (Friedrich and Wong 1986, Vance et al. 2007)
		//-------------------------------------------------------------------

		// Calculate T' in each layer over time, eq (2) of Vance et al. (2007)
		// T' is the temperature at zero stress from thermal mismatch

		for (r=0;r<NR;r++) {
			if (dTdt[r][t] == 0.0) dTdt[r][t] = 1.0e-6; // To ensure continuity of T', otherwise T'=0
			Tprime[r][t] = Q/R_G/log(12.0*Omega*D0_deltab*E_Young/
							(sqrt(3.0)*n*k_B*L*L*L*fabs(dTdt[r][t])/Gyr2sec));
		}
	}

	// Calculate the stress intensity K_I in each layer over time,
	// eq (4) of Vance et al. (2007)

	float deltaT = 0.0;                // Difference between T' and T from thermal output
	float deltaT_var = 0.0;
	int deltaT_int = 0;              // deltaT index in the aTP table
	float P = 0.0;
	float P_var = 0.0;
	int P_int = 0;                   // P index in the aTP table

	float **K_I = (float**) malloc(NR*sizeof(float*));       // K_I[NR][NT], cracked zone
	if (K_I == NULL) printf("Crack: Not enough memory to create K_I[NR][NT]\n");
	for (r=0;r<NR;r++) {
		K_I[r] = (float*) malloc(NT*sizeof(float));
		if (K_I[r] == NULL) printf("Crack: Not enough memory to create K_I[NR][NT]\n");
	}

	for (t=0;t<NT;t++) {
		for (r=0;r<NR;r++) {
			K_I[r][t] = 0;
			if (Tprime[r][t] != 0) {

				// Look up the right value of a(T,P) to use in eq(4)
				deltaT = fabs(Tprime[r][t] - thoutput[r][t].tempk);
				deltaT_var = 0.0;
				P = Pressure[r][t];
				P_var = 0.0;
				deltaT_int = 0;
				P_int = 0;

				if (deltaT <= deltaT_step) {
					deltaT_int = 0;
				}
				else if (deltaT > deltaT_step*((float) (sizeaTP-1.0))) {
					deltaT_int = sizeaTP-1;
					if (warnings == 1) printf("Crack: deltaT=%g K above deltaT range for T' calculation, assuming deltaT=%g K\n",deltaT, deltaT_step*((float) (sizeaTP-1.0)));
				}
				else {
					for (j=0;j<sizeaTP;j++) {
						if (deltaT/(deltaT_var-0.5*deltaT_step) > 1 &&
								deltaT/(deltaT_var+0.5*deltaT_step) < 1) {
							deltaT_int = j;
						}
						deltaT_var = deltaT_var + deltaT_step;
					}
				}

				if (P <= P_step) {
					P_int = 0;
				}
				else if (P > P_step*((float) (sizeaTP-1.0))) {
					P_int = sizeaTP-1;
					if (warnings == 1) printf("Crack: P=%g Pa above P range for T' calculation, assuming P=%g Pa\n",P,P_step*((float) (sizeaTP-1.0)));
				}
				else {
					for (j=0;j<sizeaTP;j++) {
						if (P/(P_var-0.5*P_step) > 1 &&
								P/(P_var+0.5*P_step) < 1) {
							P_int = j;
						}
						P_var = P_var + P_step;
					}
				}

				int integralLine = (int) (aTP[deltaT_int][P_int]/a_min); // Index in the integral table

				// Calculate K_I
	            K_I[r][t] = sqrt(2.0/(PI_greek*aTP[deltaT_int][P_int]))*integralCrack[integralLine][1]*
	            		E_Young*Delta_alpha/(2.0*PI_greek*(1.0-nu_Poisson*nu_Poisson))*
	            		fabs(Tprime[r][t]-thoutput[r][t].tempk) -
	            		Pressure[r][t]*sqrt(PI_greek*aTP[deltaT_int][P_int]);
			}
		}
	}

//-------------------------------------------------------------------
//                       Determine brittle zone
//        (not ductile and where mismatch stresses open cracks)
//-------------------------------------------------------------------

	for (t=1;t<NT;t++) {                 // Need to start at t=1 because calling t-1 at some point
		for (r=0;r<NR-1;r++) {

				// Cracks appear in rock if stress is exceeded
				// or remain if there was already a crack there (is that necessary?)

				// Cooling cracks:
			if (thoutput[r][t].mrock > rho_p/rhoH2os*thoutput[r][t].mh2os && (K_I[r][t] >= K_IC || Crack[r][t-1] == 1) && dTdt[r][t] < 0) {
				Crack[r][t] = 1.0;
			}

				// Heating cracks:
			if (thoutput[r][t].mrock > rho_p/rhoH2os*thoutput[r][t].mh2os && (K_I[r][t] >= K_IC || Crack[r][t-1] == 2) && dTdt[r][t] >= 0) {
				Crack[r][t] = 2.0;
			}

				// Cracks disappear if there is no more rock or the stress at T,P is too low
			if (thoutput[r][t].mrock <= 0.0 || K_I[r][t] < K_IC) {
				Crack[r][t] = 0.0;
			}
		}
		Crack[NR-1][t] = 0.0;
	}

//-------------------------------------------------------------------
//             Expansion of pore water as it is heated
//            (Norton 1984, Le Ravalec and Gu�guen 1994)
//-------------------------------------------------------------------

	if (pore_water_expansion == 1) {

		printf("\t - Expansion of pore water as it is heated\n");

		float **P_fluid = (float**) malloc(NR*sizeof(float*));       // P_fluid[NR][NT], effective pore fluid pressure
																	 // in hydrothermal zone (scaled with pore geometry)
		if (P_fluid == NULL) printf("Crack: Not enough memory to create P_fluid[NR][NT]\n");
		for (r=0;r<NR;r++) {
			P_fluid[r] = (float*) malloc(NT*sizeof(float));
			if (P_fluid[r] == NULL) printf("Crack: Not enough memory to create P_fluid[NR][NT]\n");
		}

		float **alpha = (float**) malloc(sizeaTP*sizeof(float*));    // Thermal expansivity, K-1
		if (alpha == NULL) printf("Crack: Not enough memory to create alpha[sizeaTP][sizeaTP]\n");
		for (i=0;i<sizeaTP;i++) {
			alpha[i] = (float*) malloc(sizeaTP*sizeof(float));
			if (alpha[i] == NULL) printf("Crack: Not enough memory to create alpha[sizeaTP][sizeaTP]\n");
		}

		float **beta = (float**) malloc(sizeaTP*sizeof(float*));     // Compressibility, bar-1
		if (beta == NULL) printf("Crack: Not enough memory to create beta[sizeaTP][sizeaTP]\n");
		for (i=0;i<sizeaTP;i++) {
			beta[i] = (float*) malloc(sizeaTP*sizeof(float));
			if (beta[i] == NULL) printf("Crack: Not enough memory to create beta[sizeaTP][sizeaTP]\n");
		}

		float tempk = 0.0;                  // For the lookup. The P variables already exist.
		float tempk_var = 0.0;
		int tempk_int = 0;

		// Open alpha and beta files
		alpha = read_input (sizeaTP, sizeaTP, alpha, path, "Crack/alpha.dat");
		beta = read_input (sizeaTP, sizeaTP, beta, path, "Crack/beta.dat");

		// For now, let's say the pores are at lithostatic pressure (should not be too different from hydrostatic pressure,
		// as long there are only a few layers of cracks)
		// Also let pressure evolve with temperature.

		for (t=0;t<NT;t++) {
			for (r=1;r<NR;r++) {
				P_fluid[r][t] = 0.0;

				// Don't do calculations in undifferentiated or water areas, in dehydrated areas, or if no heating
				if (thoutput[r][t].mrock > rho_p/rhoH2os*thoutput[r][t].mh2os && Hydrated[r][t] >0 && thoutput[r][t].tempk > thoutput[r][t-1].tempk) {

					// Look up the right value of alpha and beta, given P and T
					tempk = thoutput[r][t].tempk;
					tempk_var = (float) tempk_min;
					P = Pressure[r][t]/bar;
					P_var = (float) P_bar_min;
					tempk_int = 0;
					P_int = 0;

					if (tempk <= (float) tempk_min) {
						tempk_int = 0;
						if (warnings == 1) printf("Crack: Below T range of the water alpha/beta data at t=%d, r=%d (T=%.0f K), assuming T=%g K\n",t,r,tempk,(float) tempk_min);
					}
					else if (tempk > (float) (delta_tempk*sizeaTP + tempk_min)) {
						tempk_int = sizeaTP-1;
						if (warnings == 1) printf("Crack: Above T range of the water alpha/beta data at t=%d, r=%d (T=%.0f K), assuming T=%g K\n",t,r,tempk,(float) delta_tempk*sizeaTP + tempk_min);
					}
					else {
						for (j=0;j<sizeaTP;j++) {
							if (tempk/(tempk_var-0.5*delta_tempk) > 1 &&
									tempk/(tempk_var+0.5*delta_tempk) < 1) {
								tempk_int = j;
							}
							tempk_var = tempk_var + delta_tempk;
						}
					}

					if (P <= (float) P_bar_min) {
						P_int = 0;
						if (warnings == 1) printf("Crack: Below P range of the water alpha/beta data at t=%d, r=%d (P=%.1f bar), assuming P=%g bar\n",t,r,Pressure[r][t]/bar,(float) P_bar_min);
					}
					else if (P > (float) (delta_P_bar*sizeaTP + P_bar_min)) {
						P_int = sizeaTP - 1;
						if (warnings == 1) printf("Crack: Above P range of the water alpha/beta data at t=%d, r=%d (P=%.1f bar), assuming P=%g bar\n",t,r,Pressure[r][t]/bar,(float) (delta_P_bar*sizeaTP + P_bar_min));
					}
					else {
						for (j=0;j<sizeaTP;j++) {
							if (P/(P_var-0.5*delta_P_bar) > 1 &&
									P/(P_var+0.5*delta_P_bar) < 1) {
								P_int = j;
							}
							P_var = P_var + delta_P_bar;
						}
					}

					// Calculate fluid pressure, including geometric effects (Norton 1984)
					//P_fluid[r][t] = Pressure[r][t] + alpha_var/beta_var*bar*(thoutput[r][t].tempk-thoutput[r][t-1].tempk)*(1+2*aspect_ratio);
					P_fluid[r][t] = Pressure[r][t] + alpha[tempk_int][P_int]/beta[tempk_int][P_int]*bar*(thoutput[r][t].tempk-thoutput[r][t-1].tempk)*(1+2*aspect_ratio);

					// Open crack if that pressure is high enough
					if (P_fluid[r][t] > Pressure[r][t] + Rock_strength[r][t]) {
						// Debug
						// printf("alpha = %g K-1, beta = %g Pa-1, deltaT = %g K, Rock strength = %g\n",alpha[tempk_int][P_int],beta[tempk_int][P_int]/bar,thoutput[r][t].tempk-thoutput[r][t-1].tempk,Rock_strength[r][t]);
						Crack[r][t] = 5.0;
					}
				}
			}
		}
	 	for (r=0;r<NR;r++) {
			free (P_fluid[r]);
	 	}
		for (i=0;i<sizeaTP;i++) {
			free (alpha[i]);
			free (beta[i]);
		}
		free (P_fluid);
		free (alpha);
		free (beta);
	}

//-------------------------------------------------------------------
//               Cracks from hydration - dehydration
//-------------------------------------------------------------------

	if (hydration_dehydration == 1) {

		printf("\t - Cracks from rock hydration/dehydration\n");

		float **P_hydr = (float**) malloc(NR*sizeof(float*));         // P_hydr[NR][NT], pressure of hydration (stress)
		if (P_hydr == NULL) printf("Crack: Not enough memory to create P_hydr[NR][NT]\n");
		for (r=0;r<NR;r++) {
			P_hydr[r] = (float*) malloc(NT*sizeof(float));
			if (P_hydr[r] == NULL) printf("Crack: Not enough memory to create P_hydr[NR][NT]\n");
		}

		for (t=0;t<NT;t++) {                                          // Initialize P_hydr
			for (r=0;r<NR;r++) {
				P_hydr[r][t] = 0.0;
			}
		}

		for (t=1;t<NT;t++) {
			for (r=0;r<NR;r++) {

				// Case 0: Hydration cracks already exists, remains open
				// As long as hydration and cracking are in the same areas, that's useless -- 7/2/2013
	//			if (Hydrated[r][t] == 1 && Crack[r][t] > 0 && Hydrated[r][t-1] == 1 && thoutput[r][t].mrock == dM[r][t]) {
	//				Crack[r][t] = 3;
	//			}

				// Case 1: Hydration, no cracks
				if (Hydrated[r][t] == 1.0 && Crack[r][t] <= 0.0 && Hydrated[r][t-1] == 0.0
						&& thoutput[r][t].mrock > rho_p/rhoH2os*thoutput[r][t].mh2os && thoutput[r][t].tempk < tempk_dehydration) {

					// Calculate pressure (stress) arising from rock swelling
					// P_hydr = E_Young*epsilon where epsilon is the displacement (Hooke's law if isotropy)
					// epsilon = (l_hydr - l_rock) / l_rock = l_hydr/l_rock - 1
					// Assuming a cube of rock, V_hydr/V_rock = l_hydr^3 / l_rock^3 = rho_rock/rho_hydr
					// So P_hydr = E_Young*[(rho_rock/rho_hydr)^(1/3) - 1]

					P_hydr[r][t] = E_Young*(pow((rhoRock/rhoHydr),0.333) - 1.0);

					// Now, stress is usually much larger than the sum of the confining pressure and rock strength.
					// We assume all that stress doesn't build up at once and waits for a whole time step before blowing up.
					// Let's instead consider a stress rate. Our stress rate is E*strain rate, which is E*hydration_rate/dist
					// where "dist" is a control distance. So the stress is E*hydration_rate/dist*time, where "time" is a
					// control time.

					P_hydr[r][t] = P_hydr[r][t] * hydration_rate / (r_p/NR) * timestep;

					// Cracks open if stresses are high enough
					if (P_hydr[r][t] > Pressure[r][t] + Rock_strength[r][t]) {
						Crack[r][t] = 3.0;
					}
				}

				// Case 2: Hydration, cracks
				if (Hydrated[r][t] == 1.0 && Crack[r][t] > 0.0 && Hydrated[r][t-1] == 0.0
						&& thoutput[r][t].mrock > rho_p/rhoH2os*thoutput[r][t].mh2os && thoutput[r][t].tempk < tempk_dehydration) {
					// Assuming microcracks close very fast (years) compared to the time step, we consider only compression stresses
					// that arise once the cracks are closed. Actually, some pores remain open because of asperities.
					P_hydr[r][t] = E_Young*(pow((rhoRock/rhoHydr),0.333) - 1.0) * hydration_rate / (r_p/NR) * timestep;

					// Cracks open if stresses are high enough
					if (P_hydr[r][t] > Pressure[r][t] + Rock_strength[r][t]) {
						Crack[r][t] = 3.0;
					}
				}

				// Case 3: Dehydration, doesn't matter if there are cracks (they either open or widen)
				if (Hydrated[r][t] == 0.0 && Hydrated[r][t-1] == 1.0
						&& thoutput[r][t].mrock > rho_p/rhoH2os*thoutput[r][t].mh2os && thoutput[r][t].tempk >= tempk_dehydration) {
					// Rock pulled apart as it dehydrates and shrinks
					P_hydr[r][t] = - E_Young*(pow((rhoHydr/rhoRock),0.333) - 1.0) * hydration_rate / (r_p/NR) * timestep;
					// Cracks open if stresses are high enough
					if (P_hydr[r][t] > Pressure[r][t] + Rock_strength[r][t]) {
						Crack[r][t] = 4.0;
					}
				}
			}
		}

		for (t=0;t<NT;t++) {
			for (r=0;r<NR;r++) {
				if (Crack[r][t] > 0) Hydrated[r][t] = 1.0; // Synchronize cracks and hydration again
				P_hydr[r][t] = 0.0;
			}
		}
	 	for (r=0;r<NR;r++) {
			free (P_hydr[r]);
	 	}
		free (P_hydr);
	}

//-------------------------------------------------------------------
//          Dissolution / precipitation (Bolton et al. 1997)
//-------------------------------------------------------------------

	if (dissolution_precipitation == 1) {

		printf("\t - Widening/clogging of racks from rock dissolution/precipitation\n");

		float **Crack_size = (float**) malloc(NR*sizeof(float*));       // Crack size (1/2 width in 1-D, radius in cylindrical 2-D)
		if (Crack_size == NULL) printf("Crack: Not enough memory to create Crack_size[NR][NT]\n");
		for (r=0;r<NR;r++) {
			Crack_size[r] = (float*) malloc(NT*sizeof(float));
			if (Crack_size[r] == NULL) printf("Crack: Not enough memory to create Crack_size[NR][NT]\n");
		}

		// index  species
		// -----  ------------------------------
		//   0    amorphous silica dissolution/precipitation
		//   1    serpentine (chrysotile) dissolution/precipitation
		//   2    carbonate (magnesite) dissolution/precipitation
		// -----  ------------------------------

		// No mallocs here, because we'll keep n_species small

		float R_diss[n_species_crack];                   // Dissolution/precipitation rate in mol m-3 s-1
		float surface_volume_ratio = 0.0;                // Ratio of water-rock surface to fluid volume in m-1
		float k_diss[n_species_crack];                   // Dissolution/precipitation rate "constant" in mol m-2 s-1
		float Ea_diss[n_species_crack];                  // Activation energy of dissolution/precipitation (J mol-1)
		float K_eq[n_species_crack];                     // Equilibrium constant, dimensionless
		float Molar_volume[i];                           // Molar volume in m3 mol-1

		float d_crack_size = 0.0;                        // Net change in crack size
		float Crack_size_mem = 0.0;                      // Crack size before this step

		float **Q_act = (float**) malloc(NR*sizeof(float*));         // Activity quotient, dimensionless
		if (Q_act == NULL) printf("Crack: Not enough memory to generate Q_act[NR][n_species]\n");
		for (r=0;r<NR;r++) {
			Q_act[r] = (float*) malloc(n_species_crack*sizeof(float));
			if (Q_act[r] == NULL) printf("Crack: Not enough memory to generate Q_act[NR][n_species]\n");
		}

		float **Q_act_old = (float**) malloc(NR*sizeof(float*));     // Activity quotient at the previous step
		if (Q_act_old == NULL) printf("Crack: Not enough memory to generate Q_act_old[NR][n_species]\n");
		for (r=0;r<NR;r++) {
			Q_act_old[r] = (float*) malloc(n_species_crack*sizeof(float));
			if (Q_act_old[r] == NULL) printf("Crack: Not enough memory to generate Q_act_old[NR][n_species]\n");
		}

		for (i=0;i<n_species_crack;i++) {
			R_diss[i] = 0.0;
			k_diss[i] = 0.0;
			for (r=0;r<NR;r++) {
				Q_act[r][i] = 0.0;
				Q_act_old[r][i] = 0.0;
			}
			K_eq[i] = 0.0;
		}

		Ea_diss[0] = Ea_silica;                                    // Rimstidt and Barnes (1980)
		Ea_diss[1] = Ea_chrysotile;                                // Thomassin et al. (1977)
		Ea_diss[2] = Ea_magnesite;                                 // Valid for pH 5.4, but decreases with pH (Pokrovsky et al. 2009)

		Molar_volume[0] = Molar_volume_silica;                     // CHNOSZ - HDN+78
		Molar_volume[1] = Molar_volume_chrysotile;                 // CHNOSZ - HDN+78
		Molar_volume[2] = Molar_volume_magnesite;                  // CHNOSZ - HDN+78

		// k_diss[0] varies with T, see below for init
		k_diss[1] = k_chrysotile;                                  // mol m-2 s-1, Bales and Morgan (1985) Fig. 4
		k_diss[2] = k_magnesite;                                   // mol m-2 s-1, Pokrovsky & Schott (1999) Fig. 2

		for (t=1;t<NT;t++) {                                // t must be >= 1
			for (r=0;r<NR;r++) {

				// Calculate dissolution/precipitation only where there are cracks
				if (Crack[r][t] > 0.0) {

					// Initialize crack size
					Crack_size[r][t] = smallest_crack_size;
					if (Crack[r][t-1] > 0.0) {
						Crack_size[r][t] = Crack_size[r][t-1];
					}
					Crack_size_mem = Crack_size[r][t];
					d_crack_size = 0.0;

					// Calculate dissolution/precipitation rates
					surface_volume_ratio = 2.0/Crack_size[r][t];

					// TODO For now, we take the activities of solutes to be like molalities (ideal solutions),
					// even though that clearly doesn't work with our concentrated solutions.
					// We take the activities of solids (rock and precipitates) and water to be 1.

					// Rimstidt and Barnes (1980) give k_diss[0,1] in s-1, so we assume
					// an activity coefficient gamma of 1 (low salinity) to get molalities = mol L.
					// We need to multiply by 1000 to get from L to m3, this is done in the R_diff[0]
					// calculation (see R&B 80).
					k_diss[0] = pow(10.0, -0.369 - 7.890e-4*thoutput[r][t].tempk - 3438.0/thoutput[r][t].tempk);

					// TODO Get K_eq(T,P) dynamically from CHNOSZ instead
					// subcrt(c("amorphous silica","SiO2"),c("cr","aq"),c(-1,1),T=25,P=1)
					K_eq[0] = pow(10.0,-2.713591);
					// subcrt(c("chrysotile","SiO2","Mg+2","H2O","H+"),c("cr","aq","aq","aq","aq"),c(-1,2,3,5,-6),T=25,P=1)
					K_eq[1] = pow(10.0,31.12534);
					// subcrt(c("magnesite","Mg+2","CO3-2"),c("cr","aq","aq"),c(-1,1,1),T=25,P=1)
					K_eq[2] = pow(10.0,-8.035219);

					for (i=0;i<n_species_crack;i++) Q_act[r][i] = Q_act_old[r][i];

					// Silica:
					// Equations (55) of Rimstidt and Barnes 1980 or (7-8) of Bolton et al. 1997 (porosity not included)
			   // mol m-3 s-1 =no dim (scaled to 1 m-1)*mol L-1 s-1*nd*     no dim (=nd)
					R_diss[0] = surface_volume_ratio * k_diss[0] * 1.0 * (1-Q_act[r][0]/1000.0/K_eq[0]) / 1000.0; // Assumes unit A/V
																												  // Q in mol L-1

					// Serpentine:
					// Exponent of K/Q remains 1 even though Q = a_silica^2 * a_Mg+2^3 / a_H+^6 = a_solutes^(5/6)
					// because many other stoichiometries are possible with serpentine.
					R_diss[1] = surface_volume_ratio * k_diss[1] * 1.0 * (1-Q_act[r][1]/1000.0/K_eq[1]);          // Q in mol L-1

					// Carbonate:
					// Pokrovski and Schott 1999 suggest (Q/K)^4, which makes sense because Q = a_Mg+2^2 * a_CO3-2^2
					R_diss[2] = surface_volume_ratio * k_diss[2] * 1.0 * (1-pow(Q_act[r][2]/1000.0/K_eq[2],4));   // Q in mol L-1

					for (i=0;i<n_species_crack;i++) {
						// Arrhenius temperature scaling
						R_diss[i] = R_diss[i] * exp(-Ea_diss[i]/(R_G*thoutput[r][t].tempk));

						// Update Q_act[r][i]
						Q_act[r][i] = R_diss[i]*timestep*Gyr2sec;  // Represents solute activity ~molality in mol m-3
						// Memorize the activity coefficient for the next timestep
						Q_act_old[r][i] = Q_act[r][i];

						// Update crack size (equation 61 of Rimstidt and Barnes 1980, ends up being independent of A/V)
						d_crack_size = d_crack_size + R_diss[i]*timestep*Gyr2sec*1*Molar_volume[i]/surface_volume_ratio;
					}
					// Debug
					// printf("t=%d, r=%d, T=%.0fK, R %.2g, Q %.3g, d_size = %g\n",t,r,thoutput[r][t].tempk,R_diss[0],Q_act[r][0],d_crack_size);

					if (Crack_size[r][t] + d_crack_size > 0.0)            // Update crack size
						Crack_size[r][t] = Crack_size[r][t] + d_crack_size;
					else {
						Crack_size[r][t] = 0.0;                           // Pore clogged
						for (i=0;i<n_species_crack;i++) Q_act_old[r][i] = 0.0;  // Reset old activity quotients
					}

					// Update crack matrix
					if (Crack_size[r][t] > Crack_size_mem) Crack[r][t] = 6.0;         // Dissolution widened crack
					if (Crack_size[r][t] < Crack_size_mem) Crack[r][t] = 7.0;         // Precipitation shrunk crack
					if (Crack_size[r][t] < smallest_crack_size) Crack[r][t] = -1.0;   // Precipitation clogged crack
				}
				else {
					// If the crack is closed, clear the old activity quotients
					for (i=0;i<n_species_crack;i++) {
						Q_act_old[r][i] = 0.0;
					}
				}
			}
		}
		for (r=0;r<NR;r++) {

		}
		for (r=0;r<NR;r++) {
			free (Crack_size[r]);
			free (Q_act[r]);
			free (Q_act_old[r]);
		}
		free (Crack_size);
		free (Q_act);
		free (Q_act_old);
	}

//-------------------------------------------------------------------
//                   Determine cracking depth in km
//-------------------------------------------------------------------

	double **Crack_depth = (double**) malloc(NT*sizeof(double*));         // Crack_depth[NT][2]
	if (Crack_depth == NULL) printf("Crack: Not enough memory to create Crack_depth[NT][2]\n");
	for (t=0;t<NT;t++) {
		Crack_depth[t] = (double*) malloc(2*sizeof(double));
		if (Crack_depth[t] == NULL) printf("Crack: Not enough memory to create Crack_depth[NT][2]\n");
	}

	for (t=0;t<NT;t++) {
		Crack_depth[t][0] = t*timestep;   // T in Gyr
		Crack_depth[t][1] = 0.0;
		for (r=0;r<NR;r++) {
			if (Crack[r][t] > 0.0)
			Crack_depth[t][1]++;
		}
		Crack_depth[t][1] = Crack_depth[t][1]/NR*thoutput[NR-1][t].radius;
	}

	//-------------------------------------------------------------------
	// Determine the water to rock ratio W/R by mass in cracked layer
	//-------------------------------------------------------------------

	// Depends entirely on porosity! The W/R by volume is porosity.
	// Here, we say W/R = Mliq/Mcracked_rock.

	// Calculate the mass of liquid over time
	float *Mliq = (float*) malloc(NT*sizeof(float));                   // Mliq[NT]
	if (Mliq == NULL) printf("Crack: Not enough memory to create Mliq[NT]\n");
	Mliq = calculate_mass_liquid (Mliq, NR, NT, thoutput);

	// Calculate W/R
	float *Mcracked_rock = (float*) malloc(NT*sizeof(float));          // Mcracked_rock[NT]
	if (Mcracked_rock == NULL) printf("Crack: Not enough memory to create Mcracked_rock[NT]\n");
	for (t=0;t<NT;t++) Mcracked_rock[t] = 0.0;

	double **WRratio = (double**) malloc(NT*sizeof(double*));          // WRratio[NT][2]
	if (WRratio == NULL) printf("Crack: Not enough memory to create WRratio[NT][2]\n");
	for (t=0;t<NT;t++) {
		WRratio[t] = (double*) malloc(2*sizeof(double));
		if (WRratio[t] == NULL) printf("Crack: Not enough memory to create WRratio[NT][2]\n");
	}
	WRratio[0][0] = 0.0, WRratio[0][1] = 0.0;

	for (t=1;t<NT;t++) {
		WRratio[t][0] = (double) t*timestep;                            // T in Gyr
		for (r=0;r<NR;r++) {
			if (Crack[r][t] > 0.0) {
				Mcracked_rock[t] = Mcracked_rock[t] + thoutput[r][t].mrock;
			}
		}
		if (Mcracked_rock[t] < 0.000001) WRratio[t][1] = 0.0;           // If Mcracked_rock is essentially 0, to avoid infinities
		else WRratio[t][1] = Mliq[t]/Mcracked_rock[t];
	}

//-------------------------------------------------------------------
//                           Write outputs
//-------------------------------------------------------------------

	write_output(NT, NR, Crack, path, "Crack/Crack.txt");
	write_output(2, NT, Crack_depth, path, "Crack/Crack_depth.txt");
	write_output(2, NT, WRratio, path, "Crack/WR_ratio.txt");

//-------------------------------------------------------------------
//                Close input files and free mallocs
//-------------------------------------------------------------------

	for (r=0;r<NR;r++) {
		free (Pressure[r]);
		free (Rock_strength[r]);
		free (dTdt[r]);
		free (Tprime[r]);
		free (K_I[r]);
		free (Crack[r]);
		free (Hydrated[r]);
	}
	for (t=0;t<NT;t++) {
		free (Crack_depth[t]);
		free (WRratio[t]);
	}
	for (i=0;i<int_size;i++) {
		free (integralCrack[i]); // Memory leak? but the code crashes if I loop over 2*int_size [anything above 1024], even though that was what was allocated.
	}
	for (i=0;i<sizeaTP;i++) {
		free (aTP[i]);
	}
	free (Pressure);
	free (Rock_strength);
	free (dTdt);
	free (Tprime);
	free (aTP);
	free (integralCrack);
	free (K_I);
	free (Crack);
	free (Hydrated);
	free (Crack_depth);
	free (Mliq);
	free (Mcracked_rock);
	free (WRratio);

	printf("\n Outputs successfully generated in IcyDwarf/Crack/ directory:\n");
	printf("1. Depth of cracking over time: Cracking_depth.txt\n");
	printf("2. Water/rock ratio in cracked zone over time: WR_ratio.txt\n\n");

	return 0;
}

#endif /* CRACK_H_ */
