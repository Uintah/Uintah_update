/*

The MIT License

Copyright (c) 1997-2009 Center for the Simulation of Accidental Fires and 
Explosions (CSAFE), and  Scientific Computing and Imaging Institute (SCI), 
University of Utah.

License for the specific language governing rights and limitations under
Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

*/


/*
 *  puda.cc: Print out a uintah data archive
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   February 2000
 *
 *  Copyright (C) 2000 U of U
 */

/*
 *  Support for printing out Tecplot data added by Patric Hu
 *  Department of Mechanical Enginerring, U of U, 2003.
 *
 *  Currently it only supports CCVariables.
 * 
 * Usage of converting Uintah data archive to a tecplot data file
 * puda -tecplot <i_xd> <uda directory> :
 *       print all CCVariables into different tecplot data files 
 * puda -tecplot <i_xd> <CCVariable's Name> <uda directory>:
 *       print one CCVariable into a tecplot data file
 * puda -tecplot <i_xd> <tskip> <uda directory>:
 *       print all CCVariables into different tecplot data files
 *       by every tskip time steps
 * puda -tecplot <i_xd> <CCVariable's Name> <tskip> <uda directory>:
 *       print one CCVariable into a tecplot data file
 *       by every tskip time steps
 * i_xd may be i_1d, i_2d, i_3d for 1D, 2D and 3D problem
 *
 */

#include <Uintah/StandAlone/tools/puda/puda.h>

#include <Uintah/Core/DataArchive/DataArchive.h>
#include <Uintah/Core/Disclosure/TypeDescription.h>
#include <Uintah/Core/Grid/Grid.h>
#include <Uintah/Core/Grid/Level.h>
#include <Uintah/Core/Grid/Variables/NodeIterator.h>
#include <Uintah/Core/Grid/Variables/CellIterator.h>
#include <Uintah/Core/Grid/Variables/SFCXVariable.h>
#include <Uintah/Core/Grid/Variables/SFCYVariable.h>
#include <Uintah/Core/Grid/Variables/SFCZVariable.h>
#include <Uintah/Core/Math/Matrix3.h>

#include <Uintah/StandAlone/tools/puda/asci.h>
#include <Uintah/StandAlone/tools/puda/jim1.h>
#include <Uintah/StandAlone/tools/puda/jim2.h>
#include <Uintah/StandAlone/tools/puda/rtdata.h>
#include <Uintah/StandAlone/tools/puda/tecplot.h>
#include <Uintah/StandAlone/tools/puda/util.h>
#include <Uintah/StandAlone/tools/puda/varsummary.h>

#include <Core/Containers/Array3.h>
#include <Core/Geometry/Point.h>
#include <Core/Geometry/Vector.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <cstdio>
#include <cmath>

using namespace SCIRun;
using namespace std;
using namespace Uintah;

/////////////////////////////////////////////////////////////////
// Pre-declarations:
//
void printParticleVariable(DataArchive* da, 
			   string particleVariable,
			   unsigned long time_step_lower,
			   unsigned long time_step_upper,
                           int mat);

/////////////////////////////////////////////////////////////////

void
usage( const std::string& badarg, const std::string& progname )
{
  if(badarg != "")
    cerr << "Error parsing argument: " << badarg << "\n";
  cerr << "Usage: " << progname << " [options] <archive file>\n\n";
  cerr << "Valid options are:\n";
  cerr << "  -h[elp]\n";
  cerr << "  -timesteps\n";
  cerr << "  -gridstats\n";
  cerr << "  -listvariables\n";
  cerr << "  -varsummary\n";
  cerr << "  -jim1\n";
  cerr << "  -jim2\n";
  cerr << "  -partvar <variable name>\n";
  cerr << "  -asci\n";
  cerr << "  -tecplot <variable name>\n";
  cerr << "  -no_extra_cells     (Excludes extra cells when iterating over cells.\n";
  cerr << "                       Default is to include extra cells.)\n";
  cerr << "  -cell_stresses\n";
  cerr << "  -rtdata <output directory>\n";
  cerr << "  -PTvar\n";
  cerr << "  -ptonly             (prints out only the point location\n";
  cerr << "  -patch              (outputs patch id with data)\n";
  cerr << "  -material           (outputs material number with data)\n";
  cerr << "  -NCvar <double | float | point | vector>\n";
  cerr << "  -CCvar <double | float | point | vector>\n";
  cerr << "  -verbose            (prints status of output)\n";
  cerr << "  -timesteplow <int>  (only outputs timestep from int)\n";
  cerr << "  -timestephigh <int> (only outputs timesteps upto int)\n";
  cerr << "  -matl,mat <int>         (only outputs data for matl)\n";
  cerr << "*NOTE* to use -PTvar or -NVvar -rtdata must be used\n";
  cerr << "*NOTE* ptonly, patch, material, timesteplow, timestephigh "
       << "are used in conjuntion with -PTvar.\n\n";
    
  cerr << "USAGE IS NOT FINISHED\n\n";
  exit(1);
}

