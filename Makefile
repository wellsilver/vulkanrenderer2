sources := $(shell find src -name "*.c")
objects := $(patsubst src/%,out/%,$(patsubst %.c, %.o, $(sources)))

out/game: $(objects) | shaders out
	gcc $^ -o out/game -l SDL3 -l vulkan

out/%.o: src/%.c | shaders out
	@mkdir -p $(dir $@)
	gcc $< -o $@ -c

shaders: src/gpu/shader.slang | out
	slangc src/gpu/shader.slang -entry vertex -o out/vertex.spv
	slangc src/gpu/shader.slang -entry fragment -o out/fragment.spv

out:
	mkdir out

all: out/game
run: out/game
	./out/game
clean:
	rm -r out