/*
Map Feature:
- bounded
- changable (todo)

Obstable Features:
- path blocking for ammo and tank
- destructable by ammo

Rain Feature:
- random fall
- shadow (todo)

Ammo Feature:
- hit indication effect
- hit push effect on tank

Tank Feature:
- collision push effect on other tank
- fire cooldown
- ammo quantity control
- death control

View Features:
- rotatable scene
- zoomable scene
- multiple light (todo)

Manu Feature:
- display (todo)

*/


// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <thread>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>


#define MY_PI_HALF  (3.1415926f / 2.0f)
#define BOUND_X_MIN     (-20.0f)
#define BOUND_X_MAX     ( 20.0f)
#define BOUND_Y_MIN     (-20.0f)
#define BOUND_Y_MAX     ( 20.0f)
#define BOUND_Z_MIN     ( -2.0f)
#define BOUND_Z_MAX     ( 20.0f)

static const GLfloat g_ground_vect_buf_data[] = {
    -1.0f,-1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
     1.0f, 1.0f, 0.0f,
     1.0f, 1.0f, 0.0f,
     1.0f,-1.0f, 0.0f,
    -1.0f,-1.0f, 0.0f,
};

static const GLfloat g_ground_uv_buf_data[] = {
     0.0f, 0.0f,
     0.0f, 1.0f,
     1.0f, 1.0f,
     1.0f, 1.0f,
     1.0f, 0.0f,
     0.0f, 0.0f,
};

static const GLfloat g_ground_norm_buf_data[] = {
    0.000f,  0.000f,  1.000f,
    0.000f,  0.000f,  1.000f,
    0.000f,  0.000f,  1.000f,
    0.000f,  0.000f,  1.000f,
    0.000f,  0.000f,  1.000f,
    0.000f,  0.000f,  1.000f,
};


class Sphere {
    protected:
    float x;
    float y;
    float z;
    float r;

    public:
    Sphere(float x, float y, float z, float r) : x{x}, y{y}, z{z}, r{r} {}

    static bool get_relation(Sphere const & a, Sphere const & b, float & angle_xy, float & angle_z, float & dist) {
        float x_diff = b.x - a.x;
        float y_diff = b.y - a.y;
        float z_diff = b.z - a.z;

        float xy_diff_sq = x_diff * x_diff + y_diff * y_diff;
        float z_diff_sq = z_diff * z_diff;

        angle_xy = atan2(y_diff, x_diff);
        angle_z = atan2(z_diff_sq, xy_diff_sq);
        dist = sqrt(xy_diff_sq + z_diff_sq);

        if (&a == &b) {
            return false;
        }

        return true;
    }

    static bool check_is_out_of_bound(Sphere const &a, float x_min, float x_max, float y_min, float y_max, float z_min, float z_max) {
        return (a.x < x_min) || (a.x > x_max) || (a.y < y_min) || (a.y > y_max) || (a.z < z_min) || (a.z > z_max);
    }

    static bool check_is_out_of_bound(Sphere const &a) {
        return check_is_out_of_bound(a, BOUND_X_MIN, BOUND_X_MAX, BOUND_Y_MIN, BOUND_Y_MAX, BOUND_Z_MIN, BOUND_Z_MAX);
    }

    static bool check_is_collided(Sphere const & a, Sphere const & b) {
        if (&a == &b) {
            return false;
        }

        float x_diff = a.x - b.x;
        float y_diff = a.y - b.y;
        float z_diff = a.z - b.z;
        float r_sum = a.r + b.r;

        return (x_diff * x_diff + y_diff * y_diff + z_diff * z_diff) < (r_sum * r_sum);
    }

    float get_x() const {
        return this->x;
    }

    float get_y() const {
        return this->y;
    }

    float get_z() const {
        return this->z;
    }

    float get_r() const {
        return this->r;
    }

    bool get_relation(Sphere const & b, float & angle_xy, float & angle_z, float & dist) const {
        return Sphere::get_relation(*this, b, angle_xy, angle_z, dist);
    }

    bool check_is_out_of_bound(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max) const {
        return Sphere::check_is_out_of_bound(*this, x_min, x_max, y_min, y_max, z_min, z_max);
    }

    bool check_is_out_of_bound() const {
        return Sphere::check_is_out_of_bound(*this);
    }

    bool check_is_collided(Sphere const & b) const {
        return Sphere::check_is_collided(*this, b);
    }

    bool move(float angle_xy, float angle_z, float dist) {
        this->x += dist * cos(angle_z) * cos(angle_xy);
        this->y += dist * cos(angle_z) * sin(angle_xy);
        this->z += dist * sin(angle_z);

        return true;
    }

    glm::mat4 get_model_matrix() const {
        glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(this->r, this->r, this->r));
        glm::mat4 trans_mat = glm::translate(glm::mat4(), glm::vec3(this->x, this->y, this->z));
        glm::mat4 rotat_mat = glm::rotate( glm::mat4(1.0f), 0.0f, glm::vec3(0, 0, 1) );
        glm::mat4 model_mat = trans_mat * rotat_mat * scale_mat;