void
gridstats( DataArchive* da,
           const bool tslow_set, 
           const bool tsup_set,
           unsigned long & time_step_lower,
           unsigned long & time_step_upper )
{
  vector<int> index;
  vector<double> times;
  da->queryTimesteps(index, times);
  ASSERTEQ(index.size(), times.size());
      
  findTimestep_loopLimits(tslow_set, tsup_set,times, time_step_lower, time_step_upper);
           
  for( unsigned long t = time_step_lower; t <= time_step_upper; t++ ) {
    double time = times[t];
    cout << "__________________________________\n";
    cout << "Timestep " << t << ": " << time << "\n";
    GridP grid = da->queryGrid(t);
    grid->performConsistencyCheck();
    grid->printStatistics();

    Vector domainLength;
    grid->getLength(domainLength, "minusExtraCells");
    cout << "Domain Length:        " << domainLength << "\n";

    BBox box;
    grid->getInteriorSpatialRange( box );
    cout << "\nInterior Spatial Range: " << box << "\n";

    grid->getSpatialRange( box );
    cout << "Spatial Range:          " << box << "\n\n";
  
    for(int l=0;l<grid->numLevels();l++){
      LevelP level = grid->getLevel(l);
      cout << "Level: index " << level->getIndex() << ", id " << level->getID() << "\n";
      
      BBox lbox;
      level->getInteriorSpatialRange( lbox );
      cout << "\n       Interior Spatial Range: " << lbox << "\n";
      
      level->getSpatialRange( lbox );
      cout << "       Spatial Range:          " << lbox << "\n\n";

      IntVector lo, hi;
      level->findInteriorCellIndexRange(lo,hi);
      cout << "Total Number of Cells:" << hi-lo << "\n";
      cout << "dx:                   " << level->dCell() << "\n";

      for(Level::const_patchIterator iter = level->patchesBegin();
          iter != level->patchesEnd(); iter++){
        const Patch* patch = *iter;
        cout << *patch << "\n"; 
        cout << "\t   BC types: x- " << patch->getBCType(Patch::xminus) << ", x+ "<<patch->getBCType(Patch::xplus)
             << ", y- "<< patch->getBCType(Patch::yminus) << ", y+ "<< patch->getBCType(Patch::yplus)
             << ", z- "<< patch->getBCType(Patch::zminus) << ", z+ "<< patch->getBCType(Patch::zplus) << "\n";
      }
    }
  }
} // end gridstats()


