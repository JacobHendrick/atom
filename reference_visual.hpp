#pragma once

// Desktop presentation of the original atom.cpp simulation. This file only
// handles drawing and UI; atom.cpp remains the source of the quantum math.
#include <array>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

class ReferenceVisual {
    struct CloudVertex {
        glm::vec3 position;
        glm::vec4 color;
        float density;
    };
    struct FlatVertex {
        float x, y, r, g, b, a;
    };
    GLuint cloudProgram = 0, uiProgram = 0;
    GLuint cloudVAO = 0, cloudVBO = 0, uiVAO = 0, uiVBO = 0;
    std::vector<CloudVertex> cloud;
    std::vector<FlatVertex> flat;
    int width = 1100, height = 800, panelHeight = 218;
    int activeSlider = -1;
    float colorIntensity = 1.0f;
    glm::vec3 clipping = glm::vec3(0.0f);

    static GLuint shader(GLenum type, const char* source) {
        GLuint result = glCreateShader(type);
        glShaderSource(result, 1, &source, nullptr);
        glCompileShader(result);
        GLint success = GL_FALSE;
        glGetShaderiv(result, GL_COMPILE_STATUS, &success);
        if (!success) {
            char message[2048] = {};
            glGetShaderInfoLog(result, sizeof(message), nullptr, message);
            throw std::runtime_error(std::string("Shader compile failed: ") + message);
        }
        return result;
    }

    static GLuint link(const char* vertex, const char* fragment) {
        GLuint vertexShader = shader(GL_VERTEX_SHADER, vertex);
        GLuint fragmentShader = shader(GL_FRAGMENT_SHADER, fragment);
        GLuint result = glCreateProgram();
        glAttachShader(result, vertexShader);
        glAttachShader(result, fragmentShader);
        glLinkProgram(result);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        GLint success = GL_FALSE;
        glGetProgramiv(result, GL_LINK_STATUS, &success);
        if (!success) {
            char message[2048] = {};
            glGetProgramInfoLog(result, sizeof(message), nullptr, message);
            throw std::runtime_error(std::string("Shader link failed: ") + message);
        }
        return result;
    }

    static std::array<unsigned char, 7> glyph(char c) {
        switch (std::toupper(static_cast<unsigned char>(c))) {
            case 'A': return {14,17,17,31,17,17,17};
            case 'B': return {30,17,17,30,17,17,30};
            case 'C': return {15,16,16,16,16,16,15};
            case 'D': return {30,17,17,17,17,17,30};
            case 'E': return {31,16,16,30,16,16,31};
            case 'F': return {31,16,16,30,16,16,16};
            case 'G': return {15,16,16,23,17,17,15};
            case 'H': return {17,17,17,31,17,17,17};
            case 'I': return {31,4,4,4,4,4,31};
            case 'J': return {7,2,2,2,18,18,12};
            case 'K': return {17,18,20,24,20,18,17};
            case 'L': return {16,16,16,16,16,16,31};
            case 'M': return {17,27,21,21,17,17,17};
            case 'N': return {17,25,21,19,17,17,17};
            case 'O': return {14,17,17,17,17,17,14};
            case 'P': return {30,17,17,30,16,16,16};
            case 'Q': return {14,17,17,17,21,18,13};
            case 'R': return {30,17,17,30,20,18,17};
            case 'S': return {15,16,16,14,1,1,30};
            case 'T': return {31,4,4,4,4,4,4};
            case 'U': return {17,17,17,17,17,17,14};
            case 'V': return {17,17,17,17,17,10,4};
            case 'W': return {17,17,17,21,21,21,10};
            case 'X': return {17,17,10,4,10,17,17};
            case 'Y': return {17,17,10,4,4,4,4};
            case 'Z': return {31,1,2,4,8,16,31};
            case '0': return {14,17,19,21,25,17,14};
            case '1': return {4,12,4,4,4,4,14};
            case '2': return {14,17,1,2,4,8,31};
            case '3': return {30,1,1,14,1,1,30};
            case '4': return {2,6,10,18,31,2,2};
            case '5': return {31,16,16,30,1,1,30};
            case '6': return {14,16,16,30,17,17,14};
            case '7': return {31,1,2,4,8,8,8};
            case '8': return {14,17,17,14,17,17,14};
            case '9': return {14,17,17,15,1,1,14};
            case '-': return {0,0,0,31,0,0,0};
            case '+': return {0,4,4,31,4,4,0};
            case '.': return {0,0,0,0,0,12,12};
            case '/': return {1,1,2,4,8,16,16};
            case ':': return {0,4,4,0,4,4,0};
            case '(': return {2,4,8,8,8,4,2};
            case ')': return {8,4,2,2,2,4,8};
            case '|': return {4,4,4,4,4,4,4};
            default: return {0,0,0,0,0,0,0};
        }
    }

