SHELL=/bin/bash

# if you edit the install directory, you may need to update 'install'
INSTDIR=${HOME}/bin/
EXE=stuff

SRC=$(wildcard *.c)
HDR=$(wildcard *.h)

CC=gcc
CFLAGS=-Wall -Werror -g

DEL=rm -f
CP=cp -f
MD=mkdir -p
LS=ls -l --color
CHMOD=chmod

#---------------------------------------
.PHONY: all
all: $(EXE)

#---------------------------------------
$(EXE): clean $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o $(EXE)

#---------------------------------------
.PHONY: clean
clean:
	$(DEL) $(EXE)

#---------------------------------------
.PHONY: install
install: $(EXE)
	@[[ -d ${INSTDIR} ]] || $(MD) ${INSTDIR}
	$(CP) $(EXE) $(INSTDIR)/$(EXE)
	$(CHMOD) 750 $(INSTDIR)/$(EXE)
	$(LS) $(INSTDIR)/$(EXE)

	@[[ ! :${PATH}: =~ :${INSTDIR}: ]] || { \
		echo "Install directory not in path: ${INSTDIR}" ;\
		[[ ${INSTDIR} =~ ^${HOME} ]] && echo "Try logging out and back in." ;\
	}
