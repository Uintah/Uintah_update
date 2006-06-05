#include <Packages/Uintah/Core/Grid/Variables/AMRInterpolate.h>
#include <Core/Math/MinMax.h>

using namespace SCIRun;

namespace Uintah {

void getFineLevelRange(const Patch* coarsePatch, const Patch* finePatch,
                       IntVector& cl, IntVector& ch, IntVector& fl, IntVector& fh)
{
  // don't coarsen the extra cells
  fl = finePatch->getInteriorCellLowIndex();
  fh = finePatch->getInteriorCellHighIndex();
  cl = coarsePatch->getCellLowIndex();
  ch = coarsePatch->getCellHighIndex();
  
  fl = Max(fl, coarsePatch->getLevel()->mapCellToFiner(cl));
  fh = Min(fh, coarsePatch->getLevel()->mapCellToFiner(ch));
  cl = finePatch->getLevel()->mapCellToCoarser(fl);
  ch = finePatch->getLevel()->mapCellToCoarser(fh);
}

void getFineLevelRangeNodes(const Patch* coarsePatch, const Patch* finePatch,
                            IntVector& cl, IntVector& ch,
                            IntVector& fl, IntVector& fh,IntVector ghost)
{
  cl = coarsePatch->getNodeLowIndex();
  ch = coarsePatch->getNodeHighIndex();
  fl = coarsePatch->getLevel()->mapNodeToFiner(cl) - ghost;
  fh = coarsePatch->getLevel()->mapNodeToFiner(ch) + ghost;

  fl = Max(fl, finePatch->getInteriorNodeLowIndex());
  fh = Min(fh, finePatch->getInteriorNodeHighIndex());

  cl = Max(cl, finePatch->getLevel()->mapNodeToCoarser(fl));
  ch = Min(ch, finePatch->getLevel()->mapNodeToCoarser(fh));

  if (ch.x() <= cl.x() || ch.y() <= cl.y() || ch.z() <= cl.z()) {
    // the expanded fine region was outside the coarse region, so
    // return an invalid fine region
    fl = fh;
  }

}

void getCoarseLevelRange(const Patch* finePatch, const Level* coarseLevel, 
                         IntVector& cl, IntVector& ch, IntVector& fl, IntVector& fh, int ngc)
{
  finePatch->computeVariableExtents(Patch::CellBased, IntVector(0,0,0), Ghost::AroundCells,ngc, fl, fh); 
  
  // coarse region we need to get from the dw
  cl = finePatch->getLevel()->mapCellToCoarser(fl);
  ch = finePatch->getLevel()->mapCellToCoarser(fh) + IntVector(1,1,1);
  
  //__________________________________
  // coarseHigh and coarseLow cannot lie outside
  // of the coarselevel index range
  IntVector cl_tmp, ch_tmp;
  coarseLevel->findCellIndexRange(cl_tmp,ch_tmp);
  cl = Max(cl_tmp, cl);
  ch = Min(ch_tmp, ch);

  // fine region to work over
  fl = finePatch->getInteriorCellLowIndex();
  fh = finePatch->getInteriorCellHighIndex();
}

void getCoarseFineFaceRange(const Patch* finePatch, const Level* coarseLevel, Patch::FaceType face,
                            int interOrder, IntVector& cl, IntVector& ch, IntVector& fl, IntVector& fh) 
{
    //__________________________________
    // fine level hi & lo cell iter limits
    // coarselevel hi and low index
  const Level* fineLevel = finePatch->getLevel();
  CellIterator iter_tmp = finePatch->getFaceCellIterator(face, "plusEdgeCells");
  fl = iter_tmp.begin();
  fh = iter_tmp.end();
  
  IntVector refineRatio = fineLevel->getRefinementRatio();
  cl  = fineLevel->mapCellToCoarser(fl);
  ch = fineLevel->mapCellToCoarser(fh+refineRatio - IntVector(1,1,1));
  
  //__________________________________
  // enlarge the coarselevel foot print by oneCell
  // x-           x+        y-       y+       z-        z+
  // (-1,0,0)  (1,0,0)  (0,-1,0)  (0,1,0)  (0,0,-1)  (0,0,1)
  IntVector oneCell = finePatch->faceDirection(face);
  if( face == Patch::xminus || face == Patch::yminus 
      || face == Patch::zminus) {
    ch -= oneCell;
  }
  if( face == Patch::xplus || face == Patch::yplus 
      || face == Patch::zplus) {
    cl  -= oneCell;
  }
  
  //__________________________________
  // for higher order interpolation increase the coarse level foot print
  // by the order of interpolation - 1
  if(interOrder >= 1){
    IntVector interRange(1,1,1);
    cl  -= interRange;
    ch += interRange;
  } 
  IntVector crl, crh;
  coarseLevel->findCellIndexRange(crl,crh);
  cl   = Max(cl, crl);
  ch  = Min(ch, crh); 
  
  
}

/*___________________________________________________________________
 Function~  normalizedDistance_CC
 Compute the normalized distance between the fine and
 coarse cell centers.
           |       x   |
           |     |---| |
___________|___________|________ rratio = 2    rratio = 4     rratio = 8
  |  |  |  | 3|  |  |  |         ----------    ----------     ----------
__|__|__|__|__|__|__|__|         x(0) = 1 a    x(0) = 3 a     x(0) =  7a
  |  |  |  | 2|  |  |  |         x(1) = x(0)   x(1) = 1 a     x(1) =  5a
__|__*__|__|__|__*__|__|                       x(2) = x(1)    x(2) =  3a
  |  |  |  | 1|  |  |  |                       x(3) = x(0)    x(3) =  1a
__|__|__|__|__|__|__|__|                                      x(4) = x(3)
  |  |  |  | 0| 1| 2| 3|                                      x(5) = x(2)
__|__|__|__|__|__|__|__|________                              x(6) = x(1)
           |           |                                      x(7) = x(0)
           |           |                    
           |           |                      
* coarse cell centers   
a = normalized_dx_fine_cell/2
 ____________________________________________________________________*/
void normalizedDistance_CC(const int refineRatio,
                           vector<double>& norm_dist)
{  
  // initialize
  for (int i = 0; i< refineRatio; i++){
    norm_dist[i] =0.0;
  }

  if(refineRatio > 1){
  
    int half_refineRatio = refineRatio /2;
    double normalized_dx_fineCell = 1.0/refineRatio;
    double half_normalized_dx_fineCell = 0.5 * normalized_dx_fineCell;
  
    // only compute the distance for 1/2 of the cells
    int count = refineRatio - 1;   // 7, 5, 3...
    for (int i = 0; i< half_refineRatio; i++){
      norm_dist[i] = count * half_normalized_dx_fineCell;
      count -= 2;
    }

    // make a mirror copy of the data
    count = half_refineRatio - 1;
    for (int i = half_refineRatio; i< refineRatio; i++){
      norm_dist[i] = norm_dist[count];
      count -=1;
    }
  }
}


}
