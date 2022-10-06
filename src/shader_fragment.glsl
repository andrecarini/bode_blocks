#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da cor de cada vértice, definidas em "shader_vertex.glsl" e
// "main.cpp" (array color_coefficients).
in vec2 TexCoord0;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Textura
uniform sampler2D gSampler;

void main()
{
    // Definimos a cor final de cada fragmento utilizando a cor interpolada
    // pelo rasterizador.
    color = texture2D(gSampler, TexCoord0);
} 

