#pragma once

#include <memory>
#include <map>
#include <random>
#include <pangolin/display/opengl_render_state.h>
#include <pangolin/display/viewport.h>
#include <pangolin/gl/gldraw.h>

namespace pangolin {

struct RenderParams
{
    RenderParams()
      : render_mode(GL_RENDER)
    {
    }

    GLint render_mode;
};

class Renderable
{
public:
    using guid_t = GLuint;

    static guid_t UniqueGuid()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        return (guid_t)gen();
    }

    Renderable(const std::weak_ptr<Renderable>& parent = std::weak_ptr<Renderable>())
        : guid(UniqueGuid()), parent(parent), T_pc(IdentityMatrix()), should_show(true)
    {
    }

    virtual ~Renderable()
    {
    }

    // Default implementation simply renders children.
    virtual void Render(const RenderParams& params = RenderParams()) {
        RenderChildren(params);
    }

    void RenderChildren(const RenderParams& params)
    {
        for(auto& p : children) {
            Renderable& r = *p.second;
            if(r.should_show) {
                r.T_pc.Multiply();
                r.Render(params);
            }
        }
    }

    std::shared_ptr<Renderable> FindChild(guid_t guid)
    {
        auto o = children.find(guid);
        if(o != children.end()) {
            return o->second;
        }

        for(auto& kv : children ) {
            std::shared_ptr<Renderable> c = kv.second->FindChild(guid);
            if(c) return c;
        }

        return std::shared_ptr<Renderable>();
    }

    Renderable& Add(const std::shared_ptr<Renderable>& child)
    {
        if(child) {
            children[child->guid] = child;
        };
        return *this;
    }

    // Renderable properties
    const guid_t guid;
    std::weak_ptr<Renderable> parent;
    pangolin::OpenGlMatrix T_pc;
    std::shared_ptr<Renderable> child;
    bool should_show;

    // Children
    std::map<guid_t, std::shared_ptr<Renderable>> children;
};

class Interactive
{
public:
    virtual bool Mouse(
        int button, const Eigen::Vector3d& win, const Eigen::Vector3d& obj,
        const Eigen::Vector3d& normal, bool pressed, int button_state, int pickId
    )  = 0;

    virtual bool MouseMotion(
        const Eigen::Vector3d& win, const Eigen::Vector3d& obj, const Eigen::Vector3d& normal, int button_state, int pickId
    ) = 0;

};

using SceneRoot = Renderable;

// Abstract
struct ColoredCube : public Renderable, public Interactive
{
    void Render(const RenderParams&) override {
        glPushName(guid);
        glDrawColouredCube();
        glPopName();
    }

    bool Mouse(
        int button, const Eigen::Vector3d& win, const Eigen::Vector3d& obj,
        const Eigen::Vector3d& normal, bool pressed, int button_state, int pickId
    ) override
    {
        std::cout << "Mouse" << std::endl;
        return false;
    }

    virtual bool MouseMotion(
        const Eigen::Vector3d& win, const Eigen::Vector3d& obj,
        const Eigen::Vector3d& normal, int button_state, int pickId
    ) override
    {
        std::cout << "MouseMotion" << std::endl;
        return false;
    }
};

}
