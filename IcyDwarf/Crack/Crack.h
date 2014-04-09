/*
 * Crack.h
 *
 *  Created on: Apr 29, 2013
 *      Author: Marc Neveu (mneveu@asu.edu)
 *
 *      This routine is in SI, unlike the thermal code which is in cgs.
 *
 * 		Calculation of the depth and profile of cracking over time,
 * 		taking into account:
 * 		1. Brittle-ductile transition in serpentine
 * 		2. Grain thermal expansion/contraction mismatch
 * 		3. Pore water expansion
 * 		4. Hydration/dehydration
 * 		5. Dissolution/precipitation
 *
 * 		The thermal mismatch aspect (1) is adapted from Vance et al. (2007)
 * 		and was initially coded for Scilab in Dec. 2012.
 *
 * 		To work, this routine needs:
 *
 * 		1- Temperature vs. time (T(t)) and pressure vs. time (P(t))
 *   	   from a thermal evolution model, imported from a "kbo.dat" file
 *   	   that needs to be in the same folder as this file.
 *
 * 		2- Pre-built a(T,P) and integral tables that give the flaw size in
 *    	   a mineral grain yielding the maximum stress intensity K_I
 *   	   (see Fig. 1 of Vance et al. (2007)) at a given T and P.
 *   	   To build such a table, enable calculate_grain_aTP in
 *   	   IcyDwarfInput.txt.
 *
 *   	3- Pre-built tables of the thermal expansivity alpha and
 *   	   compressibility beta of pure water. These can be generated by
 *   	   enabling calculate_alpha_beta in IcyDwarfInput.txt.
 *
 *   	Assumes R and CHNOSZ are already open.
 *
 *   	Animated plots can be generated by enabling the plot_on option in
 *   	IcyDwarfInput.txt.
 *
 *      References:
 *      Neveu et al. (2013) Cracking in Ceres' core as an opportunity for late hydrothermal activity.
 *      44th LPSC, abstract 2216.
 *
 * 		Neveu et al. (2014) Modeling core cracking, a key factor in the geophysical evolution and habitability
 * 		of Ceres. 45th LPSC, abstract 1120.
 */

#ifndef CRACK_H_
#define CRACK_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../IcyDwarf.h"

int crack(int argc, char *argv[], char path[1024], int ir, double T, double T_old, double *Pressure,
		double *Crack, double Crack_old, double *Crack_size, double Crack_size_old,
		double Xhydr, double Xhydr_old, double Tdehydr, double dtime, double Mrock, double Mrock_init, double **Act, double **Act_old,
		int warnings, int msgout, int *crack_input, int *crack_species, float **aTP, float **integral, float **alpha, float **beta,
		float **silica, float **chrysotile, float **magnesite);

