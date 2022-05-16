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
uniform sampler3D OffsetTex;

uniform float Radius;
uniform vec3 OffsetTexSize; //W, H, D

layout (binding=2) uniform sampler2D Tex1;
layout (binding=3) uniform sampler2D TranspTex;

layout (location = 0) out vec4 FragColor;


vec3 phongModelDiffAndSpec()
{
// return diffusion & specular using Blinn-Phong

	vec3 s = normalize(vec3(lights[0].Position.xyz - Position));
    float sDotN = max(dot(s,Normal), 0.0);
    vec3 diffuse = Material.Kd * sDotN * texColor;
    vec3 spec = vec3(0.0);

    if(sDotN > 0.0)
    {
        vec3 v = normalize(-Position.xyz);
        vec3 h = normalize(v + s);
        spec = Material.Ks * pow(max(dot(h,Normal), 0.0), Material.Shininess);
    }
	
	return diffuse + spec;
}

	// Sets both methods as RenderPassTypes, allows switching between through declaration
subroutine void RenderPassType();
subroutine uniform RenderPassType RenderPass;

subroutine (RenderPassType)
void shadeWithShadow()
{
	vec3 ambient = lights[0].La * Material.Ka * texColor;
	vec3 diffAndSpec = phongModelDiffAndSpec();

    ivec3 offsetCoord;
    offsetCoord.xy = ivec2( mod( gl_FragCoord.xy, OffsetTexSize.xy ) );

	float sum = 0, shadow = 1.0;
    int samplesDiv2 = int(OffsetTexSize.z);
    vec4 sc = ShadowCoord;

   vec4 TranspTexColor = texture(TranspTex, TexCoord);

    if(sc.z >= 0 && 1 - TranspTexColor.a >= 0.78) {
        for ( int i = 0; i < 4; i++ ) {
            offsetCoord.z = i;
            vec4 offsets = texelFetch(OffsetTex, offsetCoord, 0) * Radius * ShadowCoord.w;

            sc.xy = ShadowCoord.xy + offsets.xy;
            sum += textureProj(ShadowMap, sc);
        }
        shadow = sum / 8.0;

        if( shadow != 1.0 && shadow != 0.0 ){
        for( int i = 4; i < samplesDiv2; i++ ) {
            offsetCoord.z = i;
            vec4 offsets = texelFetch(OffsetTex, offsetCoord, 0) * Radius * ShadowCoord.w;

            sc.xy = ShadowCoord.xy + offsets.xy;
            sum += textureProj(ShadowMap, sc);
            sc.xy = ShadowCoord.xy + offsets.zw;
            sum += textureProj(ShadowMap, sc);
            }
            shadow = sum / float(samplesDiv2 * 2.0);
        }
    }
    
    // If alpha value is below threshold, then don't render the pixel
    if (1 - TranspTexColor.a < 0.78) 
        discard;
    else{
    	FragColor = vec4(diffAndSpec * shadow + ambient, 1.0);

        //Gamma Correction
	    FragColor = pow(FragColor, vec4(1.0 / 2.2) );
    }
}

subroutine (RenderPassType)
void recordDepth()
{

}


void main() {
	// Record the texture and the alpha map
    vec4 TranspTexColor = texture(TranspTex, TexCoord);
    vec4 Tex1Color = texture(Tex1, TexCoord);

	// Use "alpha map" to identify where to apply texture map
    texColor = Tex1Color.rgb * (1 - TranspTexColor.rgb);

    RenderPass();

}
