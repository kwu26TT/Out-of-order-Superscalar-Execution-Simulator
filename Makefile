CC = g++
OPT = -std=c++11 #-g
#OPT = -g
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

# List all your .cc/.cpp files here (source files, excluding header files)
SIM_SRC = sim_proc.cc, sim_proc.h

# List corresponding compiled object files here (.o files)
SIM_OBJ = sim_proc.o
 
#################################

# default rule

all: sim
	@echo "my work is done here..."


# rule for making sim

sim: $(SIM_OBJ)
	$(CC) -o sim $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH sim-----------"


# generic rule for converting any .cpp file to any .o file
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc

.cpp.o:
	$(CC) $(CFLAGS)  -c $*.cpp


# type "make clean" to remove all .o files plus the sim binary

clean:
	rm -f *.o sim


# type "make clobber" to remove all .o files (leaves sim binary)

clobber:
	rm -f *.o


run1: clean sim
	./sim 16 8 1 val_trace_gcc1

run2: clean sim
	./sim 16 8 2 val_trace_gcc1

run3: clean sim
	./sim 60 15 3 val_trace_gcc1

run4: clean sim
	./sim 64 16 8 val_trace_gcc1

run5: clean sim
	./sim 64 16 4 val_trace_perl1

run6: clean sim
	./sim 128 16 5 val_trace_perl1

run7: clean sim
	./sim 256 64 5 val_trace_perl1

run8: clean sim
	./sim 512 64 7 val_trace_perl1

val1: clean sim
	./sim 16 8 1 val_trace_gcc1 > ../exp/val1.txt
diff1:
	diff -iw ../exp/val1.txt ../validation/val1.txt
diff1p:
	diff -iw ../exp/val1.txt ../validation/val1.txt > ../exp/diff1.txt



val2: clean sim
	./sim 16 8 2 val_trace_gcc1 > ../exp/val2.txt
diff2:
	diff -iw ../exp/val2.txt ../validation/val2.txt
diff2p:
	diff -iw ../exp/val2.txt ../validation/val2.txt > ../exp/diff2.txt

val3: clean sim
	./sim 60 15 3 val_trace_gcc1 > ../exp/val3.txt
diff3:
	diff -iw ../exp/val3.txt ../validation/val3.txt
diff3p:
	diff -iw ../exp/val3.txt ../validation/val3.txt > ../exp/diff3.txt

val4: clean sim
	./sim 64 16 8 val_trace_gcc1 > ../exp/val4.txt
diff4:
	diff -iw ../exp/val4.txt ../validation/val4.txt
diff4p:
	diff -iw ../exp/val4.txt ../validation/val4.txt > ../exp/diff4.txt

val5: clean sim
	./sim 64 16 4 val_trace_perl1 > ../exp/val5.txt
diff5:
	diff -iw ../exp/val5.txt ../validation/val5.txt
diff5p:
	diff -iw ../exp/val5.txt ../validation/val5.txt > ../exp/diff5.txt

val6: clean sim
	./sim 128 16 5 val_trace_perl1 > ../exp/val6.txt
diff6:
	diff -iw ../exp/val6.txt ../validation/val6.txt
diff6p:
	diff -iw ../exp/val6.txt ../validation/val6.txt > ../exp/diff6.txt

val7: clean sim
	./sim 256 64 5 val_trace_perl1 > ../exp/val7.txt
diff7:
	diff -iw ../exp/val7.txt ../validation/val7.txt
diff7p:
	diff -iw ../exp/val7.txt ../validation/val7.txt > ../exp/diff7.txt


val8: clean sim
	./sim 512 64 7 val_trace_perl1 > ../exp/val8.txt
diff8:
	diff -iw ../exp/val8.txt ../validation/val8.txt
diff8p:
	diff -iw ../exp/val8.txt ../validation/val8.txt > ../exp/diff8.txt



exp1:
	rm -rf ../exp1
	mkdir ../exp1
	IQ_SIZE=8 ; while [[ $$IQ_SIZE -le 256 ]] ; do \
		run=1; WIDTH=1 ; while [[ $$WIDTH -le 8 ]] ; do \
			echo "run: "$$run "IQ:" $$IQ_SIZE "W:" $$WIDTH ; \
			./sim 512 $$IQ_SIZE $$WIDTH val_trace_gcc1 >> ../exp1/gcc.$$WIDTH.txt ;\
			./sim 512 $$IQ_SIZE $$WIDTH val_trace_perl1 >> ../exp1/perl.$$WIDTH.txt ;\
			((run = run+1)); \
	        ((WIDTH = WIDTH *2 )) ; \
		done ; \
		((IQ_SIZE = IQ_SIZE *2 )) ; \
	done
exp2:
	rm -rf ../exp2
	mkdir ../exp2
	ROB_SIZE=32 ; while [[ $$ROB_SIZE -le 512 ]] ; do \
			echo "ROB:" $$ROB_SIZE ; \
			./sim $$ROB_SIZE 8 1 val_trace_gcc1 >> ../exp2/gcc.1.txt ;\
			./sim $$ROB_SIZE 16 2 val_trace_gcc1 >> ../exp2/gcc.2.txt ;\
			./sim $$ROB_SIZE 32 4 val_trace_gcc1 >> ../exp2/gcc.4.txt ;\
			./sim $$ROB_SIZE 64 8 val_trace_gcc1 >> ../exp2/gcc.8.txt ;\
			./sim $$ROB_SIZE 8 1 val_trace_perl1 >> ../exp2/perl.1.txt ;\
			./sim $$ROB_SIZE 32 2 val_trace_perl1 >> ../exp2/perl.2.txt ;\
			./sim $$ROB_SIZE 64 4 val_trace_perl1 >> ../exp2/perl.4.txt ;\
			./sim $$ROB_SIZE 128 8 val_trace_perl1 >> ../exp2/perl.8.txt ;\
		((ROB_SIZE = ROB_SIZE *2 )) ; \
	done


