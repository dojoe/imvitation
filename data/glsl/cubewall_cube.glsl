#version 330
precision mediump float;

uniform mat4 u_matrix;
uniform vec4 u_posScale[510];

layout(location=0) in  vec3 inPosition;

out vec3 vertexPos;
flat out vec3 pos;
flat out int instanceId;

void main()
{
	vec4 scalePos = u_posScale[gl_InstanceID];
	gl_Position = u_matrix * vec4(inPosition*scalePos.w+scalePos.xyz, 1.0f);
	vertexPos = inPosition;
	instanceId = gl_InstanceID;
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

uniform vec3 u_eye;

uniform vec4 u_color[510];

flat in vec3 pos;
in vec3 vertexPos;
flat in int instanceId;
out vec4 outputColor;

void main()
{
	vec3 lightPos = vec3(2.0, 5.0, 5.0);

	vec4 diffCol = mix(u_color[instanceId], vec4(0.8, 0.6, 0.3, 1.0), 0.5);
	vec4 specCol = u_color[instanceId]; //vec4(0.9, 0.7, 0.5, 1.0);


	vec3 dir = normalize((vertexPos+pos) - lightPos);
	vec3 xd = dFdx(vertexPos);
	vec3 yd = dFdy(vertexPos);
	vec3 normal = -normalize(cross(yd, xd));

	vec3 planarPos = abs(vertexPos-normal);
	vec3 edgeNormal = vertexPos;
	float minVal = min(edgeNormal.x, min(edgeNormal.y, edgeNormal.z));
	if(edgeNormal.x == minVal) edgeNormal.x = 0.;
	if(edgeNormal.y == minVal) edgeNormal.y = 0.;
	if(edgeNormal.z == minVal) edgeNormal.z = 0.;
	edgeNormal = normalize(edgeNormal);

	float edgeFactor = pow(max(planarPos.x, max(planarPos.y, planarPos.z)), 16.);
	normal = normalize(mix(normal, edgeNormal, edgeFactor));

	vec3 hw = normalize(dir + normalize(vertexPos+pos - u_eye));

	float diffuse  = clamp(dot(-dir, normal), 0., 1.);
	float specular = clamp(dot(-hw, normal), 0., 1.);

	outputColor = diffuse*diffCol + specCol*(pow(specular, 5.));
}
