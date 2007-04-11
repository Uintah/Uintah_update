#ifndef UINTAH_MPM_ARCHES_HEATFLUXBC_H
#define UINTAH_MPM_ARCHES_HEATFLUXBC_H

#include <Packages/Uintah/CCA/Components/MPM/PhysicalBC/MPMPhysicalBC.h>
#include <Packages/Uintah/CCA/Components/MPM/PhysicalBC/LoadCurve.h>
#include <Packages/Uintah/CCA/Components/MPM/PhysicalBC/PolynomialData.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/Point.h>
#include <Packages/Uintah/Core/ProblemSpec/ProblemSpecP.h>
#include <sgi_stl_warnings_off.h>
#include <iosfwd>
#include <sgi_stl_warnings_on.h>

namespace Uintah {

using namespace SCIRun;

class GeometryPiece;
class ParticleCreator;
   
/**************************************

CLASS
   ArchesHeatFluxBC
   
   HeatFlux Boundary Conditions for MPM
 
GENERAL INFORMATION

   ArchesHeatFluxBC.h

   Biswajit Banerjee
   Department of Mechanical Engineering, University of Utah
   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
   Copyright (C) 2003 University of Utah

KEYWORDS
   ArchesHeatFluxBC

DESCRIPTION
   Stores the heatflux load curves and boundary imformation for
   heatflux boundary conditions that can be applied to surfaces
   with simple geometry -  planes, cylinders and spheres.

WARNING
  
****************************************/

   class ArchesHeatFluxBC : public MPMPhysicalBC  {

   public:

      // Construct a ArchesHeatFluxBC object that contains
      // the area over which heatflux is to be applied
      // and the value of that heatflux (in the form
      // of a load curve)
      ArchesHeatFluxBC(ProblemSpecP& ps);
      ~ArchesHeatFluxBC();
      virtual std::string getType() const;
      virtual void outputProblemSpec(ProblemSpecP& ps);

      // Locate and flag the material points to which this heatflux BC is
      // to be applied. 
       bool flagMaterialPoint(const Point& p, const Vector& dxpp) const;
      
      // Get the load curve number for this heatflux BC
       inline int loadCurveID() const {return d_loadCurve->getID();}

      // Get the surface 
      inline GeometryPiece* getSurface() const {return d_surface;}

      // Get the surface type
       inline std::string getSurfaceType() const {return d_surfaceType;}

      // Set the number of material points on the surface
      inline void numMaterialPoints(long num) {d_numMaterialPoints = num;}

      // Get the number of material points on the surface
      inline long numMaterialPoints() const {return d_numMaterialPoints;}

      // Get the area of the surface
      double getSurfaceArea() const;

      // Get the load curve 
      inline LoadCurve<double>* getLoadCurve() const {return d_loadCurve;}

      // Get the applied heatflux at time t
      inline double heatflux(double t) const {return d_loadCurve->getLoad(t);}

      // Get the flux per particle at time t
      double fluxPerParticle(double time) const;

      // Get the flux vector to be applied at a point 
      virtual double getFlux(const Point& px, double forcePerParticle) const;

   private:

      // Prevent empty constructor
      ArchesHeatFluxBC();

      // Prevent copying
      ArchesHeatFluxBC(const ArchesHeatFluxBC&);
      ArchesHeatFluxBC& operator=(const ArchesHeatFluxBC&);
      
      // Private Data
      // Surface information
      GeometryPiece* d_surface;
      std::string d_surfaceType;
      long d_numMaterialPoints;

      // Load curve information (HeatFlux and time)
      LoadCurve<double>* d_loadCurve;

      PolynomialData* d_polyData;

      friend std::ostream& operator<<(std::ostream& out, const Uintah::ArchesHeatFluxBC& bc);
   };
} // End namespace Uintah

#endif
