struct input_t {
  [[vk::location(0)]] float3 pos;
  //[[vk::location(1)]] float3 color;
};

struct output_t {
  mediump float4 pos : SV_Position;
  mediump float4 color;
};

struct vertexpushconstants {
  float4x4 proj;
  float4x4 view;
  float4x4 model;
};
[[vk::push_constant]] vertexpushconstants cameramatrices;

[shader("vertex")]
output_t vertexsmasher(input_t inv, uint id : SV_VertexID) {
  output_t output;
  output.pos = mul(cameramatrices.view, float4(inv.pos, 1));
  output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
  return output;
}

[shader("fragment")]
mediump float4 fragger(output_t vertIn) : SV_TARGET {
  return float4(vertIn.color);
}