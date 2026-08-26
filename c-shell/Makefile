kgcc -std=c23 \
  -D_POSIX_C_SOURCE=200809L \
  -D_XOPEN_SOURCE=700 \
  -Wall -Wextra -Werror \
  -Wno-unused-parameter \
  -fno-asm \
  -Iinclude \
  src/main.c src/prompt.c src/lexer.c src/parser.c src/hop.c src/frecency.c \
  src/reveal.c src/locate.c src/executor.c src/peek.c src/command.c \
  src/redirect.c src/pipeline.c \
  -o shell.out
