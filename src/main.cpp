//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   Projeto Final
//
//                BODE_BLOCKS THE GAME
//               287711 - Aline Machado
//               260843 - Andre Carini
//

#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <stdlib.h>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "collisions.h"

// Defines
#define TAO 0.7
#define TEXCOORD_SHADER_LOCATION 1

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
GLuint BuildTriangles(); // Constrói triângulos para renderização
GLuint BuildSceneryCube();
GLuint LoadShader_Vertex(const char *filename); // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowHelp(GLFWwindow* window);
void TextRendering_ShowFail(GLFWwindow *window);
void TextRendering_ShowVictory(GLFWwindow *window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);
void TextRendering_ShowBlockPosition(GLFWwindow *window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Carregamento de imagens para textura
GLuint Load_Texture_BMP(const char *file_path);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTriangles() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação do jogador na cena virtual
float g_AngleX     = 0.0f;
float g_AngleY     = 0.0f;
float g_AngleZ     = 0.0f;
// Posicionamento do jogador na cena virtual
float g_PositionX  = 0.0f;
float g_PositionZ  = 0.0f;
float g_PositionY  = 0.0f;
//Posicionamento da esfera na cena virtual
float g_sphere_position_x = 0.0f;
float g_sphere_position_y = 0.0f;
float g_sphere_position_z = 0.0f;


int block_position = 1.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 10.5f; // Distância da câmera para a origem

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis para controlar movimentação da câmera
bool g_MoveForward = false;
bool g_MoveBackward = false;
bool g_MoveLeft = false;
bool g_MoveRight = false;

bool g_CameraFreelook = true;

// Tempo
double g_LastTime = glfwGetTime();

int main()
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 800 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 800, "INF01047 - 287711-Aline Machado, 00260843-Andre Carini - >BODE_BLOCKS THE GAME< ", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetWindowSize(window, 800, 800); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carrega modelo do gatinho
    struct ObjModel catmodel("../data/cat.obj");
    BuildTrianglesAndAddToVirtualScene(&catmodel);

    // Carrega modelo da esfera
    struct ObjModel spheremodel("../data/esfera_vermelha.obj");
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    GLuint vertex_shader_id = LoadShader_Vertex("../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../src/shader_fragment.glsl");

    // Criamos um programa de GPU utilizando os shaders carregados acima
    GLuint program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Construímos a representação de um triângulo
    GLuint vertex_array_object_id = BuildTriangles();

    GLuint scenery_cube_VAO = BuildSceneryCube();

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Carregar textura
    GLuint FloorTexture = Load_Texture_BMP("../data/floor_texture.bmp");
    GLuint ExitTexture = Load_Texture_BMP("../data/exit_texture.bmp");
    GLuint PlayerTexture = Load_Texture_BMP("../data/player_texture.bmp");
    GLuint SphereTexture = Load_Texture_BMP("../data/marble_texture_2.bmp");
    GLuint SkyTexture = Load_Texture_BMP("../data/sky_texture.bmp");
    GLuint CatTexture = Load_Texture_BMP("../data/cat_texture.bmp");
    GLuint CatTexture2 = Load_Texture_BMP("../data/cat_texture_2.bmp");

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl".
    GLint model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    GLint view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    GLint projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glm::vec4 FindPoint(float t);
    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    glm::vec4 camera_position_c = glm::vec4(5.0f, 3.0f, 10.0f, 1.0f); // Ponto "c", centro da câmera
    float start=glfwGetTime();
    float last_fail=glfwGetTime();
    bool show_fail = false;
    bool show_victory = true;

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        float t=glfwGetTime()-start;
        float time_since_last_fail = glfwGetTime()-last_fail;
        if (time_since_last_fail > 4) {
            show_fail = false;
        }


        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(program_id);

        glm::vec4 camera_view_vector;
        glm::vec4 camera_up_vector;
        glm::vec4 camera_lookat_l;

        float speed = 2.0f * (float)(glfwGetTime() - g_LastTime);
        g_LastTime = glfwGetTime();

        if (g_CameraFreelook == true) {
            float x = cos(g_CameraPhi) * sin(g_CameraTheta);
            float y = sin(g_CameraPhi);
            float z = cos(g_CameraPhi) * cos(g_CameraTheta);

            camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            camera_view_vector = glm::vec4(-x, -y, -z, 0.0f);
            camera_view_vector /= norm(camera_view_vector);

            if (g_MoveForward)      { camera_position_c += speed * camera_view_vector; }
            if (g_MoveLeft)         { camera_position_c -= speed * crossproduct(camera_view_vector, camera_up_vector); }
            if (g_MoveBackward)     { camera_position_c -= speed * camera_view_vector; }
            if (g_MoveRight)        { camera_position_c += speed * crossproduct(camera_view_vector, camera_up_vector); }

        } else {
            float r = g_CameraDistance;
            float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);
            float y = r * sin(g_CameraPhi);
            float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);

            camera_position_c = glm::vec4(x+5.0f, y+1.0f, z+3.0f, 1.0f);             // Ponto "c", centro da câmera
            camera_lookat_l = glm::vec4(5.0f, 1.0f, 3.0f, 1.0f);      // Ponto "l", para onde a câmera (look-at) estará sempre olhando
            camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
            camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);     // Vetor "up" fixado para apontar para o "céu" (eito Y global)
        }

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -200.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // Desenha o cubo do cenário
        glBindVertexArray(scenery_cube_VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, SkyTexture);
        glm::mat4 skybox = Matrix_Scale(100.0f, 100.0f, 100.0f) * Matrix_Translate(-0.5f, -0.5f, -0.5f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(skybox));
        glDrawElements(
            g_VirtualScene["scenery_cube_faces"].rendering_mode, // Veja slides 182-188 do documento Aula_04_Modelagem_Geometrica_3D.pdf
            g_VirtualScene["scenery_cube_faces"].num_indices,
            GL_UNSIGNED_INT,
            (void *)g_VirtualScene["scenery_cube_faces"].first_index
        );

        // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
        // vértices apontados pelo VAO criado pela função BuildTriangles(). Veja
        // comentários detalhados dentro da definição de BuildTriangles().
        glBindVertexArray(vertex_array_object_id);

        // Desenho do mapa (chão) feito de cópias do bloco
        for (int i = 1; i <= 35; ++i)
        {
            // Colocar as texturas
            glActiveTexture(GL_TEXTURE0);
            if (i == 29) {
                glBindTexture(GL_TEXTURE_2D, ExitTexture);
            } else {
                glBindTexture(GL_TEXTURE_2D, FloorTexture);
            }

            // Cada cópia do cubo possui uma matriz de modelagem independente,
            // já que cada cópia estará em uma posição (rotação, escala, ...)
            // diferente em relação ao espaço global (World Coordinates). Veja
            // slides 2-14 e 184-190 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
            glm::mat4 model;

            if      (i == 1)    { model = Matrix_Translate(0.0f, -0.11f, 0.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 2)    { model = Matrix_Translate(1.0f, -0.11f, 0.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 3)    { model = Matrix_Translate(2.0f, -0.11f, 0.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }

            else if (i == 4)    { model = Matrix_Translate(0.0f, -0.11f, 1.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 5)    { model = Matrix_Translate(1.0f, -0.11f, 1.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 6)    { model = Matrix_Translate(2.0f, -0.11f, 1.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }

            else if (i == 7)    { model = Matrix_Translate(0.0f, -0.11f, 2.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 8)    { model = Matrix_Translate(1.0f, -0.11f, 2.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 9)    { model = Matrix_Translate(2.0f, -0.11f, 2.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }

            else if (i == 10)   { model = Matrix_Translate(1.0f, -0.11f, 3.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 11)   { model = Matrix_Translate(2.0f, -0.11f, 3.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }


            else if (i == 12)   { model = Matrix_Translate(3.0f, -0.11f, 1.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 13)   { model = Matrix_Translate(4.0f, -0.11f, 1.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 14)   { model = Matrix_Translate(5.0f, -0.11f, 1.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }

            else if (i == 15)   { model = Matrix_Translate(3.0f, -0.11f, 2.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 16)   { model = Matrix_Translate(4.0f, -0.11f, 2.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 17)   { model = Matrix_Translate(5.0f, -0.11f, 2.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }

            else if (i == 18)   { model = Matrix_Translate(3.0f, -0.11f, 3.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 19)   { model = Matrix_Translate(4.0f, -0.11f, 3.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 20)   { model = Matrix_Translate(5.0f, -0.11f, 3.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }

            else if (i == 21)   { model = Matrix_Translate(5.0f, -0.11f, 4.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }


            else if (i == 22)   { model = Matrix_Translate(6.0f, -0.11f, 2.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 23)   { model = Matrix_Translate(7.0f, -0.11f, 2.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 24)   { model = Matrix_Translate(8.0f, -0.11f, 2.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }

            else if (i == 25)   { model = Matrix_Translate(6.0f, -0.11f, 3.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 26)   { model = Matrix_Translate(7.0f, -0.11f, 3.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 27)   { model = Matrix_Translate(8.0f, -0.11f, 3.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }

            else if (i == 28)   { model = Matrix_Translate(6.0f, -0.11f, 4.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 29)   { model = Matrix_Translate(7.0f, -0.11f, 4.0f) * Matrix_Scale(0.95f, 0.2f, 0.95f); }
            else if (i == 30)   { model = Matrix_Translate(8.0f, -0.11f, 4.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }

            else if (i == 31)   { model = Matrix_Translate(6.0f, -0.11f, 5.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 32)   { model = Matrix_Translate(7.0f, -0.11f, 5.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 33)   { model = Matrix_Translate(8.0f, -0.11f, 5.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }


            else if (i == 34)   { model = Matrix_Translate(9.0f, -0.11f, 3.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }
            else if (i == 35)   { model = Matrix_Translate(9.0f, -0.11f, 4.0f) * Matrix_Scale(1.0f, 0.2f, 1.0f); }



            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));


            if (i == 29) { glBlendFunc(GL_DST_ALPHA, GL_DST_ALPHA); }

            glDrawElements(
                g_VirtualScene["cube_faces"].rendering_mode, // Veja slides 182-188 do documento Aula_04_Modelagem_Geometrica_3D.pdf
                g_VirtualScene["cube_faces"].num_indices,
                GL_UNSIGNED_INT,
                (void*)g_VirtualScene["cube_faces"].first_index
            );

            if (i == 29) { glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
        }

        //---------------------------------------esfera inimiga--------------------------------------------------------//
        t=(1+sin(t))/2;
        glm::vec4 translator = FindPoint(t);
        g_sphere_position_x = 4.0f;
        g_sphere_position_y = 0.7f;
        g_sphere_position_z = 2 * translator.z - 3.0f;

        glm::mat4 model = Matrix_Translate(g_sphere_position_x,g_sphere_position_y,g_sphere_position_z) * Matrix_Scale(0.38f, 0.38f, 0.38f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, SphereTexture);
        glBindVertexArray(g_VirtualScene["esfera_vermelha"].vertex_array_object_id);
        glDrawElements(
            g_VirtualScene["esfera_vermelha"].rendering_mode, // Veja slides 182-188 do documento Aula_04_Modelagem_Geometrica_3D.pdf
            g_VirtualScene["esfera_vermelha"].num_indices,
            GL_UNSIGNED_INT,
            (void *)g_VirtualScene["cube_faces"].first_index);

        //---------------------------------------esfera inimiga--------------------------------------------------------//

        //-------------------------------------- cubo jogador --------------------------------------------------//
        model = Matrix_Translate(0.0f, 1.0f, 0.0f);
        model = model   * Matrix_Translate(g_PositionX, g_PositionY, g_PositionZ)
                        * Matrix_Rotate_Z(g_AngleZ)  // TERCEIRO rotação Z de Euler
                        * Matrix_Rotate_Y(g_AngleY)  // SEGUNDO  rotação Y de Euler
                        * Matrix_Rotate_X(g_AngleX)
                        * Matrix_Scale(1.0f, 2.0f, 1.0f);

        if (plane_collision() || sphere_collision())
        {
            g_PositionX = 0.0f;
            g_PositionY = 0.0f;
            g_PositionZ = 0.0f;
            block_position = 1.0f;
            g_AngleX = 0.0f;
            g_AngleY = 0.0f;
            g_AngleZ = 0.0f;

            last_fail = glfwGetTime();
            show_fail = true;
        }

        if (victory_cube_collision()) {
            show_victory = true;
        } else {
            show_victory = false;
        }

        glBlendFunc(GL_DST_ALPHA, GL_DST_ALPHA);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, PlayerTexture);
        glBindVertexArray(vertex_array_object_id);
        glDrawElements(
            g_VirtualScene["cube_faces"].rendering_mode, // Veja slides 182-188 do documento Aula_04_Modelagem_Geometrica_3D.pdf
            g_VirtualScene["cube_faces"].num_indices,
            GL_UNSIGNED_INT,
            (void*)g_VirtualScene["cube_faces"].first_index
        );
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //-------------------------------------- cubo jogador --------------------------------------------------//

        //---------------------------------------gatinho-------------------------------------------------------//
        model =  Matrix_Translate(-5.0f, 3.0f, -5.0f)  * Matrix_Scale(0.1f, 0.1f, 0.1f)  * Matrix_Rotate_Y(6.3f*t);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glBindVertexArray(g_VirtualScene["cat"].vertex_array_object_id);
        glBindTexture(GL_TEXTURE_2D, CatTexture);
        glDrawElements(
            g_VirtualScene["cat"].rendering_mode,
            g_VirtualScene["cat"].num_indices,
            GL_UNSIGNED_INT,
            (void*)(g_VirtualScene["cat"].first_index * sizeof(GLuint))
        );

        model =   Matrix_Translate(translator.x * 2.0f, 0.0f, 0.0f)
                * Matrix_Translate(-1.5f, 3.0f, -5.0f)  * Matrix_Scale(0.1f, 0.1f, 0.1f) * Matrix_Rotate_Y(-6.3f*t);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, CatTexture2);
        glDrawElements(
            g_VirtualScene["cat"].rendering_mode,
            g_VirtualScene["cat"].num_indices,
            GL_UNSIGNED_INT,
            (void*)(g_VirtualScene["cat"].first_index * sizeof(GLuint))
        );

        model =  Matrix_Translate(15.0f, 3.0f, -5.0f)  * Matrix_Scale(0.1f, 0.1f, 0.1f) * Matrix_Rotate_Y(6.3f*t);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, CatTexture);
        glDrawElements(
            g_VirtualScene["cat"].rendering_mode,
            g_VirtualScene["cat"].num_indices,
            GL_UNSIGNED_INT,
            (void*)(g_VirtualScene["cat"].first_index * sizeof(GLuint))
        );
        //---------------------------------------gatinho-------------------------------------------------------//

        // "Desligamos" o VAO, evitando assim que operações posteriores venham a
        // alterar o mesmo. Isso evita bugs.
        glBindVertexArray(0);

        // Imprimimos na tela as infos de ajuda.
        //TextRendering_ShowHelp(window);

        if( show_fail ) {
            TextRendering_ShowFail(window);
        }

        if ( show_victory )
        {
            TextRendering_ShowVictory(window);
        }

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();


    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Constrói cubo do cenário
GLuint BuildSceneryCube()
{
    // Geometria: conjunto de vértices.
    GLfloat model_coefficients[] = {
        //    X      Y      Z     W
                                      // face 0
           0.0f,  0.0f,  0.0f,  1.0f, // vértice 0
           1.0f,  0.0f,  0.0f,  1.0f, // vértice 1
           0.0f,  1.0f,  0.0f,  1.0f, // vértice 2
           1.0f,  1.0f,  0.0f,  1.0f, // vértice 3

                                      // face 1
           0.0f,  0.0f,  1.0f,  1.0f, // vértice 4
           0.0f,  0.0f,  0.0f,  1.0f, // vértice 5
           0.0f,  1.0f,  1.0f,  1.0f, // vértice 6
           0.0f,  1.0f,  0.0f,  1.0f, // vértice 7

                                      // face 2
           1.0f,  0.0f,  1.0f,  1.0f, // vértice 8
           0.0f,  0.0f,  1.0f,  1.0f, // vértice 9
           1.0f,  1.0f,  1.0f,  1.0f, // vértice 10
           0.0f,  1.0f,  1.0f,  1.0f, // vértice 11

                                      // face 3
           1.0f,  0.0f,  0.0f,  1.0f, // vértice 12
           1.0f,  0.0f,  1.0f,  1.0f, // vértice 13
           1.0f,  1.0f,  0.0f,  1.0f, // vértice 14
           1.0f,  1.0f,  1.0f,  1.0f, // vértice 15

                                      // face 4
           0.0f,  0.0f,  1.0f,  1.0f, // vértice 16
           1.0f,  0.0f,  1.0f,  1.0f, // vértice 17
           0.0f,  0.0f,  0.0f,  1.0f, // vértice 18
           1.0f,  0.0f,  0.0f,  1.0f, // vértice 19

                                      // face 5
           0.0f,  1.0f,  1.0f,  1.0f, // vértice 20
           1.0f,  1.0f,  1.0f,  1.0f, // vértice 21
           0.0f,  1.0f,  0.0f,  1.0f, // vértice 22
           1.0f,  1.0f,  0.0f,  1.0f, // vértice 23
    };

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);

    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(model_coefficients), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(model_coefficients), model_coefficients);
    GLuint location = 0;            // "(location = 0)" em "shader_vertex.glsl"
    GLint number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // UV coords.: Texturas.
    GLfloat texture_coordinates[] = {
        //  U       V
                          // face 0
            0.0f,   0.0f, // coordenada UV  0
            1.0f,   0.0f, // coordenada UV  1
            0.0f,   1.0f, // coordenada UV  2
            1.0f,   1.0f, // coordenada UV  3

                          // face 1
            0.0f,   0.0f, // coordenada UV  4
            1.0f,   0.0f, // coordenada UV  5
            0.0f,   1.0f, // coordenada UV  6
            1.0f,   1.0f, // coordenada UV  7

                          // face 2
            0.0f,   0.0f, // coordenada UV  8
            1.0f,   0.0f, // coordenada UV  9
            0.0f,   1.0f, // coordenada UV  10
            1.0f,   1.0f, // coordenada UV  11

                          // face 3
            0.0f,   0.0f, // coordenada UV  12
            1.0f,   0.0f, // coordenada UV  13
            0.0f,   1.0f, // coordenada UV  14
            1.0f,   1.0f, // coordenada UV  15

                          // face 4
            0.0f,   0.0f, // coordenada UV  16
            1.0f,   0.0f, // coordenada UV  17
            0.0f,   1.0f, // coordenada UV  18
            1.0f,   1.0f, // coordenada UV  19

                          // face 5
            0.0f,   0.0f, // coordenada UV  20
            1.0f,   0.0f, // coordenada UV  21
            0.0f,   1.0f, // coordenada UV  22
            1.0f,   1.0f, // coordenada UV  23
    };

    GLuint VBO_texture_coordinates_id;
    glGenBuffers(1, &VBO_texture_coordinates_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coordinates_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinates), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(texture_coordinates), texture_coordinates);
    location = TEXCOORD_SHADER_LOCATION; // "(location = 1)" em "shader_vertex.glsl"
    number_of_dimensions = 2;            // vec2 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Triângulos.
    GLuint indices[] = {

                 // face 0
        0, 1, 2, // triângulo 0
        1, 2, 3, // triângulo 1

                 // face 1
        4, 5, 6, // triângulo 2
        5, 6, 7, // triângulo 3

                    // face 2
        8, 9,  10,  // triângulo 4
        9, 10, 11,  // triângulo 5

                    // face 3
        12, 13, 14, // triângulo 6
        13, 14, 15, // triângulo 7

                    // face 4
        16, 17, 18, // triângulo 8
        17, 18, 19, // triângulo 9

                    // face 5
        20, 21, 22, // triângulo 10
        21, 22, 23, // triângulo 11
    };

    SceneObject cube_faces;
    cube_faces.name = "Cubo (faces texturizadas)";
    cube_faces.first_index = 0;
    cube_faces.num_indices = sizeof(indices) / sizeof(indices[0]);
    cube_faces.rendering_mode = GL_TRIANGLES; // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
    g_VirtualScene["scenery_cube_faces"] = cube_faces;

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);
    glBindVertexArray(0);

    return vertex_array_object_id;
}

// Constrói triângulos para futura renderização
GLuint BuildTriangles()
{
    // Geometria: conjunto de vértices.
    GLfloat model_coefficients[] = {
        //    X      Y      Z     W
        // face 0
        -0.5f, -0.5f, -0.5f, 1.0f, // vértice 0
        0.5f, -0.5f, -0.5f, 1.0f,  // vértice 1
        -0.5f, 0.5f, -0.5f, 1.0f,  // vértice 2
        0.5f, 0.5f, -0.5f, 1.0f,   // vértice 3

        // face 1
        -0.5f, -0.5f, 0.5f, 1.0f,  // vértice 4
        -0.5f, -0.5f, -0.5f, 1.0f, // vértice 5
        -0.5f, 0.5f, 0.5f, 1.0f,   // vértice 6
        -0.5f, 0.5f, -0.5f, 1.0f,  // vértice 7

        // face 2
        0.5f, -0.5f, 0.5f, 1.0f,  // vértice 8
        -0.5f, -0.5f, 0.5f, 1.0f, // vértice 9
        0.5f, 0.5f, 0.5f, 1.0f,   // vértice 10
        -0.5f, 0.5f, 0.5f, 1.0f,  // vértice 11

        // face 3
        0.5f, -0.5f, -0.5f, 1.0f, // vértice 12
        0.5f, -0.5f, 0.5f, 1.0f,  // vértice 13
        0.5f, 0.5f, -0.5f, 1.0f,  // vértice 14
        0.5f, 0.5f, 0.5f, 1.0f,   // vértice 15

        // face 4
        -0.5f, -0.5f, 0.5f, 1.0f,  // vértice 16
        0.5f, -0.5f, 0.5f, 1.0f,   // vértice 17
        -0.5f, -0.5f, -0.5f, 1.0f, // vértice 18
        0.5f, -0.5f, -0.5f, 1.0f,  // vértice 19

        // face 5
        -0.5f, 0.5f, 0.5f, 1.0f,  // vértice 20
        0.5f, 0.5f, 0.5f, 1.0f,   // vértice 21
        -0.5f, 0.5f, -0.5f, 1.0f, // vértice 22
        0.5f, 0.5f, -0.5f, 1.0f,  // vértice 23
    };

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);

    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(model_coefficients), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(model_coefficients), model_coefficients);
    GLuint location = 0;            // "(location = 0)" em "shader_vertex.glsl"
    GLint number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // UV coords.: Texturas.
    GLfloat texture_coordinates[] = {
        //  U       V
        // face 0
        0.0f, 0.0f, // coordenada UV  0
        1.0f, 0.0f, // coordenada UV  1
        0.0f, 1.0f, // coordenada UV  2
        1.0f, 1.0f, // coordenada UV  3

        // face 1
        0.0f, 0.0f, // coordenada UV  4
        1.0f, 0.0f, // coordenada UV  5
        0.0f, 1.0f, // coordenada UV  6
        1.0f, 1.0f, // coordenada UV  7

        // face 2
        0.0f, 0.0f, // coordenada UV  8
        1.0f, 0.0f, // coordenada UV  9
        0.0f, 1.0f, // coordenada UV  10
        1.0f, 1.0f, // coordenada UV  11

        // face 3
        0.0f, 0.0f, // coordenada UV  12
        1.0f, 0.0f, // coordenada UV  13
        0.0f, 1.0f, // coordenada UV  14
        1.0f, 1.0f, // coordenada UV  15

        // face 4
        0.0f, 0.0f, // coordenada UV  16
        1.0f, 0.0f, // coordenada UV  17
        0.0f, 1.0f, // coordenada UV  18
        1.0f, 1.0f, // coordenada UV  19

        // face 5
        0.0f, 0.0f, // coordenada UV  20
        1.0f, 0.0f, // coordenada UV  21
        0.0f, 1.0f, // coordenada UV  22
        1.0f, 1.0f, // coordenada UV  23
    };

    GLuint VBO_texture_coordinates_id;
    glGenBuffers(1, &VBO_texture_coordinates_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coordinates_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinates), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(texture_coordinates), texture_coordinates);
    location = TEXCOORD_SHADER_LOCATION; // "(location = 1)" em "shader_vertex.glsl"
    number_of_dimensions = 2;            // vec2 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Triângulos.
    GLuint indices[] = {

        // face 0
        0, 1, 2, // triângulo 0
        1, 2, 3, // triângulo 1

        // face 1
        4, 5, 6, // triângulo 2
        5, 6, 7, // triângulo 3

        // face 2
        8, 9, 10,  // triângulo 4
        9, 10, 11, // triângulo 5

        // face 3
        12, 13, 14, // triângulo 6
        13, 14, 15, // triângulo 7

        // face 4
        16, 17, 18, // triângulo 8
        17, 18, 19, // triângulo 9

        // face 5
        20, 21, 22, // triângulo 10
        21, 22, 23, // triângulo 11
    };

    // Criamos um primeiro objeto virtual (SceneObject) que se refere às faces
    // coloridas do cubo.
    SceneObject cube_faces;
    cube_faces.name           = "Cubo (faces coloridas)";
    cube_faces.first_index    =  0; // Primeiro índice está em indices[0]
    cube_faces.num_indices    = 36;       // Último índice está em indices[35]; total de 36 índices.
    cube_faces.rendering_mode = GL_TRIANGLES; // Índices correspondem ao tipo de rasterização GL_TRIANGLES.

    // Adicionamos o objeto criado acima na nossa cena virtual (g_VirtualScene).
    g_VirtualScene["cube_faces"] = cube_faces;

    // Criamos um buffer OpenGL para armazenar os índices acima
    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);

    // Alocamos memória para o buffer.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);

    // Copiamos os valores do array indices[] para dentro do buffer.
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    // NÃO faça a chamada abaixo! Diferente de um VBO (GL_ARRAY_BUFFER), um
    // array de índices (GL_ELEMENT_ARRAY_BUFFER) não pode ser "desligado",
    // caso contrário o VAO irá perder a informação sobre os índices.
    //
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);

    // Retornamos o ID do VAO. Isso é tudo que será necessário para renderizar
    // os triângulos definidos acima. Veja a chamada glDrawElements() em main().
    return vertex_array_object_id;
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (!g_LeftMouseButtonPressed)
        return;

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta -= 0.01f*dx;
    g_CameraPhi   += 0.01f*dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 /2; // 90 graus, em radianos.

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        if (g_CameraFreelook == false) g_CameraFreelook = true;
        else g_CameraFreelook = false;
    }

    // WASD movimentação da camera.
    if (key == GLFW_KEY_W && action == GLFW_PRESS) { g_MoveForward = true; }
	if (key == GLFW_KEY_W && action == GLFW_RELEASE) { g_MoveForward = false; }

	if (key == GLFW_KEY_A && action == GLFW_PRESS) { g_MoveLeft = true; }
	if (key == GLFW_KEY_A && action == GLFW_RELEASE) { g_MoveLeft = false; }

	if (key == GLFW_KEY_S && action == GLFW_PRESS) { g_MoveBackward = true; }
	if (key == GLFW_KEY_S && action == GLFW_RELEASE) { g_MoveBackward = false; }

	if (key == GLFW_KEY_D && action == GLFW_PRESS) { g_MoveRight = true; }
	if (key == GLFW_KEY_D && action == GLFW_RELEASE) { g_MoveRight = false; }

    //----------------------movimentação bloco----------------------------------------------------;;

 if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT || key == GLFW_KEY_UP|| key == GLFW_KEY_DOWN )&& action == GLFW_PRESS){
    switch (block_position){
    case 1:{
         if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT )&& action == GLFW_PRESS){
                block_position = 3;
                g_AngleX = 0;
                g_AngleY = 0;
                if (g_AngleZ < 2*delta)   {g_AngleZ += delta;}  else {g_AngleZ += - delta;}
                if (key == GLFW_KEY_LEFT) {g_PositionX+= -1.5;} else {g_PositionX += +1.5;}
                g_PositionY = -0.5;
         }

         if ((key == GLFW_KEY_UP|| key == GLFW_KEY_DOWN ) && action == GLFW_PRESS){
                block_position = 2;
                g_AngleY = 0;
                g_AngleZ = 0;
                if (g_AngleX < 2*delta)   {g_AngleX += + delta;}  else {g_AngleX += - delta;}
                if (key == GLFW_KEY_DOWN) {g_PositionZ += +1.5;}  else  {g_PositionZ += -1.5;}
                g_PositionY = -0.5;
         }
         break;
    }

    case 2:{
         if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT ) && action == GLFW_PRESS){
                block_position = 2;
                g_AngleY = 0;
                if (g_AngleZ < 2*delta)   {g_AngleZ += delta;}  else {g_AngleZ += - delta;}
                if (key == GLFW_KEY_LEFT) {g_PositionX+= -1.0;} else {g_PositionX += +1.0;}
                g_PositionY = -0.5;
         }

         if ((key == GLFW_KEY_UP|| key == GLFW_KEY_DOWN ) && action == GLFW_PRESS){
                block_position = 1;
                g_AngleY = 0;
                g_AngleZ = 0;
                if (g_AngleX < 2*delta)   {g_AngleX += + delta;}  else {g_AngleX += - delta;}
                if (key == GLFW_KEY_DOWN) {g_PositionZ += +1.5;}  else  {g_PositionZ += -1.5;}
                g_PositionY = 0.0;
         }
         break;
    }
    case 3:{
         if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT ) && action == GLFW_PRESS){
                block_position = 1;
                g_AngleX = 0;
                g_AngleY = 0;
                if (g_AngleZ < 2*delta)   {g_AngleZ += delta;}  else {g_AngleZ += - delta;}
                if (key == GLFW_KEY_LEFT) {g_PositionX+= -1.5;} else {g_PositionX += +1.5;}
                g_PositionY = 0.0;

         }

         if ((key == GLFW_KEY_UP|| key == GLFW_KEY_DOWN ) && action == GLFW_PRESS){
                block_position = 3;
                g_AngleX = 0;
                if (g_AngleY < 2*delta)   {g_AngleY += delta;}  else {g_AngleY += - delta;}
                if (key == GLFW_KEY_DOWN) {g_PositionZ += +1.0;}  else  {g_PositionZ += -1.0;}
                g_PositionY = -0.5;
         }

         break;
    }}

}

    //----------------------movimentação bloco----------------------------------------------------;;
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Carregamento de imagens (textura) formato BMP
// Inspirado no tutorial https://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
GLuint Load_Texture_BMP(const char *file_path)
{
    // Cria uma textura OpenGL
    GLuint textureID;
    glGenTextures(1, &textureID);

    // Faz o bind da textura
    glBindTexture(GL_TEXTURE_2D, textureID);

    // ### Leitura do arquivo ###
    unsigned char header[54];   // Todo arquivo BMP começa com um cabeçalho de 54 bytes
    unsigned int payload_start; // Posição em que começa o payload da imagem
    unsigned int width, height;
    unsigned int size;          // = width*height*3
    unsigned char *data;        // Valores RGB
    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        printf("O arquivo com a textura nao foi localizado!\n");
        return 0;
    }
    if (fread(header, 1, 54, file) != 54)
    { // Se não tiver lido 54 bytes, o arquivo está malformado.
        printf("O arquivo para textura esta errado.\n");
        return false;
    }
    if (header[0] != 'B' || header[1] != 'M')
    { // Checagem dos magic bytes BM de todo arquivo BMP
        printf("O arquivo para textura nao e um BMP.\n");
        return 0;
    }
    // Leitura dos metadados
    payload_start = *(int *)&(header[0x0A]);
    size = *(int *)&(header[0x22]);
    width = *(int *)&(header[0x12]);
    height = *(int *)&(header[0x16]);
    // Preenchendo informações que faltam, se o arquivo BMP não forneceu corretamente
    if (size == 0)
        size = width * height * 3; // 3 : Red, Green, Blue
    if (payload_start == 0)
        payload_start = 54; // Porque sim. (especificação do formato BMP)
    // Criar o buffer para leitura do payload
    data = new unsigned char[size];
    // Leitura do arquivo para o buffer
    fread(data, 1, size, file);
    // Fechar o arquivo original
    fclose(file);
    // ### Fim da leitura do arquivo ###

    // Passa a imagem para o OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

    // Configurações necessárias
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Retorna o ID da textura
    return textureID;
}

// Mostra mensagem de que o jogador morreu!
void TextRendering_ShowFail(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Morreu! Tente novamente!\n");

    TextRendering_PrintString(window, buffer, -0.5f + pad / 10, 2 * pad / 10, 3.0f);
}

// Mostra mensagem de que o jogador morreu!
void TextRendering_ShowVictory(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Parabens!\n");

    TextRendering_PrintString(window, buffer, -0.5f + pad / 10, 2 * pad / 10, 3.0f);
}


// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowBlockPosition(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Player: Config: (%i) | Pos: (%.2f) X, (%.2f) Y, (%.2f) Z\n", block_position, g_PositionX, g_PositionY, g_PositionZ);

    TextRendering_PrintString(window, buffer, -1.0f + pad / 10, -1.0f + 2 * pad / 10, 1.0f);
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

glm::vec4 FindPoint(float t)
{
 glm::vec4 p1,p2,p3,p4,p5,p6,p7;

 p1=glm::vec4(1.0f, 1.0f, 3.0f, 0.0f);
 p2=glm::vec4(2.0f, 1.0f, 2.0f, 0.0f);
 p3=glm::vec4(3.0f, 1.0f, 1.0f, 0.0f);
 p4=glm::vec4(4.0f, 1.0f, 2.0f, 0.0f);

 p7=glm::vec4(7.0f, 1.0f, 3.0f, 0.0f);
 p6=glm::vec4(6.0f, 1.0f, 3.0f, 0.0f);
 p5=glm::vec4(5.0f, 1.0f, 3.0f, 0.0f);

 glm::vec4 c12, c23,c34, c123, c234, c;

 if(t<=TAO)
   {
    t=t/TAO;

    c12=p1+t*(p2-p1);
    c23=p2+t*(p3-p2);
    c34=p3+t*(p4-p3);

    c123=c12+t*(c23-c12);
    c234=c23+t*(c34-c23);
    c=c123+t*(c234-c123);
   }
 else
   {
    t=(t-TAO)*2;

    c12=p4+t*(p5-p4);
    c23=p5+t*(p6-p5);
    c34=p6+t*(p7-p6);

    c123=c12+t*(c23-c12);
    c234=c23+t*(c34-c23);
    c=c123+t*(c234-c123);
   }

 return(c);
}
