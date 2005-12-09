#
# Makefile fragment for this subdirectory
# $Id$
#
include $(SCIRUN_SCRIPTS)/smallso_prologue.mk

SRCDIR   := Packages/Uintah/CCA/Components/Arches/Radiation

SRCS += \
	$(SRCDIR)/RadiationModel.cc \
        $(SRCDIR)/RadiationSolver.cc \
	$(SRCDIR)/DORadiationModel.cc

ifeq ($(HAVE_PETSC),yes)
  SRCS += $(SRCDIR)/RadLinearSolver.cc
else
  SRCS += $(SRCDIR)/FakeRadLinearSolver.cc
endif

ifeq ($(HAVE_HYPRE),yes)
  SRCS += $(SRCDIR)/RadHypreSolver.cc
endif

PSELIBS := \
	Packages/Uintah/CCA/Components/Arches/Radiation/fortran \
	Packages/Uintah/Core/ProblemSpec   \
	Packages/Uintah/Core/Grid        \
	Packages/Uintah/Core/Util        \
	Packages/Uintah/Core/Disclosure  \
	Packages/Uintah/Core/Exceptions    \
	Packages/Uintah/Core/Math          \
	Core/Exceptions \
	Core/Util \
	Core/Thread     \
	Core/Geometry   

LIBS := $(XML2_LIBRARY) $(MPI_LIBRARY) $(M_LIBRARY) $(FLIBS)

ifneq ($(HAVE_PETSC),)
LIBS := $(LIBS) $(PETSC_LIBRARY) 
endif

ifneq ($(HAVE_HYPRE),)
LIBS := $(LIBS) $(HYPRE_LIBRARY) 
endif

include $(SCIRUN_SCRIPTS)/smallso_epilogue.mk

$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rordr_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rordrss_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rordrtn_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/radarray_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/radcal_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/radcoef_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/radwsgg_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rdombc_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rdomsolve_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rdomsrc_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rdomflux_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rdombmcalc_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rdomvolq_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rshsolve_fort.h
$(SRCDIR)/DORadiationModel.o: $(SRCDIR)/fortran/rshresults_fort.h



