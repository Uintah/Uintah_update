
# Makefile fragment for this subdirectory

SRCDIR := Packages/Uintah/testprograms/BNRRegridder

PROGRAM := $(SRCDIR)/bnrtest
SRCS    := $(SRCDIR)/bnrtest.cc

PSELIBS := \
        Core/Exceptions                          \
        Core/Geometry                            \
        Core/Thread                              \
        Core/Util                                \
        Packages/Uintah/Core/Disclosure          \
        Packages/Uintah/Core/Exceptions          \
        Packages/Uintah/Core/Grid                \
        Packages/Uintah/Core/Parallel            \
        Packages/Uintah/Core/Util                \
        Packages/Uintah/CCA/Ports                \
        Packages/Uintah/CCA/Components/Regridder

LIBS := $(M_LIBRARY) $(MPI_LIBRARY) $(BLAS_LIBRARY) $(LAPACK_LIBRARY) $(THREAD_LIBRARY)

include $(SCIRUN_SCRIPTS)/program.mk

