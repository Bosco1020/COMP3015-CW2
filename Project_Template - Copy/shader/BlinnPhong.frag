#version 460

in vec3 Position;
in vec3 Normal;

uniform struct LightInfo
{
    vec4 Position;
    vec3 La;
    vec3 L;
} lights[3];

uniform struct MaterialInfo
{
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float Shininess;
} Material;

uniform struct FogInfo
{
float MaxDist;
float MinDist;
vec3 Color;
} Fog;

uniform float fogStrength;


vec4 addFog(vec4 before) {

    float dist = abs( Position.y ); //distance calculations
    float fogFactor = (1 - ((Fog.MaxDist - dist) / (Fog.MaxDist - abs(Fog.MinDist))) * fogStrength);

    fogFactor = clamp( fogFactor, 0.0, 1.0 ); //we clamp values

    //we assign a colour based on the fogFactor using mix
    vec3 color = mix( Fog.Color, before.rgb, fogFactor );
    vec4 finalColour = vec4(color, 1.0); //final colour

    return finalColour;
}

layout (location = 0) out vec4 FragColor;


vec3 phongModel(int light, vec3 position, vec3 n) {

    vec3 ambient = lights[light].La * Material.Ka;

    vec3 s = normalize(vec3(lights[light].Position.xyz - position));

    float sDotN = max(dot(s,n), 0.0);

    vec3 diffuse = Material.Kd * sDotN;

    vec3 spec = vec3(0.0);

    if(sDotN > 0.0)
    {
        vec3 v = normalize(-position.xyz);
        vec3 h = normalize(v + s);
        spec = Material.Ks * pow(max(dot(h,n), 0.0), Material.Shininess);
    }

    return ambient + lights[light].L * (diffuse + spec);
}


void main() {
    vec3 Colour = vec3(0.0f, 0.0f, 0.0f);

    for( int i = 0; i < 1; i++ )
        Colour += phongModel( i, Position, Normal );

    FragColor = addFog(vec4(Colour, 1.0));
}
