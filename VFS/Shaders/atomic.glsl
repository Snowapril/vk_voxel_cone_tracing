#if !defined(ATOMIC_GLSL)
#define ATOMIC_GLSL

vec4 convRGBA8ToVec4( uint val) 
{	
	return vec4 (
		float(  val & 0x000000ff ), 
		float(( val & 0x0000ff00 ) >>  8u ),
		float(( val & 0x00ff0000 ) >> 16u ),
		float(( val & 0xff000000 ) >> 24u )
	);
}

uint convVec4ToRGBA8( vec4 val)
{
	return ( 
		((uint(val.w) & 0x000000ff) << 24u) | 
		((uint(val.z) & 0x000000ff) << 16u) | 
		((uint(val.y) & 0x000000ff) <<  8u) | 
		 (uint(val.x) & 0x000000ff)
	);
}

// void imageAtomicRGBA8Avg(volatile uimage3D image, ivec3 coords, vec4 value )
// {
// 	value.rgb *= 255.0;
// 	uint newVal = convVec4ToRGBA8(value);
// 	uint prevStoredVal = 0; uint curStoredVal;
// 
// 	// Loop as long as destination value gets changed by other threads
// 	while ( ( curStoredVal = imageAtomicCompSwap( image, coords, prevStoredVal, newVal )) != prevStoredVal) 
// 	{
// 		prevStoredVal = curStoredVal;
// 		vec4 rval = convRGBA8ToVec4(curStoredVal);
// 		rval.xyz = (rval.xyz * rval.w); 	// Denormalize
// 		vec4 curValF = rval + value; 		// Add new value
// 		curValF.xyz /= (curValF.w); 		// Renormalize
// 		newVal = convVec4ToRGBA8(curValF);
// 	}
// }

#endif