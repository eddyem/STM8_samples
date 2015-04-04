NAME=testproj
SDCC=sdcc

CCFLAGS=-DSTM8S003 -I../ -I/usr/share/sdcc/include -mstm8 --out-fmt-ihx
LDFLAGS=-mstm8 --out-fmt-ihx -lstm8
FLASHFLAGS=-cstlinkv2 -pstm8s003

SRC=$(wildcard *.c)
# ATTENTION: FIRST in list should be file with main()
OBJ=$(SRC:%.c=%.rel)
TRASH=$(OBJ) $(SRC:%.c=%.rst) $(SRC:%.c=%.asm) $(SRC:%.c=%.lst)
TRASH+=$(SRC:%.c=%.sym)  $(NAME).lk $(NAME).map
INDEPENDENT_HEADERS=../stm8l.h Makefile

all: $(NAME).ihx

$(SRC) : %.c : %.h $(INDEPENDENT_HEADERS)
	@touch $@

%.h: ;

clean:
	rm -f $(TRASH)

load: $(NAME).ihx
	stm8flash $(FLASHFLAGS) -w $(NAME).ihx

%.rel: %.c
	$(SDCC) $(CCFLAGS) -c $<

$(NAME).ihx: $(OBJ)
	$(SDCC) $(LDFLAGS) $(OBJ) -o $(NAME).ihx

.PHONY: all
