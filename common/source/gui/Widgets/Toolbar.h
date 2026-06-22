
#pragma once
#include "Widget.h"
#include <string>
#include <glm/glm.hpp>

namespace oscilleon::gui {

class Toolbar : public Widget {
public:
    Toolbar() = default;

    void SetCompanyName(const std::string& name) { m_company = name; }
    void SetPluginName(const std::string& name) { m_plugin = name; }

    void Render(Renderer& renderer) override;


private:
    std::string m_company{ "DEV" };
    std::string m_plugin{ "PLUGIN" };
};

} // namespace oscilleon::gui