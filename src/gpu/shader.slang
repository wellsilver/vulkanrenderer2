struct input_t {
  [[vk::location(0)]] float3 pos;
  //[[vk::location(1)]] float3 color;
};

struct output_t {
  float4 pos : SV_Position;
  float3 color;
};

[shader("vertex")]
output_t vertexsmasher(input_t in, uint id : SV_VertexID) {
  output_t output;
  switch (id) {
    case 0:
      output.pos = float4(0, -1, 0, 1);
      output.color = float3(1, 0, 0);
    break;
    case 1:
      output.pos = float4(1, 1, 0, 1);
      output.color = float3(0, 1, 0);
    break;
    case 2:
      output.pos = float4(-1, 1, 0, 1);
      output.color = float3(0, 0, 1);
    break;
  }
  return output;
}

[shader("fragment")]
float4 fragger(output_t vertIn) : SV_TARGET {
  
  return float4(vertIn.color, 1.0);
}