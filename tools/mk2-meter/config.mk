##############################################################################
# Project Configuration
#

PROJECT := HVmeter
PROJECT_TYPE := osc

##############################################################################
# Sources
#

CSRC = header.c
CXXSRC = unit.cc
ACSRC =
ACXXSRC =
ASMSRC =
ASMXSRC =

##############################################################################
# Include Paths
#

# Shared DSP engine
UINCDIR = $(PROJECT_ROOT)/../../core

##############################################################################
# Library Paths
#

ULIBDIR =

##############################################################################
# Libraries
#

ULIBS  = -lm
ULIBS += -lc

##############################################################################
# Macros
#

OPTIM = -O3
UDEFS = -DHOOVER_LOADMETER
