/*
 * =====================================================================================
 *
 *       Filename: main.cpp
 *        Created: 08/31/2015 08:52:57 PM
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

#include <string>
#include <cstdint>
#include <FL/Fl.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>
#include "totype.hpp"
#include "fflerror.hpp"
#include "dbcomid.hpp"
#include "dbcomrecord.hpp"
#include "mainwindow.hpp"
#include "aboutwindow.hpp"

AboutWindow *g_aboutWindow = nullptr;
MainWindow  *g_mainWindow  = nullptr;

int main(int argc, char *argv[])
{
    fflassert(argc == 3);
    const auto magicID = DBCOM_MAGICID(to_u8cstr(argv[1]));
    const auto magicDBPath = argv[2];

    const auto &mr = DBCOM_MAGICRECORD(magicID);
    fflassert(mr);
    fflassert(mr.getGfxEntry(u8"运行"));
    fflassert(mr.getGfxEntry(u8"运行").checkType(u8"跟随"));

    fl_register_images();

    g_aboutWindow = new AboutWindow();
    g_mainWindow  = new MainWindow(magicID, magicDBPath);

    g_mainWindow->showAll();
    return Fl::run();
}
