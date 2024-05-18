/*
    James William Fletcher  ( github.com/mrbid )
        May 2024

    It's good to be king. Wait, maybe. I think maybe I'm just like a little bizarre little person who walks back and forth.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define AUDIO_ON
#ifdef AUDIO_ON
    #include <alsa/asoundlib.h>
    #include <pthread.h>
    #include "assets/audio/song.h"
#endif

#define uint GLuint
#define sint GLint

#include "inc/gl.h"
#define GLFW_INCLUDE_NONE
#include "inc/glfw3.h"
#define fTime() (float)glfwGetTime()

#define GL_DEBUG
#define MAX_MODELS 9 // hard limit, be aware and increase if needed
#include "inc/esAux7.h"

#include "inc/res.h"

#include "assets/high/track.h"  //0
#include "assets/high/bg.h"     //1
#include "assets/high/car.h"    //2
#include "assets/high/train.h"  //3
#include "assets/high/e1.h"     //4
#include "assets/high/e2.h"     //5
#include "assets/high/alien.h"  //6
#include "assets/high/cia.h"    //7
#include "assets/high/terry.h"  //8

//*************************************
// globals
//*************************************
const char appTitle[]="Temple Driver";
GLFWwindow* window;
uint winw=1024, winh=768;
float t=0.f, dt=0.f, lt=0.f, fc=0.f, lfct=0.f, aspect;

// render state
mat projection, view, model, modelview;

// game vars
#define FAR_DISTANCE 333.f
#define ROAD_TILES 5
uint keystate[2] = {0};
float rspeed = 1.f;
float carx = 0.f;
float carz = 0.f;
float carr = 0.f;
float carxt = 0.f;
float roady[ROAD_TILES];
float ntrain = 0.f;
float trainy = 6.7f;
float trains = 1.f;
float cial[3];
float ciar[3];
uint dcial[3];
uint dciar[3];
uint vcial[3];
uint vciar[3];
uint score = 0;
uint spawned = 0;
float ascend = 0.1f;

void resetGame()
{
    for(uint i=0; i < ROAD_TILES; i++)
        roady[i] = 1.34f*(float)i;
    for(uint i=0; i < 3; i++)
    {
        cial[i] = -2.68f;
        ciar[i] = -2.68f;
    }
    score = 0;
    spawned = 0;
    carx = 0.f;
    carz = 0.f;
    carr = 0.f;
    carxt = 0.f;
    ntrain = 0.f;
    ascend = 0.1f;
    rspeed = 1.f;
    ntrain = t+3.f;
}

//*************************************
// utility functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}
void updateModelView()
{
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (float*)&modelview.m[0][0]);
}

//*************************************
// audio thread
//*************************************
#ifdef AUDIO_ON
#define AUDIOBUF 1024
uint raud = 0;
void *audioThread(void *arg)
{
    uint si = 0;
    unsigned int urate = 48000;
    unsigned char buf[AUDIOBUF];
    snd_pcm_t *pcm;
    snd_pcm_hw_params_t *hwp;
    while(1)
    {
        snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
        snd_pcm_hw_params_malloc(&hwp);
        snd_pcm_hw_params_any(pcm, hwp);
        snd_pcm_hw_params_set_format(pcm, hwp, SND_PCM_FORMAT_U8);
        snd_pcm_hw_params_set_rate_near(pcm, hwp, &urate, 0);
        snd_pcm_hw_params_set_channels(pcm, hwp, 1);
        snd_pcm_hw_params(pcm, hwp);
        snd_pcm_hw_params_free(hwp);
        snd_pcm_prepare(pcm);
        while(1)
        {
            if(raud == 1)
            {
                si = 288000;
                raud=0;
                break;
            }
            for(int i = 0; i < AUDIOBUF; i++)
            {
                buf[i] = song[si];
                if(++si >= song_size){si = 0;}
            }
            if(snd_pcm_writei(pcm, buf, AUDIOBUF) < 0){break;}
        }
        snd_pcm_close(pcm);
    }

    return 0;
}
#endif

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// core logic
//*************************************
    fc++;
    glfwPollEvents();
    t = fTime();
    dt = t-lt;
    lt = t;

//*************************************
// game logic
//*************************************

    // if car is not dead
    static float nbt = 0.f;
    if(carr == 0.f)
    {
        // controls
        if(keystate[0] == 1)
        {
            if(carxt > -0.35f){carxt -= 0.35f;}
        }
        else if(keystate[1] == 1)
        {
            if(carxt < 0.35f){carxt += 0.35f;}
        }

        // simulate car
        // if(carx != carxt){carx -= ((carx-carxt)*1.7f)*dt;}
        if(carx < carxt){carx -= ((carx-carxt)*1.7f)*dt;}
        else if(carx > carxt){carx += ((carxt-carx)*1.7f)*dt;}

        // bumpy road simulation
        if(t > nbt)
        {
            carz = -(randf()*0.005f);
            nbt = t+randf()*0.22f;
        }

        // road movement
        const float rdi = rspeed*dt;
        for(uint i=0; i < ROAD_TILES; i++)
        {
            roady[i] -= rdi;
            if(roady[i] < -2.68f){roady[i] += 1.34f*ROAD_TILES;}
        }

        // spawn cia
        static float ns = 0.f;
        if(t > ns)
        {
            for(uint i=0; i < 3; i++)
            {
                if(cial[i] <= -2.68f)
                {
                    cial[i] = 6.7f;
                    dcial[i] = 0;
                    spawned++;
                    break;
                }
                else if(ciar[i] <= -2.68f)
                {
                    ciar[i] = 6.7f;
                    dciar[i] = 0;
                    spawned++;
                    break;
                }
            }
            ns = t+esRandFloat(1.f, 3.f);
        }

        // flip cia
        static float nv = 0.f;
        if(t > nv)
        {
            if(randf() > 0.5f)
                vcial[0] = 1-vcial[0];
            else
                vciar[0] = 1-vciar[0];
            nv = t+esRandFloat(0.1f, 0.3f);
        }
    }

    // train
    if(t > ntrain)
    {
        trainy -= (rspeed+trains)*dt;
        if(trainy < -2.68f)
        {
            trainy = 6.7f;
            trains = esRandFloat(1.f, 4.f);
            ntrain = t+esRandFloat(3.f, 13.f);
        }
    }

    // death collision
    if(fabsf(carx) < 0.2f && fabsf(0.13f - trainy) < 0.420f)
    {
        if(carr == 0.f){raud = 1;}
        carr += 6.f*dt;
    }

    // camera
    mIdent(&view);
    mRotY(&view, 80.f*DEG2RAD);
    mSetPos(&view, (vec){0.f, -0.2f, -2.f});

//*************************************
// render
//*************************************

    // clear buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // train
    if(carr != 0.f)
    {
        mIdent(&model);
        mSetPos(&model, (vec){0.f, trainy, 0.f});
        updateModelView();
        esBindRender(4);

        if(ascend < 0.5f)
        {
            ascend += 0.066f*dt;
            mIdent(&model);
            mSetPos(&model, (vec){0.f, 0.f, ascend});
            updateModelView();
            esBindRender(8);
        }
        // else
        // {
        //     ascend += 0.066f*dt;
        //     glEnable(GL_BLEND);
        //     glUniform1f(opacity_id, 1.f-((ascend-0.5f)*2.f));
        //     mIdent(&model);
        //     mSetPos(&model, (vec){0.f, 0.f, 0.5f});
        //     updateModelView();
        //     esBindRender(10);
        //     glDisable(GL_BLEND);
        // }
    }
    else
    {
        mIdent(&model);
        mSetPos(&model, (vec){0.f, trainy, 0.f});
        updateModelView();
        esBindRender(3);
    }

    // CIA
    if(carr == 0.f)
    {
        for(uint i=0; i < 3; i++)
        {
            if(cial[i] > -2.68f)
            {
                cial[i] -= rspeed*dt;
                mIdent(&model);
                mSetPos(&model, (vec){-0.35f, cial[i], 0.f});
                if(dcial[i] == 1){mRotY(&model, 90.f*DEG2RAD);}
                else if(dcial[i] == 0 && carx <= -0.2f && cial[i] > -0.5f && cial[i] < 0.132f)
                {
                    if(vcial[i] == 1)
                    {
                        if(carxt < 0.35f){carxt += 0.35f;}
                    }
                    else
                    {
                        rspeed += 0.03f;
                        carz = 0.01f;
                        nbt = t+0.22f;
                        dcial[i] = 1;
                        score++;
                        printf("SCORE: %u/%u\n", score, spawned);
                        char tmp[256];
                        sprintf(tmp, "Temple 👽 %u 👽 Driver", score);
                        glfwSetWindowTitle(window, tmp);
                    }
                }
                updateModelView();
                if(vcial[i] == 0)
                    esBindRender(6);
                else
                    esBindRender(7);
            }
            if(ciar[i] > -2.68f)
            {
                ciar[i] -= rspeed*dt;
                mIdent(&model);
                mSetPos(&model, (vec){0.35f, ciar[i], 0.f});
                if(dciar[i] == 1){mRotY(&model, 90.f*DEG2RAD);}
                else if(dciar[i] == 0 && carx >= 0.2f && ciar[i] > -0.5f && ciar[i] < 0.132f)
                {
                    if(vciar[i] == 1)
                    {
                        if(carxt > -0.35f){carxt -= 0.35f;}
                    }
                    else
                    {
                        rspeed += 0.03f;
                        carz = 0.01f;
                        nbt = t+0.22f;
                        dciar[i] = 1;
                        score++;
                        printf("SCORE: %u/%u\n", score, spawned);
                        char tmp[256];
                        sprintf(tmp, "Temple 👽 %u 👽 Driver", score);
                        glfwSetWindowTitle(window, tmp);
                    }
                }
                updateModelView();
                if(vciar[i] == 0)
                    esBindRender(6);
                else
                    esBindRender(7);
            }
        }
    }

    // car
    mIdent(&model);
    if(carr != 0.f){mRotZ(&model, carr);}
    mSetPos(&model, (vec){carx, 0.13f, carz});
    updateModelView();
    esBindRender(2);

    // elephants
    if(carr != 0.f)
    {
        mIdent(&model);
        mSetPos(&model, (vec){0.f, 3.2f, 0.f});
        updateModelView();
        esBindRender(5);
    }

    // road 
    esBindModel(0);
    for(uint i=0; i < ROAD_TILES; i++)
    {
        mIdent(&model);
        mSetPos(&model, (vec){0.f, roady[i], 0.f});
        updateModelView();
        esRenderModel(0);
    }

    // bg
    esBindModel(1);

    mIdent(&model);
    mSetPos(&model, (vec){0.f, 4.2f, 0.f});
    updateModelView();
    esRenderModel(1);

    glEnable(GL_BLEND);

        glUniform1f(opacity_id, 0.75f);
        mIdent(&model);
        mRotY(&model, sin(t)*0.2f);
        mSetPos(&model, (vec){0.f, 3.5f, 0.f});
        updateModelView();
        esRenderModel(1);

        glUniform1f(opacity_id, 0.5f);
        mIdent(&model);
        mRotX(&model, sin(t*0.5f)*0.3f);
        mSetPos(&model, (vec){0.f, 3.0f, 0.f});
        updateModelView();
        esRenderModel(1);

        glUniform1f(opacity_id, 0.25f);
        mIdent(&model);
        mRotX(&model, -sin(t*0.5f)*0.3f);
        mSetPos(&model, (vec){0.f, 2.5f, 0.f});
        updateModelView();
        esRenderModel(1);

        if(ascend >= 0.5f)
        {
            ascend += 0.0333f*dt;
            glUniform1f(opacity_id, 1.f-((ascend-0.5f)*2.f));
            mIdent(&model);
            mSetPos(&model, (vec){0.f, 0.f, 0.5f});
            updateModelView();
            esBindRender(8);
        }

    glDisable(GL_BLEND);

    // display render
    glfwSwapBuffers(window);
}

//*************************************
// input
//*************************************
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(ascend > 1.0f){resetGame();return;}
    if(action == GLFW_PRESS && keystate[0] == 0 && keystate[1] == 0)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT){keystate[0] = 1;}
        else if(button == GLFW_MOUSE_BUTTON_RIGHT){keystate[1] = 1;}
    }
    else if(action == GLFW_RELEASE)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT){keystate[0] = 0;}
        else if(button == GLFW_MOUSE_BUTTON_RIGHT){keystate[1] = 0;}
    }
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(ascend > 1.0f){resetGame();return;}
    if(action == GLFW_PRESS && keystate[0] == 0 && keystate[1] == 0)
    {
        if(     key == GLFW_KEY_LEFT)  { keystate[0] = 1; }
        else if(key == GLFW_KEY_RIGHT) { keystate[1] = 1; }
        else if(key == GLFW_KEY_F) // show average fps
        {
            if(t-lfct > 2.0)
            {
                char strts[16];
                timestamp(&strts[0]);
                printf("[%s] FPS: %g\n", strts, fc/(t-lfct));
                lfct = t;
                fc = 0;
            }
        }
    }
    else if(action == GLFW_RELEASE)
    {
        if(     key == GLFW_KEY_LEFT)  { keystate[0] = 0; }
        else if(key == GLFW_KEY_RIGHT) { keystate[1] = 0; }
    }
}
void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width, winh = height;
    glViewport(0, 0, winw, winh);
    aspect = (float)winw / (float)winh;
    mIdent(&projection);
    mPerspective(&projection, 30.0f, aspect, 0.1f, FAR_DISTANCE);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
}

//*************************************
// process entry point
//*************************************
int main(int argc, char** argv)
{
    // allow custom msaa level
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}

    // help
    printf("----\n");
    printf("James William Fletcher (github.com/mrbid)\n");
    printf("%s - It's good to be king. Wait, maybe. I think maybe I'm just like a little bizarre little person who walks back and forth.\n", appTitle);
    printf("----\n");
    printf("The CIA glow in the dark you can see 'em if you're driving, you just run them over that's what you do.\n");
    printf("----\n");
    printf("Terry's 1st Temple.\n");
    printf("----\n");
    printf("Terry was living in his Dodge Caravan travelling around the USA at the time of his ascension to the throne, his last known mortal location was: W 1st St, The Dalles, Oregon. By the Tjunction of Bargeway Rd.\n");
    printf("https://www.google.com/maps/@45.6090711,-121.1957259,3a,75y,257.48h,82.24t/data=!3m6!1e1!3m4!1sEuZ0VE3C7GPl-Fhc1iG06g!2e0!7i13312!8i6656!5m1!1e4?coh=205409&entry=ttu\n");
    printf("----\n");
    printf("One command line argument, msaa 0-16.\n");
    printf("e.g; ./templedriver 16\n");
    printf("----\n");
    printf("Left & Right Click / Arrow Keys = Move\n");
    printf("----\n");
    printf("All assets where generated using LUMA GENIE (https://lumalabs.ai/genie) & TripoAI (https://www.tripo3d.ai).\n");
    printf("----\n");
    printf("Music: TempleOS Hymn Risen (Remix) - Dave Eddy\n");
    printf("https://soundcloud.com/daveeddy/templeosremix\n");
    printf("https://music.daveeddy.com/\n");
    printf("----\n");
    printf("Dedicated to the smartest programmer that ever lived, Terry A. Davis. (https://templeos.org/)\n");
    printf("----\n");
    printf("%s\n", glfwGetVersionString());
    printf("----\n");

    // init glfw
    if(!glfwInit()){printf("glfwInit() failed.\n"); exit(EXIT_FAILURE);}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, msaa);
    window = glfwCreateWindow(winw, winh, appTitle, NULL, NULL);
    if(!window)
    {
        printf("glfwCreateWindow() failed.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center window on desktop
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

    // set icon
    glfwSetWindowIcon(window, 1, &(GLFWimage){16, 16, (unsigned char*)icon_image});

#ifdef AUDIO_ON
    // create audio thread
    pthread_t tid;
    if(pthread_create(&tid, NULL, audioThread, NULL) != 0)
    {
        printf("pthread_create(audioThread) failed.\n");
        return 0;
    }
#endif

//*************************************
// bind vertex and index buffers
//*************************************
    esLoadModel(track);
    esLoadModel(bg);
    esLoadModel(car);
    esLoadModel(train);
    esLoadModel(e1);
    esLoadModel(e2);
    esLoadModel(alien);
    esLoadModel(cia);
    esLoadModel(terry);

//*************************************
// configure render options
//*************************************
    makeLambert();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.f, 0.f, 0.f, 0.f);

    shadeLambert(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &ambient_id, &saturate_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
    window_size_callback(window, winw, winh);
    glUniform1f(ambient_id, 0.648f);
    glUniform1f(saturate_id, 1.f);

#ifdef GL_DEBUG
    esDebug(1);
#endif

//*************************************
// execute update / render loop
//*************************************

    // init
    srand(time(0));
    srandf(time(0));
    t = fTime();
    lfct = t;

    // game init
    resetGame();

    // loop
    while(!glfwWindowShouldClose(window)){main_loop();}

    // done
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
