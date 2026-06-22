#pragma once
#include <string>
#include <glm/glm.hpp>
#include "RendererTypes.h"

namespace oscilleon::gui {

class Shader {
public:
    Shader();
    ~Shader();

    bool LoadFromSource(const std::string& vs, const std::string& fs);
    bool LoadFromFile(const std::string& vsPath, const std::string& fsPath);
    void Bind() const;
    void Unbind() const;
    void SetMat4(const std::string& name, const m4& mat) const;
    void SetVec4(const std::string& name, const v4& v) const;
    void SetVec2(const std::string& name, const v2& v) const;
    void SetFloat(const std::string& name, float value) const;
    void SetInt(const std::string& name, int value) const;
    void SetBool(const std::string& name, bool value) const;
    u32 GetID() const { return m_program; }

private:
    u32 Compile(u32 type, const std::string& src);
    u32 m_program{ 0 };
};

}