        return model_mat;
    }
};

class Obst : public Sphere {
    #define OBST_DEFAULT_HEALTH                 (10.0f)
    #define OBST_DEFAULT_TIMER_IS_HIT           (0.1f)

    float health;
    float timer_is_hit;

    public:
    Obst(float x, float y, float z, float r) : Sphere(x, y, z, r), health{OBST_DEFAULT_HEALTH}, timer_is_hit{0.0f} {}

    bool get_is_activated() const {
        return this->health > 0.0f;
    }

    bool set_is_activated(bool is_activated) {
        if (is_activated) {
            this->health = OBST_DEFAULT_HEALTH;
        }
        else {
            this->health = 0.0f;
        }
        return true;
    }

    bool get_is_hit() const {
        return this->timer_is_hit > 0.0f;
    }

    bool set_is_hit(bool is_hit) {
        if (is_hit) {
            this->timer_is_hit = OBST_DEFAULT_TIMER_IS_HIT;
            this->health -= 1.0f;
        }
        else {
            this->timer_is_hit = 0.0f;
        }
        return true;
    }

    bool refreash(float time) {
        if (this->timer_is_hit > 0.0f) {
            this->timer_is_hit -= time;
        }
        return true;
    }
};

class Ammo : public Sphere {
    bool is_fired;
    float angle_xy;
    float angle_z;
    float move_speed;

    public:
    Ammo() : Sphere(0.0f, 0.0f, 0.0f, 0.0f), is_fired{false} {}
    Ammo(float x, float y, float z, float r, float angle_xy, float angle_z, float move_speed) : Sphere(x, y, z, r), angle_xy{angle_xy}, angle_z{angle_z}, move_speed{move_speed}, is_fired{true} {}

    float get_angle_xy() const {
        return this->angle_xy;
    }

    float get_angle_z() const {
        return this->angle_z;
    }

    float get_move_speed() const {
        return this->move_speed;
    }

    bool get_is_fired() const {
        return this->is_fired;
    }

    bool set_is_fired(bool is_fired) {
        this->is_fired = is_fired;
        return true;
    }

    bool refreash(float time) {
        // do nothing
        return true;
    }
};

class Tank : public Sphere {
    #define TANK_DEFAULT_HEALTH                 (10.0f)
    #define TANK_DEFAULT_TURN_SPEED             (MY_PI_HALF / 3.0f)
    #define TANK_DEFAULT_MOVE_SPEED             (4.5f)
    #define TANK_DEFAULT_MAX_NUM_AMMO           (12)
    #define TANK_DEFAULT_TIMER_IS_HIT           (0.1f)
    #define TANK_DEFAULT_TIMER_IS_COOLING_FIRE  (0.1f)
    #define TANK_DEFAULT_TIMER_IS_LOADING_AMMO  (0.8f)

    #define TANK_R_TO_SIZE_RATIO                (0.35f)

    float health;
    float timer_is_hit;
    float timer_is_cooling_fire;
    float timer_is_loading_ammo;

    int MAX_NUM_AMMO;
    int num_ammo;
    float move_speed;
    float angle_xy;
    float angle_z;
    float turn_speed;

    public:
    Tank(float x, float y, float z, float r) : Sphere(x, y, z, r), angle_xy{0.0f}, angle_z{0.0f}, turn_speed{TANK_DEFAULT_TURN_SPEED}, move_speed{TANK_DEFAULT_MOVE_SPEED}, MAX_NUM_AMMO{TANK_DEFAULT_MAX_NUM_AMMO}, num_ammo{TANK_DEFAULT_MAX_NUM_AMMO}, health{TANK_DEFAULT_HEALTH}, timer_is_hit{0.0f}, timer_is_cooling_fire{0.0f}, timer_is_loading_ammo{0.0f} { }

    float get_angle_xy() const {
        return this->angle_xy;
    }

    float get_angle_z() const {
        return this->angle_z;
    }

    float get_move_speed() const {
        return this->move_speed;
    }

    float get_turn_speed() const {
        return this->turn_speed;
    }

    bool get_is_alive() const {
        return this->health > 0.0f;
    }

    bool set_is_alive(bool is_alive) {
        if (is_alive) {
            this->health = TANK_DEFAULT_HEALTH;
        }
        else {
            this->health = 0.0f;
        }
        return true;
    }

    bool get_is_hit() const {
        return this->timer_is_hit > 0.0f;
    }

    bool set_is_hit(bool is_hit) {
        if (is_hit) {
            this->timer_is_hit = TANK_DEFAULT_TIMER_IS_HIT;
            this->health -= 1.0f;
        }
        else {
            this->timer_is_hit = 0.0f;
        }
        return true;
    }

