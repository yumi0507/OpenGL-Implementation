#version 330 core

// Data from vertex shader.
// --------------------------------------------------------
in vec3 iPosWorld;
in vec3 iNormalWorld;
in vec2 iTexCoord;
// --------------------------------------------------------

// --------------------------------------------------------
uniform vec3 cameraPos;

// Material properties.
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;
uniform sampler2D mapKd;
uniform int isExist;

// Light data.
uniform vec3 ambientLight;
uniform vec3 dirLightDir;
uniform vec3 dirLightRadiance;
uniform vec3 pointLightPos;
uniform vec3 pointLightIntensity;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform vec3 spotLightIntensity;
uniform float spotLightTotalWidth;
uniform float spotLightCutoffStart;
// --------------------------------------------------------

out vec4 FragColor;

vec3 Diffuse(vec3 Kd, vec3 I, vec3 N, vec3 lightDir)
{
    return Kd * I * max(0, dot(N, lightDir));
}

vec3 Specular(vec3 Ks, float Ns, vec3 I, vec3 N, vec3 viewDir, vec3 lightDir)
{
    // Phong
    //vec3 vR = reflect(-lightDir, N);
    // return Ks * I * pow(max(0, dot(viewDir, vR)), Ns);

    // Bling-Phong
    vec3 vH = normalize(lightDir + viewDir);
    return Ks * I * pow(max(0, dot(N, vH)), Ns);
}


void main()
{
    // -------------------------------------------------------------
    vec3 nNormal = normalize(iNormalWorld);
    vec3 viewDir = normalize(cameraPos - iPosWorld);
    vec3 texColor;
    if(isExist == 0)
        texColor = Kd;
    else 
        texColor = texture2D(mapKd, iTexCoord).rgb;
    // -------------------------------------------------------------
    // Ambient light.
    vec3 ambient = Ka * ambientLight;
    // -------------------------------------------------------------
    // Directional light.
    vec3 wsLightDir = normalize(-dirLightDir);
    // Diffuse.
    vec3 diffuse = Diffuse(texColor, dirLightRadiance, nNormal, wsLightDir);
    // Specular.
    vec3 specular = Specular(Ks, Ns, dirLightRadiance, nNormal, viewDir, wsLightDir);
    vec3 dirLight = diffuse + specular;
    // -------------------------------------------------------------
    // Point light.
    vec3 wPointLightDir = normalize(pointLightPos - iPosWorld);
    float distSurfaceToLight = distance(pointLightPos, iPosWorld);
    float attenuation = 1.0f / (distSurfaceToLight * distSurfaceToLight);
    vec3 radiance = pointLightIntensity * attenuation;
    // Diffuse.
    diffuse = Diffuse(texColor, radiance, nNormal, wPointLightDir);
    // Specular.
    specular = Specular(Ks, Ns, radiance, nNormal, viewDir, wPointLightDir);
    vec3 pointLight = diffuse + specular;
    // -------------------------------------------------------------
    // Spot Light
    vec3 wSpotLightPos = normalize(spotLightPos - iPosWorld);
    vec3 wSpotLightDir = normalize(spotLightDir);
    distSurfaceToLight = distance(spotLightPos, iPosWorld);
    float A = degrees(acos(dot(wSpotLightPos, -wSpotLightDir)));
    float spotLightAttenuation = clamp((A - spotLightTotalWidth) / (spotLightCutoffStart - spotLightTotalWidth), 0, 1);
    attenuation = spotLightAttenuation / (distSurfaceToLight * distSurfaceToLight);
    radiance = spotLightIntensity * attenuation;
    // Diffuse.
    diffuse = Diffuse(texColor, radiance, nNormal, wSpotLightPos);
    // Specular.
    specular = Specular(Ks, Ns, radiance, nNormal, viewDir, wSpotLightPos);
    vec3 spotLight = diffuse + specular;
    // -------------------------------------------------------------
    vec3 LightingColor = ambient + dirLight + pointLight + spotLight;
    FragColor = vec4(LightingColor, 1.0);
    // -------------------------------------------------------------
}
