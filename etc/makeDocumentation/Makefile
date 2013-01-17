TOP = ../..
include $(TOP)/configure/CONFIG

# set to -d to get debugging
DEBUG :=

# this is the doxygen output dir
DOCDIR := $(TOP)/documentation/doxygen

# add the documentation files to the install target
install: $(DOCDIR)

# rule for creating the doxygen documentation
$(DOCDIR): config.src manual.src 
	mkdir -p $(DOCDIR)
	dls-make-doxygen-documentation.py -o $(DOCDIR) config.src manual.src

# don't do anything in clean
clean: ;

# Special rule to get rid of installed documentation dir
uninstall:
	$(RMDIR) $(DOCDIR)
