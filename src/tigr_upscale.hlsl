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

float2 screenSize : register(c0);
sampler2D image;

void vs_main( Vertex vtx, out Interp interp )
{
	float2 pos = vtx.pos.xy;
	pos -= float2( 0.5f, 0.5f );
	
	// convert from pixels to NDC
	pos /= screenSize;
	pos.x = pos.x*2-1;
	pos.y = -(pos.y*2-1);
	
	interp.hPos = float4( pos, vtx.pos.z, 1 );
	interp.uv = vtx.uv;
}

float4 ps_main( Interp interp ) : COLOR
{
	float4 color = tex2D( image, interp.uv );

	return color;
}
