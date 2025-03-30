#include <iostream>
#include "shader.h"
#include "utils.h"

//构造函数和析构函数
Shader::Shader() : mProgram(0) {
}

Shader::~Shader() {
    if (mProgram) {
        glDeleteProgram(mProgram);
    }
}

//检查着色器编译或程序链接错误
bool Shader::checkCompileErrors(GLuint shader, std::string type) {
    GLint success = 0;
    GLchar infoLog[1024] = { 0 };//存储错误信息
    if (type != "PROGRAM") {//如果是着色器编译，检查编译状态
        GL_CALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));//这里有个成功与否的变量
        if (!success) {
            GL_CALL(glGetShaderInfoLog(shader, 1024, nullptr, infoLog));
            errorf("SHADER_COMPILATION_ERROR of type: %s %s", type.c_str(), infoLog);
            return false;
        }//失败会获取错误日志打印错误信息
    } else {//如果是程序链接，检查链接状态
        GL_CALL(glGetProgramiv(shader, GL_LINK_STATUS, &success));
        if (!success) {
            GL_CALL(glGetProgramInfoLog(shader, 1024, nullptr, infoLog));
            errorf("PROGRAM_LINKING_ERROR of type: %s %s", type.c_str(), infoLog);
            return false;
        }
    }
    return true;
}//Q1：程序链接和着色器编译的区别是啥 Q2：GL_CALL有何用

//加载并编译着色器
bool Shader::loadShader(const char* vertexShaderCode, const char* fragmentShaderCode) {
    //查询GPU支持的顶点着色器和片段着色器的最大Uniform变量数量
    int maxVertexUniform, maxFragmentUniform;
    GL_CALL(glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniform));
    GL_CALL(glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniform));
    //infof("maxVertexUniform:%d, maxFragmentUniform:%d", maxVertexUniform, maxFragmentUniform);

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    //绑定GLSL源码glShaderSource，编译glCompileShader，下面的片段着色器同理
    GL_CALL(glShaderSource(vertex, 1, &vertexShaderCode, nullptr));
    GL_CALL(glCompileShader(vertex));
    if (!checkCompileErrors(vertex, "VERTEX")) {
        return false;
    }
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    GL_CALL(glShaderSource(fragment, 1, &fragmentShaderCode, nullptr));
    GL_CALL(glCompileShader(fragment));
    if (!checkCompileErrors(fragment, "FRAGMENT")) {
        return false;
    }
    //创建程序后附加顶点和片段着色器，连接程序并检查错误
    mProgram = glCreateProgram();
    GL_CALL(glAttachShader(mProgram, vertex));
    GL_CALL(glAttachShader(mProgram, fragment));
    GL_CALL(glLinkProgram(mProgram));
    if (!checkCompileErrors(mProgram, "PROGRAM")) {
        return false;
    }
    //删除临时着色器对象
    GL_CALL(glDeleteShader(vertex));
    GL_CALL(glDeleteShader(fragment));
    return true;
}

//激活当前着色器程序
void Shader::use() const {
    GL_CALL(glUseProgram(mProgram));
}

//返回着色器程序ID
GLuint Shader::id() const {
    return mProgram;
}

//Uniform 变量设置
void Shader::setUniformBool(const std::string& name, bool value) const {
    GL_CALL(glUniform1i(glGetUniformLocation(mProgram, name.c_str()), (int)value));
}

void Shader::setUniformInt(const std::string& name, int value) const {
    GL_CALL(glUniform1i(glGetUniformLocation(mProgram, name.c_str()), value));
}

void Shader::setUniformFloat(const std::string& name, float value) const {
    GL_CALL(glUniform1f(glGetUniformLocation(mProgram, name.c_str()), value));
}

//Q：glm::vec2是啥玩意
void Shader::setUniformVec2(const std::string& name, const glm::vec2& value) const {
    GL_CALL(glUniform2fv(glGetUniformLocation(mProgram, name.c_str()), 1, &value[0]));
}

void Shader::setUniformVec2(const std::string& name, float x, float y) const {
    GL_CALL(glUniform2f(glGetUniformLocation(mProgram, name.c_str()), x, y));
}

void Shader::setUniformVec3(const std::string& name, const glm::vec3& value) const {
    GL_CALL(glUniform3fv(glGetUniformLocation(mProgram, name.c_str()), 1, &value[0]));
}

void Shader::setUniformVec3(const std::string& name, float x, float y, float z) const {
    GL_CALL(glUniform3f(glGetUniformLocation(mProgram, name.c_str()), x, y, z));
}

void Shader::setUniformVec4(const std::string& name, const glm::vec4& value) const {
    GL_CALL(glUniform4fv(glGetUniformLocation(mProgram, name.c_str()), 1, &value[0]));
}

void Shader::setUniformVec4(const std::string& name, float x, float y, float z, float w) const {
    GL_CALL(glUniform4f(glGetUniformLocation(mProgram, name.c_str()), x, y, z, w));
}

void Shader::setUniformMat2(const std::string& name, const glm::mat2& mat) const {
    GL_CALL(glUniformMatrix2fv(glGetUniformLocation(mProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]));
}

void Shader::setUniformMat3(const std::string& name, const glm::mat3& mat) const {
    GL_CALL(glUniformMatrix3fv(glGetUniformLocation(mProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]));
}

void Shader::setUniformMat4(const std::string& name, const glm::mat4& mat) const {
    GL_CALL(glUniformMatrix4fv(glGetUniformLocation(mProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]));
}

GLuint Shader::getAttribLocation(const std::string& name) const {
    return glGetAttribLocation(mProgram, name.c_str());
}