int
main(int argc, char** argv)
{
  if (argc <= 1) {
    // Print out the usage and die
    usage("", argv[0]);
  }

  CommandLineFlags clf;

  int mat = -1; //not part of clf

  // set defaults for cout.
  cout.setf(ios::scientific,ios::floatfield);
  cout.precision(8);
  /*
   * Parse arguments
   */
  for(int i=1;i<argc;i++){
    string s=argv[i];
    if(s == "-timesteps"){
      clf.do_timesteps=true;}
    else if( s == "-no_extra_cells" ||
             s == "-no_extracells" ||
             s == "-no_ExtraCells" ) {
      clf.use_extra_cells = false;
    }
    else if(s == "-tecplot"){ 
      clf.do_tecplot = true;
      if(argc == 4) {
        clf.do_all_ccvars = true;
        clf.i_xd = argv[i+1];
        clf.tskip = 1;
      } else if(argc == 5){
        clf.do_all_ccvars = true;
        clf.i_xd = argv[i+1];
        clf.tskip = atoi(argv[i+2]);
      } else if(argc == 6 ) {
        clf.i_xd = argv[i+1];
        clf.ccVarInput = argv[i+2];
        clf.tskip = atoi(argv[i+3]);
        if (clf.ccVarInput[0] == '-')
          usage("-tecplot <i_xd> <ccVariable name> <tskip> ", argv[0]);
      }
    } else if(s == "-gridstats" ||
              s == "-gridStats" ||
              s == "-grid_stats"){
      clf.do_gridstats=true;
    } else if(s == "-listvariables" || 
              s == "-listVariables" || 
              s == "-list_variables"){
      clf.do_listvars=true;
    } else if(s == "-varsummary" ||
              s == "-varSummary" ||
              s == "-var_summary"){
      clf.do_varsummary=true;
    } else if(s == "-jim1"){
      clf.do_jim1=true;
    } else if(s == "-jim2"){
      clf.do_jim2=true;
    } else if(s == "-partvar"){
      clf.do_partvar=true;
      clf.particleVariable = argv[++i]; 
      if (clf.particleVariable[0] == '-') {
        usage("-partvar <particle variable name>", argv[0]);
      }
    } else if(s == "-asci"){
      clf.do_asci=true;
    } else if(s == "-cell_stresses"){
      clf.do_cell_stresses=true;
    } else if(s == "-rtdata") {
      clf.do_rtdata = true;
      if (++i < argc) {
        s = argv[i];
        if (s[0] == '-') {
          usage("-rtdata", argv[0]);
        }
        clf.raydatadir = s;
      }
    } else if(s == "-NCvar") {
      if (++i < argc) {
        s = argv[i];
        if (s == "double")
          clf.do_NCvar_double = true;
        else if (s == "float")
          clf.do_NCvar_float = true;
        else if (s == "point")
          clf.do_NCvar_point = true;
        else if (s == "vector")
          clf.do_NCvar_vector = true;
        else if (s == "matrix3")
          clf.do_NCvar_matrix3 = true;
        else
          usage("-NCvar", argv[0]);
      }
      else
        usage("-NCvar", argv[0]);
    } else if(s == "-CCvar") {
      if (++i < argc) {
        s = argv[i];
        if (s == "double")
          clf.do_CCvar_double = true;
        else if (s == "float")
          clf.do_CCvar_float = true;
        else if (s == "point")
          clf.do_CCvar_point = true;
        else if (s == "vector")
          clf.do_CCvar_vector = true;
        else if (s == "matrix3")
          clf.do_CCvar_matrix3 = true;
        else
          usage("-CCvar", argv[0]);
      }
      else
        usage("-CCvar", argv[0]);
    } else if(s == "-PTvar") {
      clf.do_PTvar = true;
    } else if (s == "-ptonly") {
      clf.do_PTvar_all = false;
    } else if (s == "-patch") {
      clf.do_patch = true;
    } else if (s == "-material" ||
               s == "-matl" || s == "-mat") {
      clf.matl_jim = strtoul(argv[++i],(char**)NULL,10);
      clf.do_material = true;
      mat = clf.matl_jim;

    } else if (s == "-verbose") {
      clf.do_verbose = true;
    } else if (s == "-timesteplow" ||
               s == "-timeStepLow" ||
               s == "-timestep_low") {
      clf.time_step_lower = strtoul(argv[++i],(char**)NULL,10);
      clf.tslow_set = true;
    } else if (s == "-timestephigh" ||
               s == "-timeStepHigh" ||
               s == "-timestep_high") {
      clf.time_step_upper = strtoul(argv[++i],(char**)NULL,10);
      clf.tsup_set = true;
    } else if (s == "-timestepinc" ||
               s == "-timestepInc" ||
               s == "-timestep_inc") {
      clf.time_step_inc = strtoul(argv[++i],(char**)NULL,10);
    } else if( (s == "-help") || (s == "-h") ) {
      usage( "", argv[0] );
    } else if( clf.filebase == "") {

      if( argv[i][0] == '-' ) { // File name can't start with a dash.
        usage( s, argv[0]);
      }
      clf.filebase = argv[i];
    } else {
      usage( s, argv[0]);
    }
  }

  if( clf.filebase == "" ) {
    cerr << "No archive file specified\n";
    usage("", argv[0]);
  }

  try {
    DataArchive* da = scinew DataArchive( clf.filebase );
    //__________________________________
    //  LIST TIMESTEPS
    if(clf.do_timesteps){
      vector<int> index;
      vector<double> times;
      da->queryTimesteps(index, times);
      ASSERTEQ(index.size(), times.size());
      cout << "There are " << index.size() << " timesteps:\n";
      
      // Please don't change this.  We need 16
      // significant digits for detailed comparative studies. -Todd
      cout.setf(ios::scientific,ios::floatfield);
      cout.precision(16);
      
      for(int i=0;i<(int)index.size();i++)
	cout << index[i] << ": " << times[i] << "\n";
    }
    //__________________________________

    //    DO GRIDSTATS
    if(clf.do_gridstats){
      gridstats( da, clf.tslow_set, clf.tsup_set, clf.time_step_lower, clf.time_step_upper );
    }
    
    //__________________________________
    //  LIST VARIABLES
    if(clf.do_listvars){
      vector<string> vars;
      vector<const Uintah::TypeDescription*> types;
      da->queryVariables(vars, types);
      cout << "There are " << vars.size() << " variables:\n";
      for(int i=0;i<(int)vars.size();i++){
	cout << vars[i] << ": " << types[i]->getName() << "\n";
      }
    }

    // Print a particular particle variable
    if (clf.do_partvar) {
      vector<int> index;
      vector<double> times;
      da->queryTimesteps(index, times);
      ASSERTEQ(index.size(), times.size());
      if( !clf.tslow_set ) {
	clf.time_step_lower =0;
      }
      else if (clf.time_step_lower >= times.size()) {
	cerr << "timesteplow must be between 0 and " << times.size()-1 << "\n";
	abort();
      }
      if( !clf.tsup_set ) {
	clf.time_step_upper = times.size() - 1;
      }
      else if( clf.time_step_upper >= times.size() ) {
	cerr << "timestephigh must be between 0 and " << times.size()-1 << "\n";
	abort();
      }
      printParticleVariable( da, clf.particleVariable,
                             clf.time_step_lower, clf.time_step_upper, mat );
    }
#if 0
    tecplot();
#endif
    //______________________________________________________________________
    //              V A R S U M M A R Y   O P T I O N
    if(clf.do_varsummary){
      varsummary( da, clf, mat );
    }

    if( clf.do_jim1 ){
      jim1( da, clf );
    }

    if( clf.do_jim2 ){
      jim2( da, clf );
    }

    if (clf.do_asci){
      asci( da, clf.tslow_set, clf.tsup_set, clf.time_step_lower, clf.time_step_upper );
    }

    //______________________________________________________________________
    //	       DO CELL STRESSES	
    if (clf.do_cell_stresses){
      vector<string> vars;
      vector<const Uintah::TypeDescription*> types;
      da->queryVariables(vars, types);
      ASSERTEQ(vars.size(), types.size());
      
      cout << "There are " << vars.size() << " variables:\n";
      vector<int> index;
      vector<double> times;
      da->queryTimesteps(index, times);
      ASSERTEQ(index.size(), times.size());
      
      cout << "There are " << index.size() << " timesteps:\n";
      
      findTimestep_loopLimits( clf.tslow_set, clf.tsup_set,times, clf.time_step_lower, clf.time_step_upper );
      
      // obtain the desired timesteps
      unsigned long t = 0, start_time, stop_time;

      cout << "Time Step       Value\n";
      
      for(t = clf.time_step_lower; t <= clf.time_step_upper; t++){
	double time = times[t];
	cout << "    " << t + 1 << "        "  << time << "\n";
      }
      cout << "\n";
      if (t != (clf.time_step_lower +1)){
	cout << "Enter start time-step (1 - " << t << "): ";
	cin >> start_time;
	start_time--;
	cout << "Enter stop  time-step (1 - " << t << "): ";
	cin >> stop_time;
	stop_time--;
      }
      else 
      	if(t == (clf.time_step_lower + 1)){
	  start_time = t-1;
	  stop_time  = t-1;
	}
      // end of timestep acquisition
      
      for(t=start_time;t<=stop_time;t++){
	
	double time = times[t];
	cout << "time = " << time << "\n";
	GridP grid = da->queryGrid(t);
	for(int v=0;v<(int)vars.size();v++){
	  std::string var = vars[v];
	  
	  // only dumps out data if it is variable g.stressFS
	  if (var == "g.stressFS"){
	    const Uintah::TypeDescription* td = types[v];
	    const Uintah::TypeDescription* subtype = td->getSubType();
	    cout << "\tVariable: " << var << ", type " << td->getName() << "\n";
	    for(int l=0;l<grid->numLevels();l++){
	      LevelP level = grid->getLevel(l);
	      for(Level::const_patchIterator iter = level->patchesBegin();
		  iter != level->patchesEnd(); iter++){
		const Patch* patch = *iter;
		cout << "\t\tPatch: " << patch->getID() << "\n";
                ConsecutiveRangeSet matls =
		  da->queryMaterials(var, patch, t);
	        // loop over materials
	        for(ConsecutiveRangeSet::iterator matlIter = matls.begin();
		    matlIter != matls.end(); matlIter++){
		  int matl = *matlIter;
		  if (mat != -1 && matl != mat) continue;
 
		  // dumps header and variable info to file
		  ostringstream fnum, pnum, matnum; 
		  string filename;
		  unsigned long timestepnum=t+1;
		  fnum << setw(4) << setfill('0') << timestepnum;
                  pnum << setw(4) << setfill('0') << patch->getID();
		  matnum << setw(4) << setfill('0') << matl;
		  string partroot("stress.t");
                  string partextp(".p"); 
		  string partextm(".m");
		  filename = partroot+fnum.str()+partextp+pnum.str()+partextm+matnum.str();
		  ofstream partfile(filename.c_str());
		  partfile << "# x, y, z, st11, st12, st13, st21, st22, st23, st31, st32, st33\n";
		  
		  cout << "\t\t\tMaterial: " << matl << "\n";
		  switch(td->getType()){
		  case Uintah::TypeDescription::NCVariable:
		    switch(subtype->getType()){
		    case Uintah::TypeDescription::Matrix3:{
		      NCVariable<Matrix3> value;
		      da->query(value, var, matl, patch, t);
		      cout << "\t\t\t\t" << td->getName() << " over " << value.getLowIndex()
			   << " to " << value.getHighIndex() << "\n";
		      IntVector dx(value.getHighIndex()-value.getLowIndex());
		      if(dx.x() && dx.y() && dx.z()){
			NodeIterator iter = patch->getNodeIterator__New();
			for(;!iter.done(); iter++){
			  partfile << (*iter).x() << " " << (*iter).y() << " " << (*iter).z()
				   << " " << (value[*iter])(0,0) << " " << (value[*iter])(0,1) << " " 
				   << (value[*iter])(0,2) << " " << (value[*iter])(1,0) << " "
				   << (value[*iter])(1,1) << " " << (value[*iter])(1,2) << " "
				   << (value[*iter])(2,0) << " " << (value[*iter])(2,1) << " "
                                   << (value[*iter])(2,2) << "\n";
			}
		      }
		    }
		      break;
		    default:
		      cerr << "No Matrix3 Subclass avaliable." << subtype->getType() << "\n";
		      break;
		    }
		    break;
		  default:
		    cerr << "No NC Variables avaliable." << td->getType() << "\n";
		    break;
		  }
		}
	      }
	    }
	  }
	  else
	    cout << "No g.stressFS variables avaliable at time " << t << ".\n";
	}
	if (start_time == stop_time)
	  t++;   
      }
    } // end do_cell_stresses
    
    //______________________________________________________________________
    //              DO RTDATA
    if (clf.do_rtdata) {
      rtdata( da, clf );
    }
  } catch (Exception& e) {
    cerr << "Caught exception: " << e.message() << "\n";
    abort();
  } catch(...){
    cerr << "Caught unknown exception\n";
    abort();
  }
} // end main()

