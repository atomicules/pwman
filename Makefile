# PWman - An obsolete password manager for the console

include config.mk

SRC_PWMAN = actions.c filter.c gnupg.c gnupg.h help.h launch.c misc.c options.c pwgen.c pwlist.c pwman.c pwman.h search.c ui.c ui.h uilist.c
SRC_CONVERT_PWDB = convert_pwdb.c
SRC_PWDB2CSV = pwdb2csv.c
# This one isn't currently used: import_export.c
OBJ_PWMAN = ${SRC_PWMAN:.c=.o}
OBJ_CONVERT_PWDB = ${SRC_CONVERT_PWDB:.c=.o}
OBJ_PWDB2CSV = ${SRC_PWDB2CSV:.c=.o}

all: options pwman convert_pwdb pwdb2csv

options:
	@echo pwman build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ_PWMAN}: config.h config.mk 
${OBJ_CONVERT_PWDB}: config.h config.mk 
${OBJ_PWDB2CSV}: config.h config.mk 

pwman: ${OBJ_PWMAN}
	@echo CC -o $@
	#Needs CFLAGS as well for libxml2
	@${CC} -o $@ ${OBJ_PWMAN} ${LDFLAGS} ${CFLAGS}

convert_pwdb: ${OBJ_CONVERT_PWDB}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ_CONVERT_PWDB} ${LDFLAGS}

pwdb2csv: ${OBJ_PWDB2CSV}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ_PWDB2CSV} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f pwman convert_pwdb pwdb2csv *.o

install: all
	@echo installing executable files to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f pwman ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/pwman
	@cp -f convert_pwdb ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/convert_pwdb
	@cp -f pwdb2csv ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/pwdb2csv
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < pwman.1 > ${DESTDIR}${MANPREFIX}/man1/pwman.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/pwman.1

uninstall:
	@echo removing executable files from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/pwman
	@rm -f ${DESTDIR}${PREFIX}/bin/convert_pwdb
	@rm -f ${DESTDIR}${PREFIX}/bin/pwdb2csv
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/pwman.1

.PHONY: all options clean install uninstall
