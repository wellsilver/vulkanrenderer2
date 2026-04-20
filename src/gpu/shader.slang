struct vertice {
  [[vk::location(0)]] float2 vert;
};

struct VertexOutput {
  float3 color;
  float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexOutput vertexsmasher(vertice verticedata, uint vid : SV_VertexID) {
  VertexOutput output;
  output.sv_position = float4(verticedata.vert, 0.0, 0.0);
  output.color = float3(1, 1, 1);
  return output;
}

[shader("fragment")]
float4 fragger(VertexOutput inVert) : SV_Target
{
  float3 color = inVert.color;
  return float4(color, 1.0);
}