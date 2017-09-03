#version 440  

smooth in vec3 vVaryingLightDir;
smooth in vec2 coord;
in vec3 worldPosition;

uniform vec4      ambientColor;
uniform vec4      diffuseColor;   

uniform sampler2D colorMap;
uniform sampler2D normalMap;

out vec4 fragmentColor;


double x = 6378137.0;
double y = 6378137.0;
double z = 6356752.314245;
vec3 GeodeticSurfaceNormal(vec3 positionOnEllipsoid, vec3 oneOverEllipsoidRadiiSquared)
{
    return normalize(positionOnEllipsoid * oneOverEllipsoidRadiiSquared);
}

vec2 ComputeTextureCoordinates(vec3 normal)
{
    double PI = 3.14159265358979323846;
    double og_oneOverTwoPi = 1.0 / (2.0 * PI); 
    double og_oneOverPi = 1.0 / PI;
    return vec2(atan(normal.y, normal.x) * og_oneOverTwoPi + 0.5, asin(normal.z) * og_oneOverPi + 0.5);
}

void main(void)
   { 
          vec3 u_globeOneOverRadiiSquared = vec3(1.0 / (x * x), 1.0 / (y * y), 1.0 / (z * z));
       //地理法线
	   vec3 vNormal = GeodeticSurfaceNormal(worldPosition, u_globeOneOverRadiiSquared);
       //地理纹理坐标
	   vec2 coords = ComputeTextureCoordinates(vNormal);
	   //法线纹理的法线
	   vec3 vTextureNormal = texture2D(normalMap, coords).xyz;
	   vTextureNormal =  (vTextureNormal  - 0.5) * 2.0;    
	   vTextureNormal = normalize(vTextureNormal);

		float diff = max(0.0,  dot(vTextureNormal, normalize(vVaryingLightDir)));
	    fragmentColor = diff * diffuseColor;	//漫反射光;

		fragmentColor += ambientColor;
		fragmentColor.rgb = min(vec3(1.0, 1.0, 1.0), fragmentColor.rgb);
		fragmentColor *= texture(colorMap, coords);
   }