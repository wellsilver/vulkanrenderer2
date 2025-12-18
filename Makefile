sources := $(shell find src -name "*.c")
objects := $(patsubst src/%,out/%,$(patsubst %.c, %.o, $(sources)))

out/game: $(objects) | out
	gcc $^ -o out/game -l SDL3 -l vulkan

out/%.o: src/%.c | out
	@mkdir -p $(dir $@)
	gcc $< -o $@ -c

out:
	mkdir out

all: out/game
run: out/game
	./out/game
clean:
	rm -r out