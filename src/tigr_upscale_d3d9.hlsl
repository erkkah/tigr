struct Vertex
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD0;
};

struct Interp
{
	float4 hPos : POSITION;
	float2 uv : TEXCOORD0;
};

float4 screenSize : register(c0);
float4 texSize : register(c1);
float4 params : register(c2);
sampler2D image;

void vs_main( Vertex vtx, out Interp interp )
{
	float2 pos = vtx.pos.xy;
	pos -= float2( 0.5f, 0.5f );
	
	// convert from pixels to NDC
	pos /= screenSize.xy;
	pos.x = pos.x*2-1;
	pos.y = -(pos.y*2-1);
	
	interp.hPos = float4( pos, vtx.pos.z, 1 );
	interp.uv = vtx.uv * texSize.xy;
}

float4 ps_main( Interp interp ) : COLOR
{
	float2 uv = interp.uv;
	float scanline = 1 - frac(uv.y);
	uv = lerp(floor(uv) + 0.5f, uv, params.xy);
	
	float4 color = tex2D( image, uv * texSize.zw );

	scanline = lerp(0.5f, scanline, params.z);
	color.rgb *= scanline.xxx * 2;
	
	color.rgb = lerp(float3(0.5f, 0.5f, 0.5f), color.rgb, params.w);

	return color;
}
