#
#  For more information, please see: http://software.sci.utah.edu
# 
#  The MIT License
# 
#  Copyright (c) 2004 Scientific Computing and Imaging Institute,
#  University of Utah.
# 
#  License for the specific language governing rights and limitations under
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#


# Makefile fragment for this subdirectory

SRCDIR   := SCIRun/Distributed

SRCS     += \
             $(SRCDIR)/ComponentInfo.cc \

#             $(SRCDIR)/BuilderService.cc \
#             $(SRCDIR)/ComponentEvent.cc \
#             $(SRCDIR)/ComponentEventService.cc \
#             $(SRCDIR)/ConnectionEvent.cc \
#             $(SRCDIR)/ConnectionEventService.cc \
#             $(SRCDIR)/ComponentRegistry.cc \
#             $(SRCDIR)/InternalComponentModel.cc \
#             $(SRCDIR)/FrameworkProperties.cc \
#             $(SRCDIR)/FrameworkProxyService.cc \
#             $(SRCDIR)/FrameworkInternalException.cc \
#             $(SRCDIR)/InternalServiceDescription.cc \
#             $(SRCDIR)/InternalFrameworkServiceDescription.cc
#             $(SRCDIR)/InternalServiceInstance.cc \
#             $(SRCDIR)/InternalFrameworkServiceInstance.cc \
#             $(SRCDIR)/InternalComponentInstance.cc \

#$(SRCDIR)/ComponentRegistry.o: Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/ComponentEvent.o: Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/ComponentEventService.o: Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/ConnectionEvent.o: Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/ConnectionEventService.o: Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/BuilderService.o: Core/CCA/spec/sci_sidl.h
##$(SRCDIR)/InternalComponentInstance.o: Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/InternalComponentModel.o: Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/FrameworkProperties.o: Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/FrameworkProxyService.o: Core/CCA/spec/sci_sidl.h
##$(SRCDIR)/InternalServiceInstance.o : Core/CCA/spec/sci_sidl.h
##$(SRCDIR)/InternalFrameworkServiceInstance.o : Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/InternalServiceDescription.o : Core/CCA/spec/sci_sidl.h
#$(SRCDIR)/InternalFrameworkServiceDescription.o : Core/CCA/spec/sci_sidl.h
