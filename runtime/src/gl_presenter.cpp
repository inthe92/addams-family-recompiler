#include "runtime/gl_presenter.hpp"
#include "runtime/original_presentation.hpp"
#include "runtime/frontend_timing.hpp"
#include "runtime/presentation_clock.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <array>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace nes::runtime {
template<class T> static T gl_function(const char* name) {
    const auto result = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    if (!result) throw std::runtime_error(std::string("Missing OpenGL function: ")+name);
    return result;
}
struct GLPresenter::Impl {
    SDL_Window* window;
    nes::frontend::FrontendTiming* timing;
    nes::frontend::FrontendTiming disabled_timing;
    GLuint program{}, texture{}, vao{}, buffer{}, ui_texture{}, ui_vao{}, ui_buffer{};
    PFNGLCREATESHADERPROC create_shader = gl_function<PFNGLCREATESHADERPROC>("glCreateShader");
    PFNGLSHADERSOURCEPROC shader_source = gl_function<PFNGLSHADERSOURCEPROC>("glShaderSource");
    PFNGLCOMPILESHADERPROC compile_shader = gl_function<PFNGLCOMPILESHADERPROC>("glCompileShader");
    PFNGLGETSHADERIVPROC get_shader = gl_function<PFNGLGETSHADERIVPROC>("glGetShaderiv");
    PFNGLGETSHADERINFOLOGPROC shader_log = gl_function<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");
    PFNGLDELETESHADERPROC delete_shader = gl_function<PFNGLDELETESHADERPROC>("glDeleteShader");
    PFNGLCREATEPROGRAMPROC create_program = gl_function<PFNGLCREATEPROGRAMPROC>("glCreateProgram");
    PFNGLATTACHSHADERPROC attach_shader = gl_function<PFNGLATTACHSHADERPROC>("glAttachShader");
    PFNGLBINDATTRIBLOCATIONPROC bind_attribute = gl_function<PFNGLBINDATTRIBLOCATIONPROC>("glBindAttribLocation");
    PFNGLLINKPROGRAMPROC link_program = gl_function<PFNGLLINKPROGRAMPROC>("glLinkProgram");
    PFNGLGETPROGRAMIVPROC get_program = gl_function<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");
    PFNGLDELETEPROGRAMPROC delete_program = gl_function<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");
    PFNGLUSEPROGRAMPROC use_program = gl_function<PFNGLUSEPROGRAMPROC>("glUseProgram");
    PFNGLGENVERTEXARRAYSPROC gen_vaos = gl_function<PFNGLGENVERTEXARRAYSPROC>("glGenVertexArrays");
    PFNGLBINDVERTEXARRAYPROC bind_vao = gl_function<PFNGLBINDVERTEXARRAYPROC>("glBindVertexArray");
    PFNGLDELETEVERTEXARRAYSPROC delete_vaos = gl_function<PFNGLDELETEVERTEXARRAYSPROC>("glDeleteVertexArrays");
    PFNGLGENBUFFERSPROC gen_buffers = gl_function<PFNGLGENBUFFERSPROC>("glGenBuffers");
    PFNGLBINDBUFFERPROC bind_buffer = gl_function<PFNGLBINDBUFFERPROC>("glBindBuffer");
    PFNGLBUFFERDATAPROC buffer_data = gl_function<PFNGLBUFFERDATAPROC>("glBufferData");
    PFNGLDELETEBUFFERSPROC delete_buffers = gl_function<PFNGLDELETEBUFFERSPROC>("glDeleteBuffers");
    PFNGLVERTEXATTRIBPOINTERPROC vertex_pointer = gl_function<PFNGLVERTEXATTRIBPOINTERPROC>("glVertexAttribPointer");
    PFNGLENABLEVERTEXATTRIBARRAYPROC enable_attribute = gl_function<PFNGLENABLEVERTEXATTRIBARRAYPROC>("glEnableVertexAttribArray");
    OriginalRGBA rgba{};
    uint64_t cached_publication{};
    Impl(SDL_Window* w, nes::frontend::FrontendTiming* t) : window(w), timing(t) {}
    GLuint shader(GLenum kind, const char* source) {
        const GLuint result = create_shader(kind);shader_source(result,1,&source,nullptr);compile_shader(result);
        GLint ok=0;get_shader(result,GL_COMPILE_STATUS,&ok);
        if (!ok) {char log[2048]{};shader_log(result,sizeof log,nullptr,log);delete_shader(result);throw std::runtime_error(log);}
        return result;
    }
    ~Impl() {
        if(ui_buffer)delete_buffers(1,&ui_buffer);if(ui_vao)delete_vaos(1,&ui_vao);if(ui_texture)glDeleteTextures(1,&ui_texture);
        if(buffer)delete_buffers(1,&buffer);if(vao)delete_vaos(1,&vao);
        if(texture)glDeleteTextures(1,&texture);if(program)delete_program(program);
    }
};
GLPresenter::GLPresenter(SDL_Window* window,nes::frontend::FrontendTiming* timing) : impl_(std::make_unique<Impl>(window,timing)) {
    auto& g=*impl_;
    const char* vertex="#version 150\nin vec2 position; in vec2 uv; out vec2 texcoord; void main(){ gl_Position=vec4(position,0,1); texcoord=uv; }";
    const char* fragment="#version 150\nuniform sampler2D image; in vec2 texcoord; out vec4 color; void main(){ color=texture(image,texcoord); }";
    const GLuint vs=g.shader(GL_VERTEX_SHADER,vertex),fs=g.shader(GL_FRAGMENT_SHADER,fragment);
    g.program=g.create_program();g.attach_shader(g.program,vs);g.attach_shader(g.program,fs);
    g.bind_attribute(g.program,0,"position");g.bind_attribute(g.program,1,"uv");g.link_program(g.program);
    g.delete_shader(vs);g.delete_shader(fs);GLint linked=0;g.get_program(g.program,GL_LINK_STATUS,&linked);
    if(!linked)throw std::runtime_error("OpenGL presentation shader link failed");
    const float vertices[]={-1,-1,0,1,1,-1,1,1,1,1,1,0,-1,-1,0,1,1,1,1,0,-1,1,0,0};
    g.gen_vaos(1,&g.vao);g.bind_vao(g.vao);g.gen_buffers(1,&g.buffer);g.bind_buffer(GL_ARRAY_BUFFER,g.buffer);
    g.buffer_data(GL_ARRAY_BUFFER,sizeof vertices,vertices,GL_STATIC_DRAW);
    g.vertex_pointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),nullptr);
    g.vertex_pointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),reinterpret_cast<void*>(2*sizeof(float)));
    g.enable_attribute(0);g.enable_attribute(1);
    glGenTextures(1,&g.texture);glBindTexture(GL_TEXTURE_2D,g.texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,256,240,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    g.gen_vaos(1,&g.ui_vao);g.bind_vao(g.ui_vao);g.gen_buffers(1,&g.ui_buffer);g.bind_buffer(GL_ARRAY_BUFFER,g.ui_buffer);
    g.buffer_data(GL_ARRAY_BUFFER,sizeof vertices,vertices,GL_DYNAMIC_DRAW);
    g.vertex_pointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),nullptr);
    g.vertex_pointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),reinterpret_cast<void*>(2*sizeof(float)));
    g.enable_attribute(0);g.enable_attribute(1);
    glGenTextures(1,&g.ui_texture);glBindTexture(GL_TEXTURE_2D,g.ui_texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}
