/*
 * Curvas Paramétricas com OpenGL - Exemplo
 *
 * Desenvolvido por: Rossana B Queiroz
 * Disciplina: Computação Gráfica - Unisinos
 * Versão: 1.0
 *
 * Descrição:
 * Este programa implementa a geração e renderização de curvas paramétricas,
 * incluindo curvas de Bézier e Catmull-Rom. O programa permite visualizar
 * uma grade de fundo e eixos, além de destacar pontos de controle e curvas.
 *
 * Ferramentas e Tecnologias:
 * - OpenGL, GLAD, GLFW: Para renderização gráfica e criação de janelas.
 * - GLM: Para cálculos matemáticos (vetores, matrizes, transformações).
 * - Shader: Classe utilitária para carregar e compilar shaders GLSL.
 */

#include <iostream>
#include <string>
#include <assert.h>

#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//STB_IMAGE
#include <stb_image.h>

// STL
#include <vector>

#include <random>
#include <algorithm>

// Classes utilitárias
#include "Shader.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int loadSimpleOBJ(string filePATH, int &nVertices);
GLuint loadTexture(string filePath, int &width, int &height);

struct Curve
{
    std::vector<glm::vec3> controlPoints; // Pontos de controle da curva
    std::vector<glm::vec3> curvePoints;   // Pontos da curva
    glm::mat4 M;                          // Matriz dos coeficientes da curva
};

struct GeometryGrid
{
    GLuint VAO, EBO;
    glm::vec2 dimensions; // Dimensões da grade
    glm::vec2 initialPos; // Posição inicial da grade
};

struct GeometryAxes
{
    GLuint VAO;
    GLuint VBO;
};

struct Object
{
	GLuint VAO; //Índice do buffer de geometria
	GLuint texID; //Identificador da textura carregada
	int nVertices; //nro de vértices
	glm::mat4 model; //matriz de transformações do objeto
	float ka, kd, ks; //coeficientes de iluminação - material do objeto

};


// Outras funções
void initializeBernsteinMatrix(glm::mat4x4 &matrix);
void generateBezierCurvePoints(Curve &curve, int numPoints);
void initializeCatmullRomMatrix(glm::mat4x4 &matrix);
void generateCatmullRomCurvePoints(Curve &curve, int numPoints);
void displayCurve(const Curve &curve);
GLuint generateControlPointsBuffer(vector<glm::vec3> controlPoints);

void drawOBJ(GLuint shaderID, Object obj, glm::vec3 position, glm::vec3 dimensions, float angle, glm::vec3 color = glm::vec3(0.0, 0.0, 1.0), glm::vec3 axis = glm::vec3(0.0, 0.0, 1.0));
void drawOBJ2(GLuint shaderID, Object obj, glm::vec3 position, glm::vec3 dimensions, float angle, glm::vec3 color = glm::vec3(0.0, 0.0, 1.0), glm::vec3 axis = glm::vec3(0.0, 0.0, 1.0));

int setupTriangle();

// Funções para geração da grid
GeometryGrid generateGrid(float cellSize = 0.1f);
void drawGrid(const GeometryGrid &grid, GLuint shaderID);
GeometryAxes createAxesVAO();
void drawAxesVAO(const GeometryAxes &axes, GLuint shaderID);
std::vector<glm::vec3> generateHeartControlPoints(int numPoints = 20);

void generateGlobalBezierCurvePoints(Curve &curve, float a, float b, int numPoints);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 2000, HEIGHT = 1000;

bool rotateX=false, rotateY=false, rotateZ=false;

//variavel global seleção de obj
int objSelecionado = 1;

//Variáveis globais da câmera
glm::vec3 cameraPos = glm::vec3(0.0f,0.0f,3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f,0.0,-1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f,1.0f,0.0f);

