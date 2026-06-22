#include "Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace oscilleon::gui {

Shader::Shader() {}

Shader::~Shader() {
    if (m_program) glDeleteProgram(m_program);
}

u32 Shader::Compile(u32 type, const std::string& src) {
    u32 id = glCreateShader(type);
    const char* s = src.c_str();
    glShaderSource(id, 1, &s, nullptr);
    glCompileShader(id);

    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(id, 1024, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

bool Shader::LoadFromSource(const std::string& vs, const std::string& fs) {
    unsigned int vert = Compile(GL_VERTEX_SHADER, vs);
    unsigned int frag = Compile(GL_FRAGMENT_SHADER, fs);
    if (!vert || !frag) return false;

    m_program = glCreateProgram();
    glAttachShader(m_program, vert);
    glAttachShader(m_program, frag);
    glLinkProgram(m_program);

    glDeleteShader(vert);
    glDeleteShader(frag);

    int success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    return success != 0;
}

bool Shader::LoadFromFile(const std::string& vsPath, const std::string& fsPath) {
    std::ifstream vsFile(vsPath);
    std::ifstream fsFile(fsPath);

    if (!vsFile.is_open() || !fsFile.is_open()) return false;

    std::stringstream vsStream, fsStream;
    vsStream << vsFile.rdbuf();
    fsStream << fsFile.rdbuf();

    return LoadFromSource(vsStream.str(), fsStream.str());
}

void Shader::Bind() const { 
    glUseProgram(m_program); 
}

void Shader::Unbind() const { 
    glUseProgram(0); 
}

void Shader::SetMat4(const std::string& name, const m4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(m_program, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetVec4(const std::string& name, const v4& v) const {
    glUniform4fv(glGetUniformLocation(m_program, name.c_str()), 1, &v[0]);
}

void Shader::SetVec2(const std::string& name, const v2& v) const {
    glUniform2fv(glGetUniformLocation(m_program, name.c_str()), 1, &v[0]);
}

void Shader::SetFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_program, name.c_str()), value);
}

void Shader::SetInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_program, name.c_str()), value);
}

void Shader::SetBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(m_program, name.c_str()), (int)value);
}

}