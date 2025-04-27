// OpenGL simulation of the Graviton Field Theory

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cmath>

struct TrailPoint {
    glm::vec3 position;
    glm::vec3 color;
};

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const int GRID_SIZE = 64;
const float GRAVITON_SPACING = 1.0f;

struct Graviton {
    glm::vec3 position;
    glm::vec3 momentum;
    glm::vec3 accumulatedForce;
};

struct MassBody {
    glm::vec3 position;
    glm::vec3 velocity;
    float mass;
    glm::vec3 color;
};

std::vector<Graviton> field;
std::vector<MassBody> masses;
std::vector<std::vector<TrailPoint>> massTrails;
const size_t MAX_TRAIL_LENGTH = 100;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Camera state
glm::vec3 fieldCenter = glm::vec3(GRID_SIZE / 2.0f) * GRAVITON_SPACING;
glm::vec3 cameraPos = fieldCenter + glm::vec3(20.0f, 20.0f, 20.0f);
glm::vec3 cameraFront = glm::normalize(fieldCenter - cameraPos);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float yaw = -135.0f;
float pitch = -35.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool paused = true;

void processInput(GLFWwindow* window) {
    float cameraSpeed = 10.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    static bool spacePressedLastFrame = false;
    bool spacePressedNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spacePressedNow && !spacePressedLastFrame) paused = !paused;
    spacePressedLastFrame = spacePressedNow;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = float(xpos);
        lastY = float(ypos);
        firstMouse = false;
    }

    float xoffset = float(xpos) - lastX;
    float yoffset = lastY - float(ypos);
    lastX = float(xpos);
    lastY = float(ypos);

    float sensitivity = 0.1f;
    #ifdef __linux__
        sensitivity = 0.001f; 
    #endif
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    front.y = sin(glm::radians(pitch));
    front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    cameraFront = glm::normalize(front);
}

void initField() {
    field.clear();
    for (int x = 0; x < GRID_SIZE; ++x) {
        for (int y = 0; y < GRID_SIZE; ++y) {
            for (int z = 0; z < GRID_SIZE; ++z) {
                glm::vec3 pos = glm::vec3(x, y, z) * GRAVITON_SPACING;
                field.push_back({
                    .position = pos,
                    .momentum = glm::vec3(0.0f),
                    .accumulatedForce = glm::vec3(0.0f)
                });
            }
        }
    }
}

