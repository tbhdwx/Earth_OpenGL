#version 440  
  
in vec4 vVertex;


uniform mat4   mvpMatrix;
uniform mat4   mvMatrix;
uniform mat3   normalMatrix;
uniform vec3   vLightPosition;

double x = 6378137.0;
double y = 6378137.0;
double z = 6356752.314245;

vec3 vTangent = vec3(1.0, 0.0, 0.0);

vec3 vEyeNormal;
smooth out vec3 vVaryingLightDir;
smooth out vec2 coord;

out vec3 worldPosition;

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
	   vec3 vNormal = GeodeticSurfaceNormal(vVertex.xyz, u_globeOneOverRadiiSquared);
       //地理纹理坐标
	 //  coord = ComputeTextureCoordinates(vNormal);

	   vEyeNormal = normalMatrix * vNormal;
	   vec4 vPosition4 = mvMatrix * vVertex;
	   vec3 vPosition3 = vPosition4.xyz / vPosition4.w;

	   vVaryingLightDir = normalize(vLightPosition - vPosition3);

	   //构造切平面
	   vec3 b, t, v;

	   t = normalize(normalMatrix * vTangent);
	   b = cross(vEyeNormal, t);
	   v.x = dot(vVaryingLightDir, t);
	   v.y = dot(vVaryingLightDir, b);
	   v.z = dot(vVaryingLightDir, vEyeNormal);

	   vVaryingLightDir = normalize(v);
	   gl_Position = mvpMatrix * vVertex;
	   worldPosition = vVertex.xyz;
    }