    bool turn(float angle_xy) {
        this->angle_xy += angle_xy * this->turn_speed;
        return true;
    }

    Ammo fire() {
        Ammo ammo;
        if (this->get_is_alive() == false || this->timer_is_cooling_fire > 0.0f || this->num_ammo <= 0) {
            // ammo.is_fired() == false;
        }
        else {
            this->timer_is_cooling_fire = TANK_DEFAULT_TIMER_IS_COOLING_FIRE;
            this->num_ammo --;
            ammo = Ammo(this->x, this->y, this->z, this->r / 4.0f, this->angle_xy, this->angle_z, this->move_speed * 5.0f);
            ammo.move(ammo.get_angle_xy(), ammo.get_angle_z(), this->r + ammo.get_r());
        }
        return ammo;
    }

    bool refreash(float time) {
        if (this->get_is_alive() == false) {
            return false;
        }
        if (this->timer_is_hit > 0.0f) {
            this->timer_is_hit -= time;
        }
        if (this->timer_is_cooling_fire > 0.0f) {
            this->timer_is_cooling_fire -= time;
        }
        if (this->timer_is_loading_ammo > 0.0f) {
            this->timer_is_loading_ammo -= time;
        }
        else {
            if (this->num_ammo < this->MAX_NUM_AMMO) {
                this->num_ammo ++;
                this->timer_is_loading_ammo = TANK_DEFAULT_TIMER_IS_LOADING_AMMO;
            }
        }
        return true;
    }

    glm::mat4 get_model_matrix() const {
        glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(this->r * TANK_R_TO_SIZE_RATIO, this->r * TANK_R_TO_SIZE_RATIO, this->r * TANK_R_TO_SIZE_RATIO));
        glm::mat4 trans_mat = glm::translate(glm::mat4(), glm::vec3(this->x, this->y, this->z));
        glm::mat4 rotat_mat = glm::rotate( glm::mat4(1.0f), this->angle_xy, glm::vec3(0, 0, 1) );
        glm::mat4 model_mat = trans_mat * rotat_mat * scale_mat;

        return model_mat;
    }
};

typedef struct TankAction_s {
    float turn_angle_xy;
    float advance_dist;
    int is_firing;
}TankAction_s;

static void get_tank_act_from_user_idx(TankAction_s & tank_act, int user_idx)
{
    tank_act.turn_angle_xy = 0.0f;
    tank_act.advance_dist = 0.0f;
    tank_act.is_firing = 0;

    if (user_idx == 0) {
        static int cnt = 0;
        if (glfwGetKey( window, GLFW_KEY_W ) == GLFW_PRESS){
            tank_act.advance_dist = 1.0f;
        }
        if (glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS){
            tank_act.advance_dist = -1.0f;
        }
        if (glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS){
            tank_act.turn_angle_xy = 1.0f;
        }
        if (glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS){
            tank_act.turn_angle_xy = -1.0f;
        }
        if (glfwGetKey( window, GLFW_KEY_F ) == GLFW_PRESS) {
            cnt ++;
        }
        else if (cnt > 0) {
            cnt --;
        }

        if (cnt > 150) {
            cnt = 0;
            tank_act.is_firing = 1;
        }
    }
    else if (user_idx == 1) {
        static int cnt = 0;
        if (glfwGetKey( window, GLFW_KEY_I ) == GLFW_PRESS){
            tank_act.advance_dist = 1.0f;
        }
        if (glfwGetKey( window, GLFW_KEY_K ) == GLFW_PRESS){
            tank_act.advance_dist = -1.0f;
        }
        if (glfwGetKey( window, GLFW_KEY_J ) == GLFW_PRESS){
            tank_act.turn_angle_xy = 1.0f;
        }
        if (glfwGetKey( window, GLFW_KEY_L ) == GLFW_PRESS){
            tank_act.turn_angle_xy = -1.0f;
        }
        if (glfwGetKey( window, GLFW_KEY_H ) == GLFW_PRESS) {
            cnt ++;
        }
        else if (cnt > 0) {
            cnt --;
        }

        if (cnt > 150) {
            cnt = 0;
            tank_act.is_firing = 1;
        }
    }
    else {
        // do nothing
    }
}

typedef struct Environment_s {
    int is_terminated;
    std::vector<Obst> obst_vec;
    std::vector<Ammo> ammo_vec;
    std::vector<Ammo> rain_vec;
    std::vector<Tank> tank_vec;
} Environment_s;