int main()
{

    // Inicialização da GLFW
    glfwInit();
    // Criação da janela GLFW
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola Curvas Parametricas!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

    // GLAD: carrega todos os ponteiros d funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    // Compilando e buildando o programa de shader
    Shader shader = Shader("./hello-curves.vs", "./hello-curves.fs");
    Shader shaderTri = Shader("./hello-triangle.vs", "./hello-curves.fs");
	Shader shaderOBJ("phong.vs","phong.fs");

    Object obj,obj2;
	obj.VAO = loadSimpleOBJ("../Modelos3D/aratwearingabackpack/obj/model.obj",obj.nVertices);
	obj2.VAO = loadSimpleOBJ("../Modelos3D/pieceofcheese/obj/model.obj",obj2.nVertices);
	int texWidth,texHeight;
	obj.texID = loadTexture("../Modelos3D/aratwearingabackpack/textures/texture_1.jpeg",texWidth,texHeight);
    obj2.texID = loadTexture("../Modelos3D/pieceofcheese/textures/texture_1.jpeg",texWidth,texHeight);

	glUseProgram(shaderOBJ.ID);

    //Matriz de modelo
	glm::mat4 model = glm::mat4(1); //matriz identidade;
	GLint modelLoc = glGetUniformLocation(shaderOBJ.ID, "model");
	model = glm::rotate(model, /*(GLfloat)glfwGetTime()*/glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//Matriz de view
	glm::mat4 view = glm::lookAt(cameraPos,cameraPos + cameraFront,cameraUp);
	glUniformMatrix4fv(glGetUniformLocation(shaderOBJ.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
	//Matriz de projeção
	//glm::mat4 projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -1.0f, 1.0f);
	glm::mat4 projection = glm::perspective(glm::radians(39.6f),(float)WIDTH/HEIGHT,0.1f,100.0f);
	glUniformMatrix4fv(glGetUniformLocation(shaderOBJ.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//Buffer de textura no shader
	glUniform1i(glGetUniformLocation(shaderOBJ.ID, "texBuffer"), 0);

	glEnable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);

	//Propriedades da superfície
	shaderOBJ.setFloat("ka",0.2);
	shaderOBJ.setFloat("ks", 0.5);
	shaderOBJ.setFloat("kd", 0.5);
	shaderOBJ.setFloat("q", 10.0);

	//Propriedades da fonte de luz
	shaderOBJ.setVec3("lightPos",0.0, 20.0, 0.0);
	shaderOBJ.setVec3("lightColor",3.0, 3.0, 3.0);

    // Criando a geometria do triângulo
    GLuint VAO = setupTriangle();
    glm::vec3 position;
    glm::vec3 dimensions = glm::vec3(0.2, 0.2, 1.0);
    int index = 0;
    float lastTime = 0.0;
    float FPS = 60.0;
    float angle = 0.0;

    // Estrutura para armazenar a curva de Bézier e pontos de controle
    Curve curvaBezier;
    Curve curvaCatmullRom;

    std::vector<glm::vec3> controlPoints = generateHeartControlPoints();

    curvaBezier.controlPoints = controlPoints;
  
    // Para os pontos de controle da Catmull Rom precisamos duplicar o primeiro e o último
    curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[0]);
    for (int i = 0; i < curvaBezier.controlPoints.size(); i++)
    {
        curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[i]);
    }
    curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[curvaBezier.controlPoints.size() - 1]);

    // curvaBezier.controlPoints = { glm::vec3(-0.8f, -0.4f, 0.0f), glm::vec3(-0.4f, 0.4f, 0.0f),
    //                               glm::vec3(0.4f, 0.4f, 0.0f), glm::vec3(0.8f, -0.4f, 0.0f) };

    // Gerar pontos da curva de Bézier
    int numCurvePoints = 100; // Quantidade de pontos por segmento na curva
    
    float a = 1.0f; // Semi-eixo maior
    float b = 0.5f; // Semi-eixo menor
    int numPoints = 100; // Número de pontos da elipse

    generateGlobalBezierCurvePoints(curvaBezier, a ,b , numCurvePoints);
    // generateBezierCurvePoints(curvaBezier, numCurvePoints);
    generateCatmullRomCurvePoints(curvaCatmullRom, 10);

    // Cria a grid de debug
    GeometryGrid grid = generateGrid();
    GeometryAxes axes = createAxesVAO();

    // Cria os buffers de geometria dos pontos da curva
    GLuint VAOControl = generateControlPointsBuffer(curvaBezier.controlPoints);
    GLuint VAOBezierCurve = generateControlPointsBuffer(curvaBezier.curvePoints);
    GLuint VAOCatmullRomCurve = generateControlPointsBuffer(curvaCatmullRom.curvePoints);

    cout << curvaBezier.controlPoints.size() << endl;
    cout << curvaBezier.curvePoints.size() << endl;
    cout << curvaCatmullRom.curvePoints.size() << endl;

    shader.Use();

    // Loop da aplicação - "game loop"
    while (!glfwWindowShouldClose(window))
    {
        // Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
        glfwPollEvents();

        // Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        // Limpa o buffer de cor
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //shaderTri.Use();
        shaderOBJ.Use();
        // Desenhar o triângulo
        position = curvaBezier.curvePoints[index];

        // Incrementando o índice do frame apenas quando fechar a taxa de FPS desejada
        float now = glfwGetTime();
        float dt = now - lastTime;
        if (dt >= 1 / FPS)
        {
            index = (index + 1) % curvaBezier.curvePoints.size(); // incrementando ciclicamente o indice do Frame
            lastTime = now;
            glm::vec3 nextPos = curvaBezier.curvePoints[index];
            glm::vec3 dir = glm::normalize(nextPos - position);
            angle = atan2(dir.y, dir.x) + glm::radians(-90.0f);
        }
        
        drawOBJ(shaderOBJ.ID, obj, position, dimensions, angle);

        //obj2
        if(objSelecionado == 1){
            obj2.model = glm::mat4(1); //matriz identidade 
            if (rotateX)
            {
                obj2.model = glm::rotate(obj2.model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
                
            }
            else if (rotateY)
            {
                obj2.model = glm::rotate(obj2.model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

            }
            else if (rotateZ)
            {
                obj2.model = glm::rotate(obj2.model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

            }
        }
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(obj2.model));
      
        //Atualizar a matriz de view
		//Matriz de view
		view = glm::lookAt(cameraPos,cameraPos + cameraFront,cameraUp);
		glUniformMatrix4fv(glGetUniformLocation(shaderOBJ.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		
		// Chamada de desenho - drawcall
		// Poligono Preenchido - GL_TRIANGLES
		glBindVertexArray(obj2.VAO);
        glBindTexture(GL_TEXTURE_2D,obj2.texID);
		glDrawArrays(GL_TRIANGLES, 0, obj2.nVertices);
		
        //drawOBJ2(shaderOBJ.ID, obj2, position, dimensions, angle);

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }
    // Pede pra OpenGL desalocar os buffers
    glDeleteVertexArrays(1, &VAOControl);
    glDeleteVertexArrays(1, &VAOBezierCurve);
    glDeleteVertexArrays(1, &VAOCatmullRomCurve);
    // Finaliza a execução da GLFW, limpando os recursos alocados por ela
    glfwTerminate();

    return 0;
}

void initializeBernsteinMatrix(glm::mat4 &matrix)
{
    matrix[0] = glm::vec4(-1.0f, 3.0f, -3.0f, 1.0f); // Primeira coluna
    matrix[1] = glm::vec4(3.0f, -6.0f, 3.0f, 0.0f);  // Segunda coluna
    matrix[2] = glm::vec4(-3.0f, 3.0f, 0.0f, 0.0f);  // Terceira coluna
    matrix[3] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);   // Quarta coluna
}

void initializeCatmullRomMatrix(glm::mat4 &matrix)
{
    matrix[0] = glm::vec4(-0.5f, 1.5f, -1.5f, 0.5f); // Primeira linha
    matrix[1] = glm::vec4(1.0f, -2.5f, 2.0f, -0.5f); // Segunda linha
    matrix[2] = glm::vec4(-0.5f, 0.0f, 0.5f, 0.0f);  // Terceira linha
    matrix[3] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);   // Quarta linha
}

void generateBezierCurvePoints(Curve &curve, int numPoints)
{
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    initializeBernsteinMatrix(curve.M);
    // Calcular os pontos ao longo da curva com base em Bernstein
    // Loop sobre os pontos de controle em grupos de 4

    float piece = 1.0 / (float)numPoints;
    float t;
    for (int i = 0; i < curve.controlPoints.size() - 3; i += 3)
    {

        // Gera pontos para o segmento atual
        for (int j = 0; j < numPoints; j++)
        {
            t = j * piece;

            // Vetor t para o polinômio de Bernstein
            glm::vec4 T(t * t * t, t * t, t, 1);

            glm::vec3 P0 = curve.controlPoints[i];
            glm::vec3 P1 = curve.controlPoints[i + 1];
            glm::vec3 P2 = curve.controlPoints[i + 2];
            glm::vec3 P3 = curve.controlPoints[i + 3];

            glm::mat4x3 G(P0, P1, P2, P3);

            // Calcula o ponto da curva multiplicando tVector, a matriz de Bernstein e os pontos de controle
            glm::vec3 point = G * curve.M * T;

            curve.curvePoints.push_back(point);
        }
    }
}

void generateCatmullRomCurvePoints(Curve &curve, int numPoints)
{
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    initializeCatmullRomMatrix(curve.M);
    // Calcular os pontos ao longo da curva com base em Bernstein
    // Loop sobre os pontos de controle em grupos de 4

    float piece = 1.0 / (float)numPoints;
    float t;
    for (int i = 0; i < curve.controlPoints.size() - 3; i++)
    {

        // Gera pontos para o segmento atual
        for (int j = 0; j < numPoints; j++)
        {
            t = j * piece;

            // Vetor t para o polinômio de Bernstein
            glm::vec4 T(t * t * t, t * t, t, 1);

            glm::vec3 P0 = curve.controlPoints[i];
            glm::vec3 P1 = curve.controlPoints[i + 1];
            glm::vec3 P2 = curve.controlPoints[i + 2];
            glm::vec3 P3 = curve.controlPoints[i + 3];

            glm::mat4x3 G(P0, P1, P2, P3);

            // Calcula o ponto da curva multiplicando tVector, a matriz de Bernstein e os pontos de controle
            glm::vec3 point = G * curve.M * T;
            curve.curvePoints.push_back(point);
        }
    }
}

GLuint generateControlPointsBuffer(vector<glm::vec3> controlPoints)
{
    GLuint VBO, VAO;

    // Geração do identificador do VBO
    glGenBuffers(1, &VBO);

    // Faz a conexão (vincula) do buffer como um buffer de array
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Envia os dados do array de floats para o buffer da OpenGl
    glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(GLfloat) * 3, controlPoints.data(), GL_STATIC_DRAW);

    // Geração do identificador do VAO (Vertex Array Object)
    glGenVertexArrays(1, &VAO);

    // Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
    // e os ponteiros para os atributos
    glBindVertexArray(VAO);

    // Atributo posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
    // atualmente vinculado - para que depois possamos desvincular com segurança
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
    glBindVertexArray(0);

    return VAO;
}

GeometryGrid generateGrid(float cellSize)
{
    GeometryGrid grid;
    grid.dimensions = glm::vec2(2.0f, 2.0f);   // Dimensões totais da grid de -1 a 1 em X e Y
    grid.initialPos = glm::vec2(-1.0f, -1.0f); // Posição inicial de desenho

    std::vector<glm::vec3> vertices;
    std::vector<GLuint> indices;

    int numCells = static_cast<int>(2.0f / cellSize); // Calcula o número de células de 0.1 entre -1 e 1

    // Gera os vértices da grid
    for (int i = 0; i <= numCells; i++)
    {
        float pos = grid.initialPos.x + i * cellSize;

        // Linhas verticais
        vertices.push_back(glm::vec3(pos, grid.initialPos.y, 0.0f));        // Parte inferior
        vertices.push_back(glm::vec3(pos, grid.initialPos.y + 2.0f, 0.0f)); // Parte superior

        // Linhas horizontais
        vertices.push_back(glm::vec3(grid.initialPos.x, pos, 0.0f));        // Parte esquerda
        vertices.push_back(glm::vec3(grid.initialPos.x + 2.0f, pos, 0.0f)); // Parte direita
    }

    // Índices de elementos para conectar as linhas
    for (int i = 0; i <= numCells; i++)
    {
        // Índices das linhas verticais
        indices.push_back(i * 2);
        indices.push_back(i * 2 + 1);

        // Índices das linhas horizontais
        indices.push_back((numCells + 1) * 2 + i * 2);
        indices.push_back((numCells + 1) * 2 + i * 2 + 1);
    }

    // Configuração dos buffers VAO e EBO
    glGenVertexArrays(1, &grid.VAO);
    glGenBuffers(1, &grid.EBO);

    glBindVertexArray(grid.VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Configura o layout dos atributos dos vértices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); // Desvincula o VAO atual

    // Limpeza
    glDeleteBuffers(1, &VBO);

    return grid;
}

void drawGrid(const GeometryGrid &grid, GLuint shaderID)
{
    glUseProgram(shaderID);

    // Define a cor cinza médio para a grid
    GLint colorLocation = glGetUniformLocation(shaderID, "finalColor");
    glUniform4f(colorLocation, 0.5f, 0.5f, 0.5f, 1.0f); // RGBA: cinza médio

    // Ativa o VAO da grid
    glBindVertexArray(grid.VAO);

    // Largura da grid
    glLineWidth(1.0f);

    // Desenha a grid como linhas usando GL_LINES para contorno
    glDrawElements(GL_LINES, (grid.dimensions.x / 0.1f + 1) * 4, GL_UNSIGNED_INT, 0);

    // Desvincula o VAO
    glBindVertexArray(0);
}

GeometryAxes createAxesVAO()
{
    GeometryAxes axes;
    glm::vec3 axisVertices[] = {
        glm::vec3(-1.0f, 0.0f, 0.0f), // X axis start
        glm::vec3(1.0f, 0.0f, 0.0f),  // X axis end
        glm::vec3(0.0f, -1.0f, 0.0f), // Y axis start
        glm::vec3(0.0f, 1.0f, 0.0f)   // Y axis end
    };

    glGenVertexArrays(1, &axes.VAO);
    glGenBuffers(1, &axes.VBO);

    glBindVertexArray(axes.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, axes.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); // Unbind VAO
    return axes;
}

void drawAxesVAO(const GeometryAxes &axes, GLuint shaderID)
{
    glUseProgram(shaderID);

    // Desenha o eixo X em vermelho
    GLint colorLocation = glGetUniformLocation(shaderID, "finalColor");
    glUniform4f(colorLocation, 1.0f, 0.0f, 0.0f, 1.0f); // Cor vermelha

    // Largura dos eixos
    glLineWidth(3.0f);

    glBindVertexArray(axes.VAO);
    glDrawArrays(GL_LINES, 0, 2); // Desenha o eixo X

    // Desenha o eixo Y em azul
    glUniform4f(colorLocation, 0.0f, 0.0f, 1.0f, 1.0f); // Cor azul
    glDrawArrays(GL_LINES, 2, 2);                       // Desenha o eixo Y

    glBindVertexArray(0); // Unbind VAO
}

std::vector<glm::vec3> generateHeartControlPoints(int numPoints)
{
    std::vector<glm::vec3> controlPoints;

    // Define o intervalo para t: de 0 a 2 * PI, dividido em numPoints
    float step = 2 * 3.14159 / (numPoints - 1);

    for (int i = 0; i < numPoints - 1; i++)
    {
        float t = i * step;

        // Calcula x(t) e y(t) usando as fórmulas paramétricas
        float x = 16 * pow(sin(t), 3);
        float y = 13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t);

        // Normaliza os pontos para mantê-los dentro de [-1, 1] no espaço 3D
        x /= 16.0f; // Dividir por 16 para normalizar x entre -1 e 1
        y /= 16.0f; // Dividir por 16 para normalizar y aproximadamente entre -1 e 1
        y += 0.15;
        // Adiciona o ponto ao vetor de pontos de controle
        controlPoints.push_back(glm::vec3(x, y, 0.0f));
    }
    controlPoints.push_back(controlPoints[0]);

    return controlPoints;
}

void generateGlobalBezierCurvePoints(Curve &curve, float a, float b, int numPoints)
{
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    float t;
    float step = 2.0f * glm::pi<float>() / (float)numPoints; // Incremento de t em radianos

    for (int j = 0; j <= numPoints; ++j)
    {
        t = j * step;

        // Coordenadas paramétricas da elipse
        float x = a * cos(t);
        float y = b * sin(t);
        float z = 0.0f; // Elipse no plano XY, Z constante

        curve.curvePoints.push_back(glm::vec3(x, y, z));
    }
}

int setupTriangle()
{
    // Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
    // sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
    // Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
    // Pode ser arazenado em um VBO único ou em VBOs separados
    GLfloat vertices[] = {
        // x    y    z
        // T0
        -0.5, -0.5, 0.0, // v0
        0.5, -0.5, 0.0,  // v1
        0.0, 0.5, 0.0,   // v2
    };

    GLuint VBO, VAO;
    // Geração do identificador do VBO
    glGenBuffers(1, &VBO);
    // Faz a conexão (vincula) do buffer como um buffer de array
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Envia os dados do array de floats para o buffer da OpenGl
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Geração do identificador do VAO (Vertex Array Object)
    glGenVertexArrays(1, &VAO);
    // Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
    // e os ponteiros para os atributos
    glBindVertexArray(VAO);
    // Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
    //  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
    //  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
    //  Tipo do dado
    //  Se está normalizado (entre zero e um)
    //  Tamanho em bytes
    //  Deslocamento a partir do byte zero
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
    // atualmente vinculado - para que depois possamos desvincular com segurança
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
    glBindVertexArray(0);

    return VAO;
}

void drawOBJ(GLuint shaderID, Object obj, glm::vec3 position, glm::vec3 dimensions, float angle, glm::vec3 color, glm::vec3 axis)
{
    glBindVertexArray(obj.VAO);
    // Matriz de modelo: transformações na geometria (objeto)
    glm::mat4 model = glm::mat4(1); // matriz identidade
    
    if(objSelecionado == 0){
        if (rotateX)
            {
                model = glm::rotate(obj.model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
                
            }
            else if (rotateY)
            {
                model = glm::rotate(obj.model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

            }
            else if (rotateZ)
            {
                model = glm::rotate(obj.model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

            }
    }else{

        
        // Translação
        model = glm::translate(model, position);
        // Rotação
        model = glm::rotate(model, angle, axis);
        // Escala
        model = glm::scale(model, dimensions);
    }

    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform4f(glGetUniformLocation(shaderID, "finalColor"), color.r, color.g, color.b, 1.0f); // enviando cor para variável uniform inputColor
                           

    glBindTexture(GL_TEXTURE_2D,obj.texID);
    glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
}

void drawOBJ2(GLuint shaderID, Object obj2, glm::vec3 position, glm::vec3 dimensions, float angle, glm::vec3 color, glm::vec3 axis)
{
    glBindVertexArray(obj2.VAO);

    glBindVertexArray(obj2.VAO);
    glDrawArrays(GL_TRIANGLES, 0, obj2.nVertices);
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS){
		objSelecionado = objSelecionado == 1 ? 0 : 1 ;
		rotateX = false;
		rotateY = false;
		rotateZ = false;
        std::cout << objSelecionado << std::endl;
	}

	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		rotateX = true;
		rotateY = false;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = true;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = false;
		rotateZ = true;
	}

	//Verifica a movimentação da câmera
	float cameraSpeed = 0.05f;

	if ((key == GLFW_KEY_W || key == GLFW_KEY_UP) && action == GLFW_PRESS)
	{
		cameraPos += cameraSpeed * cameraFront;
	}
	if ((key == GLFW_KEY_S || key == GLFW_KEY_DOWN) && action == GLFW_PRESS)
	{
		cameraPos -= cameraSpeed * cameraFront;
	}
	if ((key == GLFW_KEY_A || key == GLFW_KEY_LEFT) && action == GLFW_PRESS)
	{
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if ((key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) && action == GLFW_PRESS)
	{
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
}

int loadSimpleOBJ(string filePath, int &nVertices)
{
	vector <glm::vec3> vertices;
	vector <glm::vec2> texCoords;
	vector <glm::vec3> normals;
	vector <GLfloat> vBuffer;

	glm::vec3 color = glm::vec3(1.0, 0.0, 0.0);

	ifstream arqEntrada;

	arqEntrada.open(filePath.c_str());
	if (arqEntrada.is_open())
	{
		//Fazer o parsing
		string line;
		while (!arqEntrada.eof())
		{
			getline(arqEntrada,line);
			istringstream ssline(line);
			string word;
			ssline >> word;
			if (word == "v")
			{
				glm::vec3 vertice;
				ssline >> vertice.x >> vertice.y >> vertice.z;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				vertices.push_back(vertice);

			}
			if (word == "vt")
			{
				glm::vec2 vt;
				ssline >> vt.s >> vt.t;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				texCoords.push_back(vt);

			}
			if (word == "vn")
			{
				glm::vec3 normal;
				ssline >> normal.x >> normal.y >> normal.z;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				normals.push_back(normal);

			}
			else if (word == "f")
			{
				while (ssline >> word) 
				{
					int vi, ti, ni;
					istringstream ss(word);
    				std::string index;

    				// Pega o índice do vértice
    				std::getline(ss, index, '/');
    				vi = std::stoi(index) - 1;  // Ajusta para índice 0

    				// Pega o índice da coordenada de textura
    				std::getline(ss, index, '/');
    				ti = std::stoi(index) - 1;

    				// Pega o índice da normal
    				std::getline(ss, index);
    				ni = std::stoi(index) - 1;

					//Recuperando os vértices do indice lido
					vBuffer.push_back(vertices[vi].x);
					vBuffer.push_back(vertices[vi].y);
					vBuffer.push_back(vertices[vi].z);
					
					//Atributo cor
					vBuffer.push_back(color.r);
					vBuffer.push_back(color.g);
					vBuffer.push_back(color.b);

					//Atributo coordenada de textura
					vBuffer.push_back(texCoords[ti].s);
					vBuffer.push_back(texCoords[ti].t);

					//Atributo vetor normal
					vBuffer.push_back(normals[ni].x);
					vBuffer.push_back(normals[ni].y);
					vBuffer.push_back(normals[ni].z);
					
        			
        			// Exibindo os índices para verificação
       				// std::cout << "v: " << vi << ", vt: " << ti << ", vn: " << ni << std::endl;
    			}
				
			}
		}

		arqEntrada.close();

		cout << "Gerando o buffer de geometria..." << endl;
		GLuint VBO, VAO;

	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 
	
	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//Atributo coordenada de textura - s, t
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6*sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	//Atributo vetor normal - x, y, z
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8*sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	nVertices = vBuffer.size() / 11;
	return VAO;

	}
	else
	{
		cout << "Erro ao tentar ler o arquivo " << filePath << endl;
		return -1;
	}
}

GLuint loadTexture(string filePath, int &width, int &height)
{
	GLuint texID; // id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // assume que é 4 canais png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}