# Makefile fragment for this subdirectory

include $(SCIRUN_SCRIPTS)/smallso_prologue.mk

SRCDIR   := Packages/Uintah/Dataflow/Modules/Readers

SRCS     += \
	$(SRCDIR)/ArchiveReader.cc\
#[INSERT NEW CODE FILE HERE]

PSELIBS := \
	Packages/Uintah/Core/Datatypes \
	Packages/Uintah/CCA/Ports \
	Dataflow/Network \
	Dataflow/Ports \
	Core/Containers \
	Core/Persistent \
	Core/Exceptions \
	Core/GuiInterface \
	Core/Thread \
	Core/Datatypes \
	Core/Geom
LIBS := $(XML_LIBRARY)

include $(SCIRUN_SCRIPTS)/smallso_epilogue.mk

