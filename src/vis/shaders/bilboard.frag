
//VTK::Color::Impl
float dist = dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy);
if (dist > 1.0) {
    discard;
} 

vec3 fogColor = vec3(1, 1, 1);

float depth =  (gl_FragCoord.z / gl_FragCoord.w);

float fog_start = 0.9f;
float fog_end = 0.8f;

float fogFactor = min(max((fog_end - depth) / (fog_end - fog_start), 0.0), 1.0);
float scale = (1 - 0.5 * dist) ;
ambientColor *= scale;
diffuseColor *= scale;

ambientColor = mix(ambientColor, fogColor, fogFactor);
diffuseColor = mix(diffuseColor, fogColor, fogFactor);

//ambientColor = vec3(0, 0, 0);
//diffuseColor = vec3(fogFactor, fogFactor, fogFactor);
diffuseColor = vec3(depth, depth, depth);



 