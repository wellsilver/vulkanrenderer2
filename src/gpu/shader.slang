WTexture2D<float4> image;

[shader("compute")]
void maincompute(uint3 threadid: SV_DispatchThreadID, uniform uint2 imagesize) {
  image.Store(threadid.xy, float4(((float) threadid.x) / imagesize.x, 0, 0, 1));
}