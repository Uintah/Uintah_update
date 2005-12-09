# Makefile fragment for this subdirectory

include $(SRCTOP)/scripts/smallso_prologue.mk

SRCDIR   := Packages/Uintah/Dataflow/Modules/Operators

SRCS     += \
	$(SRCDIR)/EigenEvaluator.cc \
	$(SRCDIR)/ParticleEigenEvaluator.cc \
	$(SRCDIR)/ScalarFieldAverage.cc \
	$(SRCDIR)/ScalarFieldBinaryOperator.cc \
	$(SRCDIR)/ScalarFieldNormalize.cc \
	$(SRCDIR)/ScalarFieldOperator.cc \
	$(SRCDIR)/ScalarMinMax.cc \
	$(SRCDIR)/Schlieren.cc \
	$(SRCDIR)/TensorFieldOperator.cc \
	$(SRCDIR)/TensorParticlesOperator.cc \
	$(SRCDIR)/TensorToTensorConvertor.cc \
	$(SRCDIR)/VectorFieldOperator.cc \
	$(SRCDIR)/VectorParticlesOperator.cc \
[INSERT NEW CODE FILE HERE]

PSELIBS := \
	Packages/Uintah/Core/Datatypes     \
	Packages/Uintah/Core/DataArchive     \
	Packages/Uintah/Core/Disclosure     \
	Packages/Uintah/CCA/Ports          \
	Packages/Uintah/Core/Grid          \
	Packages/Uintah/Core/Math          \
	Packages/Uintah/Core/Util          \
	Packages/Uintah/Core/ProblemSpec   \
	Packages/Uintah/Core/Exceptions    \
	Dataflow/Network  \
	Dataflow/Ports    \
	Core/Basis        \
	Core/Containers   \
	Core/Persistent   \
	Core/Exceptions   \
	Core/GuiInterface \
	Core/Thread       \
	Core/Datatypes    \
	Core/Geom         \
	Core/Geometry     \
	Core/GeomInterface \
	Core/Malloc \
	Core/Util \

LIBS := $(XML2_LIBRARY) $(M_LIBRARY) $(MPI_LIBRARY)

include $(SRCTOP)/scripts/smallso_epilogue.mk

ifeq ($(LARGESOS),no)
UINTAH_SCIRUN := $(UINTAH_SCIRUN) $(LIBNAME)
endif