    void triangle(float x1,float y1,float x2,float y2,float x3,float y3,
                  const glm::vec4& color) {
        flat.push_back({x1,y1,color.r,color.g,color.b,color.a});
        flat.push_back({x2,y2,color.r,color.g,color.b,color.a});
        flat.push_back({x3,y3,color.r,color.g,color.b,color.a});
    }

    void rectangle(float x,float y,float w,float h,const glm::vec4& color) {
        triangle(x,y,x+w,y,x+w,y+h,color);
        triangle(x,y,x+w,y+h,x,y+h,color);
    }

    void label(float x,float y,const std::string& value,float size,
               const glm::vec4& color) {
        for (char ch : value) {
            const auto pixels = glyph(ch);
            for (int row=0;row<7;++row)
                for (int col=0;col<5;++col)
                    if (pixels[row] & (1 << (4-col)))
                        rectangle(x+col*size,y+row*size,size*.79f,size*.79f,color);
            x += 6*size;
        }
    }

    float panelTop() const { return float(height-panelHeight); }
    float columnX(int column) const {
        return column==0 ? 43.0f : width*.52f;
    }
    float columnWidth(int column) const {
        return column==0 ? width*.44f-43.0f : width*.44f;
    }

    struct Slider {
        float left, right, y, value, low, high;
        const char* title;
    };

    Slider slider(int index) const {
        const float top=panelTop();
        const int column=index>=3?1:0;
        const float x=columnX(column), w=columnWidth(column);
        if (index<3) {
            const float y=top+64+index*49;
            if (index==0)return {x,x+w,y,float(n),1,7,"N (PRINCIPAL)"};
            if (index==1)return {x,x+w,y,float(l),0,float(n-1),"L (AZIMUTHAL)"};
            return {x,x+w,y,float(m),float(-l),float(l),"M (MAGNETIC)"};
        }
        if (index==3)return {x,x+w,top+64,colorIntensity,.1f,10,"COLOR INTENSITY"};
        const int axis=index-4;
        const float segment=(w-20)/3;
        const float left=x+axis*(segment+10);
        return {left,left+segment,top+164,clipping[axis],-100,100,
                axis==0?"X":axis==1?"Y":"Z"};
    }

    void drawSlider(int index) {
        const Slider s=slider(index);
        const glm::vec4 text(.63f,.65f,.69f,1), track(.18f,.19f,.21f,1);
        const float labelY=s.y-26;
        label(s.left,labelY,s.title,1.6f,text);
        std::ostringstream value;
        if(index==3)value<<std::fixed<<std::setprecision(1)<<s.value;
        else value<<int(s.value);
        if(index<4)label(s.right-(index==3?36:14),labelY,value.str(),1.6f,text);
        rectangle(s.left,s.y,s.right-s.left,3,track);
        float ratio=s.high>s.low?(s.value-s.low)/(s.high-s.low):.5f;
        ratio=std::clamp(ratio,0.0f,1.0f);
        const float knob=s.left+ratio*(s.right-s.left);
        rectangle(knob-5,s.y-4,10,11,glm::vec4(.96f,.96f,.96f,1));
    }

