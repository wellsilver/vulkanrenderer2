struct vertice {
  uint32_t meshid;
  float3 vert;
};

struct VertexOutput {
  float3 color;
  float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexOutput vertexsmasher(uint vid : SV_VertexID) {
  VertexOutput output;
  output.sv_position = float4(, 0.0, 1.0);
  output.color = colors[vid];
  return output;
}

[shader("fragment")]
float4 fragger(VertexOutput inVert) : SV_Target
{
  float3 color = inVert.color;
  return float4(color, 1.0);
}