GLPresenter::~GLPresenter()=default;
const char* GLPresenter::version() const { return reinterpret_cast<const char*>(glGetString(GL_VERSION)); }
void GLPresenter::present(const VideoState& video) {
    present(video.frame().data(),VideoState::width,VideoState::height);
}
void GLPresenter::present(const nes::frontend::FrameSnapshot& snapshot) {
    auto& g=*impl_;
    if(!snapshot.valid()){present(snapshot.pixels().data(),256,240);return;}
    const bool upload=g.cached_publication!=snapshot.publication();
    if(present_pixels(snapshot.pixels().data(),256,240,upload)) {
        g.cached_publication=snapshot.publication();
        if(!upload && g.timing)g.timing->record(nes::frontend::TimingCount::cached_draws);
    }
}
void GLPresenter::present(const uint8_t* indices,unsigned image_width,unsigned image_height) {
    impl_->cached_publication=0; // Raw/video paths may overwrite the same texture.
    present_pixels(indices,image_width,image_height,true);
}
bool GLPresenter::present_pixels(const uint8_t* indices,unsigned image_width,unsigned image_height,bool upload) {
    auto& g=*impl_;
    if(upload) { auto& metrics=g.timing?*g.timing:g.disabled_timing;
      nes::frontend::FrontendTiming::Scope measure(metrics,nes::frontend::TimingStage::conversion);
      original_rgba(indices,image_width,image_height,g.rgba);
      metrics.record(nes::frontend::TimingCount::nes_conversions); }
    int width=0,height=0;SDL_GL_GetDrawableSize(g.window,&width,&height);
    glViewport(0,0,width,height);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
    const auto viewport=original_viewport(width,height,
        (SDL_GetWindowFlags(g.window)&SDL_WINDOW_FULLSCREEN)!=0);
    if(!viewport.width || !viewport.height)return false;
    glViewport(viewport.x,viewport.y,viewport.width,viewport.height);
    g.use_program(g.program);g.bind_vao(g.vao);glBindTexture(GL_TEXTURE_2D,g.texture);
    if(upload) { auto& metrics=g.timing?*g.timing:g.disabled_timing;
      nes::frontend::FrontendTiming::Scope measure(metrics,nes::frontend::TimingStage::upload);
      glTexSubImage2D(GL_TEXTURE_2D,0,0,0,static_cast<GLsizei>(image_width),static_cast<GLsizei>(image_height),GL_RGBA,GL_UNSIGNED_BYTE,g.rgba.data());
      metrics.record(nes::frontend::TimingCount::nes_uploads); }
    glDrawArrays(GL_TRIANGLES,0,6);
    if(g.timing)g.timing->record(nes::frontend::TimingCount::nes_draws);
    if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL presentation failed");
    return true;
}
void GLPresenter::present_overlay(const nes::ui::FrontendUI& ui,double now) {
    const auto bitmap=nes::ui::overlay_bitmap(ui,now);if(!bitmap.width)return;
    int w=0,h=0;SDL_GL_GetDrawableSize(impl_->window,&w,&h);
    const auto r=nes::ui::overlay_rect(w,h,ui.modal());
    present_panel(bitmap,r);
}
void GLPresenter::present_panel(const nes::ui::Bitmap& bitmap,nes::ui::Rect r) {
    if(!bitmap.width||!bitmap.height||!r.width||!r.height)return;
    auto& g=*impl_;int w=0,h=0;SDL_GL_GetDrawableSize(g.window,&w,&h);if(w<=0||h<=0)return;
    const float left=2.0f*r.x/w-1,right=2.0f*(r.x+r.width)/w-1;
    const float top=1-2.0f*r.y/h,bottom=1-2.0f*(r.y+r.height)/h;
    const float v[]={left,bottom,0,1,right,bottom,1,1,right,top,1,0,left,bottom,0,1,right,top,1,0,left,top,0,0};
    GLint old_viewport[4];glGetIntegerv(GL_VIEWPORT,old_viewport);glViewport(0,0,w,h);
    g.use_program(g.program);g.bind_vao(g.ui_vao);g.bind_buffer(GL_ARRAY_BUFFER,g.ui_buffer);g.buffer_data(GL_ARRAY_BUFFER,sizeof v,v,GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_2D,g.ui_texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,bitmap.width,bitmap.height,0,GL_RGBA,GL_UNSIGNED_BYTE,bitmap.rgba.data());
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDrawArrays(GL_TRIANGLES,0,6);glDisable(GL_BLEND);
    if(g.timing)g.timing->record(nes::frontend::TimingCount::ui_draws);
    glViewport(old_viewport[0],old_viewport[1],old_viewport[2],old_viewport[3]);
    if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL UI overlay failed");
}
void GLPresenter::capture(const std::string& path) {
    int width=0,height=0;SDL_GL_GetDrawableSize(impl_->window,&width,&height);
    std::vector<uint8_t> pixels(static_cast<std::size_t>(width)*static_cast<unsigned>(height)*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL capture failed");
    std::ofstream out(path,std::ios::binary);if(!out)throw std::runtime_error("Cannot open OpenGL frame output");
    out<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y)out.write(reinterpret_cast<const char*>(pixels.data()+static_cast<std::size_t>(y)*static_cast<unsigned>(width)*3),static_cast<std::streamsize>(width)*3);
    if(!out)throw std::runtime_error("Cannot save OpenGL frame output");
}
}
