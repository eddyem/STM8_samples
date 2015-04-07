NAME=testproj
SDCC=sdcc

CCFLAGS=-DSTM8S105 -I../ -I/usr/share/sdcc/include -mstm8 --out-fmt-ihx
LDFLAGS= -mstm8 --out-fmt-ihx -lstm8
FLASHFLAGS=-cstlinkv2 -pstm8s105

SRC=$(wildcard *.c)

OBJ=$(SRC:%.c=%.rel)
TRASH=$(OBJ) $(SRC:%.c=%.rst) $(SRC:%.c=%.asm) $(SRC:%.c=%.lst)
TRASH+=$(SRC:%.c=%.sym) $(NAME).lk $(NAME).map
INDEPENDENT_HEADERS=../stm8l.h ports_definition.h Makefile

all: $(NAME).ihx

#$(SRC) : %.c : %.h $(INDEPENDENT_HEADERS)
#	@touch $@
#
#%.h: ;

clean:
	rm -f $(TRASH)

load: $(NAME).ihx
	stm8flash $(FLASHFLAGS) -w $(NAME).ihx

%.rel: %.c
	$(SDCC) $(CCFLAGS) -c $<

$(NAME).ihx: $(OBJ)
	$(SDCC) $(LDFLAGS) $(OBJ) -o $(NAME).ihx

.PHONY: all
