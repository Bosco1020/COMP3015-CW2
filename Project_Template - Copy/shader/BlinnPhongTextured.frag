#version 460

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;
in vec4 ShadowCoord;

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

vec3 texColor;
uniform sampler2DShadow ShadowMap;

layout (binding=1) uniform sampler2D Tex1;
layout (binding=2) uniform sampler2D Tex2;

layout (location = 0) out vec4 FragColor;


vec3 phongModel(int light, vec3 position, vec3 n) { //* texColor

    vec3 ambient = lights[light].La * Material.Ka * texColor;

    vec3 s = normalize(vec3(lights[light].Position.xyz - position));

    float sDotN = max(dot(s,n), 0.0);

    vec3 diffuse = Material.Kd * sDotN * texColor;

    vec3 spec = vec3(0.0);

    if(sDotN > 0.0)
    {
        vec3 v = normalize(-position.xyz);
        vec3 h = normalize(v + s);
        spec = Material.Ks * pow(max(dot(h,n), 0.0), Material.Shininess);
    }

    return ambient + lights[light].L * (diffuse + spec);
}

vec3 phongModelDiffAndSpec()
{
	vec3 n = Normal;
	vec3 s = normalize(vec3(lights[0].Position) - Position);
	vec3 v = normalize(-Position.xyz);
	vec3 r = reflect( -s, n);
	float sDotN = max( dot(s,n), 0.0 );
	vec3 diffuse = lights[0].L * Material.Kd * sDotN;
	vec3 spec = vec3(0.0);
	if( sDotN > 0.0 )
		spec = lights[0].L * Material.Ks * pow( max( dot(r,v), 0.0 ), Material.Shininess );
	
	return diffuse + spec;
}


subroutine void RenderPassType();
subroutine uniform RenderPassType RenderPass;

subroutine (RenderPassType)
void shadeWithShadow()
{
	vec3 ambient = lights[0].La * Material.Ka;
	vec3 diffAndSpec = phongModelDiffAndSpec();

	float shadow = 1.0;
	if( ShadowCoord.z >= 0 ) {
		shadow = textureProj(ShadowMap, ShadowCoord);
	}

	FragColor = vec4(diffAndSpec * shadow + ambient, 1.0);

	FragColor = pow(FragColor, vec4(1.0 / 2.2) );
}

subroutine (RenderPassType)
void recordDepth()
{

}


void main() {
    RenderPass();

/*
    vec4 Tex1Color = texture(Tex1, TexCoord);
    vec4 Tex2Color = texture(Tex2, TexCoord);

    //texColor = Tex1Color.rgb;
    texColor = vec3(0.9, 0.9, 0.9);

    //texColor = mix(Tex1Color.rgb, vec3(0.9f), Tex1Color.a);


    vec3 Colour = vec3(0.0f, 0.0f, 0.0f);

    for( int i = 0; i < 1; i++ )
        Colour += phongModel( i, Position, Normal );

    FragColor = vec4(Colour, 1 );
    //FragColor = vec4(1.0);
    */

}
