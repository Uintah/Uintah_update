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


#include <Uintah/CCA/Components/ProblemSpecification/ProblemSpecReader.h>
#include <Uintah/Core/Exceptions/ProblemSetupException.h>
#include <Uintah/Core/Grid/Variables/Array3.h>
#include <Uintah/Core/Grid/Variables/CellIterator.h>
#include <Uintah/Core/Grid/Grid.h>
#include <Uintah/Core/Grid/Level.h>
#include <Uintah/Core/Grid/Patch.h>
#include <Uintah/Core/GeometryPiece/GeometryPieceFactory.h>
#include <Uintah/Core/GeometryPiece/UnionGeometryPiece.h>
#include <Uintah/Core/GeometryPiece/FileGeometryPiece.h>
#include <Uintah/Core/GeometryPiece/CylinderGeometryPiece.h>
#include <Uintah/Core/Grid/GridP.h>
#include <Uintah/Core/ProblemSpec/ProblemSpecP.h>
#include <Uintah/Core/Grid/Variables/VarTypes.h>
#include <Uintah/Core/Parallel/ProcessorGroup.h>
#include <Uintah/Core/Parallel/Parallel.h>
#include <Uintah/Core/Math/Primes.h>
#include <Core/Geometry/IntVector.h>
#include <Core/Malloc/Allocator.h>

#include <cstdio>
#include <sgi_stl_warnings_off.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sgi_stl_warnings_on.h>

using namespace Uintah;
using namespace std;

typedef unsigned char byte;

// forwared function declarations
void usage( char *prog_name );
//void parseArgs( int argc, char* argv[], string & infile, bool & binmode);
GridP CreateGrid(ProblemSpecP ups);
bool ReadImage(const char* szfile, int nsize, byte* pb);

inline Point CreatePoint(int n, vector<int>& res, double dx, double dy, double dz)
{
  int k = n / (res[0]*res[1]); n -= k* res[0]*res[1];
  int j = n / res[0];
  int i = n % res[0];

  return Point( dx*((double)i + 0.5), dy*(((double)(res[1]-1-j)) + 0.5),dz*((double)k + 0.5));
//  return Point( dx*((double)i + 0.5), dy*(((double)(j)) + 0.5),dz*((double)k + 0.5));
}

