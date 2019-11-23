# nRF application makefile
PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52832
SDK_VERSION = 15
SOFTDEVICE_MODEL = s132

# Path to application library directory
# NOTE: Source files are assumed to be one subdirectory deep
# i.e. lib/<feature>/<feature>.c
APP_LIB = ../../lib/

# Source and header files
APP_HEADER_PATHS += . $(wildcard $(APP_LIB)/*/)
APP_SOURCE_PATHS += . $(wildcard $(APP_LIB)/*/)
APP_SOURCES = $(notdir $(wildcard ./*.c))
APP_SOURCES += $(notdir $(wildcard $(APP_LIB)/*/*.c))

# Path to base of nRF52-base repo
NRF_BASE_DIR = ../../buckler/software/nrf52x-base/

# Include board Makefile (if any)
include ../../buckler/software/boards/buckler_revB/Board.mk

# Include main Makefile
include $(NRF_BASE_DIR)make/AppMakefile.mk
