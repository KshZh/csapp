# Makefile for the CS:APP Shell Lab

TEAM = NOBODY
VERSION = 1
HANDINDIR = /afs/cs/academic/class/15213-f02/L5/handin
DRIVER = ./sdriver.pl
TSH = ./tsh
TSHREF = ./tshref
TSHARGS = "-p"
CC = gcc
CFLAGS = -Wall -O2
FILES = $(TSH) ./myspin ./mysplit ./mystop ./myint

all: $(FILES)

##################
# Handin your work
##################
handin:
	cp tsh.c $(HANDINDIR)/$(TEAM)-$(VERSION)-tsh.c

tsh: tsh.o
	$(CC) $(CFLAGS) $^ -o tsh

myspin: myspin.o
	$(CC) $(CFLAGS) $^ -o myspin

mysplit: mysplit.o
	$(CC) $(CFLAGS) $^ -o mysplit

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

##################
# Regression tests
##################

# Run tests using the student's shell program
test01:
	$(DRIVER) -v -t trace01.txt -s $(TSH) -a $(TSHARGS)
test02:
	$(DRIVER) -v -t trace02.txt -s $(TSH) -a $(TSHARGS)
test03:
	$(DRIVER) -v -t trace03.txt -s $(TSH) -a $(TSHARGS)
test04:
	$(DRIVER) -v -t trace04.txt -s $(TSH) -a $(TSHARGS)
test05:
	$(DRIVER) -v -t trace05.txt -s $(TSH) -a $(TSHARGS)
test06:
	$(DRIVER) -v -t trace06.txt -s $(TSH) -a $(TSHARGS)
test07:
	$(DRIVER) -v -t trace07.txt -s $(TSH) -a $(TSHARGS)
test08:
	$(DRIVER) -v -t trace08.txt -s $(TSH) -a $(TSHARGS)
test09:
	$(DRIVER) -v -t trace09.txt -s $(TSH) -a $(TSHARGS)
test10:
	$(DRIVER) -v -t trace10.txt -s $(TSH) -a $(TSHARGS)
test11:
	$(DRIVER) -v -t trace11.txt -s $(TSH) -a $(TSHARGS)
test12:
	$(DRIVER) -v -t trace12.txt -s $(TSH) -a $(TSHARGS)
test13:
	$(DRIVER) -v -t trace13.txt -s $(TSH) -a $(TSHARGS)
test14:
	$(DRIVER) -v -t trace14.txt -s $(TSH) -a $(TSHARGS)
test15:
	$(DRIVER) -v -t trace15.txt -s $(TSH) -a $(TSHARGS)
test16:
	$(DRIVER) -v -t trace16.txt -s $(TSH) -a $(TSHARGS)

# Run the tests us-v ing the reference shell program
rtest01:
	$(DRIVER) -v -t trace01.txt -s $(TSHREF) -a $(TSHARGS)
rtest02:
	$(DRIVER) -v -t trace02.txt -s $(TSHREF) -a $(TSHARGS)
rtest03:
	$(DRIVER) -v -t trace03.txt -s $(TSHREF) -a $(TSHARGS)
rtest04:
	$(DRIVER) -v -t trace04.txt -s $(TSHREF) -a $(TSHARGS)
rtest05:
	$(DRIVER) -v -t trace05.txt -s $(TSHREF) -a $(TSHARGS)
rtest06:
	$(DRIVER) -v -t trace06.txt -s $(TSHREF) -a $(TSHARGS)
rtest07:
	$(DRIVER) -v -t trace07.txt -s $(TSHREF) -a $(TSHARGS)
rtest08:
	$(DRIVER) -v -t trace08.txt -s $(TSHREF) -a $(TSHARGS)
rtest09:
	$(DRIVER) -v -t trace09.txt -s $(TSHREF) -a $(TSHARGS)
rtest10:
	$(DRIVER) -v -t trace10.txt -s $(TSHREF) -a $(TSHARGS)
rtest11:
	$(DRIVER) -v -t trace11.txt -s $(TSHREF) -a $(TSHARGS)
rtest12:
	$(DRIVER) -v -t trace12.txt -s $(TSHREF) -a $(TSHARGS)
rtest13:
	$(DRIVER) -v -t trace13.txt -s $(TSHREF) -a $(TSHARGS)
rtest14:
	$(DRIVER) -v -t trace14.txt -s $(TSHREF) -a $(TSHARGS)
rtest15:
	$(DRIVER) -v -t trace15.txt -s $(TSHREF) -a $(TSHARGS)
rtest16:
	$(DRIVER) -v -t trace16.txt -s $(TSHREF) -a $(TSHARGS)


# clean up
clean:
	rm -f $(FILES) *.o *~


