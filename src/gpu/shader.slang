struct input_t {

};

struct output_t {

};

[shader("vertex")]
output_t vertexsmasher(input_t input) {
  return {};
}

[shader("fragment")]
float4 fragger(output_t vertIn) : SV_TARGET {
  return 0;
}