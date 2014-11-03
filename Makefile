# Some variables
CC 		= gcc
CFLAGS		= -g -Wall -DDEBUG
LDFLAGS		= -lm
TESTDEFS	= -DTESTING			# comment this out to disable debugging code
OBJS		= peer.o bt_parse.o spiffy.o debug.o input_buffer.o chunk.o sha.o chunklist.o request.o conn.o queue.o congest.o
MK_CHUNK_OBJS   = make_chunks.o chunk.o sha.o
SOURCE=src
VPATH=$(SOURCE)

BINS            = peer make-chunks
TESTBINS        = test_debug test_input_buffer

# Implicit .o target
.c.o:
	$(CC) $(TESTDEFS) -c $(CFLAGS) $<

# Explit build and testing targets

all: ${BINS} ${TESTBINS} clean_obj

run: peer_run
	./peer_run

run_client:
	./peer -p nodes.map -c example/A.haschunks -f example/C.chunks -m 4 -i 1 -d 0

run_server:
	./peer -p nodes.map -c example/B.haschunks -f example/C.chunks -m 4 -i 2 -d 0

run_server_2:
	./peer -p nodes.map -c example/B.haschunks -f example/C.chunks -m 4 -i 3 -d 0

spiffy:
	./hupsim.pl -m topo.map -n nodes.map -p 12345 -v 0 &

test: peer_test
	./peer_test

peer: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

make-chunks: $(MK_CHUNK_OBJS)
	$(CC) $(CFLAGS) $(MK_CHUNK_OBJS) -o $@ $(LDFLAGS)



clean:
	rm -f *.o $(BINS) $(TESTBINS)

clean_obj:
	rm -f *.o


bt_parse.c: bt_parse.h

# The debugging utility code

debug-text.h: src/debug.h
	./debugparse.pl < debug.h > debug-text.h

test_debug.o: src/debug.c src/debug-text.h
	${CC} src/debug.c ${INCLUDES} ${CFLAGS} -c -D_TEST_DEBUG_ -o $@

test_input_buffer:  test_input_buffer.o input_buffer.o
