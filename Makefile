CC = gcc
CFLAGS = --std=c99 -Wall -Wextra -pedantic

SRCDIR = src
OBJDIR = obj

INCLUDE = $(shell find $(SRCDIR) -type f -name '*.h')
SRC = $(shell find $(SRCDIR) -type f -name '*.c')
OBJ = $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

NAME = sed

all: $(NAME)

$(NAME): $(OBJDIR) $(OBJ)
	$(CC) $(OBJ) -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDE)
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(NAME) $(OBJ)

re: clean all

.PHONY: all clean re
