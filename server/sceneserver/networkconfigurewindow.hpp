// generated by Fast Light User Interface Designer (fluid) version 1.0303

#ifndef networkconfigurewindow_hpp
#define networkconfigurewindow_hpp
#include <FL/Fl.H>
#include <cmath>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Value_Input.H>

class NetworkConfigureWindow {
public:
  NetworkConfigureWindow();
  Fl_Double_Window *m_Window;
  Fl_Input *m_GateServerIP;
  Fl_Value_Input *m_GateServerPort;
  Fl_Input *m_DatabaseIP;
  Fl_Value_Input *m_DatabasePort;
  Fl_Input *m_UserName;
  Fl_Input *m_Password;
  Fl_Input *m_DatabaseName;
  void ShowAll();
  const char * GateServerIP();
  int GateServerPort();
  const char * DatabaseIP();
  int DatabasePort();
  const char * DatabaseName();
  const char * UserName();
  const char * Password();
};
#endif