static bool tank_move_and_check(Tank & tank, float angle_xy, float angle_z, float dist, Environment_s & env, int itr_cnt) {
    if (itr_cnt > 10) {
        return false;
    }
    if (tank.get_is_alive() == false) {
        return false;
    }
    tank.move(angle_xy, angle_z, dist);
    if (tank.check_is_out_of_bound()) {
        tank.move(angle_xy, angle_z, -dist);
        return false;
    }
    for (int obst_idx = 0; obst_idx < env.obst_vec.size(); obst_idx ++) {
        Obst const & obst = env.obst_vec[obst_idx];
        if (obst.get_is_activated() && Sphere::check_is_collided(tank, obst)) {
            tank.move(angle_xy, angle_z, -dist);
            return false;
        }
    }
    for (int tank_idx = 0; tank_idx < env.tank_vec.size(); tank_idx ++) {
        Tank & tank_collided = env.tank_vec[tank_idx];
        if (tank_collided.get_is_alive() && Sphere::check_is_collided(tank, tank_collided)) {
            float angle_xy_collided = 0.0f;
            float angle_z_collided = 0.0f;
            float dist_collided = 0.0f;
            Sphere::get_relation(tank, tank_collided, angle_xy_collided, angle_z_collided, dist_collided);
            dist_collided = tank.get_r() + tank_collided.get_r() - dist_collided;
            if (tank_move_and_check(tank_collided, angle_xy_collided, angle_z_collided, dist_collided, env, itr_cnt + 1)) {
                break;
            }
            else {
                tank.move(angle_xy, angle_z, -dist);
                return false;
            }
        }
    }
    return true;
}

static bool ammo_move_and_check(Ammo & ammo, float angle_xy, float angle_z, float dist, Environment_s & env) {
    if (ammo.get_is_fired() == false) {
        return false;
    }
    ammo.move(angle_xy, angle_z, dist);
    if (ammo.check_is_out_of_bound()) {
        ammo.set_is_fired(false);
        return false;
    }
    for (int obst_idx = 0; obst_idx < env.obst_vec.size(); obst_idx ++) {
        Obst & obst = env.obst_vec[obst_idx];
        if (obst.get_is_activated() && Sphere::check_is_collided(ammo, obst)) {
            obst.set_is_hit(true);
            ammo.set_is_fired(false);
            return false;
        }
    }
    for (int tank_idx = 0; tank_idx < env.tank_vec.size(); tank_idx ++) {
        Tank & tank_collided = env.tank_vec[tank_idx];
        if (tank_collided.get_is_alive() && Sphere::check_is_collided(ammo, tank_collided)) {
            float angle_xy_collided = 0.0f;
            float angle_z_collided = 0.0f;
            float dist_collided = 0.0f;
            Sphere::get_relation(ammo, tank_collided, angle_xy_collided, angle_z_collided, dist_collided);
            tank_collided.set_is_hit(true);
            tank_move_and_check(tank_collided, angle_xy_collided, angle_z_collided, ammo.get_r(), env, 0);
            ammo.set_is_fired(false);
            return false;
        }
    }

    return true;
}

static bool env_init(Environment_s & env) {
    env.is_terminated = 0;

    srand(0);

    for (int obst_idx = 0; obst_idx < 20; obst_idx ++) {
        float x = BOUND_X_MIN + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (BOUND_X_MAX - BOUND_X_MIN)));
        float y = BOUND_Y_MIN + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (BOUND_Y_MAX - BOUND_Y_MIN)));
        float z = 0.0f;
        float r = 1.0f;

        Obst obstacle(x, y, z, r);
        env.obst_vec.push_back(obstacle);
    }

    for (int rain_idx = 0; rain_idx < 0; rain_idx ++) {
        Ammo rain;
        env.rain_vec.push_back(rain);
    }

    env.tank_vec.push_back(Tank(-5.0f, -5.0f, 0.0f, 2.0f));
    env.tank_vec.push_back(Tank(5.0f, 5.0f, 0.0f, 2.0f));

    return true;
}

static bool env_refresh(Environment_s & env, float time) {
    for (int obst_idx = 0; obst_idx < env.obst_vec.size(); obst_idx ++) {
        Obst & obst = env.obst_vec[obst_idx];
        if (obst.get_is_activated()) {
            obst.refreash(time);
        }
    }

    for (int tank_idx = 0; tank_idx < env.tank_vec.size(); tank_idx ++) {
        Tank & tank = env.tank_vec[tank_idx];
        if (tank.get_is_alive()) {
            tank.refreash(time);
        }
    }

    return true;
}


