CC = gcc
CCFLAGS = --std=c99 -Wall -Wextra -pedantic

TEST_LDFLAGS = $(LDFLAGS) --coverage -lcriterion

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

NAME      = sed
TEST_NAME = $(NAME)_test

all: $(NAME)

$(NAME): $(OBJDIR) $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDE)
	$(CC) $(CCFLAGS) -c $< -o $@

test: CCFLAGS += --coverage -g
test: $(TEST_NAME)

test_run: test
	./$(TEST_NAME) -j4 2>&1 | sed '/^sed:.*/d'

$(TEST_NAME): $(OBJDIR) $(TEST_OBJDIR) $(TEST_OBJ)
	$(CC) $(TEST_LDFLAGS) $(TEST_OBJ) -o $@

$(TEST_OBJDIR):
	mkdir $(TEST_OBJDIR)

$(TEST_OBJDIR)/%.o: $(TEST_SRCDIR)/%.c
	$(CC) -I$(SRCDIR) $(CCFLAGS) -c $< -o $@

clean:
	- rm $(NAME) $(TEST_NAME) $(OBJ) $(TEST_OBJ)
	- rm $(wildcard $(OBJDIR)/*.gcda)
	- rm $(wildcard $(OBJDIR)/*.gcno)
	- rm $(wildcard $(TEST_OBJDIR)/*.gcda)
	- rm $(wildcard $(TEST_OBJDIR)/*.gcno)
	- rmdir $(OBJDIR) $(TEST_OBJDIR)

re: clean all

format:
	clang-format -i $(SRC) $(INCLUDE) $(TEST_SRC)

report: test_run
	gcovr

.PHONY: all clean re test format
