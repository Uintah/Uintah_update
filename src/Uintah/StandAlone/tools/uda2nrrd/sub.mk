# 
# 
# The MIT License
# 
# Copyright (c) 1997-2009 Center for the Simulation of Accidental Fires and 
# Explosions (CSAFE), and  Scientific Computing and Imaging Institute (SCI), 
# University of Utah.
# 
# License for the specific language governing rights and limitations under
# Permission is hereby granted, free of charge, to any person obtaining a 
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation 
# the rights to use, copy, modify, merge, publish, distribute, sublicense, 
# and/or sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included 
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
# DEALINGS IN THE SOFTWARE.
# 
# 
# 
# 
SRCDIR  := Uintah/StandAlone/tools/uda2nrrd
PROGRAM := $(SRCDIR)/uda2nrrd

#ifeq ($(findstring teem, $(TEEM_LIBRARY)),teem)
  ifeq ($(LARGESOS),yes)
    PSELIBS := Datflow Uintah
  else
    PSELIBS := \
        Uintah/CCA/Components/ProblemSpecification \
        Uintah/CCA/Ports          \
        Uintah/Core/DataArchive   \
        Uintah/Core/Disclosure    \
        Uintah/Core/Exceptions    \
        Uintah/Core/Grid          \
        Uintah/Core/Math          \
        Uintah/Core/Parallel      \
        Uintah/Core/ProblemSpec   \
        Uintah/Core/Util          \
        Core/Basis        \
        Core/Containers   \
        Core/Datatypes    \
        Core/Exceptions   \
        Core/Geometry     \
        Core/Math         \
        Core/Persistent   \
        Core/Thread       \
        Core/Util         

  endif

  SRCS := \
	$(SRCDIR)/bc.cc                  \
	$(SRCDIR)/build.cc               \
	$(SRCDIR)/handleVariable.cc      \
	$(SRCDIR)/uda2nrrd.cc            \
	$(SRCDIR)/update_mesh_handle.cc  \
	$(SRCDIR)/particles.cc           \
	$(SRCDIR)/wrap_nrrd.cc 

  LIBS := $(XML2_LIBRARY) $(TEEM_LIBRARY) $(PNG_LIBRARY) $(Z_LIBRARY) $(M_LIBRARY) $(MPI_LIBRARY) $(F_LIBRARY)

  include $(SCIRUN_SCRIPTS)/program.mk

#endif

uda2nrrd: prereqs Uintah/StandAlone/tools/uda2nrrd/uda2nrrd