static void env_proc_main(Environment_s * p_arg) {
    // glfwGetTime is called only once, the first time this function is called
    static double last_time = glfwGetTime();

    while (p_arg != NULL && p_arg->is_terminated == 0) {
        Environment_s & env = *p_arg;

        // Compute time difference between current and last frame
        double curr_time = glfwGetTime();
        float delta_time = float(curr_time - last_time);
        last_time = curr_time;

        env_refresh(env, delta_time);

        TankAction_s tank_act;
        for (int tank_idx = 0; tank_idx < p_arg->tank_vec.size(); tank_idx ++) {
            Tank & tank = p_arg->tank_vec[tank_idx];

            get_tank_act_from_user_idx(tank_act, tank_idx);
            tank.turn(tank_act.turn_angle_xy * delta_time);
            tank_move_and_check(tank, tank.get_angle_xy(), tank.get_angle_z(), tank_act.advance_dist * delta_time, env, 0);
            if (tank_act.is_firing == 1) {
                p_arg->ammo_vec.push_back(tank.fire());
            }
        }

        auto ammo_itr = p_arg->ammo_vec.begin();
        while (ammo_itr != p_arg->ammo_vec.end()) {
            ammo_move_and_check((*ammo_itr), ammo_itr->get_angle_xy(), ammo_itr->get_angle_z(), ammo_itr->get_move_speed() * delta_time, env);
            if (ammo_itr->get_is_fired()) {
                ammo_itr ++;
            }
            else {
                ammo_itr = p_arg->ammo_vec.erase(ammo_itr);
            }
        }

        for (int rain_idx = 0; rain_idx < p_arg->rain_vec.size(); rain_idx ++) {
            Ammo & rain = p_arg->rain_vec[rain_idx];
            rain.move(rain.get_angle_xy(), rain.get_angle_z(), rain.get_move_speed() * delta_time);
            if (rain.check_is_out_of_bound()) {
                rain.set_is_fired(false);
            }

            if (rain.get_is_fired()) {
                for (int obst_idx = 0; obst_idx < p_arg->obst_vec.size(); obst_idx ++) {
                    Obst & obst = p_arg->obst_vec[obst_idx];
                    if (obst.get_is_activated() && Sphere::check_is_collided(rain, obst)) {
                        obst.set_is_hit(true);

                        rain.set_is_fired(false);
                    }
                }
            }

            if (rain.get_is_fired()) {
                for (int tank_idx = 0; tank_idx < p_arg->tank_vec.size(); tank_idx ++) {
                    Tank & tank = p_arg->tank_vec[tank_idx];
                    if (tank.get_is_alive() && Sphere::check_is_collided(rain, tank)) {
                        tank.set_is_hit(true);

                        rain.set_is_fired(false);
                    }
                }
            }

            if (rain.get_is_fired() == false) {
                rain = Ammo(
                BOUND_X_MIN + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (BOUND_X_MAX - BOUND_X_MIN))),
                BOUND_Y_MIN + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (BOUND_Y_MAX - BOUND_Y_MIN))),
                BOUND_Z_MAX,
                0.1f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (0.5f - 0.1f))),
                0.0f,
                -MY_PI_HALF,
                1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (5.0f - 1.0f))));
            }
        }

        // printf("p_arg->ammo_vec.size() = %d\n", p_arg->ammo_vec.size());

        // Compute the MVP matrix from keyboard and mouse input
        computeMatricesFromInputs();
    }
}


