sources := $(shell find src -name "*.cpp") $(shell find src -name "*.c")
objects := $(patsubst src/%,out/%,$(addsuffix .o, $(sources)))

out/game: $(objects) vma.so | out/shaders.spv out
	gcc $^ -o out/game vma.so -l SDL3 -l vulkan

out/%.c.o: src/%.c | out/shaders.spv out
	@mkdir -p $(dir $@)
	gcc $< -o $@ -c

out/%.cpp.o: src/%.cpp | out/shaders.spv out
	@mkdir -p $(dir $@)
	g++ $< -o $@ -c

vma.so: vma.cpp
	g++ vma.cpp -o vma.so -l vulkan -shared -fPIE -fPIC

out/shaders.spv: src/gpu/shader.slang | out
	slangc src/gpu/shader.slang -entry vertexsmasher -entry fragger -profile spirv_1_4 -emit-spirv-directly -o out/shaders.spv

out:
	mkdir out

all: out/game
run: out/game
	./out/game
clean:
	rm -r out