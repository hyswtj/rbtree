CC=gcc
SRC_LIST = \
	main.c \
	rbtree.c
INC_DIR  = ./
CFLAGS = -Wall -march=native -g -m64 -lz -lstdc++ -lc -I$(INC_DIR)
OBJS = $(SRC_LIST:%.c=%.o)

TARGET = rbtree_sample

all:$(TARGET)
$(TARGET): $(OBJS) Makefile
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS)

clean:
	@rm -f $(OBJS) $(TARGET)

