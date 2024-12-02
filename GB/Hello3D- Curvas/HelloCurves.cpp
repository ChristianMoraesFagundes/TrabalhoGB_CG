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

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STL
#include <vector>

#include <random>
#include <algorithm>

// Classes utilitárias
#include "Shader.h"

struct Curve {
    std::vector<glm::vec3> controlPoints; // Pontos de controle da curva
    std::vector<glm::vec3> curvePoints;   // Pontos da curva
    glm::mat4 M;          // Matriz dos coeficientes da curva
};

struct GeometryGrid {
    GLuint VAO, EBO;
    glm::vec2 dimensions;   // Dimensões da grade
    glm::vec2 initialPos;   // Posição inicial da grade
};

struct GeometryAxes {
    GLuint VAO;
    GLuint VBO;
};


// Outras funções
void initializeBernsteinMatrix(glm::mat4x4 &matrix);
void generateBezierCurvePoints(Curve &curve, int numPoints);
void initializeCatmullRomMatrix(glm::mat4x4 &matrix);
void generateCatmullRomCurvePoints(Curve &curve, int numPoints);
void displayCurve(const Curve &curve);
GLuint generateControlPointsBuffer(vector <glm::vec3> controlPoints);

//Funções para geração da grid
GeometryGrid generateGrid(float cellSize = 0.1f);
void drawGrid(const GeometryGrid& grid, GLuint shaderID);
GeometryAxes createAxesVAO();
void drawAxesVAO(const GeometryAxes& axes, GLuint shaderID);
std::vector<glm::vec3> generateHeartControlPoints(int numPoints = 20);

void generateGlobalBezierCurvePoints(Curve &curve, int numPoints);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 600, HEIGHT = 600;

int main()
{

    // Inicialização da GLFW
    glfwInit();
    // Criação da janela GLFW
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola Curvas Parametricas!", nullptr, nullptr);
    glfwMakeContextCurrent(window);


    // GLAD: carrega todos os ponteiros d funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    // Compilando e buildando o programa de shader
    Shader shader = Shader("./hello-curves.vs", "./hello-curves.fs");

    // Estrutura para armazenar a curva de Bézier e pontos de controle
    Curve curvaBezier;
    Curve curvaCatmullRom;

    std::vector<glm::vec3> controlPoints = generateHeartControlPoints();

    curvaBezier.controlPoints = controlPoints;  
    // curvaBezier.controlPoints = {
    // glm::vec3(-0.6, -0.4, 0.0),
	// glm::vec3(-0.4, -0.6, 0.0),
	// glm::vec3(-0.2, -0.2, 0.0),
	// glm::vec3(0.0, 0.0, 0.0),
	// glm::vec3(0.2, 0.2, 0.0),
	// glm::vec3(0.4, 0.6, 0.0),
	// glm::vec3(0.6, 0.4, 0.0),
    // };

    //Para os pontos de controle da Catmull Rom precisamos duplicar o primeiro e o último
    curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[0]);
    for (int i=0; i<curvaBezier.controlPoints.size(); i++)
    {
        curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[i]);
    }
    curvaCatmullRom.controlPoints.push_back(curvaBezier.controlPoints[curvaBezier.controlPoints.size()-1]);
    
    //curvaBezier.controlPoints = { glm::vec3(-0.8f, -0.4f, 0.0f), glm::vec3(-0.4f, 0.4f, 0.0f),
    //                              glm::vec3(0.4f, 0.4f, 0.0f), glm::vec3(0.8f, -0.4f, 0.0f) };

    // Gerar pontos da curva de Bézier
    int numCurvePoints = 100; // Quantidade de pontos por segmento na curva
    generateGlobalBezierCurvePoints(curvaBezier,numCurvePoints);
    //generateBezierCurvePoints(curvaBezier, numCurvePoints);
    generateCatmullRomCurvePoints(curvaCatmullRom, numCurvePoints);

    //Cria a grid de debug
    GeometryGrid grid = generateGrid();
    GeometryAxes axes = createAxesVAO();

    //Cria os buffers de geometria dos pontos da curva
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
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // cor de fundo
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Desenhar a grid
        drawGrid(grid,shader.ID);
        drawAxesVAO(axes,shader.ID);

        // Desenhar pontos da curva de Bezier e conectar com linhas
        glBindVertexArray(VAOBezierCurve);
        shader.setVec4("finalColor", 1.0f, 0.0f, 1.0f,1.0f); // Magenta para a curva
        glLineWidth(5.0);
        glDrawArrays(GL_LINE_STRIP, 0, curvaBezier.curvePoints.size()); // Desenha a curva como uma linha contínua
      
        // Desenhar pontos da curva de Catmull e conectar com linhas
        glBindVertexArray(VAOCatmullRomCurve);
        shader.setVec4("finalColor", 0.0f, 1.0f, 0.0f,1.0f); // Verde para a curva
        glDrawArrays(GL_LINE_STRIP, 0, curvaCatmullRom.curvePoints.size()); // Desenha a curva como uma linha contínua

         // Desenhar pontos de controle maiores e com cor diferenciada
        glBindVertexArray(VAOControl);
        shader.setVec4("finalColor", 0.0f, 0.0f, 0.0f,1.0f); // Preto para pontos de controle
        glPointSize(12.0f);
        glDrawArrays(GL_POINTS, 0, curvaBezier.controlPoints.size());
        

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
    //matrix[0] = glm::vec4(1.0f, -3.0f, 3.0f, -1.0f);
    //matrix[1] = glm::vec4(0.0f, 3.0f, -6.0f, 3.0f);
    //matrix[2] = glm::vec4(0.0f, 0.0f, 3.0f, -3.0f);
    //matrix[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    matrix[0] = glm::vec4(-1.0f, 3.0f, -3.0f, 1.0f); // Primeira coluna
    matrix[1] = glm::vec4(3.0f, -6.0f, 3.0f, 0.0f);  // Segunda coluna
    matrix[2] = glm::vec4(-3.0f, 3.0f, 0.0f, 0.0f);  // Terceira coluna
    matrix[3] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);    // Quarta coluna

}


