/*
 * =====================================================================================
 *
 *       Filename: controlboard.cpp
 *        Created: 08/21/2016 04:12:57
 *    Description:
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#include <stdexcept>
#include <algorithm>
#include <functional>

#include "log.hpp"
#include "colorf.hpp"
#include "totype.hpp"
#include "pngtexdb.hpp"
#include "sdldevice.hpp"
#include "imageboard.hpp"
#include "processrun.hpp"
#include "dbcomrecord.hpp"
#include "controlboard.hpp"

// for texture 0X00000012 and 0X00000013
// I split it into many parts to fix different screen size
// for screen width is not 800 we build a new interface using these two
//
// 0X00000012 : 800 x 133:  left and right
// 0X00000013 : 456 x 131:  middle log
// 0X00000022 : 127 x 41 :  title
//
//                         +-----------+                           ---
//                          \  title  /                             ^
// +------+==----------------+       +----------------==+--------+  |  --- <-- left/right is 133, middle is 131
// |      $                                        +---+$        | 152  | ---
// |      |                                        |   ||        |  |  133 | 120 as underlay log
// |      |                                        +---+|        |  V   |  |
// +------+---------------------------------------------+--------+ --- -- ---
// ^      ^    ^           ^           ^          ^     ^        ^
// | 178  | 50 |    110    |   127     |    50    | 119 |   166  | = 800
//
// |---fixed---|-------------repeat---------------|---fixed------|

//
// 0X00000027 : 456 x 298: char box frame
//
//                         +-----------+                        ---
//                          \  title  /                          ^
//        +==----------------+       +----------------==+ ---    |  ---- <-- startY
//        $                                             $  ^     |   47
//        |                                             |  |     |  ----
//        |                                             |  |     |   |
//        |                                             |  |     |  196: use to repeat, as m_stretchH
//        |                                             | 298   319  |
//        +---------------------------------------+-----+  |     |  ----
//        |                                       |     |  |     |   55
//        |                                       |() ()|  |     |   |
//        |                                       |     |  v     v   |
// +------+---------------------------------------+-----+--------+ --- -- ---
// ^      ^    ^           ^           ^          ^     ^        ^
// | 178  | 50 |    110    |   127     |    50    | 119 |   166  | = 800
//
// |---fixed---|-------------repeat---------------|---fixed------|

extern Log *g_log;
extern PNGTexDB *g_progUseDB;
extern SDLDevice *g_sdlDevice;

ControlBoard::ControlBoard(int boardW, int startY, ProcessRun *proc, Widget *pwidget, bool autoDelete)
    : Widget(DIR_UPLEFT, 0, startY, boardW, 133, pwidget, autoDelete)
    , m_processRun(proc)
    , m_left
      {
          DIR_UPLEFT,
          0,
          0,
          178,
          133,
          this,
      }

    , m_right
      {
          DIR_UPLEFT,
          boardW - 166,
          0,
          166,
          133,
          this,
      }

    , m_middle
      {
          178,
          2, // middle tex height is 131, not 133
          boardW - 178 - 166,
          131,
          this,
      }

    , m_buttonQuickAccess
      {
          DIR_UPLEFT,
          148,
          2,
          {SYS_TEXNIL, 0X0B000000, 0X0B000001},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("QuickAccessBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_left,
      }

    , m_buttonClose
      {
          DIR_UPLEFT,
          8,
          72,
          {SYS_TEXNIL, 0X0000001E, 0X0000001F},

          nullptr,
          nullptr,
          []()
          {
              std::exit(0);
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_left,
      }

    , m_buttonMinize
      {
          DIR_UPLEFT,
          109,
          72,
          {SYS_TEXNIL, 0X00000020, 0X00000021},

          nullptr,
          nullptr,
          nullptr,

          0,
          0,
          0,
          0,

          true,
          true,
          &m_left,
      }

    , m_buttonExchange
      {
          DIR_UPLEFT,
          4,
          6,

          1,
          1,
          10,

          80,
          colorf::WHITE + colorf::A_SHF(255),
          0X00000042,

          nullptr,
          nullptr,
          [this]()
          {
              addLog(0, "exchange doesn't implemented yet.");
          },

          true,
          &m_right,
      }

    , m_buttonMiniMap
      {
          DIR_UPLEFT,
          4,
          40,

          1,
          1,
          10,

          80,
          colorf::WHITE + colorf::A_SHF(255),
          0X00000043,

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = dynamic_cast<MiniMapBoard *>(m_processRun->getWidget("MiniMapBoard"))){
                  if(p->getMiniMapTexture()){
                      p->flipMiniMapShow();
                  }
                  else{
                      addLog(CBLOG_ERR, to_cstr(u8"没有可用的地图"));
                  }
              }
          },

          true,
          &m_right,
      }

    , m_buttonMagicKey
      {
          DIR_UPLEFT,
          4,
          75,

          1,
          1,
          10,

          80,
          colorf::WHITE + colorf::A_SHF(255),
          0X00000044,

          nullptr,
          nullptr,
          [this]()
          {
              m_processRun->flipDrawMagicKey();
          },

          true,
          &m_right,
      }

    , m_buttonInventory
      {
          DIR_UPLEFT,
          48,
          33,
          {SYS_TEXNIL, 0X00000030, 0X00000031},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("InventoryBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_right,
      }

    , m_buttonHeroState
      {
          DIR_UPLEFT,
          77,
          31,
          {SYS_TEXNIL, 0X00000033, 0X00000032},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("PlayerStateBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_right,
      }

    , m_buttonHeroMagic
      {
          DIR_UPLEFT,
          105,
          33,
          {SYS_TEXNIL, 0X00000035, 0X00000034},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("SkillBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_right,
      }

    , m_buttonGuild
      {
          DIR_UPLEFT,
          40,
          11,
          {SYS_TEXNIL, 0X00000036, 0X00000037},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("InventoryBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_right,
      }

    , m_buttonTeam
      {
          DIR_UPLEFT,
          72,
          8,
          {SYS_TEXNIL, 0X00000038, 0X00000039},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("InventoryBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_right,
      }

    , m_buttonTask
      {
          DIR_UPLEFT,
          108,
          11,
          {SYS_TEXNIL, 0X0000003A, 0X0000003B},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("InventoryBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_right,
      }

    , m_buttonHorse
      {
          DIR_UPLEFT,
          40,
          61,
          {SYS_TEXNIL, 0X0000003C, 0X0000003D},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("InventoryBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_right,
      }

    , m_buttonEnvConfig
      {
          DIR_UPLEFT,
          72,
          72,
          {SYS_TEXNIL, 0X0000003E, 0X0000003F},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("InventoryBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_right,
      }

    , m_buttonSysMessage
      {
          DIR_UPLEFT,
          108,
          61,
          {SYS_TEXNIL, 0X00000040, 0X00000041},

          nullptr,
          nullptr,
          [this]()
          {
              if(auto p = m_processRun->getWidget("InventoryBoard")){
                  flipShow(p);
              }
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_right,
      }

    , m_buttonAC
      {
          DIR_UPLEFT,
          1,
          105,

          proc,
          {
              "AC",
              "MA",
          },

          &m_right,
      }

    , m_buttonDC
      {
          DIR_UPLEFT,
          84,
          105,

          proc,
          {
              "DC",
              "MC",
          },

          &m_right,
      }

    , m_buttonSwitchMode
      {
          DIR_UPLEFT,
          boardW - 178 - 181,
          3,
          {SYS_TEXNIL, 0X00000028, 0X00000029},

          nullptr,
          nullptr,
          [this]()
          {
              switchExpandMode();
          },

          0,
          0,
          0,
          0,

          true,
          true,
          &m_middle,
      }

    , m_buttonEmoji
      {
          DIR_UPLEFT,
          boardW - 178 - 260,
          87,
          {SYS_TEXNIL, 0X00000023, 0X00000024},

          nullptr,
          nullptr,
          nullptr,

          0,
          0,
          0,
          0,

          true,
          true,
          &m_middle,
      }

    , m_buttonMute
      {
          DIR_UPLEFT,
          boardW - 178 - 220,
          87,
          {SYS_TEXNIL, 0X00000025, 0X00000026},

          nullptr,
          nullptr,
          nullptr,

          0,
          0,
          0,
          0,

          true,
          true,
          &m_middle,
      }

    , m_levelBox
      {
          DIR_NONE,
          m_middle.w() / 2,
          4,
          proc,

          [this](int dy)
          {
              if(!m_expand){
                  return;
              }

              m_stretchH = std::max<int>(m_stretchH - dy, m_stretchHMin);
              m_stretchH = std::min<int>(m_stretchH, g_sdlDevice->getRendererHeight() - 47 - 55);
              setButtonLoc();
          },
          [this]()
          {
              const int winH = g_sdlDevice->getRendererHeight();
              if(!m_expand){
                  switchExpandMode();
                  m_stretchH = winH - 47 - 55;
                  setButtonLoc();
                  return;
              }

              if(m_stretchH != m_stretchHMin){
                  m_stretchH = m_stretchHMin;
              }
              else{
                  m_stretchH = winH - 47 - 55;
              }
              setButtonLoc();
          },
          &m_middle,
      }

    , m_arcAniBoard
      {
          DIR_UPLEFT,
          (boardW - 178 - 166) / 2 - 18,
         -13,
          0X04000000,
          4,
          1,
          true,
          true,
          &m_middle,
      }

    , m_slider
      {
          DIR_UPLEFT,
          boardW - 178 - 176,
          40,

          60,
          0,
          nullptr,

          &m_middle,
      }

    , m_cmdLine
      {
          DIR_UPLEFT,
          7,
          105,
          343 + (boardW - 800),
          17,

          1,
          12,

          0,
          colorf::WHITE + colorf::A_SHF(255),

          2,
          colorf::WHITE + colorf::A_SHF(255),

          nullptr,
          [this]()
          {
              inputLineDone();
          },

          &m_middle,
      }

    , m_logBoard
      {
          DIR_UPLEFT,
          9,
          0, // need reset
          341 + (boardW - 800),
          false,

          {0, 0, 0, 0},
          false,

          1,
          12,
          0,

          colorf::WHITE + colorf::A_SHF(255),
          0,

          LALIGN_JUSTIFY,
          0,
          0,

          nullptr,
          &m_middle,
      }
{
    if(!proc){
        throw fflerror("invalid ProcessRun provided to ControlBoard()");
    }

    auto fnAssertImage = [](uint32_t img, int w, int h)
    {
        if(auto ptex = g_progUseDB->retrieve(img)){
            int readw = -1;
            int readh = -1;
            if(!SDL_QueryTexture(ptex, 0, 0, &readw, &readh)){
                if(w == readw && h == readh){
                    return;
                }
            }
        }
        throw fflerror("image assertion failed: img = %llu, w = %d, h = %d", to_llu(img), w, h);
    };

    fnAssertImage(0X00000012, 800, 133);
    fnAssertImage(0X00000013, 456, 131);
    fnAssertImage(0X00000022, 127,  41);
    fnAssertImage(0X00000027, 456, 298);

    if(x() != 0 || y() + h() != g_sdlDevice->getRendererHeight() || w() != g_sdlDevice->getRendererWidth()){
        throw fflerror("ControlBoard has wrong location or size");
    }
}

void ControlBoard::update(double fUpdateTime)
{
    m_cmdLine.update(fUpdateTime);
    m_logBoard.update(fUpdateTime);
    m_arcAniBoard.update(fUpdateTime);
}

void ControlBoard::drawLeft() const
{
    const int nY0 = y();

    // draw left part
    if(auto pTexture = g_progUseDB->retrieve(0X00000012)){
        g_sdlDevice->drawTexture(pTexture, 0, nY0, 0, 0, 178, 133);
    }

    // draw HP and MP texture
    {
        auto pHP = g_progUseDB->retrieve(0X00000018);
        auto pMP = g_progUseDB->retrieve(0X00000019);

        if(pHP && pMP){

            // we need to call query
            // so need to validate two textures here

            int nHPH = -1;
            int nHPW = -1;
            int nMPH = -1;
            int nMPW = -1;

            SDL_QueryTexture(pHP, nullptr, nullptr, &nHPW, &nHPH);
            SDL_QueryTexture(pMP, nullptr, nullptr, &nMPW, &nMPH);

            if(auto pMyHero = m_processRun->getMyHero()){
                const double fLostHPRatio = 1.0 - pMyHero->getHealthRatio().at(0);
                const double fLostMPRatio = 1.0 - pMyHero->getHealthRatio().at(1);

                const auto nLostHPH = to_d(std::lround(nHPH * fLostHPRatio));
                const auto nLostMPH = to_d(std::lround(nMPH * fLostMPRatio));

                g_sdlDevice->drawTexture(pHP, 33, nY0 + 9 + nLostHPH, 0, nLostHPH, nHPW, nHPH - nLostHPH);
                g_sdlDevice->drawTexture(pMP, 73, nY0 + 9 + nLostMPH, 0, nLostMPH, nMPW, nMPH - nLostMPH);
            }
        }
    }

    drawHeroLoc();
    m_buttonClose.draw();
    m_buttonMinize.draw();
    m_buttonQuickAccess.draw();

    drawRatioBar(153, nY0 + 115, m_processRun->getMyHero()->getLevelRatio());
    drawRatioBar(166, nY0 + 115, m_processRun->getMyHero()->getInventoryRatio());
}

void ControlBoard::drawRight() const
{
    const int nY0 = y();
    const int nW0 = w();

    // draw right part
    if(auto pTexture = g_progUseDB->retrieve(0X00000012)){
        g_sdlDevice->drawTexture(pTexture, nW0 - 166, nY0, 800 - 166, 0, 166, 133);
    }

    m_buttonExchange.draw();
    m_buttonMiniMap.draw();
    m_buttonMagicKey.draw();

    m_buttonGuild.draw();
    m_buttonTeam.draw();
    m_buttonTask.draw();
    m_buttonHorse.draw();
    m_buttonEnvConfig.draw();
    m_buttonSysMessage.draw();

    m_buttonAC.draw();
    m_buttonDC.draw();

    m_buttonInventory.draw();
    m_buttonHeroState.draw();
    m_buttonHeroMagic.draw();
}

std::tuple<int, int> ControlBoard::scheduleStretch(int dstSize, int srcSize)
{
    // use same way for default or expand mode
    // this requires texture 0X00000013 and 0X00000027 are of width 456

    if(dstSize < srcSize){
        return {0, dstSize};
    }

    if(dstSize % srcSize == 0){
        return {dstSize / srcSize, 0};
    }

    const double fillRatio = (1.0 * (dstSize % srcSize)) / srcSize;
    if(fillRatio < 0.5){
        return {dstSize / srcSize - 1, srcSize + (dstSize % srcSize)};
    }
    return {dstSize / srcSize, dstSize % srcSize};
}

void ControlBoard::drawMiddleDefault() const
{
    const int nY0 = y();
    const int nW0 = w();

    // draw black underlay for the logBoard and actor face
    g_sdlDevice->fillRectangle(colorf::RGBA(0X00, 0X00, 0X00, 0XFF), 178 + 2, nY0 + 14, nW0 - (178 + 2) - (166 + 2), 120);

    m_cmdLine.draw();
    drawLogBoardDefault();
    drawInputGreyBackground();

    // draw current creature face
    // draw '?' when the face texture is not available
    if(auto faceTexPtr = [this]() -> SDL_Texture *
    {
        if(auto texPtr = g_progUseDB->retrieve(m_processRun->GetFocusFaceKey())){
            return texPtr;
        }
        else if(auto texPtr = g_progUseDB->retrieve(0X010007CF)){
            return texPtr;
        }
        else{
            return nullptr;
        }
    }()){
        const auto [texW, texH] = SDLDeviceHelper::getTextureSize(faceTexPtr);
        g_sdlDevice->drawTexture(faceTexPtr, nW0 - 267, nY0 + 19, 86, 96, 0, 0, texW, texH);
    }

    // draw middle part
    if(auto texPtr = g_progUseDB->retrieve(0X00000013)){
        g_sdlDevice->drawTexture(texPtr,             178, nY0 + 2,         0, 0,  50, 131);
        g_sdlDevice->drawTexture(texPtr, nW0 - 166 - 119, nY0 + 2, 456 - 119, 0, 119, 131);

        const int repeatW = 456 - 50 - 119;
        const int drawW   = nW0 - 50 - 119 - 178 - 166;

        const auto [repeat, stretch] = scheduleStretch(drawW, repeatW);
        for(int i = 0; i < repeat; ++i){
            g_sdlDevice->drawTexture(texPtr, 178 + 50 + i * repeatW, nY0 + 2, 50, 0, repeatW, 131);
        }

        // for the rest area
        // need to stretch or shrink
        if(stretch > 0){
            g_sdlDevice->drawTexture(texPtr, 178 + 50 + repeat * repeatW, nY0 + 2, stretch, 131, 50, 0, repeatW, 131);
        }
    }

    // draw title
    // the title texture is not symmetric, add 1 pixel offset
    // then the levelBox can anchor at the middle by m_middle.w() / 2

    if(auto texPtr = g_progUseDB->retrieve(0X00000022)){
        const auto [titleW, titleH] = SDLDeviceHelper::getTextureSize(texPtr);
        const int titleDstX = 178 + (nW0 - 178 - 166 - titleW) / 2 + 1;
        const int titleDstY = nY0 - 19;
        g_sdlDevice->drawTexture(texPtr, titleDstX, titleDstY);
    }

    m_arcAniBoard.draw();
    m_buttonSwitchMode.draw();
    m_levelBox.draw();
    m_slider.draw();
}

void ControlBoard::drawLogBoardDefault() const
{
    const int dstX = 187;
    const int dstY = logBoardStartY();

    const int srcX = 0;
    const int srcY = std::max<int>(0, std::lround((m_logBoard.h() - 83) * m_slider.getValue()));
    const int srcW = m_logBoard.w();
    const int srcH = 83;

    m_logBoard.drawEx(dstX, dstY, srcX, srcY, srcW, srcH);
}

void ControlBoard::drawLogBoardExpand() const
{
    const int dstX = 187;
    const int dstY = logBoardStartY();

    const int boardFrameH = m_stretchH + 47 + 55 - 70;
    const int srcX = 0;
    const int srcY = std::max<int>(0, std::lround(m_logBoard.h() - boardFrameH) * m_slider.getValue());
    const int srcW = m_logBoard.w();
    const int srcH = boardFrameH;

    m_logBoard.drawEx(dstX, dstY, srcX, srcY, srcW, srcH);
}

void ControlBoard::drawMiddleExpand() const
{
    const int nY0 = y();
    const int nW0 = w();
    const int nH0 = h();

    // use this position to calculate all points
    // the Y-axis on screen that the big chat-frame starts
    const int startY = nY0 + nH0 - 55 - m_stretchH - 47;

    // draw black underlay for the big log board and input box
    g_sdlDevice->fillRectangle(colorf::RGBA(0X00, 0X00, 0X00, 0XF0), 178 + 2, startY + 2, nW0 - (178 + 2) - (166 + 2), 47 + m_stretchH + 55);

    drawInputGreyBackground();
    m_cmdLine.draw(); // cursor can be over-sized

    if(auto texPtr = g_progUseDB->retrieve(0X00000027)){

        // draw four corners
        g_sdlDevice->drawTexture(texPtr,             178,                   startY,         0,        0,  50, 47);
        g_sdlDevice->drawTexture(texPtr, nW0 - 166 - 119,                   startY, 456 - 119,        0, 119, 47);
        g_sdlDevice->drawTexture(texPtr,             178, startY + 47 + m_stretchH,         0, 298 - 55,  50, 55);
        g_sdlDevice->drawTexture(texPtr, nW0 - 166 - 119, startY + 47 + m_stretchH, 456 - 119, 298 - 55, 119, 55);

        // draw two stretched vertical bars
        const int repeatH = 298 - 47 - 55;
        const auto [repeatHCnt, stretchH] = scheduleStretch(m_stretchH, repeatH);

        for(int i = 0; i < repeatHCnt; ++i){
            g_sdlDevice->drawTexture(texPtr,             178, startY + 47 + i * repeatH,         0, 47,  50, repeatH);
            g_sdlDevice->drawTexture(texPtr, nW0 - 166 - 119, startY + 47 + i * repeatH, 456 - 119, 47, 119, repeatH);
        }

        if(stretchH > 0){
            g_sdlDevice->drawTexture(texPtr,             178, startY + 47 + repeatHCnt * repeatH,  50, stretchH,         0, 47,  50, repeatH);
            g_sdlDevice->drawTexture(texPtr, nW0 - 166 - 119, startY + 47 + repeatHCnt * repeatH, 119, stretchH, 456 - 119, 47, 119, repeatH);
        }

        // draw horizontal top bar and bottom input area
        const int repeatW = 456 - 50 - 119;
        const int drawW   = nW0 - 50 - 119 - 178 - 166;

        const auto [repeatWCnt, stretchW] = scheduleStretch(drawW, repeatW);
        for(int i = 0; i < repeatWCnt; ++i){
            g_sdlDevice->drawTexture(texPtr, 178 + 50 + i * repeatW,                   startY, 50,        0, repeatW, 47);
            g_sdlDevice->drawTexture(texPtr, 178 + 50 + i * repeatW, startY + 47 + m_stretchH, 50, 298 - 55, repeatW, 55);
        }

        if(stretchW > 0){
            g_sdlDevice->drawTexture(texPtr, 178 + 50 + repeatWCnt * repeatW,                   startY, stretchW, 47, 50,        0, repeatW, 47);
            g_sdlDevice->drawTexture(texPtr, 178 + 50 + repeatWCnt * repeatW, startY + 47 + m_stretchH, stretchW, 55, 50, 298 - 55, repeatW, 55);
        }
    }

    if(auto texPtr = g_progUseDB->retrieve(0X00000022)){
        const auto [titleW, titleH] = SDLDeviceHelper::getTextureSize(texPtr);
        const int titleDstX = 178 + (nW0 - 178 - 166 - titleW) / 2 + 1;
        const int titleDstY = startY - 2 - 19;
        g_sdlDevice->drawTexture(texPtr, titleDstX, titleDstY);
    }

    m_arcAniBoard.draw();
    m_buttonSwitchMode.draw();
    m_levelBox.draw();
    m_buttonEmoji.draw();
    m_buttonMute.draw();
    drawLogBoardExpand();
    m_slider.draw();
}

void ControlBoard::drawEx(int, int, int, int, int, int) const
{
    drawLeft();

    if(m_expand){
        drawMiddleExpand();
    }
    else{
        drawMiddleDefault();
    }

    drawRight();
}

bool ControlBoard::processEvent(const SDL_Event &event, bool valid)
{
    bool takeEvent = false;

    takeEvent |= m_levelBox         .processEvent(event, valid && !takeEvent);
    takeEvent |= m_slider           .processEvent(event, valid && !takeEvent);
    takeEvent |= m_cmdLine          .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonClose      .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonMinize     .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonQuickAccess.processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonExchange   .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonMiniMap    .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonMagicKey   .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonGuild      .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonTeam       .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonTask       .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonHorse      .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonEnvConfig  .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonSysMessage .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonAC         .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonDC         .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonInventory  .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonHeroState  .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonHeroMagic  .processEvent(event, valid && !takeEvent);
    takeEvent |= m_buttonSwitchMode .processEvent(event, valid && !takeEvent);

    if(m_expand){
        takeEvent |= m_buttonEmoji.processEvent(event, valid && !takeEvent);
        takeEvent |= m_buttonMute .processEvent(event, valid && !takeEvent);
    }

    if(takeEvent){
        return true;
    }

    switch(event.type){
        case SDL_KEYDOWN:
            {
                switch(event.key.keysym.sym){
                    case SDLK_RETURN:
                        {
                            m_cmdLine.focus(true);
                            return true;
                        }
                    default:
                        {
                            return false;
                        }
                }
            }
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEMOTION:
        default:
            {
                return false;
            }
    }
}

void ControlBoard::inputLineDone()
{
    const std::string fullInput = m_cmdLine.getRawString();
    const auto inputPos = fullInput.find_first_not_of(" \n\r\t");
    const std::string realInput = (inputPos == std::string::npos) ? "" : fullInput.substr(inputPos);

    m_cmdLine.clear();
    m_cmdLine.focus(false);

    if(realInput.empty()){
        return;
    }

    switch(realInput[0]){
        case '!': // broadcast
            {
                break;
            }
        case '@': // user command
            {
                if(m_processRun){
                    m_processRun->userCommand(realInput.c_str() + 1);
                }
                break;
            }
        case '$': // lua command for super user
            {
                if(m_processRun){
                    m_processRun->luaCommand(realInput.c_str() + 1);
                }
                break;
            }
        default: // normal talk
            {
                addLog(0, realInput.c_str());
                break;
            }
    }
}

void ControlBoard::addLog(int logType, const char *log)
{
    if(!log){
        throw fflerror("null log string");
    }

    switch(logType){
        case CBLOG_ERR:
            {
                g_log->addLog(LOGTYPE_WARNING, "%s", log);
                break;
            }
        default:
            {
                g_log->addLog(LOGTYPE_INFO, "%s", log);
                break;
            }
    }

    tinyxml2::XMLDocument xmlDoc;
    const char *xmlString = [logType]() -> const char *
    {
        // use hex to give alpha
        // color::String2Color has no alpha component

        switch(logType){
            case CBLOG_SYS: return "<par bgcolor = \"rgb(0x00, 0x80, 0x00)\"></par>";
            case CBLOG_DBG: return "<par bgcolor = \"rgb(0x00, 0x00, 0xff)\"></par>";
            case CBLOG_ERR: return "<par bgcolor = \"rgb(0xff, 0x00, 0x00)\"></par>";
            case CBLOG_DEF:
            default       : return "<par></par>";
        }
    }();

    if(xmlDoc.Parse(xmlString) != tinyxml2::XML_SUCCESS){
        throw fflerror("parse xml template failed: %s", xmlString);
    }

    // to support <, >, / in xml string
    // don't directly pass the raw string to addParXML
    xmlDoc.RootElement()->SetText(log);

    tinyxml2::XMLPrinter printer;
    xmlDoc.Print(&printer);
    m_logBoard.addParXML(m_logBoard.parCount(), {0, 0, 0, 0}, printer.CStr());
    m_slider.setValue(1.0f);
}

bool ControlBoard::CheckMyHeroMoved()
{
    return true;
}

void ControlBoard::switchExpandMode()
{
    if(m_expand){
        m_expand = false;
        m_logBoard.setLineWidth(m_logBoard.getLineWidth() - 87);
    }
    else{
        m_expand = true;
        m_stretchH = m_stretchHMin;
        m_logBoard.setLineWidth(m_logBoard.getLineWidth() + 87);
    }
    setButtonLoc();
}

void ControlBoard::setButtonLoc()
{
    // diff of height of texture 0X00000013 and 0X00000027
    // when you draw something on default log board at (X, Y), (0, 0) is left-top
    // if you need to keep the same location on expand log board, draw on(X, Y - modeDiffY)

    const int boardW = w();
    const int modeDiffY = (298 - 131) + (m_stretchH - m_stretchHMin);

    if(m_expand){
        m_buttonSwitchMode.moveTo(boardW - 178 - 181, 3 - modeDiffY);
        m_levelBox.moveTo((boardW - 178 - 166) / 2, 4 - modeDiffY);
        m_arcAniBoard.moveTo((boardW - 178 - 166 - m_arcAniBoard.w()) / 2, -13 - modeDiffY);

        m_buttonEmoji.moveTo(boardW - 178 - 260, 87);
        m_buttonMute .moveTo(boardW - 178 - 220, 87);

        m_slider.moveTo(w() - 178 - 176, 40 - modeDiffY);
        m_slider.resizeHeight(60 + modeDiffY);
    }
    else{
        m_buttonSwitchMode.moveTo(boardW - 178 - 181, 3);
        m_levelBox.moveTo((boardW - 178 - 166) / 2, 4);
        m_arcAniBoard.moveTo((boardW - 178 - 166 - m_arcAniBoard.w()) / 2, -13);

        m_slider.moveTo(w() - 178 - 176, 40);
        m_slider.resizeHeight(60);
    }
}

int ControlBoard::logBoardStartY() const
{
    if(!m_expand){
        return g_sdlDevice->getRendererHeight() - 120;
    }
    return g_sdlDevice->getRendererHeight() - 55 - m_stretchH - 47 + 12; // 12 is texture top-left to log line distane
}

void ControlBoard::onWindowResize(int winW, int winH)
{
    const auto prevWidth = w();
    m_right.moveBy(winW - w(), 0);
    m_w = winW;

    m_logBoard.setLineWidth(m_logBoard.getLineWidth() + (winW - prevWidth));
    const int maxStretchH = winH - 47 - 55;

    if(m_expand && (m_stretchH > maxStretchH)){
        m_stretchH = maxStretchH;
    }

    m_middle.resetWidth(w() - 178 - 166);
    setButtonLoc();
}

void ControlBoard::drawInputGreyBackground() const
{
    if(!m_cmdLine.focus()){
        return;
    }

    const auto color = colorf::GREY + colorf::A_SHF(48);
    SDLDeviceHelper::EnableRenderBlendMode enableDrawBlendMode(SDL_BLENDMODE_BLEND);

    if(m_expand){

    }
    else{
        g_sdlDevice->fillRectangle(color, m_middle.x() + 7, m_middle.y() + 104, m_middle.w() - 110, 17);
    }
}

void ControlBoard::drawHeroLoc() const
{
    // support different map uses same map name, i.e.
    //
    // 石墓迷宫_1
    // 石墓迷宫_2
    // 石墓迷宫_3
    //
    // we use same map gfx for different map
    // but user still see same map name: 石墓迷宫

    const auto mapName = std::string(to_cstr(DBCOM_MAPRECORD(m_processRun->mapID()).name));
    const auto locStr = str_printf(u8"%s: %d %d", mapName.substr(0, mapName.find('_')).c_str(), m_processRun->getMyHero()->x(), m_processRun->getMyHero()->y());
    LabelBoard locBoard
    {
        DIR_UPLEFT,
        0, // need reset
        0,

        locStr.c_str(),
        1,
        12,
        0,

        colorf::WHITE + colorf::A_SHF(255),
    };

    const int locBoardStartX = (136 - locBoard.w()) / 2;
    const int locBoardStartY = 109;

    locBoard.moveBy(m_left.x() + locBoardStartX, m_left.y() + locBoardStartY);
    locBoard.draw();
}

void ControlBoard::drawRatioBar(int x, int y, double r) const
{
    ImageBoard barImage
    {
        DIR_DOWN,
        x,
        y,

        [](const ImageBoard *)
        {
            return g_progUseDB->retrieve(0X000000A0);
        },

        colorf::RGBA(to_u8(255 * r), to_u8(255 * (1 - r)), 0, 255),
    };

    barImage.setSizeRatio({}, r);
    barImage.draw();
}
