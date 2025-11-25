sources := $(shell find src -name "*.c")
objects := $(patsubst %.c, out/%.o, $(notdir $(sources)))

out/game: $(objects) | out
	gcc $^ -o out/game -l SDL3 -l vulkan

out/%.o: src/%.c | out
	gcc $< -o $@ -c

out/%.o: src/%.cpp | out
	g++ $< -o $@ -c

out:
	mkdir out

all: out/game
run: out/game
	./out/game
clean:
	rm -r out