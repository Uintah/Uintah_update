#
# Makefile fragment for this subdirectory
#

# *** NOTE ***
# 
# Do not remove or modify the comment line:
#
# #[INSERT NEW ?????? HERE]
#
# It is required by the module maker to properly edit this file.
# if you want to edit this file by hand, see the "Create A New Module"
# documentation on how to do it correctly.

include $(SRCTOP)/scripts/smallso_prologue.mk

SRCDIR   := PSECommon/Modules/Writers

SRCS     += \
	$(SRCDIR)/ColorMapWriter.cc\
	$(SRCDIR)/ColumnMatrixWriter.cc\
	$(SRCDIR)/GeometryWriter.cc\
	$(SRCDIR)/MatrixWriter.cc\
	$(SRCDIR)/MeshWriter.cc\
	$(SRCDIR)/MultiScalarFieldWriter.cc\
	$(SRCDIR)/ScalarFieldWriter.cc\
	$(SRCDIR)/SurfaceWriter.cc\
	$(SRCDIR)/TetraWriter.cc\
	$(SRCDIR)/VectorFieldWriter.cc\
	$(SRCDIR)/VoidStarWriter.cc\
	$(SRCDIR)/PathWriter.cc\
#[INSERT NEW CODE FILE HERE]

PSELIBS := PSECore/Dataflow PSECore/Datatypes SCICore/Persistent \
	SCICore/Thread SCICore/Exceptions SCICore/TclInterface \
	SCICore/Geom SCICore/Containers SCICore/Datatypes
LIBS := 

include $(SRCTOP)/scripts/smallso_epilogue.mk