void initMasses() {
    masses.clear();
    float scale = 5.0f;
    float velScale = 0.05f;

    // Sun 
    {
        glm::vec3 pos = fieldCenter;
        masses.push_back({ pos, glm::vec3(0), 1000.0f, glm::vec3(1.0f, 1.0f, 0.0f) });
    }

    // Mercury 
    {
        glm::vec3 pos = fieldCenter + glm::vec3(scale * 0.39f, 0, 0);
        glm::vec3 vel = glm::normalize(glm::cross(glm::vec3(0,1,0), glm::normalize(pos - fieldCenter))) * velScale * 1.6f;
        masses.push_back({ pos, vel, 0.000165f, glm::vec3(0.8f, 0.8f, 0.8f) });
    }

    // Venus 
    {
        glm::vec3 pos = fieldCenter + glm::vec3(scale * 0.72f, 0, 0);
        glm::vec3 vel = glm::normalize(glm::cross(glm::vec3(0,1,0), glm::normalize(pos - fieldCenter))) * velScale * 1.2f;
        masses.push_back({ pos, vel, 0.00245f, glm::vec3(1.0f, 0.8f, 0.5f) });
    }

    // Earth 
    {
        glm::vec3 pos = fieldCenter + glm::vec3(scale * 1.0f, 0, 0);
        glm::vec3 vel = glm::normalize(glm::cross(glm::vec3(0,1,0), glm::normalize(pos - fieldCenter))) * velScale;
        masses.push_back({ pos, vel, 0.003f, glm::vec3(0.0f, 0.5f, 1.0f) });
    }

    // Mars 
    {
        glm::vec3 pos = fieldCenter + glm::vec3(scale * 1.52f, 0, 0);
        glm::vec3 vel = glm::normalize(glm::cross(glm::vec3(0,1,0), glm::normalize(pos - fieldCenter))) * velScale * 0.8f;
        masses.push_back({ pos, vel, 0.000323f, glm::vec3(1.0f, 0.3f, 0.3f) });
    }

    // Jupiter 
    {
        glm::vec3 pos = fieldCenter + glm::vec3(scale * 5.2f, 0, 0);
        glm::vec3 vel = glm::normalize(glm::cross(glm::vec3(0,1,0), glm::normalize(pos - fieldCenter))) * velScale * 0.44f;
        masses.push_back({ pos, vel, 0.954f, glm::vec3(1.0f, 0.9f, 0.6f) });
    }

    // These require a larger field which in turn requires better parallelization
    // // Saturn 
    // {
    //     glm::vec3 pos = fieldCenter + glm::vec3(scale * 9.58f, 0, 0);
    //     glm::vec3 vel = glm::normalize(glm::cross(glm::vec3(0,1,0), glm::normalize(pos - fieldCenter))) * velScale * 0.32f;
    //     masses.push_back({ pos, vel, 0.2857f, glm::vec3(1.0f, 0.85f, 0.5f) });
    // }

    // // Uranus 
    // {
    //     glm::vec3 pos = fieldCenter + glm::vec3(scale * 19.2f, 0, 0);
    //     glm::vec3 vel = glm::normalize(glm::cross(glm::vec3(0,1,0), glm::normalize(pos - fieldCenter))) * velScale * 0.23f;
    //     masses.push_back({ pos, vel, 0.0436f, glm::vec3(0.6f, 1.0f, 1.0f) });
    // }

    // // Neptune 
    // {
    //     glm::vec3 pos = fieldCenter + glm::vec3(scale * 30.05f, 0, 0);
    //     glm::vec3 vel = glm::normalize(glm::cross(glm::vec3(0,1,0), glm::normalize(pos - fieldCenter))) * velScale * 0.19f;
    //     masses.push_back({ pos, vel, 0.0515f, glm::vec3(0.3f, 0.3f, 1.0f) });
    // }

    // // Pluto 
    // {
    //     glm::vec3 pos = fieldCenter + glm::vec3(scale * 39.48f, 0, 0);
    //     glm::vec3 vel = glm::normalize(glm::cross(glm::vec3(0,1,0), glm::normalize(pos - fieldCenter))) * velScale * 0.16f;
    //     masses.push_back({ pos, vel, 0.0000655f, glm::vec3(0.7f, 0.6f, 0.6f) });
    // }

    massTrails.clear();
    for (const auto& m : masses) {
        std::ignore = m;
        massTrails.push_back({});
    }
}

void updateField() {
    // Reset accumulated forces on gravitons
    #pragma omp parallel for
    for (auto& g : field) {
        g.accumulatedForce = glm::vec3(0.0f);
    }

    // For each mass, propagate influence to nearby gravitons
    for (const auto& m : masses) {
        #pragma omp parallel for
        for (auto& g : field) {
            glm::vec3 dir = g.position - m.position;
            float dist = glm::length(dir);
            if (dist < 0.0001f) continue;

            glm::vec3 influence = dir * (m.mass / (dist * dist * dist));
            g.accumulatedForce += influence;
        }
    }

    // Update graviton momentum with accumulated forces and damping
    #pragma omp parallel for
    for (auto& g : field) {
        g.momentum = g.accumulatedForce;
    }
}

void updateMasses() {
    // For each mass, accumulate forces from nearby gravitons
    #pragma omp parallel for
    for (auto& m : masses) {
        glm::vec3 totalForce(0.0f);
        #pragma omp parallel for
        for (const auto& g : field) {
            glm::vec3 dir = g.position - m.position;
            float dist = glm::length(dir);
            if (dist < 0.0001f) continue;

            glm::vec3 force = dir / (glm::length(g.momentum) / (dist * dist));
            totalForce += force;
        }
        // Update velocity and position of mass
        glm::vec3 acceleration = totalForce / m.mass;
        // m.velocity += acceleration * deltaTime * 0.000001f;
        // m.position += m.velocity * deltaTime;
        m.velocity += acceleration * 1.0f * 0.000000000000001f;
        m.position += m.velocity * 1.0f;
    }

    #pragma omp parallel for
    for (size_t i = 0; i < masses.size(); ++i) {
        TrailPoint tp;
        tp.position = masses[i].position * GRAVITON_SPACING;
        tp.color = masses[i].color;
        massTrails[i].push_back(tp);
        if (massTrails[i].size() > MAX_TRAIL_LENGTH) {
            massTrails[i].erase(massTrails[i].begin());
        }
    }
}

GLuint fieldVAO = 0, fieldVBO = 0;
GLuint massVAO = 0, massVBO = 0;
GLuint shaderProgram = 0;

const char* vertexShaderSource = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 uVP;
out vec3 vColor;
void main() {
    gl_Position = uVP * vec4(aPos, 1.0);
    vColor = aColor;
    gl_PointSize = 10.0;
}
)";