int crack(int argc, char *argv[], char path[1024], int ir, double T, double T_old, double *Pressure,
		double *Crack, double Crack_old, double *Crack_size, double Crack_size_old,
		double Xhydr, double Xhydr_old, double Tdehydr, double dtime, double Mrock, double Mrock_init, double **Act, double **Act_old,
		int warnings, int msgout, int *crack_input, int *crack_species, float **aTP, float **integral, float **alpha, float **beta,
		float **silica, float **chrysotile, float **magnesite) {

	//-------------------------------------------------------------------
	//                 Declarations and initializations
	//-------------------------------------------------------------------

    int thermal_mismatch = 0;                                    // Grain thermal expansion/contraction mismatch effects
	int pore_water_expansion = 0;                                // Pore water expansion effects
	int hydration_dehydration = 0;                               // Rock hydration/dehydration effects
	int dissolution_precipitation = 0;                           // Rock dissolution/precipitation effects

	int i = 0;

	float Crack_size_mem = 0.0;                                  // Memorize crack size in m between phenomena
	double Brittle_strength = 0.0;                               // Brittle rock strength in Pa
	double Ductile_strength = 0.0;                               // Ductile rock strength in Pa
	float Rock_strength = 0.0;									 // Rock strength in Pa
	float dTdt = 0.0;                  							 // Heating/cooling rate in K/Gyr

	// Thermal mismatch-specific variables
	int deltaT_int = 0;                                          // deltaT index in the aTP table
	int P_int = 0;                                               // P index in the aTP table
	float Tprime = 0.0;                                          // Temperature at zero stress from thermal mismatch in K
	float K_I = 0.0;                                             // Stress intensity from thermal mismatch in Pa m1/2

	// Pore fluid heating-specific variables
	int tempk_int = 0;                                           // T index in the alpha and beta tables (P index is P_int)
	float P_fluid = 0.0;                                         // Effective pore fluid pressure in Pa

	// Rock hydration/dehydration-specific variables
	float P_hydr = 0.0;                                          // Compressive stress from hydration in Pa

	// index  species
	// -----  ------------------------
	//   0    amorphous silica
	//   1    serpentine (chrysotile)
	//   2    carbonate (magnesite)
	// -----  ------------------------
	// No mallocs here, because we keep n_species small
	float R_diss[n_species_crack];                               // Dissolution/precipitation rate in mol m-3 s-1
	float nu_prod[n_species_crack];                              // Stoichiometric coefficient of the dissolution product(s)
	float mu_Xu[n_species_crack];                                // Exponent of Q/K in kinetic rate law (Xu and Pruess 2001)
	double K_eq[n_species_crack];                                // Equilibrium constant, dimensionless
	float Ea_diss[n_species_crack];                              // Activation energy of dissolution/precipitation in J mol-1
	float Molar_volume[n_species_crack];                         // Molar volume in m3 mol-1

	float surface_volume_ratio = 0.0;                            // Ratio of water-rock surface to fluid volume in m-1
	float d_crack_size = 0.0;                                    // Net change in crack size in m
	float Crack_size_diss_old = 0.0;                             // Crack size before this step in m

	for (i=0;i<n_species_crack;i++) {
		R_diss[i] = 0.0;
		Ea_diss[i] = 0.0;
		K_eq[i] = 0.0;
		Molar_volume[i] = 0.0;
	}

	thermal_mismatch = crack_input[0];
	pore_water_expansion = crack_input[1];
	hydration_dehydration = crack_input[2];
	dissolution_precipitation = crack_input[3];

	if (dissolution_precipitation == 1) {
		// Silica: Equations (55) of Rimstidt and Barnes 1980 or (7-8) of Bolton et al. 1997 (porosity not included)
		// mol m-3 s-1 =no dim (scaled to 1 m-1)*mol L-1 s-1*nd*     no dim (=nd)
		mu_Xu[0] = 1.0;

		// Serpentine: Exponent of Q/K remains 1 even though Q = a_silica^2 * a_Mg+2^3 / a_H+^6 = a_solutes^(5/6)
		// because many other stoichiometries are possible with serpentine.
		mu_Xu[1] = 1.0;

		// Carbonate: Pokrovski and Schott 1999 suggest (Q/K)^4, which makes sense because Q = a_Mg+2^2 * a_CO3-2^2
		mu_Xu[2] = 4.0;

		nu_prod[0] = 1.0;                                          // SiO2 only product
		nu_prod[1] = 11.0;										   // 2 SiO2, 3 Mg+2, 6 OH-
		nu_prod[2] = 2.0;										   // 1 Mg+2, 1 CO3-2
		Ea_diss[0] = Ea_silica;                                    // Rimstidt and Barnes (1980)
		Ea_diss[1] = Ea_chrysotile;                                // Thomassin et al. (1977)
		Ea_diss[2] = Ea_magnesite;                                 // Valid for pH 5.4, but decreases with pH (Pokrovsky et al. 2009)
		Molar_volume[0] = Molar_volume_silica;                     // CHNOSZ - HDN+78
		Molar_volume[1] = Molar_volume_chrysotile;                 // CHNOSZ - HDN+78
		Molar_volume[2] = Molar_volume_magnesite;                  // CHNOSZ - HDN+78
	}

	//-------------------------------------------------------------------
	//                 Calculate heating/cooling rate in K/Gyr
	//-------------------------------------------------------------------

	dTdt = (T - T_old)/dtime;
	// dTdt = -1.0e9;                                               // Arbitrary cooling rate of Vance et al. (2007)

	//-------------------------------------------------------------------
	//                    Calculate rock strength in Pa
	//-------------------------------------------------------------------

	/* Find radius of brittle-ductile transition. In principle, the transition between brittle faulting and
	ductile flow depends on P, T, initial porosity, and rheological parameters such as grain size and strain
	rate (Wong and Baud 2012), as well as mineralogy (Kohlstedt et al. 1995). 	Here, we consider only P and T.

	We mix up brittle-ductile and brittle-plastic transitions, although we shouldn't (Kohlstedt et al. 1995).
	The transition is when the brittle strength equals the ductile strength.
	The brittle strength is given by a friction/low-P Byerlee type law: stress = mu*P.
	The ductile strength is given by a flow law: epsilon = A*sigma^n*exp[(-Ea+P*V)/RT]. See crack parameters. */
	Rock_strength = 0.0;
	Brittle_strength = mu_Escartin*Pressure[ir];
	Ductile_strength = pow(strain_rate,(1.0/n_flow_law)) * pow(A_flow_law,-1.0/n_flow_law) * pow(d_flow_law,p_flow_law/n_flow_law)
					 * exp((Ea_flow_law + Pressure[ir]*V_flow_law)/(n_flow_law*R_G*T));
	if (Brittle_strength <= Ductile_strength) Rock_strength = Brittle_strength;
	else Rock_strength = Ductile_strength;

	//-------------------------------------------------------------------
	// Cracks open from thermal expansion / contraction mismatch
	// (Friedrich and Wong 1986, Vance et al. 2007)
	//-------------------------------------------------------------------

	if (thermal_mismatch == 1) {

		// Calculate T' in each layer over time, eq (2) of Vance et al. (2007)
		// T' is the temperature at zero stress from thermal mismatch

		if (dTdt == 0.0) dTdt = 1.0e-6; // To ensure continuity of T', otherwise T'=0
		Tprime = Q/R_G/log(12.0*Omega*D0_deltab*E_Young/
						(sqrt(3.0)*n_fit*k_B*L_size*L_size*L_size*fabs(dTdt)/Gyr2sec));

		// Calculate the stress intensity K_I in each layer over time,
		// eq (4) of Vance et al. (2007)
		K_I = 0.0;
		if (Tprime != 0) {

			// Look up the right value of a(T,P) to use in eq(4)
			deltaT_int = look_up (fabs(Tprime - T), 0.0, deltaT_step, sizeaTP, warnings);
			P_int = look_up (Pressure[ir], 0.0, P_step, sizeaTP, warnings);
			int integralLine = (int) (aTP[deltaT_int][P_int]/a_min); // Index in the integral table

			// Calculate K_I
			K_I = sqrt(2.0/(PI_greek*aTP[deltaT_int][P_int]))*integral[integralLine][1]*
					E_Young*Delta_alpha/(2.0*PI_greek*(1.0-nu_Poisson*nu_Poisson))*
					fabs(Tprime-T) -
					Pressure[ir]*sqrt(PI_greek*aTP[deltaT_int][P_int]);
		}
	}

	//-------------------------------------------------------------------
	//               Cracks from hydration - dehydration
	//-------------------------------------------------------------------
	/* Dehydration would widen cracks. But at the dehydration temperature (800 K-ish for silicates)
	 we are far into the ductile regime (above 400 K-ish). So there is no point looking at cracks.

	 Calculate crack shrinking arising from rock swelling:
	 if epsilon is the displacement
	 epsilon = (l_hydr - l_rock) / l_rock = l_hydr/l_rock - 1
	 Assuming a cube of rock, V_hydr/V_rock = l_hydr^3 / l_rock^3 = rho_rock/rho_hydr

	 If cracks close completely, then stress can build up as in Hooke's law (if isotropy):
	 P_hydr = E_Young*epsilon
	 So P_hydr = E_Young*[(rho_rock/rho_hydr)^(1/3) - 1]
	 Actually, some pores remain open because of asperities.*/

	if (hydration_dehydration == 1) {

		P_hydr = 0.0;
		// Only where there are cracks, where hydration has increased, and where it's not fully hydrated
		if (Crack_old > 0.0 && Xhydr > Xhydr_old && Xhydr < 1.0 && T < Tdehydr) {

			// Initialize crack size
			(*Crack_size) = smallest_crack_size;  // I guess because smallest_crack_size is a #define, the code adds a residual 4.74e-11.
											   // No changes bigger than that residual will trigger a change in the cracking.
			if (Crack_old > 0.0 && Crack_size_old > 0.0) {
				(*Crack_size) = Crack_size_old;
			}
			d_crack_size = 0.0;
			if (T < Tdehydr) { // Hydration
				d_crack_size = - 2.0*(pow((rhoRock/rhoHydr),0.333) - 1.0) * hydration_rate * dtime;
				if ((*Crack_size) + d_crack_size < 0.0) {
					P_hydr = E_Young*(-d_crack_size-(*Crack_size))/(hydration_rate*dtime); // Residual rock swell builds up stresses
					(*Crack_size) = 0.0;          // Crack closes completely
				}
				else {
					P_hydr = 0.0;
					(*Crack_size) = (*Crack_size) + d_crack_size;
				}
				// Debug printf("t=%d, r=%d, Old crack size=%g, Crack_size=%g, d_crack_size=%g, P_hydr=%g\n",
				// t,r,Crack_size_hydr_old,Crack_size,d_crack_size,P_hydr);
			}
			Crack_size_mem = (*Crack_size);
		}
	}

	//-------------------------------------------------------------------
	//             Expansion of pore water as it is heated
	//            (Norton 1984, Le Ravalec and Gu�guen 1994)
	//-------------------------------------------------------------------

	if (pore_water_expansion == 1) {

		// For now, let's say the pores are at lithostatic pressure (should not be too different from hydrostatic pressure,
		// as long there are only a few layers of cracks)
		// Also let pressure evolve with temperature.

		P_fluid = 0.0;

		// Don't do calculations in undifferentiated or water areas, in dehydrated areas, or if no heating
		if (Xhydr >= 0.0 && T < Tdehydr && Mrock > Mrock_init && T > T_old) {

			// Look up the right value of alpha and beta, given P and T
			tempk_int = look_up (T, (float) tempk_min, delta_tempk, sizeaTP, warnings);
			P_int = look_up (Pressure[ir]/bar, (float) P_bar_min, delta_P_bar, sizeaTP, warnings);

			// Calculate fluid pressure, including geometric effects (Le Ravalec & Gu�guen 1994)
			P_fluid = Pressure[ir] + alpha[tempk_int][P_int] * (T-T_old)
							/ (beta[tempk_int][P_int]/bar + aspect_ratio*3.0*(1.0-2.0*nu_Poisson)/E_Young)
							* (1+2*aspect_ratio);
			// Version of Norton (1984) without elastic relaxation
			// P_fluid = Pressure[r] + alpha[tempk_int][P_int] * (thoutput[r][t].tempk-thoutput[r][t-1].tempk)
			//				/ (beta[tempk_int][P_int]/bar) * (1+2*aspect_ratio);
		}
	}

	//-------------------------------------------------------------------
	//          Dissolution / precipitation (Bolton et al. 1997)
	//-------------------------------------------------------------------
	/* TODO For now, we take the activities of solutes to be like molalities (ideal solutions),
	 * even though that clearly doesn't work with our concentrated solutions.
	 * We take the activities of solids (rock and precipitates) and water to be 1.
	 */

	if (dissolution_precipitation == 1) {
		// timestep = timestep/1.0e6; // Debug
		// Calculate dissolution/precipitation only where there are cracks
		if (Crack_old > 0.0 && Xhydr > 0.0) {

			// Initialize crack size
			(*Crack_size) = smallest_crack_size;       // I guess because smallest_crack_size is a #define, the code adds a residual 4.74e-11.
													// No changes bigger than that residual will trigger a change in the cracking.
			if (hydration_dehydration == 1) {
				if (Crack_size_mem > 0.0) {         // Check crack size after hydration cracking calculations
					(*Crack_size) = Crack_size_mem;
				}
			}
			else {
				if (Crack_old > 0.0 && Crack_size_old > 0.0) {
					(*Crack_size) = Crack_size_old;
				}
			}
			Crack_size_diss_old = (*Crack_size);
			d_crack_size = 0.0;
			surface_volume_ratio = 2.0/(*Crack_size);  // Rimstidt and Barnes (1980) Fig. 6 for a cylinder/fracture

			// Use CHNOSZ to get reaction constants at given T and P
			tempk_int = look_up (T, (float) tempk_min_species, delta_tempk_species, sizeaTP, warnings);
			P_int = look_up (Pressure[ir]/bar, (float) P_bar_min, delta_P_bar, sizeaTP, warnings);

			// subcrt(c("SiO2","SiO2"),c(-1,1),c("cr","aq"))
			K_eq[0] = pow(10.0,silica[tempk_int][P_int]);
			// subcrt(c("chrysotile","SiO2","Mg+2","OH-","H2O"),c(-1,2,3,6,-1),c("cr","aq","aq","aq","liq"))
			K_eq[1] = pow(10.0,chrysotile[tempk_int][P_int]);
			// subcrt(c("MgCO3","Mg+2","CO3-2"),c(-1,1,1),c("cr","aq","aq"))
			K_eq[2] = pow(10.0,magnesite[tempk_int][P_int]);

			// if (r == 130 && t < 100) printf("t=%d, r=%d\n",t,r); // Debug

			for (i=0;i<n_species_crack;i++) {                                    // Include whichever species are needed
				if (crack_species[i] > 0) {
					(*Act)[i] = (*Act_old)[i];
					// if (r == 130 && t < 100) printf("\t (*Act)[%d]=%g, Q[%d]=%g, K_eq[%d]=%g\n",i,(*Act)[i]/rhoH2ol,i,pow((*Act)[i]/rhoH2ol,nu_prod[i]),i,K_eq[i]); // Debug

					// (Act_prod in mol L-1 to scale with K, silica equation (i=0) assumes unit A/V).
					// The Arrhenius term is equivalent to a dissociation rate constant kdiss in mol m-2 s-1.
					R_diss[i] = surface_volume_ratio * exp(-Ea_diss[i]/(R_G*T)) * 1.0 * (1-pow( pow((*Act)[i]/rhoH2ol,nu_prod[i])/K_eq[i], mu_Xu[i]));
					// if (r == 130 && t < 100) printf("\t R_diss[%d]=%g\n",i,R_diss[i]); // Debug

					// Update crack size (equation 61 of Rimstidt and Barnes 1980, ends up being independent of A/V)
					// and update Act_prod[i] (mol m-3)
					if (-R_diss[i]*dtime*Gyr2sec > (*Act)[i]) {  // Everything precipitates
						// if (r == 130 && t < 100) printf("\t Every bit of species %d precipitates\n",i); // Debug

						// The change in size is everything that could precipitate (Q^nu), not everything that should have precipitated (Rdiss*timestep)
						d_crack_size = d_crack_size - (*Act)[i]*Molar_volume[i]/surface_volume_ratio; // Rimstidt and Barnes (1980) Eq 61
						// if (r == 130 && t < 100) printf("\t d_crack_size=%g\n",d_crack_size); // Debug

						(*Act)[i] = 0.0;                                           // Can't have negative concentrations!
						// if (r == 130 && t < 100) printf("\t New Act_prod[%d]=%g\n",i,(*Act)[i]/rhoH2ol); // Debug

					}
					else {
						// if (r== 130 && t < 100) printf("\t Some species %d in solution\n",i); // Debug

						d_crack_size = d_crack_size + R_diss[i]*dtime*Gyr2sec*Molar_volume[i]/surface_volume_ratio; // Rimstidt and Barnes (1980) Eq 61
						// if (r == 130 && t < 100) printf("\t d_crack_size=%g\n",d_crack_size); // Debug

						(*Act)[i] = (*Act)[i] + nu_prod[i]*R_diss[i]*dtime*Gyr2sec;
						// if (r == 130 && t < 100) printf("\t New Act_prod[%d]=%g\n",i,(*Act)[i]/rhoH2ol); // Debug

					}
					(*Act_old)[i] = (*Act)[i];                          // Memorize the activity of products for the next timestep
				}
			}
			if ((*Crack_size) + d_crack_size > 0.0)                        // Update crack size
				(*Crack_size) = (*Crack_size) + d_crack_size;
			else {
				(*Crack_size) = 0.0;                                       // Pore clogged
				for (i=0;i<n_species_crack;i++) (*Act_old)[i] = 0.0;    // Reset old activity quotients
			}
		}
		else {
			// If the crack is closed, clear the old activity quotients
			for (i=0;i<n_species_crack;i++) {
				(*Act_old)[i] = 0.0;
			}
		}
	}

	//-------------------------------------------------------------------
	//                   Determine type of cracking
	//-------------------------------------------------------------------

	// Cases where cracks appear
	if (thermal_mismatch == 1) {          // Mismatch stresses open cracks
		if (Mrock > Mrock_init
				&& (K_I >= K_IC || Crack_old == 1) && dTdt < 0)
			(*Crack) = 1.0;               // Cooling cracks
		if (Mrock > Mrock_init
				&& (K_I >= K_IC || Crack_old == 2) && dTdt >= 0)
			(*Crack) = 2.0;               // Heating cracks
	}
	if (hydration_dehydration == 1) {
		if (P_hydr > Pressure[ir] + Rock_strength)
			(*Crack) = 3.0;               // Compressive hydration cracks
	}
	if (pore_water_expansion == 1) {      // Open crack if the fluid pressure is high enough
		if (P_fluid > Pressure[ir] + Rock_strength)
			(*Crack) = 5.0;
	}
	if (dissolution_precipitation == 1) {
		if (Crack_old > 0.0 && (*Crack_size) > Crack_size_diss_old)
			(*Crack) = 6.0;               // Dissolution widened crack
		if (Crack_old > 0.0 && (*Crack_size) < Crack_size_diss_old)
			(*Crack) = 7.0;               // Precipitation shrunk crack
	}

	// Cases where cracks disappear
	if (Mrock <= Mrock_init)
		(*Crack) = 0.0;                   // Trivial: not enough rock
	if (Rock_strength < 0.99*Brittle_strength) // 0.99 to beat machine error
		(*Crack) = 0.0;                   // Ductile zone
	if (hydration_dehydration == 1) {
		if (P_hydr > 0.0 && P_hydr <= Pressure[ir] + Rock_strength) {
			(*Crack) = -1.0;              // Crack closed because of hydration
		}
	}
	if (dissolution_precipitation == 1) {
		if (Crack_old > 0.0 && (*Crack_size) <= 0.0) {
			(*Crack) = -1.0;              // Crack closed after precipitation
			(*Crack_size) = 0.0;
		}
	}

	return 0;
}

#endif /* CRACK_H_ */
