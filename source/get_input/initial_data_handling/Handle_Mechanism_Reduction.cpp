/*
 * Handle_MechanismReduction.cpp
 *
 *  Created on: 19 Mar 2016
 *      Author: DetlevCM
 */

#include <MyHeaders.h>


void Handle_Mechanism_Reduction(InitParam& InitialParameters, vector<string> Input)
{
	int i;
	for(i=0;i<(int)Input.size();i++)
	{
		if(Test_If_Word_Found(Input[i], "Use New Lumping"))
		{
			InitialParameters.UseNewLumping = true;
		}
		if(Test_If_Word_Found(Input[i], "Use Slow New Lumping"))
		{
			InitialParameters.UseNewLumping = true;
			InitialParameters.UseFastLumping = false;
		}
		if(Test_If_Word_Found(Input[i], "Use Slow Lumping"))
		{
			InitialParameters.UseFastLumping = false;
		}
		if(Test_If_Word_Found(Input[i], "ReduceReactions"))
		{
			char * cstr, *p;
			string str;
			str = Input[i];//a String - For some reason cannot pass line1 directly...
			cstr = new char [str.size()+1];
			strcpy (cstr, str.c_str());

			p=strtok (cstr," \t"); // break at space or tab
			p=strtok(NULL," \t"); // break again as first is the keyword

			if(p!=NULL){ // only read remainder is something is left
				InitialParameters.ReduceReactions = strtod(p,NULL);
				p=strtok(NULL," \t");
			}
			delete[] cstr;
		}
	}
}
