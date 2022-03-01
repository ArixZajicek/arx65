# C++ Compiler
CC=g++

# Important direcories, relative to build folder
SRCDIR = ../src
INCDIR = ../include
OBJDIR = obj
DEPDIR = dep

# Main Target object & name
TARGET = arx65

# Compile flags
CLIBS=-lSDL2 -lSDL2_image -lglog
CFLAGS=$(CLIBS) -I$(INCDIR) -O2

# All Files we need
SRCS=$(wildcard $(SRCDIR)/*.cpp) $(wildcard $(SRCDIR)/*/*.cpp)
OBJS=$(subst $(SRCDIR),$(OBJDIR),$(SRCS:.cpp=.o))
DEPS=$(subst $(SRCDIR),$(DEPDIR),$(SRCS:.cpp=.d))

# Main output
$(TARGET): $(OBJS)
	@echo Building final target $(TARGET)...
	@$(CC) -o $@ $^ $(CFLAGS)
	@echo Done.

# Generate the dependency files using the C/C++ preprocessor -M arguments
$(DEPS): $(subst $(DEPDIR),$(SRCDIR),$(@:.d=.cpp))
	@echo Generating dependency file '$@'...
	@mkdir -p $(@D)
	@cpp $(CFLAGS) $(subst $(DEPDIR),$(SRCDIR),$(@:.d=.cpp)) -MM -MT $(subst $(DEPDIR),$(OBJDIR),$(@:.d=.o)) >$@
	@echo '	@echo Building $$@...' >> $@
	@echo '	@mkdir -p $$(@D)' >> $@
	@echo '	@$$(CC) -c -o $$@ $$(subst $$(OBJDIR),$$(SRCDIR),$$(@:.o=.cpp)) $$(CFLAGS)' >> $@

# Include these new dependency files
-include $(DEPS)

.PHONY: lib
lib:
	@echo Creating lib/$(TARGET).a
	@mkdir -p ../lib
	@rm -f ../lib/$(TARGET).a
	@ar rvs ../lib/$(TARGET).a $(TARGET)
	@echo Done!

.PHONY: clean 
clean:
	@echo Removing $(OBJDIR), $(DEPDIR), and $(TARGET)...
	@rm -fr $(OBJDIR)
	@rm -fr $(DEPDIR)
	@rm -f $(TARGET)