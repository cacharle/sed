CC = gcc
CCFLAGS = --std=c99 -Wall -Wextra -pedantic

SRCDIR  = src
OBJDIR  = obj

TEST_SRCDIR = test
TEST_OBJDIR = test_obj

INCLUDE = $(shell find $(SRCDIR) -type f -name '*.h')
SRC     = $(shell find $(SRCDIR) -type f -name '*.c')
OBJ     = $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TEST_SRC = $(shell find $(TEST_SRCDIR) -type f -name '*.c')
TEST_OBJ = $(TEST_SRC:$(TEST_SRCDIR)/%.c=$(TEST_OBJDIR)/%.o)
TEST_OBJ += $(filter-out $(OBJDIR)/main.o, $(OBJ))

$(info $(TEST_OBJ))

NAME      = sed
TEST_NAME = $(NAME)_test

all: $(NAME)

$(NAME): $(OBJDIR) $(OBJ)
	$(CC) $(OBJ) -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDE)
	$(CC) $(CCFLAGS) -c $< -o $@

test: $(TEST_NAME)

test_run: test
	./$(TEST_NAME) --verbose 1 -j1

$(TEST_NAME): $(OBJDIR) $(TEST_OBJDIR) $(TEST_OBJ)
	$(CC) -lcriterion $(TEST_OBJ) -o $@

$(TEST_OBJDIR):
	mkdir $(TEST_OBJDIR)

$(TEST_OBJDIR)/%.o: $(TEST_SRCDIR)/%.c
	$(CC) -I$(SRCDIR) $(CCFLAGS) -c $< -o $@

clean:
	- rm $(NAME) $(TEST_NAME) $(OBJ) $(TEST_OBJ)
	- rmdir $(OBJDIR) $(TEST_OBJDIR)

re: clean all

format:
	clang-format -i $(SRC) $(INCLUDE) $(TEST_SRC)

.PHONY: all clean re test format
