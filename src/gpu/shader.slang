struct input_t {
  [[vk::location(0)]] float3 pos;
  [[vk::location(1)]] float3 color;
};

struct output_t {
  float4 pos : SV_Position;
  float3 color;
};

[shader("vertex")]
output_t vertexsmasher(input_t input) {
  output_t output;
  output.pos = float4(input.pos, 1);
  output.color = float3(input.color);
  return output;
}

[shader("fragment")]
float4 fragger(output_t vertIn) : SV_TARGET {
  
  return float4(vertIn.color, 1.0);
}