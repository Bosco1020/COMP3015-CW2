#version 460

in vec3 Position;
in vec3 Normal;
//in vec2 TexCoord;
in vec4 ShadowCoord;

uniform struct LightInfo
{
    vec4 Position;
    vec3 La;
    vec3 L;
} lights[1];

uniform struct MaterialInfo
{
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float Shininess;
} Material;

layout (location = 0) out vec4 FragColor;

uniform sampler2DShadow ShadowMap;


vec3 phongModel(int light, vec3 position, vec3 n) {

    //vec3 ambient = lights[light].La * Material.Ka;

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

    return lights[light].L *(diffuse + spec);
}

subroutine void RenderPassType();
subroutine uniform RenderPassType RenderPass;

subroutine (RenderPassType)
void shadeWithShadow(){
    
    vec3 ambient = vec3(0.0f);
    ambient = lights[0].La * Material.Ka;

    vec3 diffAndSpec = vec3(0.0);
    vec3 n = Normal;

    diffAndSpec = phongModel(0, Position, n);

    float shadow = 1.0;
    if (ShadowCoord.z >= 0) {
        shadow = textureProj(ShadowMap, ShadowCoord);
    }

    FragColor = vec4(diffAndSpec * shadow + ambient, 1.0);

    //Gamma Correction
    //FragColor = pow( FragColor, vec4(1.0 / 2.2) );
}

subroutine (RenderPassType)
void RecordDepth() {
    //Does nothing, auto records depth
}


void main() {
    RenderPass();
}