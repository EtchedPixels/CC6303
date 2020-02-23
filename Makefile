
all: cc68 as68 copt frontend

.PHONY: cc68 as68 frontend

cc68:
	+(cd common; make)
	+(cd cc68; make)

as68:
	+(cd as68; make)

copt: copt.c

frontend:
	+(cd frontend; make)

clean:
	(cd common; make clean)
	(cd cc68; make clean)
	(cd as68; make clean)
	(cd frontend; make clean)
	rm copt

#
#	This aspect needs work
#
install:
	mkdir -p /opt/cc68/bin
	mkdir -p /opt/cc68/lib
	cp cc68/cc68 /opt/cc68/lib
	cp as68/as68 /opt/cc68/bin
	cp as68/ld68 /opt/cc68/bin
	cp as68/nm68 /opt/cc68/bin
	cp copt /opt/cc68/lib
	cp frontend/cc68 /opt/cc68/bin/
	cp cc68.rules /opt/cc68/lib