    void drawPanel() {
        flat.clear();
        const float top=panelTop();
        rectangle(0,0,float(width),float(height),glm::vec4(0,0,0,0));
        rectangle(0,top,float(width),float(panelHeight),glm::vec4(.073f,.073f,.077f,1));
        rectangle(0,top,float(width),1,glm::vec4(.20f,.20f,.22f,1));
        label(28,26,"ATOM / HYDROGEN QUANTUM ORBITAL VISUALIZER",2.0f,
              glm::vec4(.72f,.73f,.76f,.9f));
        label(29,54,"DRAG TO ROTATE  /  SCROLL TO ZOOM",1.35f,
              glm::vec4(.39f,.41f,.44f,.9f));
        label(28,top-24,"QUANTUM ORBITAL PROBABILITY DENSITY VISUALIZER",1.25f,
              glm::vec4(.46f,.49f,.54f,1));
        label(columnX(0),top+19,"QUANTUM NUMBERS",1.65f,
              glm::vec4(.53f,.55f,.59f,1));
        label(columnX(1),top+19,"VISUALIZATION",1.65f,
              glm::vec4(.53f,.55f,.59f,1));
        for(int index=0;index<4;++index)drawSlider(index);
        label(columnX(1),top+100,"PLANE CLIPPING",1.6f,
              glm::vec4(.63f,.65f,.69f,1));
        for(int index=4;index<7;++index)drawSlider(index);
        label(43,top+197,"W/S: N   E/D: L   R/F: M   T/G: SAMPLES",1.25f,
              glm::vec4(.39f,.41f,.44f,1));

        glViewport(0,0,width,height);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(uiProgram);
        glUniform2f(glGetUniformLocation(uiProgram,"screen"),float(width),float(height));
        glBindVertexArray(uiVAO);
        glBindBuffer(GL_ARRAY_BUFFER,uiVBO);
        glBufferData(GL_ARRAY_BUFFER,flat.size()*sizeof(FlatVertex),flat.data(),GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES,0,GLsizei(flat.size()));
    }

public:
    ReferenceVisual() {
        static const char* pointVertex=R"glsl(
            #version 330 core
            layout(location=0) in vec3 aPosition;
            layout(location=1) in vec4 aColor;
            layout(location=2) in float aDensity;
            uniform mat4 view,projection;
            uniform float pointScale;
            out vec4 color;
            out float density;
            out vec3 worldPosition;
            void main() {
                vec4 eye=view*vec4(aPosition,1.0);
                gl_Position=projection*eye;
                gl_PointSize=clamp(pointScale/max(1.0,-eye.z),1.0,28.0);
                color=aColor;
                density=aDensity;
                worldPosition=aPosition;
            }
        )glsl";
        static const char* pointFragment=R"glsl(
            #version 330 core
            in vec4 color;
            in float density;
            in vec3 worldPosition;
            uniform float intensity, opacity, clipRadius;
            uniform vec3 clipping;
            out vec4 outputColor;
            void main() {
                for(int axis=0;axis<3;++axis) {
                    float cut=clipping[axis];
                    if(cut>0.5 && worldPosition[axis]<cut*.01*clipRadius)discard;
                    if(cut<-.5 && worldPosition[axis]>cut*.01*clipRadius)discard;
                }
                vec2 q=gl_PointCoord*2.0-1.0;
                float d=dot(q,q);
                if(d>1.0)discard;
                float halo=exp(-d*5.0);
                float amount=opacity*halo*(.38+.62*density)*sqrt(intensity);
                vec3 rgb=color.rgb*intensity;
                outputColor=vec4(rgb,amount);
            }
        )glsl";
        static const char* flatVertex=R"glsl(
            #version 330 core
            layout(location=0) in vec2 position;
            layout(location=1) in vec4 vertexColor;
            uniform vec2 screen;
            out vec4 color;
            void main() {
                vec2 clip=vec2(position.x/screen.x*2.0-1.0,
                               1.0-position.y/screen.y*2.0);
                gl_Position=vec4(clip,0,1);
                color=vertexColor;
            }
        )glsl";
        static const char* flatFragment=R"glsl(
            #version 330 core
            in vec4 color;
            out vec4 outputColor;
            void main(){outputColor=color;}
        )glsl";
        cloudProgram=link(pointVertex,pointFragment);
        uiProgram=link(flatVertex,flatFragment);
        glGenVertexArrays(1,&cloudVAO);
        glGenBuffers(1,&cloudVBO);
        glBindVertexArray(cloudVAO);
        glBindBuffer(GL_ARRAY_BUFFER,cloudVBO);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(CloudVertex),(void*)offsetof(CloudVertex,position));
        glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(CloudVertex),(void*)offsetof(CloudVertex,color));
        glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(CloudVertex),(void*)offsetof(CloudVertex,density));
        for(int index=0;index<3;++index)glEnableVertexAttribArray(index);
        glGenVertexArrays(1,&uiVAO);
        glGenBuffers(1,&uiVBO);
        glBindVertexArray(uiVAO);
        glBindBuffer(GL_ARRAY_BUFFER,uiVBO);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(FlatVertex),(void*)offsetof(FlatVertex,x));
        glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(FlatVertex),(void*)offsetof(FlatVertex,r));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnable(GL_PROGRAM_POINT_SIZE);
    }

    void resize(GLFWwindow* window) {
        glfwGetFramebufferSize(window,&width,&height);
        width=std::max(1,width);
        height=std::max(1,height);
        panelHeight=std::min(218,std::max(160,height/3));
    }

    void render(GLFWwindow* window,const std::vector<Particle>& particles) {
        resize(window);
        const int canvasHeight=std::max(1,height-panelHeight);
        glViewport(0,0,width,height);
        glClearColor(.006f,.006f,.008f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glViewport(0,panelHeight,width,canvasHeight);
        const glm::mat4 projection=glm::perspective(
            glm::radians(45.0f),float(width)/canvasHeight,.1f,2000.0f);
        const glm::mat4 view=glm::lookAt(camera.position(),camera.target,
                                         glm::vec3(0,1,0));
        cloud.clear();
        cloud.reserve(particles.size());
        for(const Particle& p:particles)
            cloud.push_back({p.pos,p.color,p.density});
        glBindBuffer(GL_ARRAY_BUFFER,cloudVBO);
        glBufferData(GL_ARRAY_BUFFER,cloud.size()*sizeof(CloudVertex),
                     cloud.data(),GL_STREAM_DRAW);
        glUseProgram(cloudProgram);
        glUniformMatrix4fv(glGetUniformLocation(cloudProgram,"view"),1,GL_FALSE,
                           glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(cloudProgram,"projection"),1,
                           GL_FALSE,glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(cloudProgram,"intensity"),colorIntensity);
        glUniform1f(glGetUniformLocation(cloudProgram,"clipRadius"),float(n*n*5));
        glUniform3fv(glGetUniformLocation(cloudProgram,"clipping"),1,
                     glm::value_ptr(clipping));
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glBindVertexArray(cloudVAO);
        glUniform1f(glGetUniformLocation(cloudProgram,"pointScale"),440.0f);
        glUniform1f(glGetUniformLocation(cloudProgram,"opacity"),.035f);
        glDrawArrays(GL_POINTS,0,GLsizei(cloud.size()));
        glUniform1f(glGetUniformLocation(cloudProgram,"pointScale"),140.0f);
        glUniform1f(glGetUniformLocation(cloudProgram,"opacity"),.12f);
        glDrawArrays(GL_POINTS,0,GLsizei(cloud.size()));
        drawPanel();
    }

    // x/y are GLFW window coordinates. Resize uses framebuffer pixels, so
    // map them before checking the slider hit boxes on HiDPI desktops.
    bool mouseButton(GLFWwindow* window,int button,int action,double x,double y) {
        if(button!=GLFW_MOUSE_BUTTON_LEFT)return false;
        int ww=1,wh=1;glfwGetWindowSize(window,&ww,&wh);
        const float px=float(x)*width/std::max(1,ww);
        const float py=float(y)*height/std::max(1,wh);
        if(action==GLFW_RELEASE){const bool handled=activeSlider>=0;activeSlider=-1;return handled;}
        if(py<panelTop())return false;
        for(int index=0;index<7;++index){
            const Slider s=slider(index);
            if(px>=s.left-12 && px<=s.right+12 && py>=s.y-15 && py<=s.y+13){
                activeSlider=index;
                drag(window,x);
                return true;
            }
        }
        return true;
    }

    bool drag(GLFWwindow* window,double x) {
        if(activeSlider<0)return false;
        int ww=1,wh=1;glfwGetWindowSize(window,&ww,&wh);
        const float px=float(x)*width/std::max(1,ww);
        const Slider s=slider(activeSlider);
        const float t=std::clamp((px-s.left)/(s.right-s.left),0.0f,1.0f);
        float value=s.low+t*(s.high-s.low);
        if(activeSlider==0 || activeSlider==1 || activeSlider==2){
            const int rounded=int(std::round(value));
            bool changed=false;
            if(activeSlider==0 && rounded!=n){n=rounded;changed=true;}
            if(activeSlider==1 && rounded!=l){l=rounded;changed=true;}
            if(activeSlider==2 && rounded!=m){m=rounded;changed=true;}
            if(changed){
                l=std::clamp(l,0,n-1);
                m=std::clamp(m,-l,l);
                generateParticles(N);
                camera.radius=std::max(20.0f,float(n*n*5));
            }
        }else if(activeSlider==3){
            colorIntensity=std::round(value*10.0f)/10.0f;
        }else{
            clipping[activeSlider-4]=std::round(value);
            if(std::abs(clipping[activeSlider-4])<1.0f)
                clipping[activeSlider-4]=0.0f;
        }
        return true;
    }

    bool usingSlider() const { return activeSlider>=0; }

    void savePPM(const std::string& path) const {
        std::vector<unsigned char> pixels(size_t(width)*height*3);
        glPixelStorei(GL_PACK_ALIGNMENT,1);
        glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        std::ofstream output(path,std::ios::binary);
        output<<"P6\n"<<width<<" "<<height<<"\n255\n";
        for(int row=height-1;row>=0;--row)
            output.write(reinterpret_cast<const char*>(pixels.data()+size_t(row)*width*3),
                         size_t(width)*3);
    }
};
