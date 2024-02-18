#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

//booleans
uniform bool lightOn;
uniform bool transparent;
uniform bool refl;
//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 pointLightPos;
//textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;
uniform samplerCube skybox;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.95f;

float computeShadow()
{
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	normalizedCoords = normalizedCoords * 0.5 + 0.5;
	
	if (normalizedCoords.z > 1.0f) return 0.0f;

	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;

	float currentDepth = normalizedCoords.z;

	float bias = 0.005;
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
	
	return shadow;
}

vec3 computeDirLight(float shadow)
{
    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;

    return min((ambient + (1.0f - shadow)*diffuse) * texture(diffuseTexture, fTexCoords).rgb + (1.0f - shadow) * specular * texture(specularTexture, fTexCoords).rgb, 1.0f);
}

vec3 computePointLight()
{
    vec4 fragPosEye = view * model * vec4(fPosition, 1.0f);
    //vec3 lightPosEye = view * model * pointLightPos;
    vec3 lightDirN = normalize(pointLightPos - fPosition);
    //vec3 normalEye = normalize(fNormal);
    float constant = 1.0f;
    float linear = 0.35f;
    float quadratic = 0.44f;
    vec3 lightColor2 = vec3(1.0f, 1.0f, 1.0f);

    //compute distance to light
    float dist = length(pointLightPos - fPosition);
    //compute attenuation
    float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));

    vec3 reflectDir = reflect(-lightDirN, fNormal);
    vec3 viewDir = normalize(- fragPosEye.xyz);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);

    // combine results
    ambient = att * ambientStrength * lightColor2;
    diffuse = att * max(dot(fNormal, lightDirN), 0.0f) * lightColor2;
    specular = att * specularStrength * specCoeff * lightColor2;

    ambient *= texture(diffuseTexture, fTexCoords).rgb;
    diffuse *= texture(diffuseTexture, fTexCoords).rgb;
    specular *= texture(specularTexture, fTexCoords).rgb;

    return (ambient + diffuse + specular);
}

float computeFog()
{
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    float fogDensity = 0.1f;
    float fragmentDistance = length(fPosEye);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
    return fogFactor;
}

vec3 computeSkyboxColor()
{
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 viewDir = normalize(-fPosEye.xyz);
    vec3 I = normalize(-viewDir);
    vec3 R = reflect(I, normalize(fNormal));
    return vec3(texture(skybox, R).rgb);
}

void main() 
{
    vec3 skyColor = computeSkyboxColor();
    float shadow = computeShadow();

    vec3 color = computeDirLight(shadow);
    if(lightOn)
        color += computePointLight();

    if(refl)
        color = mix(skyColor, color, 0.95f);

    float fogFactor = computeFog();
    vec3 fogColor = 0.5f * lightColor;
    
    float a;
    if(transparent)
        a = 0.2f;
    else
        a = 1.0f;

    fColor = vec4(mix(fogColor, color ,fogFactor), a);
}
