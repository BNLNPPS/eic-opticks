#version 410 core
uniform mat4 MVP;

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNrm;

out vec4 v_color;

void main()
{
    gl_Position = MVP * vec4(vPos, 1.0);
    //v_color = vec4( vNrm*0.5 + 0.5, 1.0 );

    v_color = vec4( clamp( vNrm*0.5 + 0.8, 0.0, 1.0) , 1.0 );

}



