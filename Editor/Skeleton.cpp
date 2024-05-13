
#include "framework.h"
using namespace std;
// vertex shader in GLSL

const char *vertexSource = R"(
    #version 330
    precision highp float;
    uniform mat4 MVP;        // Model-View-Projection matrix in row-major format
    layout(location = 0) in vec2 vertexPosition;   // Attrib Array 0
    layout(location = 1) in vec2 vertexUV;       // Attrib Array 1
    out vec2 texCoord;                      // output attribute
    void main() {
       texCoord = vertexUV;                                          // copy texture coordinates
       gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP;       // transform to clipping space
    }
)";
// fragment shader in GLSL

const char *fragmentSource = R"(
    #version 330
    precision highp float;
    uniform sampler2D textureUnit;
    in vec2 texCoord;        // variable input: interpolated texture coordinates
    out vec4 fragmentColor;       // output that goes to the raster memory as told by glBindFragDataLocation

    void main() {
          fragmentColor = texture(textureUnit, texCoord); // fetch color from texture
    }
)";

// 2D camera
class Camera2D {
    vec2 wCenter; // center in world coordinates
    vec2 wSize;   // width and height in world coordinates
public:
    Camera2D() : wCenter(20, 30), wSize(150, 150) {}
    mat4 V() { return TranslateMatrix(-wCenter); }
    mat4 P() const { return ScaleMatrix(vec2(2 / wSize.x, 2 / wSize.y)); }
};
Camera2D camera;       // 2D camera
GPUProgram gpuProgram; // vertex and fragment shaders

class Poincare : public Texture { // Csala Bálint konzi segítségével
private:
    int width, height;
public:
    void setWidth(int width) {
        Poincare::width += width;
    }

    void setHeight(int height) {
        Poincare::height += height;
    }

private:
    vector<vec3> points;
    vector<vec2> projected_points;
    vector<vec2> circles_origo;
    vector<float> circles_rad;
public:
    Poincare(int width, int height) : width(width), height(height) {
        calc_hyper();
        calc_circle();
        std::vector<vec4> image = RenderToTexture(width, height);
        create(width, height, image);
    }

    void res() {
        std::vector<vec4> image = RenderToTexture(width, height);
        create(width, height, image);
    }



    void calc_hyper() {
        vec3 kezdo = vec3(0, 0, 1);
        for (int i = 0; i < 360; i+=40) {
            float phi = i  * M_PI / 180;
            vec3 irany_v = vec3(cos(phi), sin(phi), 0);
            for (float dist = 0.5; dist <= 5.5; dist++) {

                vec3 new_point =kezdo * cosh(dist) + (kezdo * sinh(dist) + irany_v * cosh(dist)) * sinh(dist);
                new_point.x = new_point.x /(new_point.z + 1);
                new_point.y= new_point.y /(new_point.z + 1);
                projected_points.push_back(vec2(new_point.x, new_point.y));
            }
        }
    }

    void calc_circle() {
        for (size_t i = 0; i < projected_points.size(); i++) {
            float origo_proj = length(projected_points[i]);
            float origo_inverz = 1.0f / origo_proj;
            float r = (origo_inverz - origo_proj) / 2;
            circles_rad.push_back(r);
            vec2 dir_vec =  normalize(projected_points[i]) ;
            circles_origo.push_back(vec2(projected_points[i].x + dir_vec.x * r, projected_points[i].y + dir_vec.y * r));
        }
    }
    int circle_count(vec2 point) {
        int n = 0;
        for (int i = 0; i < circles_origo.size(); i++) {
            if (length(point-circles_origo[i]) <= circles_rad[i]) {
                n++;
            }
        }
        return n;
    }
    std::vector<vec4> RenderToTexture(int width, int height) {
        std::vector<vec4> poin_texture;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                vec2 pos = vec2(2.0f * x / width - 1.0f, 1.0f - 2.0f * y / height);
                int n = circle_count(pos);
                if(length(pos) > 1) {
                    poin_texture.push_back(vec4(0.0f, 0.0f, 0.0f, 1.0f));
                }
                else if(n % 2 == 0) {
                    poin_texture.push_back(vec4(1.0f, 1.0f, 0.0f, 1.0f));
                }
                else {
                    poin_texture.push_back(vec4(0.0f, 0.0f, 1.0f, 1.0f));
                }
            }
        }
        return poin_texture;
    }
};


class Star {
private:
    unsigned int vao, vbo[2];
    vector<vec2> star_pos;
    vector<vec2> text_pos;
    Poincare texture;
    float phi, phi2;
public:
    bool animate = false;
    Poincare &getTex() {
        return texture;
    }

    Star(int width, int height) : texture(width, height) {
        star_pos.push_back(vec2(50, 30));
        text_pos.push_back(vec2(0.5, 0.5));
        star_pos.push_back(vec2(90, 30));//1
        text_pos.push_back(vec2(1, 0.5));
        star_pos.push_back(vec2(90, 70));
        text_pos.push_back(vec2(1, 1));
        star_pos.push_back(vec2(50, 70));//3
        text_pos.push_back(vec2(0.5, 1));
        star_pos.push_back(vec2(10, 70));
        text_pos.push_back(vec2(0, 1));
        star_pos.push_back(vec2(10, 30));//5
        text_pos.push_back(vec2(0, 0.5));
        star_pos.push_back(vec2(10, -10));
        text_pos.push_back(vec2(0, 0));
        star_pos.push_back(vec2(50, -10));//7
        text_pos.push_back(vec2(0.5, 0));
        star_pos.push_back(vec2(90, -10));
        text_pos.push_back(vec2(1, 0));
        star_pos.push_back(vec2(90, 30));
        text_pos.push_back(vec2(1, 0.5));
        Animate();
        openglthings();
    }

