#pragma once
#include "Component.h"
#include <string>
#include <map>
#include <functional>

namespace AbsoluteEngine {

class ComponentFactory {
public:
    using CreatorFunc = std::function<std::unique_ptr<IComponent>()>;

    static ComponentFactory& GetInstance() {
        static ComponentFactory instance;
        return instance;
    }

    void Register(const std::string& name, CreatorFunc func) {
        creators_[name] = func;
    }

    std::unique_ptr<IComponent> Create(const std::string& name) {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            return it->second();
        }
        return nullptr;
    }

    std::vector<std::string> GetRegisteredComponentNames() const {
        std::vector<std::string> names;
        for (const auto& pair : creators_) {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    ComponentFactory() = default;
    std::map<std::string, CreatorFunc> creators_;
};

} // namespace AbsoluteEngine
