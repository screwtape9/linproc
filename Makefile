CC=g++
CXXFLAGS=-std=c++11 -c -Wall -Wextra -Wshadow -Wredundant-decls -Wunreachable-code -Winline

ifeq ($(DEBUG),1)
CXXFLAGS+=-g
endif

OBJ:=demo.o Process.o
FWOBJ:=fw.o Process.o
EXE=demo
FW=fw

COMPILE.1=$(CC) $(CXXFLAGS) $(INCLUDES) -o $@ $<
ifeq ($(VERBOSE),)
COMPILE=@printf "  > compiling %s\n" $(<F) && $(COMPILE.1)
else
COMPILE=$(COMPILE.1)
endif

%.o: %.cpp
	$(COMPILE)

.PHONY: all clean rebuild

all: $(EXE) $(FW)

$(EXE): $(OBJ)
	$(CC) -o $@ $(OBJ)

$(FW): $(FWOBJ)
	$(CC) -o $@ $(FWOBJ)

clean:
	rm -f $(EXE) $(FW) $(OBJ) $(FWOBJ)

rebuild: clean all