void initializeCatmullRomMatrix(glm::mat4 &matrix)
{
    //matrix[0] = glm::vec4(-1.0f, 3.0f, -3.0f, 1.0f);
    //matrix[1] = glm::vec4(2.0f, -5.0f, 4.0f, -1.0f);
    //matrix[2] = glm::vec4(-1.0f, 0.0f, 1.0f, 0.0f);
    //matrix[3] = glm::vec4(0.0f, 2.0f, 0.0f, 0.0f);

    matrix[0] = glm::vec4(-0.5f, 1.5f, -1.5f, 0.5f);  // Primeira linha
    matrix[1] = glm::vec4(1.0f, -2.5f, 2.0f, -0.5f);   // Segunda linha
    matrix[2] = glm::vec4(-0.5f, 0.0f, 0.5f, 0.0f);   // Terceira linha
    matrix[3] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);    // Quarta linha
}

void generateBezierCurvePoints(Curve &curve, int numPoints) {
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    initializeBernsteinMatrix(curve.M);
    // Calcular os pontos ao longo da curva com base em Bernstein
     // Loop sobre os pontos de controle em grupos de 4

    float piece = 1.0/ (float) numPoints;
    float t;
    for (int i = 0; i < curve.controlPoints.size() - 3; i+=3) {
        
        // Gera pontos para o segmento atual
        for (int j = 0; j < numPoints;j++) {
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

void generateCatmullRomCurvePoints(Curve &curve, int numPoints) {
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    initializeCatmullRomMatrix(curve.M);
    // Calcular os pontos ao longo da curva com base em Bernstein
     // Loop sobre os pontos de controle em grupos de 4

    float piece = 1.0 / (float) numPoints;
    float t;
    for (int i = 0; i < curve.controlPoints.size()-3; i++) {
        
        // Gera pontos para o segmento atual
        for (int j = 0; j < numPoints;j++) {
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


GLuint generateControlPointsBuffer(vector <glm::vec3> controlPoints)
{
	GLuint VBO, VAO;

	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(GLfloat)* 3, controlPoints.data(), GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);

	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

GeometryGrid generateGrid(float cellSize) {
    GeometryGrid grid;
    grid.dimensions = glm::vec2(2.0f, 2.0f);  // Dimensões totais da grid de -1 a 1 em X e Y
    grid.initialPos = glm::vec2(-1.0f, -1.0f); // Posição inicial de desenho

    std::vector<glm::vec3> vertices;
    std::vector<GLuint> indices;

    int numCells = static_cast<int>(2.0f / cellSize); // Calcula o número de células de 0.1 entre -1 e 1

    // Gera os vértices da grid
    for (int i = 0; i <= numCells; i++) {
        float pos = grid.initialPos.x + i * cellSize;

        // Linhas verticais
        vertices.push_back(glm::vec3(pos, grid.initialPos.y, 0.0f));          // Parte inferior
        vertices.push_back(glm::vec3(pos, grid.initialPos.y + 2.0f, 0.0f));   // Parte superior

        // Linhas horizontais
        vertices.push_back(glm::vec3(grid.initialPos.x, pos, 0.0f));          // Parte esquerda
        vertices.push_back(glm::vec3(grid.initialPos.x + 2.0f, pos, 0.0f));   // Parte direita
    }

    // Índices de elementos para conectar as linhas
    for (int i = 0; i <= numCells; i++) {
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); // Desvincula o VAO atual

    // Limpeza
    glDeleteBuffers(1, &VBO);

    return grid;
}

void drawGrid(const GeometryGrid& grid, GLuint shaderID) {
    glUseProgram(shaderID);

    // Define a cor cinza médio para a grid
    GLint colorLocation = glGetUniformLocation(shaderID, "finalColor");
    glUniform4f(colorLocation, 0.5f, 0.5f, 0.5f, 1.0f); // RGBA: cinza médio

    // Ativa o VAO da grid
    glBindVertexArray(grid.VAO);

    //Largura da grid
    glLineWidth(1.0f);

    // Desenha a grid como linhas usando GL_LINES para contorno
    glDrawElements(GL_LINES, (grid.dimensions.x / 0.1f + 1) * 4, GL_UNSIGNED_INT, 0);

    // Desvincula o VAO
    glBindVertexArray(0);
}

GeometryAxes createAxesVAO() {
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); // Unbind VAO
    return axes;
}

void drawAxesVAO(const GeometryAxes& axes, GLuint shaderID) {
    glUseProgram(shaderID);

    // Desenha o eixo X em vermelho
    GLint colorLocation = glGetUniformLocation(shaderID, "finalColor");
    glUniform4f(colorLocation, 1.0f, 0.0f, 0.0f, 1.0f); // Cor vermelha

    //Largura dos eixos
    glLineWidth(3.0f);

    glBindVertexArray(axes.VAO);
    glDrawArrays(GL_LINES, 0, 2); // Desenha o eixo X

    // Desenha o eixo Y em azul
    glUniform4f(colorLocation, 0.0f, 0.0f, 1.0f, 1.0f); // Cor azul
    glDrawArrays(GL_LINES, 2, 2); // Desenha o eixo Y

    glBindVertexArray(0); // Unbind VAO
}

std::vector<glm::vec3> generateHeartControlPoints(int numPoints) {
    std::vector<glm::vec3> controlPoints;

    // Define o intervalo para t: de 0 a 2 * PI, dividido em numPoints
    float step = 2 * 3.14159 / (numPoints-1);

    for (int i = 0; i < numPoints-1; i++) {
        float t = i * step;

        // Calcula x(t) e y(t) usando as fórmulas paramétricas
        float x = 16 * pow(sin(t), 3);
        float y = 13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t);

        // Normaliza os pontos para mantê-los dentro de [-1, 1] no espaço 3D
        x /= 16.0f;  // Dividir por 16 para normalizar x entre -1 e 1
        y /= 16.0f;  // Dividir por 16 para normalizar y aproximadamente entre -1 e 1
        y += 0.15;
        // Adiciona o ponto ao vetor de pontos de controle
        controlPoints.push_back(glm::vec3(x, y, 0.0f));
    }
    controlPoints.push_back(controlPoints[0]);

    return controlPoints;
}

void generateGlobalBezierCurvePoints(Curve &curve, int numPoints) {
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    int n = curve.controlPoints.size() - 1; // Grau da curva
    float t;
    float piece = 1.0f / (float)numPoints;

    for (int j = 0; j <= numPoints; ++j) {
        t = j * piece;
        glm::vec3 point(0.0f); // Ponto na curva

        // Calcula o ponto da curva usando a fórmula de Bernstein
        for (int i = 0; i <= n; ++i) {
            // Coeficiente binomial
            float binomialCoeff = (float) (tgamma(n + 1) / (tgamma(i + 1) * tgamma(n - i + 1)));
            // Polinômio de Bernstein
            float bernsteinPoly = binomialCoeff * pow(1 - t, n - i) * pow(t, i);
            // Soma ponderada dos pontos de controle
            point += bernsteinPoly * curve.controlPoints[i];
        }

        curve.curvePoints.push_back(point);
    }
}

