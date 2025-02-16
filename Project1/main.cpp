/**
* Author: [Haoxuan Wang]
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION 

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h" 


enum AppStatus { RUNNING, TERMINATED };

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;


constexpr char FIRE_SPRITE[] = "fire.jpg";
constexpr char WATER_SPRITE[] = "water.png";

constexpr glm::vec3 INIT_POS_fire = glm::vec3(0.5f, 0.5f, 0.0f);
constexpr glm::vec3 INIT_POS_water = glm::vec3(-1.0f, -1.0f, 0.0f);
constexpr glm::vec3 INIT_SCALE_water = glm::vec3(1.0f, 1.0f, 0.0f);

GLuint g_player_texture_id;

constexpr int WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr int TRIANGLE_RED = 1.0,
TRIANGLE_BLUE = 0.4,
TRIANGLE_GREEN = 0.4,
TRIANGLE_OPACITY = 1.0;

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix,
g_model_matrix,
g_projection_matrix;

glm::mat4 g_water_matrix;
glm::mat4 g_fire_matrix;

GLuint g_water_texture_id;
GLuint g_fire_texture_id;

float g_previous_ticks = 0;

glm::vec3 g_fire_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_fire_movement = glm::vec3(0.2f, 0.2f, 0.0f);

glm::vec3 g_water_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_water_movement = glm::vec3(0.3f, 0.3f, 0.0f);

glm::vec3 g_water_scale = glm::vec3(0.1f, 0.1f, 0.0f);
glm::vec3 g_water_size = glm::vec3(1.0f, 1.0f, 0.0f);

float ROT_ANGLE_POS = glm::radians(0.0f);
float ROT_ANGLE_MOVE = glm::radians(20.0f);

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Transformation Exercise",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_model_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_water_matrix = glm::mat4(1.0f);
    g_fire_matrix = glm::mat4(1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    g_shader_program.set_colour(TRIANGLE_RED, TRIANGLE_BLUE, TRIANGLE_GREEN, TRIANGLE_OPACITY);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_water_texture_id = load_texture(WATER_SPRITE);
    g_fire_texture_id = load_texture(FIRE_SPRITE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_water_position += g_water_movement * delta_time;
    g_fire_position += g_fire_movement * delta_time;

    ROT_ANGLE_POS += ROT_ANGLE_MOVE * delta_time;

    g_water_size += g_water_scale * delta_time;

    g_water_matrix = glm::mat4(1.0f);
    g_fire_matrix = glm::mat4(1.0f);

    g_fire_matrix = glm::translate(g_fire_matrix, INIT_POS_fire);
    g_water_matrix = glm::translate(g_water_matrix, INIT_POS_water);

    g_fire_matrix = glm::translate(g_fire_matrix, g_fire_position);
    g_water_matrix = glm::translate(g_water_matrix, g_water_position);

    g_fire_matrix = glm::rotate(g_fire_matrix, ROT_ANGLE_POS, glm::vec3(0.0f, 0.0f, 1.0f));

    g_water_matrix = glm::scale(g_water_matrix, INIT_SCALE_water);
    g_water_matrix = glm::scale(g_water_matrix, g_water_size);

    if (ROT_ANGLE_POS == glm::radians(360.0f))
    {
        ROT_ANGLE_POS = glm::radians(0.0f);
    }

}

void draw_object(glm::mat4& g_object_model_matrix, GLuint& g_object_texture_id)
{
    g_shader_program.set_model_matrix(g_object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, g_object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(g_fire_matrix, g_fire_texture_id);
    draw_object(g_water_matrix, g_water_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
