#
#  The contents of this file are subject to the University of Utah Public
#  License (the "License"); you may not use this file except in compliance
#  with the License.
#  
#  Software distributed under the License is distributed on an "AS IS"
#  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
#  License for the specific language governing rights and limitations under
#  the License.
#  
#  The Original Source Code is SCIRun, released March 12, 2001.
#  
#  The Original Source Code was developed by the University of Utah.
#  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
#  University of Utah. All Rights Reserved.
#

#
# Makefile fragment for this subdirectory
#

SRCDIR   := Core/CCA/tools/strauss
STRAUSS_SRCDIR := $(SRCDIR)

SUBDIRS := \
        $(SRCDIR)/c++ruby

include $(SCIRUN_SCRIPTS)/recurse.mk

GENSRCS := $(SRCDIR)/parser.cc $(SRCDIR)/strauss.h $(SRCDIR)/strauss.cc
SRCS := $(SRCDIR)/main.cc $(SRCDIR)/strauss.cc 

PROGRAM := $(SRCDIR)/strauss
STRAUSS_EXE := $(PROGRAM)
PSELIBS := SCIRun Core/CCA/tools/strauss/c++ruby
LIBS := $(XML_LIBRARY) -L/usr/sci/projects/CCA/Thirdparty/1.0.0/Linux/gcc-3.2-32bit/lib -lruby-static -lcrypt -ldl

include $(SCIRUN_SCRIPTS)/program.mk

$(SRCDIR)/strauss.o: $(SRCDIR)/strauss.cc
	$(CXX)  $(INCLUDES) $(CC_DEPEND_REGEN) -c $< -o $@

$(SRCDIR)/main.o: $(SRCDIR)/main.cc
	$(CXX) $(INCLUDES) $(CC_DEPEND_REGEN) -c $< -o $@


