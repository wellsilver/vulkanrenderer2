sources := $(shell find src -name "*.c")
objects := $(patsubst src/%,out/%,$(patsubst %.c, %.o, $(sources)))

out/game: $(objects) | out/shaders.spv out
	gcc $^ -o out/game -l SDL3 -l vulkan

out/%.o: src/%.c | out/shaders.spv out
	@mkdir -p $(dir $@)
	gcc $< -o $@ -c

out/shaders.spv: src/gpu/shader.slang | out
	slangc src/gpu/shader.slang -entry vertexsmasher -entry fragger -profile spirv_1_3 -emit-spirv-directly -o out/shaders.spv

out:
	mkdir out

all: out/game
run: out/game
	./out/game
clean:
	rm -r out