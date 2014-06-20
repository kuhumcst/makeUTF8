PNAME=makeISO
VERSION=1
MINOR_NUMBER=1

PNAMESTATIC=$(PNAME)s
PNAMEDYNAMIC=$(PNAME)
PNAMEDYNAMICLIB=$(PNAME)dl
LINKERNAME=lib$(PNAME).so
SONAME=$(LINKERNAME).$(VERSION)
REALNAME=$(SONAME).$(MINOR_NUMBER)


SRCDIR     = .
INCLUDEDIR = -I$(SRCDIR) 

CC=g++ $(INCLUDEDIR) -O3 -Wall -pedantic -DNDEBUG 

# -fPIC or -fpic: enable 'position independent code' generation. Necessary for shared libs
# -fpic may generate smaller and faster code, but will have platform-dependent limitations
PIC=-fPIC
#PIC=

CCLINKSTATIC=g++ -static
CCLINKDYNAMIC=g++ 
CCCREATELIB=g++ -shared -Wl,-soname,$(SONAME)

#DEBUG=-g
DEBUG=

GCCLINK=-L/usr/local/lib -lstdc++
RM=rm -f

PROJECTSRC=

CSTPROJECTSRC=main.cpp 

PROJECTOBJS=

CSTPROJECTOBJS=main.o 

#                               #
#   Dependencies start here.    #
#                               #


$(PNAMEDYNAMIC): $(CSTPROJECTOBJS) $(PROJECTOBJS)
	$(CCLINKDYNAMIC) $(CSTPROJECTOBJS) $(PROJECTOBJS) -o $@ $(GCCLINK)

$(PNAMESTATIC): $(CSTPROJECTOBJS) $(PROJECTOBJS)
	$(CCLINKSTATIC) $(CSTPROJECTOBJS) $(PROJECTOBJS) -o $@ $(GCCLINK)

$(REALNAME) : $(PROJECTOBJS)
	$(CCCREATELIB) -o $@ $(PROJECTOBJS)
	ln -sf $(REALNAME) $(SONAME)
	ln -sf $(SONAME) $(LINKERNAME)

$(PNAMEDYNAMICLIB): $(REALNAME) $(CSTPROJECTOBJS)
	$(CCLINKDYNAMIC) $(CSTPROJECTOBJS) -L. -l $(PNAME) -o $@ $(GCCLINK)


all: $(PNAMESTATIC) $(PNAMEDYNAMIC) $(REALNAME) $(PNAMEDYNAMICLIB)

.cpp.o:
	$(CC) $(PIC) $(DEBUG) $(GCCINC) -c $(CSTPROJECTSRC) $(PROJECTSRC)

#.cpp.o:
#	$(CC) $(PIC) $(DEBUG) $(GCCINC) -c $?

clean:
	$(RM) *.o
	$(RM) $(LINKERNAME)
	$(RM) $(SONAME)
	$(RM) $(REALNAME)
