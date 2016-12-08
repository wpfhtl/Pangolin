#pragma once

#include <memory>
#include <map>
#include <random>
#include <pangolin/display/opengl_render_state.h>
#include <pangolin/display/viewport.h>
#include <pangolin/gl/gldraw.h>

namespace pangolin {

class Renderable
{
public:
    using guid_t = std::mt19937::result_type;

    struct Child
    {
        pangolin::OpenGlMatrix T_pc;
        std::shared_ptr<Renderable> child;
        bool should_show;
    };

    static guid_t UniqueGuid()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        return gen();
    }

    Renderable(const std::weak_ptr<Renderable>& parent = std::weak_ptr<Renderable>())
        : guid(UniqueGuid()), parent(parent)
    {
    }

    virtual ~Renderable()
    {
    }

    // Default implementation simply renders children.
    virtual void Render() {
        RenderChildren();
    }

    void RenderChildren()
    {
        for(auto& p : children) {
            Child& c = p.second;
            if(c.should_show && c.child) {
                c.T_pc.Multiply();
                c.child->Render();
            }
        }
    }

    Renderable& Add(const std::shared_ptr<Renderable>& child)
    {
        if(child) {
            children[child->guid] = {IdentityMatrix(), child, true};
        };
        return *this;
    }

    const guid_t guid;

    std::weak_ptr<Renderable> parent;
    std::map<guid_t, Child> children;
};

using SceneRoot = Renderable;

// Abstract
struct ColoredCube : public Renderable
{
    virtual void Render() {
        glDrawColouredCube();
    }
};

}
