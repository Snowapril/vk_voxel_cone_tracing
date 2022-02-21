#if !defined(RANDOM_GLSL)
#define RANDOM_GLSL

// Pseudo random generator from geeks3d
// https://www.geeks3d.com/20100831/shader-library-noise-and-pseudo-random-number-generator-in-glsl/

float randGen( in int n )
{
	n = (n << 13) ^ n; 
  	return float((n * (n*n*15731+789221) + 1376312589) & 0x7fffffff);
}

float noise3f(in vec3 p)
{
  	ivec3 ip = ivec3(floor(p));
  	vec3 u = fract(p);
  	u = u*u*(3.0-2.0*u);

  	int n = ip.x + ip.y*57 + ip.z*113;

  	float res = mix(mix(mix(randGen(n+(0+57*0+113*0)),
  	                        randGen(n+(1+57*0+113*0)),
  	                        u.x),
  	                    mix(randGen(n+(0+57*1+113*0)),
  	                        randGen(n+(1+57*1+113*0)),
  	                        u.x),
  	                	  u.y),
  	                mix(mix(randGen(n+(0+57*0+113*1)),
  	                        randGen(n+(1+57*0+113*1)),
  	                        u.x),
  	                    mix(randGen(n+(0+57*1+113*1)),
  	                        randGen(n+(1+57*1+113*1)),
  	                        u.x),
  	                    u.y),
  	                u.z);

  	return 1.0 - res*(1.0/1073741824.0);
}

#endif