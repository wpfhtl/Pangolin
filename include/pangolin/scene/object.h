#pragma once

#include <memory>
#include <map>
#include <random>
#include <pangolin/display/opengl_render_state.h>
#include <pangolin/display/viewport.h>
#include <pangolin/gl/gldraw.h>
#include <Eigen/Geometry>

namespace pangolin {

struct RenderParams
{
    RenderParams()
      : render_mode(GL_RENDER)
    {
    }

    GLint render_mode;
};

struct Interactive
{
    static thread_local GLuint current_id;

    virtual bool Mouse(
        int button, const Eigen::Vector3d& win, const Eigen::Vector3d& obj,
        const Eigen::Vector3d& normal, bool pressed, int button_state, int pickId
    )  = 0;

    virtual bool MouseMotion(
        const Eigen::Vector3d& win, const Eigen::Vector3d& obj, const Eigen::Vector3d& normal, int button_state, int pickId
    ) = 0;
};

struct Manipulator : public Interactive
{
    virtual void Render(const RenderParams& params) = 0;
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
                glPushMatrix();
                r.T_pc.Multiply();
                r.Render(params);
                if(r.manipulator) {
                    r.manipulator->Render(params);
                }
                glPopMatrix();
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

    // Manipulator (handler, thing)
    std::shared_ptr<Manipulator> manipulator;
};

class InteractiveIndex
{
public:
    class Token
    {
    public:
        friend class InteractiveIndex;

        Token()
            : id(0)
        {
        }

        Token(Token&& o)
            : id(o.id)
        {
            o.id = 0;
        }

        GLuint Id() const
        {
            return id;
        }

        ~Token()
        {
            if(id) {
                InteractiveIndex::I().Unstore(*this);
            }
        }

    private:
        Token(GLuint id)
            : id(id)
        {
        }


        GLuint id;
    };

    static InteractiveIndex& I()
    {
        static InteractiveIndex instance;
        return instance;
    }

    Interactive* Find(GLuint id)
    {
        auto kv = index.find(id);
        if(kv != index.end()) {
            return kv->second;
        }
        return nullptr;
    }

    Token Store(Interactive* r)
    {
        index[next_id] = r;
        return Token(next_id++);
    }

    void Unstore(Token& t)
    {
        index.erase(t.id);
        t.id = 0;
    }

private:
    // Private constructor.
    InteractiveIndex()
        : next_id(1)
    {
    }

    GLuint next_id;
    std::map<GLuint, Interactive*> index;
};

// Abstract
struct Axis : public Renderable, public Interactive
{
    Axis()
        : axis_length(1.0),
          label_x(InteractiveIndex::I().Store(this)),
          label_y(InteractiveIndex::I().Store(this)),
          label_z(InteractiveIndex::I().Store(this))
    {
    }

    void Render(const RenderParams&) override {
        glColor4f(1,0,0,1);
        glPushName(label_x.Id());
        glDrawLine(0,0,0, axis_length,0,0);
        glPopName();

        glColor4f(0,1,0,1);
        glPushName(label_y.Id());
        glDrawLine(0,0,0, 0,axis_length,0);
        glPopName();

        glColor4f(0,0,1,1);
        glPushName(label_z.Id());
        glDrawLine(0,0,0, 0,0,axis_length);
        glPopName();
    }

    bool Mouse(
        int button, const Eigen::Vector3d& win, const Eigen::Vector3d& obj,
        const Eigen::Vector3d& normal, bool pressed, int button_state, int pickId
    ) override
    {
        if((button == MouseWheelUp || button == MouseWheelDown) ) {
            float scale = (button == MouseWheelUp) ? 0.01 : -0.01;
            if(button_state & KeyModifierShift) scale /= 10;

            Eigen::Vector3d rot = Eigen::Vector3d::Zero();
            Eigen::Vector3d xyz = Eigen::Vector3d::Zero();


            if(button_state & KeyModifierCtrl) {
//                if(m_rotatable)
                {
                    // rotate
                    if(pickId == label_x.Id()) {
                        rot << 1,0,0;
                    }else if(pickId == label_y.Id()) {
                        rot << 0,1,0;
                    }else if(pickId == label_z.Id()) {
                        rot << 0,0,1;
                    }else{
                        return false;
                    }
                }
            }else if(button_state & KeyModifierShift){
//                if(m_translatable)
                {
                    // translate
                    if(pickId == label_x.Id()) {
                        xyz << 1,0,0;
                    }else if(pickId == label_y.Id()) {
                        xyz << 0,1,0;
                    }else if(pickId == label_z.Id()) {
                        xyz << 0,0,1;
                    }else{
                        return false;
                    }
                }
            }else{
                return false;
            }

            // old from new
            Eigen::Matrix<double,4,4> T_on = Eigen::Matrix<double,4,4>::Identity();
            T_on.block<3,3>(0,0) = Eigen::AngleAxis<double>(scale,rot).toRotationMatrix();
            T_on.block<3,1>(0,3) = scale*xyz;

            // Update
            T_pc = (ToEigen<double>(T_pc) * T_on.inverse()).eval();

            return true;
        }

        return false;
    }

    virtual bool MouseMotion(
        const Eigen::Vector3d& win, const Eigen::Vector3d& obj,
        const Eigen::Vector3d& normal, int button_state, int pickId
    ) override
    {
        std::cout << "MouseMotion: " << pickId << std::endl;
        return false;
    }

    float axis_length;
    const InteractiveIndex::Token label_x;
    const InteractiveIndex::Token label_y;
    const InteractiveIndex::Token label_z;
};

}