    void openglthings()
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(2, &vbo[0]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, star_pos.size() * sizeof(vec2), star_pos.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*) nullptr);        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, text_pos.size() * sizeof(vec2), text_pos.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*) nullptr);
    }

    void toogleAnimation() {
        animate = !animate;
    }
    void Draw() {
        mat4 MVPTransform = M()*camera.V() * camera.P();
        gpuProgram.setUniform(MVPTransform, "MVP");
        glBindTexture(GL_TEXTURE_2D, texture.textureId);
        glBindVertexArray(vao);

       glDrawArrays(GL_TRIANGLE_FAN, 0, 10);

    }

    float in_rad(float degree) {
        return degree - (2.0f *M_PI) * floor(degree / (2.0f *M_PI));
    }

    void Animate(float t=0) {
       float rotationSpeed =0.2* 2.0f * M_PI ; // 10 másodperc alatt teljes kör (2PI rad/s)
        phi = in_rad(t *rotationSpeed*0.5); // szög radianban
        phi2 = in_rad(t * rotationSpeed*0.7); // szög radianban
        // Update the buffer data
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, star_pos.size() * sizeof(vec2), star_pos.data(), GL_DYNAMIC_DRAW);
    }
    mat4 M() {
        mat4 translateToCenter = TranslateMatrix(-vec3(20, 30, 0.0f));
        mat4 rotation = RotationMatrix(phi, vec3(0.0f, 0.0f, 1.0f));
        mat4 translateBack = TranslateMatrix(vec3(20, 30, 0.0f));

        mat4 selftrans = TranslateMatrix(-star_pos[0]);
        mat4 selfrot = RotationMatrix(phi2, vec3(0, 0, 1));
        mat4 selftransback = TranslateMatrix(star_pos[0]);
        return  selftrans *selfrot*selftransback*translateToCenter * rotation * translateBack;
    }

    void resize(float s) {
        star_pos[1].x -=s;
        star_pos[3].y -=s;
        star_pos[5].x +=s;
        star_pos[7].y +=s;
        star_pos[9].x -=s;
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, star_pos.size() * sizeof(vec2), star_pos.data(), GL_DYNAMIC_DRAW); // copy to that part of the memory which is modified
    }
};



Star *star; // The virtual world: one object
// Initialization, create an OpenGL context
void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);
    int width = 300, height = 300;
    std::vector<vec4> image(width * height);
    star = new Star(width, height);
    star->resize(20);
    gpuProgram.create(vertexSource, fragmentSource, "fragmentColor");
}

// Window has become invalid: Redraw

void onDisplay() {
    glClearColor(0, 0, 0, 0);                            // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
    star->Draw();
    glutSwapBuffers();                                    // exchange the two buffers
}

// Key of ASCII code pressed
float minus_time;
void onKeyboard(unsigned char key, int pX, int pY) {
    switch(key)
    {
        case 'h':
        {
            star->resize(-10);
            glutPostRedisplay();
            break;
        }
        case 'H':
        {
            star->resize(10);

            glutPostRedisplay();
            break;
        }
        case 'a':
        {
            minus_time = glutGet(GLUT_ELAPSED_TIME);
            star->toogleAnimation();
            glutPostRedisplay();
            break;
        }
        case 't':
        {

            glBindTexture(GL_TEXTURE_2D, star->getTex().textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glutPostRedisplay();
            break;
        }
        case 'T':
        {
            glBindTexture(GL_TEXTURE_2D, star->getTex().textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glutPostRedisplay();
            break;
        }
        case 'r':
        {
            star->getTex().setWidth(100);
            star->getTex().setHeight(100);
            star->getTex().res();
            glutPostRedisplay();
            break;
        }
        case 'R':
        {
            star->getTex().setWidth(-100);
            star->getTex().setHeight(-100);
            star->getTex().res();
            glutPostRedisplay();
            break;
        }
    }
}
// Ky of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}
// Move mouse with key pressed

void onMouseMotion(int pX, int pY) {

}



// Mouse click event

void onMouse(int button, int state, int pX, int pY) {
}

// Idle event indicating that some minus_time elapsed: do animation here

void onIdle() {
    if (star->animate) {
        long getTime = glutGet(GLUT_ELAPSED_TIME); // AktuĂƒÂĄlis idĂ…â€˜ lekĂƒÂŠrdezĂƒÂŠse
        float actualTime = (getTime - minus_time) / 1000.0f; // Eltelt idĂ…â€˜ szĂƒÂĄmĂƒÂ­tĂƒÂĄsa mĂƒÂĄsodpercekben
        star->Animate(actualTime);
        glutPostRedisplay();
    }



}