
R"(
//VTK::Color::Impl
float dist = dot(offsetVCVSOutput.xy,offsetVCVSOutput.xy);
if (dist > 1.0) {
    discard;
} 

float scale = (1 - 0.5 * dist) ;
ambientColor *= scale;
diffuseColor *= scale;
 )"