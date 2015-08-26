/*
 * ParamExploration.h
 *
 *  Created on: Jan 27, 2015
 *      Author: Marc Neveu (mneveu@asu.edu)
 *
 *      Explore the geochemistry of water-rock interactions using the IPHREEQC module to run batch PHREEQC simulations.
 *
 *      Reference: Charlton & Parkhurst (2011), Computer & Geosciences 37, 1653-1663.
 */

#ifndef PARAMEXPLORATION_H_
#define PARAMEXPLORATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <IPhreeqc.h>
#include <omp.h>

#include "../IcyDwarf.h"

int ParamExploration(char path[1024], double Tmin, double Tmax, double Tstep, double Pmin, double Pmax, double Pstep,
		double pHmin, double pHmax, double pHstep, double pemin, double pemax, double pestep, double WRmin, double WRmax, double WRstep);

int EHandler(int phreeqc);

int ExtractWrite(int instance, int nvar, double** data);

const char* ConCat(const char *str1, const char *str2);

int WritePHREEQCInput(const char *TemplateFile, double temp, double pressure, double pH, double pe, double WR, char *tempinput[1024]);

int ParamExploration(char path[1024], double Tmin, double Tmax, double Tstep, double Pmin, double Pmax, double Pstep,
		double pHmin, double pHmax, double pHstep, double pemin, double pemax, double pestep, double WRmin, double WRmax, double WRstep) {

	int thread_id;
	int phreeqc = 0;
	int nvar = 1830;                                              // Number of variables stored in each PHREEQC simulation
	int i = 0;
	int j = 0;

	int itemp = 0;
	int iPressure = 0;
	int ipH = 0;
	int ipe = 0;
	int iWR = 0;

	int nTempIter = 0;
	int nPressureIter = 0;
	int npHiter = 0;
	int npeIter = 0;
	int nWRiter = 0;

	double T = 0.0;                                              // Temperature (celsius)
	double P = 0.0;												 // Pressure (bar)
	double pH = 0.0;
	double pe = 0.0;
	double WR = 0.0;											 // Water:rock ratio by mass

	char *dbase = (char*)malloc(1024);                           // Path to thermodynamic database
	char *infile = (char*)malloc(1024);                          // Path to initial input file
	char *outfile = (char*)malloc(1024);                         // Path to output file

	int nloops = 0;
	double logfO2 = 0.0;                                         // O2 fugacity for the Fayalite-Magnetite-Quartz buffer at T,P
	double logKO2H2O = 0.0;                                      // log K for reaction 4 H+ + 4 e- + O2 = 2 H2O
	double FMQ = 0.0;                                            // pe corresponding to logfO2
	//-------------------------------------------------------------------
	//                         Initializations
	//-------------------------------------------------------------------

	pemin = 0.25*pemin;
	pemax = 0.25*pemax;
	pestep = 0.25*pestep;

	nTempIter = floor((Tmax-Tmin)/Tstep);
	nPressureIter = floor((Pmax-Pmin)/Pstep);
	npHiter = floor((pHmax-pHmin)/pHstep);
	npeIter = floor((pemax-pemin)/pestep);
	nWRiter = ceil((log(WRmax)-log(WRmin))/log(WRstep));

	double **simdata = (double**) malloc((npHiter+1)*sizeof(double*));  // Compilation of data generated by multiple PHREEQC simulations
	if (simdata == NULL) printf("ParamExploration: Not enough memory to create simdata[npHiter]\n");

	for (i=0;i<npHiter+1;i++) {
		simdata[i] = (double*) malloc(nvar*sizeof(double));
		if (simdata[i] == NULL) printf("ParamExploration: Not enough memory to create simdata[npHiter][nvar]\n");
	}
	for (i=0;i<npHiter+1;i++) {
		for (j=0;j<nvar;j++) {
			simdata[i][j] = 0.0;
		}
	}

	dbase[0] = '\0';
	infile[0] = '\0';
	outfile[0] = '\0';

	if (v_release == 1) strncat(dbase,path,strlen(path)-16);
	else if (cmdline == 1) strncat(dbase,path,strlen(path)-18);
	else strncat(dbase,path,strlen(path)-16);
	strcat(dbase,"PHREEQC-3.1.2/core5.dat");

	strncat(infile,dbase,strlen(dbase)-9);
	strcat(infile,"io/inputIcyDwarf");

	nloops = 0;

	// Create output
	if (v_release == 1) strncat(outfile,path,strlen(path)-16);
	else if (cmdline == 1) strncat(outfile,path,strlen(path)-18);
	else strcpy(outfile,path);
	create_output(path, "Outputs/ParamExploration.txt");

	// nsim PHREEQC simulations = (1 + nTempIter)*(1 + nPressureIter)*(1 + npHiter)*(1 + npeIter)*(1 + nWRiter)
	for (iPressure=0;iPressure<=nPressureIter;iPressure++) {
		P = Pmin + Pstep*(double) iPressure;

		for (itemp=0;itemp<=nTempIter;itemp++) {
			T = Tmin + Tstep*(double) itemp;

			if (T == 0.0) T = 5.0; // PHREEQC crashes at 0 celsius

			// Use CHNOSZ to get log fO2 for F-M-Q buffer at given T and P
			logfO2 = -3.0*CHNOSZ_logK("quartz", "cr", T, P, "SUPCRT92")
				     -2.0*CHNOSZ_logK("magnetite", "cr", T, P, "SUPCRT92")
			         +3.0*CHNOSZ_logK("fayalite", "cr", T, P, "SUPCRT92")
				     +1.0*CHNOSZ_logK("O2", "g", T, P, "SUPCRT92");
			logKO2H2O = -4.0*CHNOSZ_logK("H+", "aq", T, P, "SUPCRT92")
						-4.0*CHNOSZ_logK("e-", "aq", T, P, "SUPCRT92")
						-1.0*CHNOSZ_logK("O2", "g", T, P, "SUPCRT92")
						+2.0*CHNOSZ_logK("H2O", "liq", T, P, "SUPCRT92");

			for(iWR=0;iWR<=nWRiter;iWR++) { // Using log because of multiplicative step. log ratio = ln ratio.
				WR = WRmax/pow(WRstep,(double) iWR);

				for (ipe=0;ipe<=npeIter;ipe++) {
					pe = pemin + pestep*(double) ipe;
					printf("P=%g (%d of %d) T=%g (%d of %d) W:R=%g (%d of %d) pe=FMQ+%g (%d of %d), "
							"parallel calculations over %d values of pH\n",
							P,iPressure+1,nPressureIter+1,T,itemp+1,nTempIter+1,
							WR,iWR+1,nWRiter+1,pe,ipe+1,npeIter+1,npHiter+1);
#pragma omp parallel private(thread_id, phreeqc, pH, FMQ, nloops)
					{
					char *tempinput = (char*)malloc(1024);
					tempinput[0] = '\0';

					thread_id = omp_get_thread_num();
					nloops = 0;

					for (i=0;i<npHiter+1;i++) {
						for (j=0;j<nvar;j++) {
							simdata[i][j] = 0.0;
						}
					}

#pragma omp for
					for (ipH=0;ipH<=npHiter;ipH++) {
						pH = pHmin + pHstep*(double) ipH;

						FMQ = -pH + 0.25*(logfO2+logKO2H2O);
						printf("FMQ pe is %g at T=%g C, P=%g bar, and pH %g\n",FMQ,T,P,pH);
						WritePHREEQCInput(infile, T, P, pH, FMQ+pe, WR, &tempinput);
						phreeqc = CreateIPhreeqc(); // Run PHREEQC
						if (LoadDatabase(phreeqc,dbase) != 0) OutputErrorString(phreeqc);
						SetSelectedOutputFileOn(phreeqc,1);
						if (RunFile(phreeqc,tempinput) != 0) OutputErrorString(phreeqc);
						simdata[ipH][1] = P;
						simdata[ipH][4] = FMQ;
						simdata[ipH][5] = pe*4.0;
						ExtractWrite(phreeqc, nvar, &simdata[ipH]);

						++nloops;
						if (DestroyIPhreeqc(phreeqc) != IPQ_OK) OutputErrorString(phreeqc);
					}
					printf("Thread %d performed %d iterations of the pH loop.\n", thread_id, nloops);

					free(tempinput);
					} // Rejoin threads

					// Write output
					for (ipH=0;ipH<=npHiter;ipH++) {
						append_output(nvar, simdata[ipH], path, "Outputs/ParamExploration.txt");
					}
				}
			}
		}
	}

	// Free mallocs
	for(i=0;i<npHiter+1;i++) {
		free(simdata[i]);
	}
	free(simdata);
	free(infile);
	free(outfile);
	free(dbase);

	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine EHandler
 *
 * Error handler from Charlton & Parkhurst (2011), Computer &
 * Geosciences 37, 1653-1663.
 *
 *--------------------------------------------------------------------*/
int EHandler(int instance) {
	OutputErrorString(instance);
	// exit(EXIT_FAILURE);
	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine ExtractWrite
 *
 * Write selected output from PHREEQC
 *
 *--------------------------------------------------------------------*/
int ExtractWrite(int instance, int nvar, double** data) {
	VAR v;
	int i = 0;
	VarInit(&v);

	GetSelectedOutputValue(instance,1,3,&v);           // temp
	(*data)[0] = v.dVal;

	GetSelectedOutputValue(instance,1,1,&v);           // pH
	(*data)[2] = v.dVal;

	GetSelectedOutputValue(instance,1,2,&v);           // pe
	(*data)[3] = v.dVal;

	GetSelectedOutputValue(instance,1,5,&v);           // W:R
	(*data)[6] = v.dVal;

	for (i=1;i<nvar-6;i++) {                           // Rest of parameters
		GetSelectedOutputValue(instance,2,i,&v);
		if (fabs(v.dVal) < 1e-50) (*data)[i+6] = 0.0;
		else (*data)[i+6] = v.dVal;
	}
	return 0;
}

/*--------------------------------------------------------------------
 *
 * Subroutine ConCat
 *
 * Concatenation. Takes 2 strings and returns the concatenated string.
 *
 *--------------------------------------------------------------------*/
const char* ConCat(const char *str1, const char *str2) {
	char buffer[100];
	buffer[0] = '\0';

	strcpy(buffer,str1);
	return strcat(buffer,str2);
}

/*--------------------------------------------------------------------
 *
 * Subroutine ChangePHREEQCInput
 *
 * Generate input file from a template.
 * Modifies P, T, pH, pe, W:R
 *
 *--------------------------------------------------------------------*/
int WritePHREEQCInput(const char *TemplateFile, double temp, double pressure, double pH, double pe, double WR, char **tempinput) {

	// Open input file
	FILE *fin;
	FILE *fout;
	char temp_str[10];
	char pressure_str[10];
	char pH_str[10];
	char pe_str[10];
	char WR_str[10];
	temp_str[0] = '\0';
	pressure_str[0] = '\0';
	pH_str[0] = '\0';
	pe_str[0] = '\0';
	WR_str[0] = '\0';
	int line_length = 300;
	char line[line_length]; // Individual line
	int line_no = 0; // Line number

	sprintf(temp_str,"%g",temp);
	sprintf(pressure_str,"%g",pressure);
	sprintf(pH_str,"%g",pH);
	sprintf(pe_str,"%g",pe);
	sprintf(WR_str,"%g",WR);

	strcpy(*tempinput,TemplateFile);
	strcat(*tempinput,"T");
	strcat(*tempinput,temp_str);
	strcat(*tempinput,"P");
	strcat(*tempinput,pressure_str);
	strcat(*tempinput,"pH");
	strcat(*tempinput,pH_str);
	strcat(*tempinput,"pe");
	strcat(*tempinput,pe_str);
	strcat(*tempinput,"WR");
	strcat(*tempinput,WR_str);

	fin = fopen (TemplateFile,"r");
	if (fin == NULL) printf("ParamExploration: Missing input file.\n");
	fout = fopen (*tempinput,"w");
	if (fout == NULL) printf("ParamExploration: Missing output file.\n");

	while (fgets(line, line_length, fin)) {
		line_no++;
		if (line_no == 5) {
			fputs(ConCat("\t pH \t \t",pH_str),fout);
			fputs("\n",fout);
		}
		else if (line_no == 6) {
			fputs(ConCat("\t temp \t \t",temp_str),fout);
			fputs("\n",fout);
		}
		else if (line_no == 7) {
			fputs(ConCat("\t pressure \t",pressure_str),fout);
			fputs("\n",fout);
		}
		else if (line_no == 8) {
			fputs(ConCat("\t pe \t \t",pe_str),fout);
			fputs("\n",fout);
		}
		else if (line_no == 9) {
			fputs(ConCat("\t -water \t",WR_str),fout);
			fputs("\n",fout);
		}
		else if (line[1] == '-' && line[2] == 'p' && line[3] == 'r' && line[4] == 'e' && line[5] == 's') {
			fputs(ConCat("\t -pressure \t",pressure_str),fout);
			fputs("\n",fout);
		}
		else if (line[1] == '-' && line[2] == 't' && line[3] == 'e' && line[4] == 'm' && line[5] == 'p') {
			fputs(ConCat("\t -temperature \t",temp_str),fout);
			fputs("\n",fout);
		}
		else fputs(line,fout);
	}
	if (ferror(fin)) {
		printf("ParamExploration: Error reading template input file %s\n",TemplateFile);
		return 1;
	}

	fclose(fin);
	fclose(fout);

	return 0;
}

#endif /* PARAMEXPLORATION_H_ */
