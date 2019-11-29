#include <cstdio>
#include <string>
#include <fstream>
#include <iostream>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/vec3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/mat4x4.hpp>

double gettime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec * 1e-9;
}

size_t getFileSize(const char* path) {
    struct stat fs;
    stat(path, &fs);
    return fs.st_size;
}

std::string readEntireFile(const char* path) {
    std::string str;
    std::ifstream stream(path);
    size_t size = getFileSize(path);
    str.resize(size, 0);
    stream.read(&str[0], size);
    stream.close();

    return str;
}

struct GLMesh {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    int numElements;
};

GLMesh createGLMesh(const aiMesh* aimesh) {
    GLMesh mesh;

    mesh.numElements = aimesh->mNumFaces * 3;

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ibo);

    struct Vert {
        glm::vec3 pos;
        glm::vec3 norm;
    };
    auto* verts = new Vert[aimesh->mNumVertices];
    auto* indices = new GLuint[aimesh->mNumFaces * 3];

    for (int i = 0; i < aimesh->mNumVertices; ++i) {
        Vert& vert = verts[i];
        auto& pos = aimesh->mVertices[i];
        auto& norm = aimesh->mNormals[i];
        vert.pos = {pos.x, pos.y, pos.z};
        vert.norm = {norm.x, norm.y, norm.z};
    }
    for (int i = 0; i < aimesh->mNumFaces; ++i) {
        const auto* aiindices = aimesh->mFaces[i].mIndices;
        indices[i * 3 + 0] = aiindices[0];
        indices[i * 3 + 1] = aiindices[1];
        indices[i * 3 + 2] = aiindices[2];
    }

    glBindVertexArray(mesh.vao);
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (const GLvoid*) (GLintptr) offsetof(Vert, pos));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), (const GLvoid*) (GLintptr) offsetof(Vert, norm));
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vert) * aimesh->mNumVertices, verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * aimesh->mNumFaces * 3, indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    delete[] verts;
    delete[] indices;

    return mesh;
}

void disposeGLMesh(GLMesh& mesh) {
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ibo);
}

void drawGLMesh(const GLMesh& mesh) {
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.numElements, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

bool handleGLShaderError(GLuint shader) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint errlen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &errlen);
        std::string err;
        err.resize(errlen + 1, 0);
        glGetShaderInfoLog(shader, errlen, nullptr, &err[0]);
        fprintf(stderr, "shader error:\n%s\n", err.c_str());
        return true;
    }
    return false;
}
bool handleGLProgramError(GLuint program) {
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint errlen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &errlen);
        std::string err;
        err.resize(errlen + 1, 0);
        glGetProgramInfoLog(program, errlen, nullptr, &err[0]);
        fprintf(stderr, "program error:%s\n", err.c_str());
        return true;
    }
    return false;
}

