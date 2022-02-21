#if !defined(FILTER_GLSL)
#define FILTER_GLSL

#define DOUBLE_PI 6.28318530718
#define GAUSSIAN_DIRECTIONS 32.0
#define GAUSSIAN_QUALITY 8.0

// Gaussian blur referenced on shadertoy(https://www.shadertoy.com/view/Xltfzj)
vec4 gaussianBlur(sampler2D targetTexture, vec2 texCoord, float blurSize)
{
    vec2 radius = blurSize / vec2(textureSize(targetTexture, 0));
    
    vec4 color = texture(targetTexture, texCoord);
    
    for( float d = 0.0; d < DOUBLE_PI; d += DOUBLE_PI / GAUSSIAN_DIRECTIONS)
    {
		for(float i = 1.0 / GAUSSIAN_QUALITY; i <= 1.0; i += 1.0 / GAUSSIAN_QUALITY)
        {
			color += texture(targetTexture, texCoord + vec2(cos(d),sin(d)) * radius * i);		
        }
    }
    
    return color / (GAUSSIAN_QUALITY * GAUSSIAN_DIRECTIONS - 15.0);
}

// Bilateral filter referenced on shadertoy(https://www.shadertoy.com/view/4dfGDH)
float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

float normpdf3(in vec3 v, in float sigma)
{
	return 0.39894*exp(-0.5*dot(v,v)/(sigma*sigma))/sigma;
}

const float KERNEL[15] = float[15](
	0.031225216, 0.033322271, 0.035206333, 0.036826804, 0.038138565, 
	0.039104044, 0.039695028, 0.039894000, 0.039695028, 0.039104044, 
	0.038138565, 0.036826804, 0.035206333, 0.033322271, 0.031225216
);
#define BILATERAL_BSIGMA 10.0

vec3 bilateralFilter(sampler2D targetTexture, vec2 texCoord)
{
	vec3 finalColor = vec3(0.0);
	vec3 centerColor = texture(targetTexture, texCoord).rgb;
	float Z = 0.0;
	float bZ = 1.0 / normpdf(0.0, BILATERAL_BSIGMA);
	vec2 invResolution = 1.0 / vec2(textureSize(targetTexture, 0));
	for (int i = -7; i <= 7; ++i)
	{
		for (int j = -7; j <= 7; ++j)
		{
			vec3 sampleColor = texture(targetTexture, texCoord + vec2(float(i), float(j)) * invResolution).rgb;
			float factor = normpdf3(sampleColor - centerColor, BILATERAL_BSIGMA) * bZ * KERNEL[7+j] * KERNEL[7+i];
			Z += factor;
			finalColor += factor * sampleColor;
		}
	}

	return finalColor / Z;
}

#endif