//-----------------------------------------------------------------------------------------
/*
pfs2 is used in conjunction with particle geometries derived by 
thresholding image data. (pfs=Particle File Splitter)  
Given an raw file pfs2 reads an input file which contains an "image"
geometry piece description, and it also reads in the data in the raw file
and the intensity range associated with that
geometry.  pfs2 then creates a separate file for each patch, and places
in that file those points which lie within that patch.  These individual 
files may be ASCII or binary.  All of this is 
done to prevent numerous procs from trying to access the same file, which is
hard on file systems.

Can create a cylinder out of the image data given the coordinates of the
bottom and top, and the radius of the cylinder. The cylinder must be inside
the image data. Note that the bounding box is not changed.
*/
// function main : main entry point of application
//
int main(int argc, char *argv[])
{
  try {
    // Do some Uintah initialization
    Uintah::Parallel::determineIfRunningUnderMPI( argc, argv );
    Uintah::Parallel::initializeManager( argc, argv, "" );

    string infile;
    bool binmode = false;
    bool do_cylinder = false;
    CylinderGeometryPiece* cylinder;

    //default cylinder
    double xb = 0.0, yb = 0.0, zb = 0.0; //coordinates of the bottom
    double xt = 1.0, yt = 1.0, zt = 1.0; //coordinates of the top
    double radius = 1.0;                 //radius of the cylinder

    // parse the command arguments
    for (int i=1; i<argc; i++){
        string s=argv[i];
        if (s == "-b" || s == "-B") {
          binmode = true;
        } else if (s == "-cyl") {
            do_cylinder = true;
            xb = atof(argv[++i]);
            yb = atof(argv[++i]);
            zb = atof(argv[++i]);
            xt = atof(argv[++i]);
            yt = atof(argv[++i]);
            zt = atof(argv[++i]);
            radius = atof(argv[++i]);
        }
    }
    
    infile = argv[argc-1];

    if( argc < 2 || argc > 11 ) usage( argv[0] );

    if (do_cylinder) {
        try {
            Point bottom(xb,yb,zb);
            Point top(xt,yt,zt);
            cylinder = scinew CylinderGeometryPiece(top, bottom, radius);
            fprintf(stderr, "Cylinder height, volume: %g, %g\n", cylinder->height(), cylinder->volume());
        } catch (Exception& e) {
            cerr << "Caught exception: " << e.message() << endl;
            abort();
        } catch(...){
            cerr << "Caught unknown exception\n";
            abort();
        }
    }

    // Get the problem specification
    ProblemSpecP ups = ProblemSpecReader().readInputFile( infile );

    if( !ups ) {
      throw ProblemSetupException("Cannot read problem specification", __FILE__, __LINE__);
    }
    
    if( ups->getNodeName() != "Uintah_specification" ) {
      throw ProblemSetupException("Input file is not a Uintah specification", __FILE__, __LINE__);
    }

    // Create the grid
    GridP grid = CreateGrid(ups);

    // repeat for all grid levels
    for (int l = 0; l < grid->numLevels(); l++) {
      LevelP level = grid->getLevel(l);

      // calculate voxel size
      Vector DX = level->dCell();

      fprintf(stderr, "Voxel size : %g, %g, %g\n", DX.x(), DX.y(), DX.z());      

      // make sure the grid level is one that we can handle
      IntVector low, high;
      level->findCellIndexRange(low, high);
      IntVector diff = high-low;
      long cells = diff.x()*diff.y()*diff.z();
      if(cells != level->totalCells())
        throw ProblemSetupException("pfs can only handle square grids", __FILE__, __LINE__);

      // Parse the geometry from the UPS
      ProblemSpecP mp = ups->findBlock("MaterialProperties");
      ProblemSpecP mpm = mp->findBlock("MPM");
      for (ProblemSpecP child = mpm->findBlock("material"); child != 0;
                        child = child->findNextBlock("material")) {
        for (ProblemSpecP geom_obj_ps = child->findBlock("geom_object");
          geom_obj_ps != 0;
          geom_obj_ps = geom_obj_ps->findNextBlock("geom_object") ) {

          fprintf(stderr, "\n--- Reading geometry object --- \n");

          string imgname;  // raw image file name
          vector<int> res; // image resolution
          vector<int> ppc; // nr particles per cell
          vector<int> L;   // lower and upper threshold
          string f_name;   // the base name of the output file
          string of_name;  // actual output file name
          int ncols = 0;   // nr. of additional data columns
          int ncheck = 0;  // check to make sure we have a "image" and a "file" section

          // read the particles per cell
          geom_obj_ps->require("res", ppc);

          fprintf(stderr, "ppc = %d, %d, %d\n", ppc[0], ppc[1], ppc[2]);
          double dx = DX.x() / ppc[0];
          double dy = DX.y() / ppc[1];
          double dz = DX.z() / ppc[2];
          fprintf(stderr, "Voxel dimensions : %g, %g, %g\n", dx, dy, dz);

          for(ProblemSpecP child = geom_obj_ps->findBlock(); child != 0;
                           child = child->findNextBlock()){
            std::string go_type = child->getNodeName();

          // get the image data
          if (go_type == "image") {
              child->require("name", imgname);
              cout << "Image name : " << imgname << endl;
              child->require("res", res);
              cout << "Resolution : " << res[0] << ", " << res[1] << ", " << res[2] << endl;
              child->require("threshold", L);
              cout << "Min threshold : " << L[0] << endl;
              cout << "Max threshold : " << L[1] << endl;
              ncheck++;
          }
            
            // Read the output file data
            if (go_type == "file"){
              child->require("name",f_name);
              // count number of vars, and their sizes
              for(ProblemSpecP varblock = child->findBlock("var");
                  varblock;varblock=varblock->findNextBlock("var")) {
                string next_var_name("");
                varblock->get(next_var_name);
                if      (next_var_name=="p.volume")        ncols += 1;
                else if (next_var_name=="p.temperature")   ncols += 1;
                else if (next_var_name=="p.color")         ncols += 1;
                else if (next_var_name=="p.externalforce") ncols += 3;
                else if (next_var_name=="p.fiberdir")      ncols += 3;
                else 
                  throw ProblemSetupException("Unexpected field variable of '"+next_var_name+"'", __FILE__, __LINE__);
              }
              cout << "Output file name : " << f_name << endl;
              cout << "Nr of additional columns :" << ncols << endl;
              ncheck++;
            }
          }

          // only do the following if we found an image section and a file section
          if (ncheck != 2) {
            fprintf(stderr, "  ...Skipping\n");
            continue;
          }

          // read the image data
          int nsize = res[0]*res[1]*res[2];
          cout << "Reading " << nsize << " bytes\n";
          byte* pimg = scinew byte[nsize];
          if (ReadImage(imgname.c_str(), nsize, pimg) == false) {
            cout << "FATAL ERROR : Failed reading image data" << endl;
            exit(0);
          }

          // these points define the extremas of the grid
          Point min(1.e30,1.e30,1.e30),max(-1.e30,-1.e30,-1.e30);


          // create the points
          // It was noted that the original algorithm was using
          // a lot of memory. To reduce memory we don't store the
          // actual points anymore but an index that can be used
          // to recreate the points
          int npatches = level->numPatches();
          //vector< vector<Point> > points(npatches);
          vector< vector<int> > points(npatches);
          vector<int> sizes(npatches);

          int i, j, k, n;
          Point pt;
          byte* pb = pimg;

          // first determine the nr of points for each patch
          for (i=0; i<npatches; i++) sizes[i] = 0;

          const Patch* currentpatch;
          n = 0;
          for (k=0; k<res[2]; k++) {
            for (j=0; j<res[1]; j++) {
              for (i=0; i<res[0]; i++, pb++, n++) {
                if ((*pb >= L[0]) && (*pb <= L[1])) {
                  pt = CreatePoint(n, res, dx, dy, dz);
                  currentpatch = level->selectPatchForCellIndex(level->getCellIndex(pt));
                  int pid = currentpatch->getID();
                  sizes[pid]++;
                }
              }
            }
            fprintf(stderr, "%s : %.1f\n", "Preprocessing ", 50.0*(k+1.0)/(double)res[2]);
          }

          // allocate storage for the patches
          for (i=0; i<npatches; i++) { points[i].resize(sizes[i]); sizes[i] = 0; }

          // put the points in the correct patches
          pb = pimg;

          n = 0;
          for (k=0; k<res[2]; k++) {
            for (j=0; j<res[1]; j++) {
              for (i=0; i<res[0]; i++, pb++, n++) {
                if ((*pb >= L[0])  && (*pb <= L[1])) {

                  pt = CreatePoint(n, res, dx, dy, dz);

                 const Patch* currentpatch =
                                      level->selectPatchForCellIndex(level->getCellIndex(pt));
                  int pid = currentpatch->getID();
                  min = Min(pt,min);
                  max = Max(pt,max);
                  points[pid][ sizes[pid] ] = n;
                  sizes[pid]++;
                }
              }
            }
            fprintf(stderr, "%s : %.1f\r", "Preprocessing ", 50+50.0*(k+1.0)/(double)res[2]);
          }

          // clean up image data
          delete [] pimg;

          // loop over all patches
          for(Level::const_patchIterator iter = level->patchesBegin();
              iter != level->patchesEnd(); iter++){
            const Patch* patch = *iter;
            int pid = patch->getID();

            char fnum[5];
            sprintf(fnum,".%d",pid);
            of_name = f_name+fnum;
            fprintf(stderr, "Writing %s   \r", of_name.c_str());

            // ADB: change this to always be 128 bytes, so that we can 
            // cleanly read the header off a binary file
            FILE* dest = fopen(of_name.c_str(), "wb");
            if(dest==0){
              cout << "FATAL ERROR : Failed opening points file" << endl;
              exit(0);
            }
            double x[6];
            x[0] = min.x(), x[1] = min.y(), x[2] = min.z();
            x[3] = max.x(), x[4] = max.y(), x[5] = max.z();
            if(binmode) {
              fwrite(x, sizeof(double),6,dest);
            } else {
              fprintf(dest, "%g %g %g %g %g %g\n", x[0],x[1],x[2],x[3],x[4],x[5]);
            }
            for (int I = 0; I < sizes[pid]; I++) {
                  pt = CreatePoint(points[pid][I], res, dx, dy, dz);
                  x[0] = pt.x();
                  x[1] = pt.y();
                  x[2] = pt.z();
                  
                  //points outside the cylinder are ignored.
                  if (do_cylinder && !cylinder->inside(pt)) continue;

                  // FIXME: should have way of specifying endiness
                  if(binmode) {
                    fwrite(x, sizeof(double), 3, dest);
                  } else {
                    fprintf(dest, "%g %g %g\n", x[0], x[1], x[2]);
                  }
            }
            fclose(dest);
          } // loop over patches
        } // loop over geometry objects
      } // loop over materials
    } // loop over grid levels
    
  } catch (Exception& e) {
    cerr << "Caught exception: " << e.message() << '\n';
    if(e.stackTrace())
      cerr << "Stack trace: " << e.stackTrace() << '\n';
  } catch(...){
    cerr << "Caught unknown exception\n";
  }
}