GLuint createProgram(const char* vertsrc, const char* fragsrc) {
    bool err = false;
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertsrc, nullptr);
    glCompileShader(vert);
    err |= handleGLShaderError(vert);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragsrc, nullptr);
    glCompileShader(frag);
    err |= handleGLShaderError(frag);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    err |= handleGLProgramError(prog);

    glDeleteShader(vert);
    glDeleteShader(frag);
    if (err) {
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

int main(int, char**) {
    int width = 1280;
    int height = 720;


    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window;
    window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, ctx);
    glewInit();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("../shape.fbx", aiProcess_Triangulate);
    const aiMesh* aimesh = scene->mMeshes[0];

    GLMesh mesh = createGLMesh(aimesh);

    GLuint meshprog = 
        createProgram(
            readEntireFile("../shaders/mesh_v.glsl").c_str(),
            readEntireFile("../shaders/mesh_f.glsl").c_str());
    GLuint u_mvp = glGetUniformLocation(meshprog, "u_mvp");

    GLuint rectprog =
        createProgram(
            readEntireFile("../shaders/rect_v.glsl").c_str(),
            readEntireFile("../shaders/rect_f.glsl").c_str());
    GLuint u_backimg = glGetUniformLocation(rectprog, "u_backimg");
    GLuint u_frontimg = glGetUniformLocation(rectprog, "u_frontimg");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LEQUAL);


    GLfloat verts[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        -1.0f, +1.0f, 0.0f, 1.0f,
        +1.0f, +1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        +1.0f, +1.0f, 1.0f, 1.0f,
        +1.0f, -1.0f, 1.0f, 0.0f
    };
    GLuint rectvao;
    GLuint rectvbo;
    glGenVertexArrays(1, &rectvao);
    glGenBuffers(1, &rectvbo);
    glBindVertexArray(rectvao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, rectvbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid*) (GLintptr) (sizeof(GLfloat) * 0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const GLvoid*) (GLintptr) (sizeof(GLfloat) * 2));
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    GLuint fbdepthtex_back;
    glGenTextures(1, &fbdepthtex_back);
    glBindTexture(GL_TEXTURE_2D, fbdepthtex_back);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32, width, height);

    GLuint framebuffer_back;
    glGenFramebuffers(1, &framebuffer_back);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_back);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbdepthtex_back, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint fbdepthtex_front;
    glGenTextures(1, &fbdepthtex_front);
    glBindTexture(GL_TEXTURE_2D, fbdepthtex_front);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32, width, height);

    GLuint fbstenciltex;
    glGenTextures(1, &fbstenciltex);
    glBindTexture(GL_TEXTURE_2D, fbstenciltex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_STENCIL_INDEX8, width, height);

    GLuint framebuffer_front;
    glGenFramebuffers(1, &framebuffer_front);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_front);
    glBindTexture(GL_TEXTURE_2D, fbdepthtex_front);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbdepthtex_front, 0);
    glBindTexture(GL_TEXTURE_2D, fbstenciltex);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fbstenciltex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glStencilFunc(GL_EQUAL, 1, 0xff);


    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float) width / height, 0.01f, 10.0f);
    
    double elapsed = 0.0;
    double last = gettime();

    glClearColor(0.0, 0.6, 1.0, 1.0);
    bool running = true;
    while (running) {
        double now = gettime();
        double delta = now - last;
        elapsed += delta;
        last = now;

        for (SDL_Event e; SDL_PollEvent(&e);) {
            if (e.type == SDL_QUIT) {
                running = false;

            }
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 model = 
            glm::translate(glm::mat4 {1.0f}, glm::vec3 {0.0f, 0.0f, -2.0f}) *
            glm::rotate(glm::mat4 {1.0f}, glm::radians((float) elapsed * 45.0f), glm::vec3 {0.5f, 1.0f, 0.0f}) *
            glm::scale(glm::mat4 {1.0f}, glm::vec3 {1.0f});
        ;

        glm::mat4 mvp = proj * model;
        
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        // glDepthFunc(GL_LEQUAL);
        // glClearDepth(1.0f);

        GLenum buffers[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
        glDrawBuffers(1, buffers);

        glUseProgram(meshprog);
        glDrawBuffer(GL_DEPTH_ATTACHMENT);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_back);
        glClearDepth(0.0f);
        glDepthFunc(GL_GEQUAL);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT);
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (const GLfloat*) &mvp);
        drawGLMesh(mesh);
        glUseProgram(0);

        glDrawBuffers(1, buffers);


        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_front);
        glClearDepth(1.0f);
        glDepthFunc(GL_LEQUAL);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_BACK);
        glUseProgram(meshprog);
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (const GLfloat*) &mvp);
        drawGLMesh(mesh);
        glUseProgram(0);

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);


        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(rectprog);
        glBindTexture(GL_TEXTURE_2D, fbdepthtex_back);
        glActiveTexture(GL_TEXTURE0 + 0);
        glUniform1i(u_backimg, 0);
        glBindTexture(GL_TEXTURE_2D, fbdepthtex_front);
        glActiveTexture(GL_TEXTURE0 + 1);
        glUniform1i(u_frontimg, 1);

        glBindVertexArray(rectvao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glUseProgram(0);


        SDL_GL_SwapWindow(window);
    }

    glDeleteVertexArrays(1, &rectvao);
    glDeleteBuffers(1, &rectvbo);

    glDeleteFramebuffers(1, &framebuffer_back);
    glDeleteFramebuffers(1, &framebuffer_front);
    glDeleteTextures(1, &fbdepthtex_back);
    glDeleteTextures(1, &fbdepthtex_front);
    disposeGLMesh(mesh);
    glDeleteProgram(meshprog);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
