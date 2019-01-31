/*
 * liquid_phase_odepack.cpp
 *
 *  Created on: 02.07.2015
 *      Author: DetlevCM
 */
#include <Headers.hpp>

// placeholder functions
void Jacobian_Matrix_DASKR(){};
void PSOL(){};
void RT(){};

// Not a perfect solution, but stick integrator into its own void with global variables via a namespace
void Integrate_Liquid_Phase_DDASKR(
		Filenames OutputFilenames,
		vector< double > SpeciesConcentration,
		Reaction_Mechanism reaction_mechanism,
		Initial_Data InitialParameters,
		vector< double >& KeyRates,
		vector< vector < str_RatesAnalysis > >& RatesAnalysisData
)
{

	/*
C      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
C      INTEGER NEQ, INFO(N), IDID, LRW, LIW, IWORK(LIW), IPAR(*)
C      DOUBLE PRECISION T, Y(*), YPRIME(*), TOUT, RTOL(*), ATOL(*),
C         RWORK(LRW), RPAR(*)
C      EXTERNAL RES, JAC, PSOL, RT
C
C      CALL DDASKR (RES, NEQ, T, Y, YPRIME, TOUT, INFO, RTOL, ATOL,
C     *             IDID, RWORK, LRW, IWORK, LIW, RPAR, IPAR, JAC, PSOL,
C     *             RT, NRT, JROOT)
C
C  Quantities which may be altered by the code are:
C     T, Y(*), YPRIME(*), INFO(1), RTOL, ATOL, IDID, RWORK(*), IWORK(*)
	 */

	vector< TrackSpecies > ProductsForRatesAnalysis;

	using namespace Jacobian_ODE_RHS;
	using namespace ODE_RHS;
	using namespace Jacobian;


	Number_Species = (int)reaction_mechanism.Species.size();
	Number_Reactions = (int)reaction_mechanism.Reactions.size();

	// outputting mechanism size in integration routing so that it is printed every time
	cout << "The mechanism to be integrated contains " << Number_Species << " species and " << Number_Reactions << " Reactions.\n" << std::flush;


	Thermodynamics = reaction_mechanism.Thermodynamics; // "Hack" - to fix a regression


	ofstream ReactionRatesOutput;
	ofstream ConcentrationOutput (OutputFilenames.Species.c_str(),ios::app);

	if(InitialParameters.PrintReacRates)
	{
		ReactionRatesOutput.open(OutputFilenames.Rates.c_str(),ios::app);
	}

	// general variables
	int i, n;

	// general solver settings
	double RTOL, ATOL;

	// LSODA specific settings
	int LRW, LIW;
	int ITOL = 1;
	int ITASK = 1;
	int ISTATE = 1;
	int IOPT = 0;

	// calculate size of working vectors for LSODA
	// LRS = 22 + 9*NEQ + NEQ**2           if JT = 1 or 2,
	n = Number_Species + 1;
	if ( (20 + 16 * n) > (22 + 9 * n + n * n) ) {
		LRW = (20 + 16 * n);
	} else {
		LRW = (22 + 9 * n + n * n);
	}
	LIW = 20 + n;

	// some vectors for LSODA
	vector<int> vector_IWORK(LIW);
	vector<double> vector_RWORK(LRW);

	// For performance assessment, use a clock:
	clock_t cpu_time_begin, cpu_time_end, cpu_time_current;

	// Some tolerances for the solver:
	RTOL = InitialParameters.Solver_Parameters.rtol;
	ATOL = InitialParameters.Solver_Parameters.atol;

	Delta_N = Get_Delta_N(reaction_mechanism.Reactions); // just make sure the Delta_N is current
	// Reduce the matrix from a sparse matrix to something more manageable and quicker to use


	ReactionParameters = Process_Reaction_Parameters(reaction_mechanism.Reactions);

	ReactantsForReactions = Reactants_ForReactionRate(reaction_mechanism.Reactions);

	ProductsForReactions = Products_ForReactionRate(reaction_mechanism.Reactions,false);


	if(InitialParameters.MechanismAnalysis.MaximumRates ||
			InitialParameters.MechanismAnalysis.StreamRatesAnalysis ||
			InitialParameters.MechanismAnalysis.RatesAnalysisAtTime ||
			InitialParameters.MechanismAnalysis.RatesOfSpecies)
	{
		ProductsForRatesAnalysis = Products_ForReactionRate(reaction_mechanism.Reactions,true);
	}

	SpeciesLossAll = PrepareSpecies_ForSpeciesLoss(reaction_mechanism.Reactions); // New method of listing species


	Concentration.clear(); // ensure the concentrations array is empty
	Concentration = SpeciesConcentration; // set it to the initial values, also ensures it has the right length
	//double* y = SpeciesConcentration.data();

	double time_current, time_step, time_step1, time_end;

	time_current = 0; // -> Solver is designed for t_0 = 0
	time_step1 = InitialParameters.TimeStep[0];
	time_end = InitialParameters.TimeEnd[0];
	int TimeChanges = (int) InitialParameters.TimeStep.size();
	int tracker = 0;

	//cout << "\nEnd Time: " << time_end << " Time Step: " << time_step1 << "\n";

	/* -- Initial values at t = 0 -- */

	Number_Reactions = (int) ReactionParameters.size();

	CalculatedThermo.resize(Number_Species);

	InitialDataConstants.EnforceStability = InitialParameters.EnforceStability;
	InitialDataConstants.temperature = InitialParameters.temperature;

	// in case we want to simulate a TGA
	if(InitialParameters.TGA_used)
	{
		InitialDataConstants.TGA_used = InitialParameters.TGA_used;
		InitialDataConstants.TGA_rate = InitialParameters.TGA_rate;
		InitialDataConstants.TGA_target = InitialParameters.TGA_target;
	}

	// set constant concentration if desired
	InitialDataConstants.ConstantConcentration = InitialParameters.ConstantConcentration;
	if(InitialParameters.ConstantConcentration)
	{
		cout << "Constant Species desired\n";

		InitialDataConstants.ConstantSpecies.clear();
		InitialDataConstants.ConstantSpecies.resize(Number_Species);

		// zero just to make sure
		for(i=0;i<Number_Species;i++)
		{
			InitialDataConstants.ConstantSpecies[i] = 0;
		}

		for(i=0;i<(int)InitialParameters.ConstantSpecies.size();i++)
		{// fix initial concentrations
			InitialDataConstants.ConstantSpecies[InitialParameters.ConstantSpecies[i]] =
					SpeciesConcentration[InitialParameters.ConstantSpecies[i]];
		}
	}

	Evaluate_Thermodynamic_Parameters(CalculatedThermo, Thermodynamics, SpeciesConcentration[Number_Species]);

	/*for(i=0;i<Number_Species;i++)
	{
	cout << CalculatedThermo[i].Hf << " " << CalculatedThermo[i].Cp << " " << CalculatedThermo[i].S <<"\n";
	}//*/

	Kf.clear();
	Kr.clear();
	Kf.resize(Number_Reactions);
	Kr.resize(Number_Reactions);

	for(i=0;i<Number_Reactions;i++)
	{
		Kr[i] = 0;
	}
	Rates.resize(Number_Reactions);


	// prepare the jacobian matrix
	Prepare_Jacobian_Matrix(JacobianMatrix,reaction_mechanism.Reactions);


	// Get the rate Constants, forward and backwards
	Calculate_Rate_Constant(Kf, Kr, SpeciesConcentration[Number_Species],ReactionParameters, CalculatedThermo, SpeciesLossAll, Delta_N);
	CalculateReactionRates(Rates, SpeciesConcentration, Kf, Kr, ReactantsForReactions, ProductsForReactions);

	// Don't forget Rates Analysis for Mechanism Recution at t=0 - or is this nonsense?
	if(InitialParameters.MechanismReduction.ReduceReactions != 0)
	{
		ReactionRateImportance(KeyRates, Rates, InitialParameters.MechanismReduction.ReduceReactions);
	}


	// do not forget output at time = 0
	StreamConcentrations(
			ConcentrationOutput,
			InitialParameters.Solver_Parameters.separator,
			InitialParameters.GasPhase,
			Number_Species,
			time_current,
			InitialParameters.GasPhasePressure,
			SpeciesConcentration
	);

	// Only stream if the user desires it, still doesn't prevent file creation...
	if(InitialParameters.PrintReacRates)
	{
		StreamReactionRates(
				ReactionRatesOutput,
				InitialParameters.Solver_Parameters.separator,
				time_current,
				Rates
		);
	}

	// not happy with this more widely available, needs a cleanup...
	vector< vector< int > > ReactionsForSpeciesSelectedForRates;
	// Not the best place to put it, but OK for now:
	if(InitialParameters.MechanismAnalysis.RatesOfSpecies)
	{
		int tempi, tempj;

		vector< vector< int > > TempMatrix;
		vector< int > TempRow;
		int Temp_Number_Species = (int) reaction_mechanism.Species.size();

		for(tempi=0;tempi<(int)reaction_mechanism.Reactions.size();tempi++){
			TempRow.resize((int)reaction_mechanism.Species.size());
			for(tempj=0;tempj<Temp_Number_Species;tempj++)
			{
				if(reaction_mechanism.Reactions[tempi].Reactants[tempj] != 0)
				{
					TempRow[tempj] = 1;
				}
				if(reaction_mechanism.Reactions[tempi].Products[tempj] != 0)
				{
					TempRow[tempj] = 1;
				}
			}
			TempMatrix.push_back(TempRow);
			TempRow.clear();
		}

		int Number_Of_Selected_Species_Temp = (int) InitialParameters.MechanismAnalysis.SpeciesSelectedForRates.size();

		for(tempj=0;tempj<Number_Of_Selected_Species_Temp;tempj++)
		{
			int SpeciesID = InitialParameters.MechanismAnalysis.SpeciesSelectedForRates[tempj];
			vector< int > temp;

			for(tempi=0;tempi<(int)reaction_mechanism.Reactions.size();tempi++)
			{
				if(TempMatrix[tempi][SpeciesID] !=0 )
				{
					temp.push_back(tempi);
				}

			}
			ReactionsForSpeciesSelectedForRates.push_back(temp);
			temp.clear();
		}

		Prepare_Print_Rates_Per_Species(
				InitialParameters.Solver_Parameters.separator,
				reaction_mechanism.Species,
				InitialParameters.MechanismAnalysis.SpeciesSelectedForRates,
				ReactionsForSpeciesSelectedForRates
		);
	}


	vector< double > SpeciesConcentrationChange = SpeciesLossRate(Number_Species, Rates, SpeciesLossAll);


	/* -- Got values at t = 0 -- */


	// enables reset of Rates Analysis
	int RatesAnalysisTimepoint = 0;

	// set the solver:
	bool Use_Analytical_Jacobian;
	Use_Analytical_Jacobian = InitialParameters.Solver_Parameters.Use_Analytical_Jacobian;

	// start the clock:
	cpu_time_begin = cpu_time_current = clock();


	do
	{
		time_step = time_current + time_step1;

		vector<double> YPRIME;
		vector<int> INFO;

		vector<double> RPAR;
		vector<int> IPAR;
		vector<int> JROOT;
		int IDID;
		int NRT;

		    /*  SUBROUTINE DDASKR (RES, NEQ, T, Y, YPRIME, TOUT, INFO, RTOL, ATOL,
		     *   IDID, RWORK, LRW, IWORK, LIW, RPAR, IPAR, JAC, PSOL,
		     *   RT, NRT, JROOT)//*/
			ddaskr_(
					(void*) ODE_RHS_Liquid_Phase, // needs redoing to fit requirements
					&n,
					&time_current,
					SpeciesConcentration.data(),
					YPRIME.data(),
					&time_step,
					INFO.data(),
					&RTOL,
					&ATOL,
					&IDID,
					vector_RWORK.data(),
					&LRW,
					vector_IWORK.data(),
					&LIW,
					RPAR.data(),
					IPAR.data(),
					(void*) Jacobian_Matrix_DASKR,
					(void*) PSOL,
					(void*) RT,
					&NRT,
					JROOT.data()
					);


		if (ISTATE < 0)
		{
			printf("\n LSODA routine exited with error code %4d\n",ISTATE);
			// Break means it should leave the do loop which would be fine for an error response as it stops the solver
			break ;
		}


		if(InitialParameters.MechanismAnalysis.MaximumRates)
		{
			MaxRatesAnalysis(RatesAnalysisData,ProductsForRatesAnalysis,ReactantsForReactions,Rates,time_current);
		}


		if(InitialParameters.MechanismAnalysis.RatesAnalysisAtTime &&
				InitialParameters.MechanismAnalysis.RatesAnalysisAtTimeData[RatesAnalysisTimepoint] == time_current)
		{
			RatesAnalysisAtTimes(
					ProductsForRatesAnalysis,
					ReactantsForReactions,
					Rates,
					time_current,
					reaction_mechanism.Species,
					reaction_mechanism.Reactions
			);
			RatesAnalysisTimepoint = RatesAnalysisTimepoint + 1;
		}


		// Function for new per species output
		if(InitialParameters.MechanismAnalysis.RatesOfSpecies)
		{
			Print_Rates_Per_Species(
					ProductsForRatesAnalysis,
					ReactantsForReactions,
					InitialParameters.Solver_Parameters.separator,
					Rates,
					time_current,
					reaction_mechanism.Species,
					InitialParameters.MechanismAnalysis.SpeciesSelectedForRates,
					ReactionsForSpeciesSelectedForRates//,
					//reaction_mechanism.Reactions
			);
		}


		if(InitialParameters.MechanismAnalysis.StreamRatesAnalysis)
		{
			StreamRatesAnalysis(
					OutputFilenames.Prefix,
					ProductsForRatesAnalysis,
					ReactantsForReactions,
					Rates,
					time_current,
					Number_Species
			);
		}


		double pressure = 0;
		/*
		if(InitialParameters.GasPhase)
		{
			double R = 8.31451; // Gas Constant
			// Pressure Tracking for Gas Phase Kinetics
		double total_mol = 0;
		for(i=0;i<Number_Species;i++)
		{
			total_mol = total_mol + SpeciesConcentration[i];
		}
		pressure = (total_mol  * R * SpeciesConcentration[Number_Species])/InitialParameters.GasPhaseVolume;
		}//*/

		StreamConcentrations(
				ConcentrationOutput,
				InitialParameters.Solver_Parameters.separator,
				false, //InitialParameters.GasPhase,
				Number_Species,
				time_current,
				pressure,
				SpeciesConcentration
		);

		if(InitialParameters.PrintReacRates)
		{
			StreamReactionRates(
					ReactionRatesOutput,
					InitialParameters.Solver_Parameters.separator,
					time_current,
					Rates
			);
		}


		if(InitialParameters.MechanismReduction.ReduceReactions != 0)
		{
			ReactionRateImportance(KeyRates, Rates, InitialParameters.MechanismReduction.ReduceReactions);
		}


		if(tracker < (TimeChanges-1) && time_step >= InitialParameters.TimeEnd[tracker])
		{
			cout << "CPU Time: " << ((double) (clock() - cpu_time_current)) / CLOCKS_PER_SEC << " seconds\n";
			cpu_time_current = clock();

			tracker = tracker + 1;
			time_step1 = InitialParameters.TimeStep[tracker];
			time_end = InitialParameters.TimeEnd[tracker];
			cout << "End Time: " << time_end << " Time Step: " << time_step1 << "\n";
		}


	} while (time_step < time_end);


	cout << "CPU Time: " << ((double) (clock() - cpu_time_current)) / CLOCKS_PER_SEC << " seconds\n";

	// close output files
	ConcentrationOutput.close();
	ReactionRatesOutput.close();

	if(InitialParameters.MechanismAnalysis.MaximumRates)
	{
		WriteMaxRatesAnalysis(RatesAnalysisData, reaction_mechanism.Species, reaction_mechanism.Reactions,OutputFilenames.Prefix);
	}


	// stop the clock
	cpu_time_end = clock();
	cout << "\nTotal CPU time: " << ((double) (cpu_time_end - cpu_time_begin)) / CLOCKS_PER_SEC << " seconds\n\n";
}