////////////////////////////////////////////////////////////////////////////
//
// Print ParticleVariable
//
void
printParticleVariable( DataArchive* da, 
                       string particleVariable,
                       unsigned long time_step_lower,
                       unsigned long time_step_upper, int mat)
{
  // Check if the particle variable is available
  vector<string> vars;
  vector<const Uintah::TypeDescription*> types;
  da->queryVariables(vars, types);
  ASSERTEQ(vars.size(), types.size());
  bool variableFound = false;
  for(unsigned int v=0;v<vars.size();v++){
    std::string var = vars[v];
    if (var == particleVariable) variableFound = true;
  }
  if (!variableFound) {
    cerr << "Variable " << particleVariable << " not found\n"; 
    exit(1);
  }

  // Now that the variable has been found, get the data for all 
  // available time steps // from the data archive
  vector<int> index;
  vector<double> times;
  da->queryTimesteps(index, times);
  ASSERTEQ(index.size(), times.size());
  //cout << "There are " << index.size() << " timesteps:\n";
      
  bool useParticleID = true;

  // Loop thru all time steps and store the volume and variable (stress/strain)
  for(unsigned long t=time_step_lower;t<=time_step_upper;t++){
    double time = times[t];
    //cout << "Time = " << time << "\n";
    GridP grid = da->queryGrid(t);

    // Loop thru all the levels
    for(int l=0;l<grid->numLevels();l++){
      LevelP level = grid->getLevel(l);

      // Loop thru all the patches
      Level::const_patchIterator iter = level->patchesBegin(); 
      int patchIndex = 0;
      for(; iter != level->patchesEnd(); iter++){
	const Patch* patch = *iter;
        ++patchIndex; 

	// Loop thru all the variables 
	for(int v=0;v<(int)vars.size();v++){
	  std::string var = vars[v];
	  const Uintah::TypeDescription* td = types[v];
	  const Uintah::TypeDescription* subtype = td->getSubType();

	  // Check if the variable is a ParticleVariable
	  if(td->getType() == Uintah::TypeDescription::ParticleVariable) { 

	    // loop thru all the materials
	    ConsecutiveRangeSet matls = da->queryMaterials(var, patch, t);
	    ConsecutiveRangeSet::iterator matlIter = matls.begin(); 
	    for(; matlIter != matls.end(); matlIter++){
	      int matl = *matlIter;
              if (mat != -1 && matl != mat) continue;

	      // Find the name of the variable
	      if (var == particleVariable) {
		//cout << "Material: " << matl << "\n";
		switch(subtype->getType()){
		case Uintah::TypeDescription::double_type:
		  {
		    ParticleVariable<double> value;
		    da->query(value, var, matl, patch, t);
		    ParticleVariable<long64> pid;
                    if( useParticleID ) {
                      try {
                        // If particleID wasn't saved, just move on...
                        da->query(pid, "p.particleID", matl, patch, t);
                      } catch( Exception & e ) {
                        useParticleID = false;
                      }
                    }
		    ParticleSubset* pset = value.getParticleSubset();
		    if(pset->numParticles() > 0){
		      ParticleSubset::iterator iter = pset->begin();
		      for(;iter != pset->end(); iter++){
                        cout << time << " " << patchIndex << " " << matl; 
                        if( useParticleID ) {
                          cout << " " << pid[*iter];
                        }
                        cout << " " << value[*iter] << "\n";
		      }
		    }
		  }
		break;
		case Uintah::TypeDescription::float_type:
		  {
		    ParticleVariable<float> value;
		    da->query(value, var, matl, patch, t);
                    ParticleVariable<long64> pid;
                    if( useParticleID ) {
                      try {
                        // If particleID wasn't saved, just move on...
                        da->query(pid, "p.particleID", matl, patch, t);
                      } catch( Exception & e ) {
                        useParticleID = false;
                      }
                    }
		    ParticleSubset* pset = value.getParticleSubset();
		    if(pset->numParticles() > 0){
		      ParticleSubset::iterator iter = pset->begin();
		      for(;iter != pset->end(); iter++){
                        cout << time << " " << patchIndex << " " << matl ;
                        if( useParticleID ) {
                          cout << " " << pid[*iter];
                        }
                        cout << " " << value[*iter] << "\n";
		      }
		    }
		  }
		break;
		case Uintah::TypeDescription::int_type:
		  {
		    ParticleVariable<int> value;
		    da->query(value, var, matl, patch, t);
		    ParticleSubset* pset = value.getParticleSubset();
                    ParticleVariable<long64> pid;
                    if( useParticleID ) {
                      try {
                        // If particleID wasn't saved, just move on...
                        da->query(pid, "p.particleID", matl, patch, t);
                      } catch( Exception & e ) {
                        useParticleID = false;
                      }
                    }
		    if(pset->numParticles() > 0){
		      ParticleSubset::iterator iter = pset->begin();
		      for(;iter != pset->end(); iter++){
                        cout << time << " " << patchIndex << " " << matl;
                        if( useParticleID ) {
                          cout << " " << pid[*iter];
                        }
                        cout << " " << value[*iter] << "\n";
		      }
		    }
		  }
		break;
		case Uintah::TypeDescription::Point:
		  {
		    ParticleVariable<Point> value;
		    da->query(value, var, matl, patch, t);
		    ParticleSubset* pset = value.getParticleSubset();
                    ParticleVariable<long64> pid;
                    if( useParticleID ) {
                      try {
                        // If particleID wasn't saved, just move on...
                        da->query(pid, "p.particleID", matl, patch, t);
                      } catch( Exception & e ) {
                        useParticleID = false;
                      }
                    }
		    if(pset->numParticles() > 0){
		      ParticleSubset::iterator iter = pset->begin();
		      for(;iter != pset->end(); iter++){
                        cout << time << " " << patchIndex << " " << matl ;
                        if( useParticleID ) {
                          cout << " " << pid[*iter];
                        }
                        cout << " " << value[*iter](0) 
                             << " " << value[*iter](1)
                             << " " << value[*iter](2) << "\n";
		      }
		    }
		  }
		break;
		case Uintah::TypeDescription::Vector:
		  {
		    ParticleVariable<Vector> value;
		    da->query(value, var, matl, patch, t);
                    ParticleVariable<long64> pid;
                    if( useParticleID ) {
                      try {
                        // If particleID wasn't saved, just move on...
                        da->query(pid, "p.particleID", matl, patch, t);
                      } catch( Exception & e ) {
                        useParticleID = false;
                      }
                    }
		    ParticleSubset* pset = value.getParticleSubset();
		    if(pset->numParticles() > 0){
		      ParticleSubset::iterator iter = pset->begin();
		      for(;iter != pset->end(); iter++){
                        if( useParticleID ) {
                          cout << time << " " << patchIndex << " " << matl ;
                        }
                        cout << " " << pid[*iter];
			cout << " " << value[*iter][0] 
                             << " " << value[*iter][1]
                             << " " << value[*iter][2] << "\n";
		      }
		    }
		  }
		break;
		case Uintah::TypeDescription::Matrix3:
		  {
		    ParticleVariable<Matrix3> value;
		    da->query(value, var, matl, patch, t);
                    ParticleVariable<long64> pid;
                    if( useParticleID ) {
                      try {
                        // If particleID wasn't saved, just move on...
                        da->query(pid, "p.particleID", matl, patch, t);
                      } catch( Exception & e ) {
                        useParticleID = false;
                      }
                    }
		    ParticleSubset* pset = value.getParticleSubset();
		    if(pset->numParticles() > 0){
		      ParticleSubset::iterator iter = pset->begin();
		      for(;iter != pset->end(); iter++){
                        cout << time << " " << patchIndex << " " << matl ;
                        if( useParticleID ) {
                          cout << " " << pid[*iter];
                        }
                        for (int ii = 0; ii < 3; ++ii) {
                          for (int jj = 0; jj < 3; ++jj) {
			    cout << " " << value[*iter](ii,jj) ;
                          }
                        }
			cout << "\n";
		      }
		    }
		  }
		break;
		case Uintah::TypeDescription::long64_type:
		  {
		    ParticleVariable<long64> value;
		    da->query(value, var, matl, patch, t);
		    ParticleSubset* pset = value.getParticleSubset();
		    if(pset->numParticles() > 0){
		      ParticleSubset::iterator iter = pset->begin();
		      for(;iter != pset->end(); iter++){
                        cout << time << " " << patchIndex << " " << matl ;
			cout << " " << value[*iter] << "\n";
		      }
		    }
		  }
		break;
		default:
		  cerr << "Particle Variable of unknown type: " 
		       << subtype->getType() << "\n";
		  break;
		}
	      } // end of var compare if
	    } // end of material loop
	  } // end of ParticleVariable if
	} // end of variable loop
      } // end of patch loop
    } // end of level loop
  } // end of time step loop
} // end printParticleVariable()
