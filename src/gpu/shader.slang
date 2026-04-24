struct VSInput {
  float2 inPosition;
};

struct VSOutput
{
  float4 pos : SV_Position;
  float3 color;
};

[shader("vertex")]
VSOutput vertexsmasher(VSInput input) {
  VSOutput output;
  output.pos = float4(input.inPosition, 0.0, 1.0);
  output.color = float3(1,1,1);
  return output;
}

[shader("fragment")]
float4 fragger(VSOutput vertIn) : SV_TARGET {
  return float4(vertIn.color, 1.0);
}