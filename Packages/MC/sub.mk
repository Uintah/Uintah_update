include $(SCIRUN_SCRIPTS)/largeso_prologue.mk

SRCDIR := Packages/MC

SUBDIRS := \
        $(SRCDIR)/Core \
        $(SRCDIR)/Dataflow \

include $(SCIRUN_SCRIPTS)/recurse.mk

PSELIBS := 
LIBS := $(TK_LIBRARY) $(GL_LIBS) -lm

include $(SCIRUN_SCRIPTS)/largeso_epilogue.mk


