/*
    James William Fletcher  ( github.com/mrbid )
        May 2024

    It's good to be king. Wait, maybe. I think maybe I'm just like a little bizarre little person who walks back and forth.
*/

#include <stdio.h>
#include <time.h>

#define AUDIO_ON
#ifdef AUDIO_ON
    #include "assets/audio/song.h"
#endif

#define uint GLuint
#define sint GLint

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

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
SDL_Window* wnd;
SDL_GLContext glc;
SDL_Surface* s_icon = NULL;
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
    rspeed = 1.f;
    carx = 0.f;
    carz = 0.f;
    carr = 0.f;
    carxt = 0.f;
    ntrain = 0.f;
    ntrain = t+3.f;
    trainy = 6.7f;
    trains = 1.f;
    score = 0;
    spawned = 0;
    ascend = 0.1f;
}

//*************************************
// utility functions
//*************************************
void timestamp(char* ts){const time_t tt=time(0);strftime(ts,16,"%H:%M:%S",localtime(&tt));}
float fTime(){return ((float)SDL_GetTicks())*0.001f;}
SDL_Surface* surfaceFromData(const Uint32* data, Uint32 w, Uint32 h)
{
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    memcpy(s->pixels, data, s->pitch*h);
    return s;
}
void updateModelView()
{
    mMul(&modelview, &model, &view);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (float*)&modelview.m[0][0]);
}
void updateWindowSize(int width, int height)
{
    winw = width, winh = height;
    glViewport(0, 0, winw, winh);
    aspect = (float)winw / (float)winh;
    mIdent(&projection);
    mPerspective(&projection, 30.0f, aspect, 0.1f, FAR_DISTANCE);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (float*)&projection.m[0][0]);
}

//*************************************
// audio thread
//*************************************
#ifdef AUDIO_ON
uint saud = 0;
void audioCallback(void* unused, Uint8* stream, int len)
{
    for(int i = 0; i < len; i++)
    {
        stream[i] = song[saud];
        if(++saud >= song_size){saud = 0;}
    }
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
    t = fTime();
    dt = t-lt;
    lt = t;

//*************************************
// game logic
//*************************************

    static uint last_focus_mouse = 0;
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_WINDOWEVENT:
            {
                switch(event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        updateWindowSize(event.window.data1, event.window.data2);
                    }
                    break;
                }
            }
            break;

            case SDL_KEYDOWN:
            {
                if(ascend > 1.0f){resetGame();break;}
                if(keystate[0] == 0 && keystate[1] == 0)
                {
                    if(event.key.keysym.sym      == SDLK_LEFT) { keystate[0] = 1; }
                    else if(event.key.keysym.sym == SDLK_RIGHT){ keystate[1] = 1; }
                }
            }
            break;

            case SDL_KEYUP:
            {
                if(event.key.keysym.sym      == SDLK_LEFT)  {keystate[0] = 0;}
                else if(event.key.keysym.sym == SDLK_RIGHT) {keystate[1] = 0;}
                else if(event.key.keysym.sym == SDLK_f)
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
            break;

            case SDL_MOUSEBUTTONDOWN:
            {
                if(ascend > 1.0f){resetGame();break;}
                if(keystate[0] == 0 && keystate[1] == 0)
                {
                    if(event.button.button      == SDL_BUTTON_LEFT) { keystate[0] = 1; }
                    else if(event.button.button == SDL_BUTTON_RIGHT){ keystate[1] = 1; }
                }
            }
            break;

            case SDL_MOUSEBUTTONUP:
            {
                if(ascend > 1.0f){resetGame();break;}
                if(event.button.button      == SDL_BUTTON_LEFT) { keystate[0] = 0; }
                else if(event.button.button == SDL_BUTTON_RIGHT){ keystate[1] = 0; }
            }
            break;

            case SDL_QUIT:
            {
                SDL_FreeSurface(s_icon);
                SDL_GL_DeleteContext(glc);
                SDL_DestroyWindow(wnd);
                SDL_Quit();
                exit(0);
            }
            break;
        }
    }

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
#ifdef AUDIO_ON
        if(carr == 0.f){saud = 288000;}
#endif
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
                        SDL_SetWindowTitle(wnd, tmp);
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
                        SDL_SetWindowTitle(wnd, tmp);
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
    SDL_GL_SwapWindow(wnd);
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
    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    printf("Compiled against SDL version %u.%u.%u.\n", compiled.major, compiled.minor, compiled.patch);
    printf("Linked against SDL version %u.%u.%u.\n", linked.major, linked.minor, linked.patch);
    printf("----\n");

    // init sdl
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) < 0)
    {
        printf("ERROR: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }
    if(msaa > 0)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    while(wnd == NULL)
    {
        msaa--;
        if(msaa == 0)
        {
            printf("ERROR: SDL_CreateWindow(): %s\n", SDL_GetError());
            return 1;
        }
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
        wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    }
    SDL_GL_SetSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync
    glc = SDL_GL_CreateContext(wnd);
    if(glc == NULL)
    {
        printf("ERROR: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    // set icon
    s_icon = surfaceFromData((Uint32*)&icon_image, 16, 16);
    SDL_SetWindowIcon(wnd, s_icon);

#ifdef AUDIO_ON
    // open audio device
    SDL_AudioSpec sdlaudioformat;
    sdlaudioformat.freq = song_freq;
    sdlaudioformat.format = AUDIO_U8;
    sdlaudioformat.channels = 1;
    sdlaudioformat.samples = 1024;
    sdlaudioformat.callback = audioCallback;
    sdlaudioformat.userdata = NULL;
    SDL_OpenAudio(&sdlaudioformat, 0);
    SDL_PauseAudio(0);
#endif

//*************************************
// bind vertex and index buffers
//*************************************
    register_track();
    register_bg();
    register_car();
    register_train();
    register_e1();
    register_e2();
    register_alien();
    register_cia();
    register_terry();

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
    updateWindowSize(winw, winh);
    glUniform1f(ambient_id, 0.648f);
    glUniform1f(saturate_id, 1.f);

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
    while(1){main_loop();}
    return 0;
}