//--------------------------------------------------------------------------------------
// function CreateGrid : creates a grid object from the ProblemSpec
//
GridP CreateGrid(ProblemSpecP ups)
{
    // Setup the initial grid
    GridP grid=scinew Grid();
    IntVector extraCells(0,0,0);

    // save and remove the extra cells before the problem setup
    ProblemSpecP g = ups->findBlock("Grid");
    for( ProblemSpecP levelspec = g->findBlock("Level"); levelspec != 0;
         levelspec = levelspec->findNextBlock("Level")) {
      for (ProblemSpecP box = levelspec->findBlock("Box"); box != 0 ; 
           box = box->findNextBlock("Box")) {
        
        ProblemSpecP cells = box->findBlock("extraCells");
        if (cells != 0) {
          box->get("extraCells", extraCells);
          box->removeChild(cells);
        }
      }
    }
    const ProcessorGroup* world = Uintah::Parallel::getRootProcessorGroup();

    grid->problemSetup(ups, world, false);  

    return grid;
}

//------------------------------------------------------------------------------------------------
// function usage : prints a message on how to use the program
//
void usage( char *prog_name )
{
  cout << "Usage: " << prog_name << " [-b] [-B] [-cyl <args>] infile \n";
  cout << "-b,B: binary output \n";
  cout << "-cyl: defines a cylinder within the geometry \n";
  cout << "args = xbot ybot zbot xtop ytop ztop radius \n";
  exit( 1 );
}

//-----------------------------------------------------------------------------------------------
// function ReadImage : Reads the image data from file and stores it in a buffer
//
bool ReadImage(const char* szfile, int nsize, byte* pb)
{
  FILE* fp = fopen(szfile, "rb");
  if (fp == 0) return false;

  int nread = fread(pb, sizeof(byte), nsize, fp);
  fclose(fp);

  return (nread == nsize);  
}
