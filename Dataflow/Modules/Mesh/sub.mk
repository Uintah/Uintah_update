# Makefile fragment for this subdirectory

# *** NOTE ***
# Do not remove or modify the comment line:
# #[INSERT NEW ?????? HERE]
# It is required by the module maker to properly edit this file.
# if you want to edit this file by hand, see the "Create A New Module"
# documentation on how to do it correctly.

include $(SRCTOP)/scripts/smallso_prologue.mk

SRCDIR   := Dataflow/Modules/Mesh

SRCS     += \
	$(SRCDIR)/Delaunay.cc\
	$(SRCDIR)/ExtractMesh.cc\
	$(SRCDIR)/HexMeshToGeom.cc\
	$(SRCDIR)/InsertDelaunay.cc\
	$(SRCDIR)/MakeScalarField.cc\
	$(SRCDIR)/MeshBoundary.cc\
	$(SRCDIR)/MeshFindSurfNodes.cc\
	$(SRCDIR)/MeshInterpVals.cc\
	$(SRCDIR)/MeshNodeCore/CCA/Component.cc\
	$(SRCDIR)/MeshRender.cc\
	$(SRCDIR)/MeshToGeom.cc\
	$(SRCDIR)/MeshView.cc\
	$(SRCDIR)/TransformMesh.cc
#[INSERT NEW MODULE HERE]

PSELIBS := Dataflow/Network Core/Datatypes Dataflow/Widgets \
	Core/Geom Core/Thread Core/Exceptions \
	Core/Containers Core/Geometry Core/Datatypes \
	Core/Persistent Core/TclInterface Core/Math
LIBS := 

include $(SRCTOP)/scripts/smallso_epilogue.mk