const char* fragmentShaderSource = R"(
#version 410 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        std::cerr << "Shader compilation failed: " << info << std::endl;
    }
    return shader;
}

void setupShaders() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, info);
        std::cerr << "Shader linking failed: " << info << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void setupBuffers() {
    glGenVertexArrays(1, &fieldVAO);
    glGenBuffers(1, &fieldVBO);
    glBindVertexArray(fieldVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fieldVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 2 * field.size(), nullptr, GL_DYNAMIC_DRAW); // conservative size
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    glGenVertexArrays(1, &massVAO);
    glGenBuffers(1, &massVBO);
    glBindVertexArray(massVAO);
    glBindBuffer(GL_ARRAY_BUFFER, massVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * masses.size(), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
}

void renderField(const glm::mat4& vp) {
    // Prepare line data: for each graviton with momentum, two vertices (start/end) with color
    std::vector<float> lineData;
    for (const auto& g : field) {
        glm::vec3 p1 = g.position * GRAVITON_SPACING;
        if (glm::length(g.momentum) > 0.01f) {
            float intensity = glm::clamp(glm::length(g.momentum) * 10.0f, 0.0f, 1.0f) * 0.01f;
            glm::vec3 color = glm::mix(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), intensity);
            glm::vec3 p2 = p1 + glm::normalize(g.momentum) * 0.5f;

            lineData.push_back(p1.x); lineData.push_back(p1.y); lineData.push_back(p1.z);
            lineData.push_back(color.r); lineData.push_back(color.g); lineData.push_back(color.b);

            lineData.push_back(p2.x); lineData.push_back(p2.y); lineData.push_back(p2.z);
            lineData.push_back(color.r); lineData.push_back(color.g); lineData.push_back(color.b);
        }
    }
    glBindVertexArray(fieldVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fieldVBO);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgram);
    GLint vpLoc = glGetUniformLocation(shaderProgram, "uVP");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, glm::value_ptr(vp));
    glDrawArrays(GL_LINES, 0, (GLsizei)(lineData.size() / 6));
    glBindVertexArray(0);
}

void renderMasses(const glm::mat4& vp) {
    std::vector<float> massData;
    for (const auto& m : masses) {
        glm::vec3 pos = m.position * GRAVITON_SPACING;
        glm::vec3 color = m.color;
        float radius = std::cbrt(m.mass) * 0.001f;
        massData.push_back(pos.x); massData.push_back(pos.y); massData.push_back(pos.z);
        massData.push_back(color.r); massData.push_back(color.g); massData.push_back(color.b);
        glPointSize(radius * 100.0f);
    }
    glBindVertexArray(massVAO);
    glBindBuffer(GL_ARRAY_BUFFER, massVBO);
    glBufferData(GL_ARRAY_BUFFER, massData.size() * sizeof(float), massData.data(), GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgram);
    GLint vpLoc = glGetUniformLocation(shaderProgram, "uVP");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, glm::value_ptr(vp));
    glDrawArrays(GL_POINTS, 0, (GLsizei)(massData.size() / 6));
    glBindVertexArray(0);
}

void renderTrails(const glm::mat4& vp) {
    std::vector<float> trailData;
    for (const auto& trail : massTrails) {
        for (const auto& tp : trail) {
            trailData.push_back(tp.position.x);
            trailData.push_back(tp.position.y);
            trailData.push_back(tp.position.z);
            trailData.push_back(tp.color.r);
            trailData.push_back(tp.color.g);
            trailData.push_back(tp.color.b);
        }
    }
    GLuint trailVAO, trailVBO;
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);
    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    glBufferData(GL_ARRAY_BUFFER, trailData.size() * sizeof(float), trailData.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glUseProgram(shaderProgram);
    GLint vpLoc = glGetUniformLocation(shaderProgram, "uVP");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, glm::value_ptr(vp));
    int offset = 0;
    for (const auto& trail : massTrails) {
        glDrawArrays(GL_LINE_STRIP, offset, (GLsizei)trail.size());
        offset += (GLsizei)trail.size();
    }

    glDeleteVertexArrays(1, &trailVAO);
    glDeleteBuffers(1, &trailVBO);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Graviton Field Sim", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    initMasses();
    initField();

    setupShaders();
    setupBuffers();

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = float(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!paused) {
            updateField();
            updateMasses();
        }

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 vp = projection * view;

        renderField(vp);
        renderMasses(vp);
        renderTrails(vp);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