int main( void ) {
    // Initialise GLFW
    if( !glfwInit() ) {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "Tutorial 09 - Loading with AssImp", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    // glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders( "StandardShading.vertexshader", "StandardShading.fragmentshader" );

    // Get a handle for our "MVP" uniform
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
    GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

    // Get a handle for our "myTextureSampler" uniform
    GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

    // Get a handle for our "LightPosition" uniform
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

    GLuint ColorAddedID = glGetUniformLocation(programID, "MaterialDiffuseColor_Added");

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    /*****************************************************************************/
    /******************************** LOAD GROUND ********************************/
    /*****************************************************************************/

    GLuint ground_texture = loadBMP_custom("ground.bmp");

    GLuint ground_vert_buf;
    glGenBuffers(1, &ground_vert_buf);
    glBindBuffer(GL_ARRAY_BUFFER, ground_vert_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_ground_vect_buf_data), g_ground_vect_buf_data, GL_STATIC_DRAW);

    GLuint ground_uv_buf;
    glGenBuffers(1, &ground_uv_buf);
    glBindBuffer(GL_ARRAY_BUFFER, ground_uv_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_ground_uv_buf_data) * sizeof(glm::vec2), g_ground_uv_buf_data, GL_STATIC_DRAW);

    GLuint ground_norm_buf;
    glGenBuffers(1, &ground_norm_buf);
    glBindBuffer(GL_ARRAY_BUFFER, ground_norm_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_ground_norm_buf_data), g_ground_norm_buf_data, GL_STATIC_DRAW);

    /*****************************************************************************/
    /******************************** LOAD OBST **********************************/
    /*****************************************************************************/

    GLuint obst_texture = loadDDS("uvmap.DDS");
    std::vector<unsigned short> obst_indices;
    std::vector<glm::vec3> obst_indexed_vertices;
    std::vector<glm::vec2> obst_indexed_uvs;
    std::vector<glm::vec3> obst_indexed_normals;
    bool is_obst_loaded = loadAssImp("suzanne_ori.obj", obst_indices, obst_indexed_vertices, obst_indexed_uvs, obst_indexed_normals);
    if (is_obst_loaded == false)
    {
        printf("Failed to load obj\n");
    }

    GLuint obst_vect_buf;
    glGenBuffers(1, &obst_vect_buf);
    glBindBuffer(GL_ARRAY_BUFFER, obst_vect_buf);
    glBufferData(GL_ARRAY_BUFFER, obst_indexed_vertices.size() * sizeof(glm::vec3), &obst_indexed_vertices[0], GL_STATIC_DRAW);

    GLuint obst_uv_buf;
    glGenBuffers(1, &obst_uv_buf);
    glBindBuffer(GL_ARRAY_BUFFER, obst_uv_buf);
    glBufferData(GL_ARRAY_BUFFER, obst_indexed_uvs.size() * sizeof(glm::vec2), &obst_indexed_uvs[0], GL_STATIC_DRAW);

    GLuint obst_norm_buf;
    glGenBuffers(1, &obst_norm_buf);
    glBindBuffer(GL_ARRAY_BUFFER, obst_norm_buf);
    glBufferData(GL_ARRAY_BUFFER, obst_indexed_normals.size() * sizeof(glm::vec3), &obst_indexed_normals[0], GL_STATIC_DRAW);

    GLuint obst_elem_buf;
    glGenBuffers(1, &obst_elem_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obst_elem_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obst_indices.size() * sizeof(unsigned short), &obst_indices[0] , GL_STATIC_DRAW);

    /*****************************************************************************/
    /******************************** LOAD TANK **********************************/
    /*****************************************************************************/

    GLuint tank_texture = loadDDS("uvmap.DDS");
    std::vector<unsigned short> tank_indices;
    std::vector<glm::vec3> tank_indexed_vertices;
    std::vector<glm::vec2> tank_indexed_uvs;
    std::vector<glm::vec3> tank_indexed_normals;


    bool is_tank_loaded = loadOBJ("tank.obj", vertices, uvs, normals);
    indexVBO(vertices, uvs, normals, tank_indices, tank_indexed_vertices, tank_indexed_uvs, tank_indexed_normals);

    // bool is_tank_loaded = loadAssImp("suzanne_ori.obj", tank_indices, tank_indexed_vertices, tank_indexed_uvs, tank_indexed_normals);
    if (is_tank_loaded == false)
    {
        printf("Failed to load obj\n");
    }

    GLuint tank_vect_buf;
    glGenBuffers(1, &tank_vect_buf);
    glBindBuffer(GL_ARRAY_BUFFER, tank_vect_buf);
    glBufferData(GL_ARRAY_BUFFER, tank_indexed_vertices.size() * sizeof(glm::vec3), &tank_indexed_vertices[0], GL_STATIC_DRAW);

    GLuint tank_uv_buf;
    glGenBuffers(1, &tank_uv_buf);
    glBindBuffer(GL_ARRAY_BUFFER, tank_uv_buf);
    glBufferData(GL_ARRAY_BUFFER, tank_indexed_uvs.size() * sizeof(glm::vec2), &tank_indexed_uvs[0], GL_STATIC_DRAW);

    GLuint tank_norm_buf;
    glGenBuffers(1, &tank_norm_buf);
    glBindBuffer(GL_ARRAY_BUFFER, tank_norm_buf);
    glBufferData(GL_ARRAY_BUFFER, tank_indexed_normals.size() * sizeof(glm::vec3), &tank_indexed_normals[0], GL_STATIC_DRAW);

    GLuint tank_elem_buf;
    glGenBuffers(1, &tank_elem_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tank_elem_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, tank_indices.size() * sizeof(unsigned short), &tank_indices[0] , GL_STATIC_DRAW);



    /*****************************************************************************/
    /******************************** LOAD AMMO **********************************/
    /*****************************************************************************/

    GLuint ammo_texture = loadDDS("uvmap.DDS");
    // GLuint ammo_texture = loadDDS("bullet.dds");

    // Read our .obj file
    std::vector<unsigned short> ammo_indices;
    std::vector<glm::vec3> ammo_indexed_vertices;
    std::vector<glm::vec2> ammo_indexed_uvs;
    std::vector<glm::vec3> ammo_indexed_normals;

    // bool is_ammo_loaded = loadOBJ("suzanne.obj", vertices, uvs, normals);
    // indexVBO(vertices, uvs, normals, ammo_indices, ammo_indexed_vertices, ammo_indexed_uvs, ammo_indexed_normals);

    bool is_ammo_loaded = loadAssImp("suzanne_ori.obj", ammo_indices, ammo_indexed_vertices, ammo_indexed_uvs, ammo_indexed_normals);
    if (is_ammo_loaded == false)
    {
        printf("Failed to load ammo obj\n");
    }

    GLuint ammo_vect_buf;
    glGenBuffers(1, &ammo_vect_buf);
    glBindBuffer(GL_ARRAY_BUFFER, ammo_vect_buf);
    glBufferData(GL_ARRAY_BUFFER, ammo_indexed_vertices.size() * sizeof(glm::vec3), &ammo_indexed_vertices[0], GL_STATIC_DRAW);

    GLuint ammo_uv_buf;
    glGenBuffers(1, &ammo_uv_buf);
    glBindBuffer(GL_ARRAY_BUFFER, ammo_uv_buf);
    glBufferData(GL_ARRAY_BUFFER, ammo_indexed_uvs.size() * sizeof(glm::vec2), &ammo_indexed_uvs[0], GL_STATIC_DRAW);

    GLuint ammo_norm_buf;
    glGenBuffers(1, &ammo_norm_buf);
    glBindBuffer(GL_ARRAY_BUFFER, ammo_norm_buf);
    glBufferData(GL_ARRAY_BUFFER, ammo_indexed_normals.size() * sizeof(glm::vec3), &ammo_indexed_normals[0], GL_STATIC_DRAW);

    GLuint ammo_elem_buf;
    glGenBuffers(1, &ammo_elem_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ammo_elem_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ammo_indices.size() * sizeof(unsigned short), &ammo_indices[0] , GL_STATIC_DRAW);



    glUseProgram(programID);

    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    Environment_s env;
    env_init(env);
    std::thread env_proc_thread(env_proc_main, &env);

    do{

        // Measure speed
        double currentTime = glfwGetTime();
        nbFrames++;
        if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
            // printf and reset
            printf("%f ms/frame\n", 1000.0/double(nbFrames));
            nbFrames = 0;
            lastTime += 1.0;
        }

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        glm::vec3 lightPos = glm::vec3(5, 5, 20);
        glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();

        /*****************************************************************************/
        /******************************** DRAW GROUND ********************************/
        /*****************************************************************************/

        glm::mat4 ground_scale_mat;
        glm::mat4 ground_rotate_mat;
        glm::mat4 ground_model_mat;
        glm::mat4 ground_mvp_mat;

        ground_scale_mat = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 20.0f));
        ground_rotate_mat = glm::rotate(ground_scale_mat, 0.0f, glm::vec3(1, 0, 0));
        ground_model_mat = ground_rotate_mat; // glm::mat4(1.0);
        ground_mvp_mat = ProjectionMatrix * ViewMatrix * ground_model_mat;

        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &ground_mvp_mat[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ground_model_mat[0][0]);
        glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
        glUniform3f(ColorAddedID, 0.0f, 0.0f, 0.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ground_texture);
        glUniform1i(TextureID, 0);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, ground_vert_buf);
        glVertexAttribPointer(
            0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, ground_uv_buf);
        glVertexAttribPointer(
            1,                                // attribute
            2,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // 3rd attribute buffer : normals
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, ground_norm_buf);
        glVertexAttribPointer(
            2,                                // attribute
            3,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0, 6*3);

        /*****************************************************************************/
        /********************************* DRAW OBST *********************************/
        /*****************************************************************************/

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, obst_texture);
        glUniform1i(TextureID, 1);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, obst_vect_buf);
        glVertexAttribPointer(
            0,                  // attribute
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, obst_uv_buf);
        glVertexAttribPointer(
            1,                                // attribute
            2,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // 3rd attribute buffer : normals
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, obst_norm_buf);
        glVertexAttribPointer(
            2,                                // attribute
            3,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // Index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obst_elem_buf);

        for (int obst_idx = 0; obst_idx < env.obst_vec.size(); obst_idx ++)
        {
            Obst const & obst = env.obst_vec[obst_idx];
            if (obst.get_is_activated() == false) {
                continue;
            }
            glm::mat4 obst_model_mat = obst.get_model_matrix(); //glm::mat4(1.0);
            glm::mat4 obst_mvp_mat = ProjectionMatrix * ViewMatrix * obst_model_mat;

            // Send our transformation to the currently bound shader,
            // in the "MVP" uniform
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &obst_mvp_mat[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &obst_model_mat[0][0]);
            glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
            glUniform3f(ColorAddedID, 0.0f, 0.0f, 0.0f);
            if (obst.get_is_hit()) {
                glUniform3f(ColorAddedID, 255.0f, 0.0f, 0.0f);
            }

            // Draw the triangles !
            glDrawElements(
                GL_TRIANGLES,      // mode
                obst_indices.size(),    // count
                GL_UNSIGNED_SHORT,   // type
                (void*)0           // element array buffer offset
            );
        }

        /*****************************************************************************/
        /********************************* DRAW TANK *********************************/
        /*****************************************************************************/

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, tank_texture);
        glUniform1i(TextureID, 2);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, tank_vect_buf);
        glVertexAttribPointer(
            0,                  // attribute
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, tank_uv_buf);
        glVertexAttribPointer(
            1,                                // attribute
            2,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // 3rd attribute buffer : normals
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, tank_norm_buf);
        glVertexAttribPointer(
            2,                                // attribute
            3,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // Index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tank_elem_buf);

        for (int tank_idx = 0; tank_idx < env.tank_vec.size(); tank_idx ++)
        {
            Tank const & tank = env.tank_vec[tank_idx];
            if (tank.get_is_alive() == false) {
                continue;
            }

            glm::mat4 tank_model_mat = tank.get_model_matrix(); //glm::mat4(1.0);
            glm::mat4 tank_mvp_mat = ProjectionMatrix * ViewMatrix * tank_model_mat;

            // Send our transformation to the currently bound shader,
            // in the "MVP" uniform
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &tank_mvp_mat[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &tank_model_mat[0][0]);
            glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
            glUniform3f(ColorAddedID, 0.0f, 0.0f, 0.0f);
            if (tank.get_is_hit()){
                glUniform3f(ColorAddedID, 255.0f, 0.0f, 0.0f);
            }

            // Draw the triangles !
            glDrawElements(
                GL_TRIANGLES,      // mode
                tank_indices.size(),    // count
                GL_UNSIGNED_SHORT,   // type
                (void*)0           // element array buffer offset
            );
        }

        /*****************************************************************************/
        /********************************* DRAW AMMO *********************************/
        /*****************************************************************************/

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, ammo_texture);
        glUniform1i(TextureID, 3);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, ammo_vect_buf);
        glVertexAttribPointer(
            0,                  // attribute
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, ammo_uv_buf);
        glVertexAttribPointer(
            1,                                // attribute
            2,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // 3rd attribute buffer : normals
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, ammo_norm_buf);
        glVertexAttribPointer(
            2,                                // attribute
            3,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
        );

        // Index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ammo_elem_buf);

        for (int ammo_idx = 0; ammo_idx < env.ammo_vec.size(); ammo_idx ++)
        {
            Ammo const & ammo = env.ammo_vec[ammo_idx];
            if (ammo.get_is_fired() == false) {
                continue;
            }

            glm::mat4 ammo_model_mat = env.ammo_vec[ammo_idx].get_model_matrix(); //glm::mat4(1.0);
            glm::mat4 ammo_mvp_mat = ProjectionMatrix * ViewMatrix * ammo_model_mat;

            // Send our transformation to the currently bound shader,
            // in the "MVP" uniform
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &ammo_mvp_mat[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ammo_model_mat[0][0]);
            glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
            glUniform3f(ColorAddedID, 0.0f, 0.0f, 0.0f);

            // Draw the triangles !
             glDrawElements(
                GL_TRIANGLES,      // mode
                ammo_indices.size(),    // count
                GL_UNSIGNED_SHORT,   // type
                (void*)0           // element array buffer offset
             );
        }

        for (int rain_idx = 0; rain_idx < env.rain_vec.size(); rain_idx ++)
        {
            Ammo const & rain = env.rain_vec[rain_idx];
            if (rain.get_is_fired() == false) {
                continue;
            }

            glm::mat4 rain_model_mat = env.rain_vec[rain_idx].get_model_matrix(); //glm::mat4(1.0);
            glm::mat4 rain_mvp_mat = ProjectionMatrix * ViewMatrix * rain_model_mat;

            // Send our transformation to the currently bound shader,
            // in the "MVP" uniform
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &rain_mvp_mat[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &rain_model_mat[0][0]);
            glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
            glUniform3f(ColorAddedID, 0.0f, 0.0f, 0.0f);

            // Draw the triangles !
             glDrawElements(
                GL_TRIANGLES,      // mode
                ammo_indices.size(),    // count
                GL_UNSIGNED_SHORT,   // type
                (void*)0           // element array buffer offset
             );
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );


    glDeleteProgram(programID);

    // Cleanup VBO and shader
    glDeleteTextures(1, &ground_texture);
    glDeleteBuffers(1, &ground_vert_buf);
    glDeleteBuffers(1, &ground_uv_buf);
    glDeleteBuffers(1, &ground_norm_buf);

    glDeleteTextures(1, &obst_texture);
    glDeleteBuffers(1, &obst_vect_buf);
    glDeleteBuffers(1, &obst_uv_buf);
    glDeleteBuffers(1, &obst_norm_buf);
    glDeleteBuffers(1, &obst_elem_buf);

    glDeleteTextures(1, &tank_texture);
    glDeleteBuffers(1, &tank_vect_buf);
    glDeleteBuffers(1, &tank_uv_buf);
    glDeleteBuffers(1, &tank_norm_buf);
    glDeleteBuffers(1, &tank_elem_buf);

    glDeleteTextures(1, &ammo_texture);
    glDeleteBuffers(1, &ammo_vect_buf);
    glDeleteBuffers(1, &ammo_uv_buf);
    glDeleteBuffers(1, &ammo_norm_buf);
    glDeleteBuffers(1, &ammo_elem_buf);

    glDeleteVertexArrays(1, &VertexArrayID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    env.is_terminated = 1;
    env_proc_thread.join();

    return 0;
}

