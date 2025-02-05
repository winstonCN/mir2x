#pragma once
#include <vector>
#include "threadpool.hpp"
#include <Fl/Fl_Multiline_Input.H>

class CommandWindow;
class CommandInput : public Fl_Multiline_Input
{
    private:
        CommandWindow *m_window = nullptr;
        std::unique_ptr<threadPool> m_worker;

    private:
        int m_inputListPos = 0;
        std::vector<std::string> m_inputList;

    public:
        CommandInput(int argX, int argY, int argW, int argH, const char *labelCPtr = nullptr)
            : Fl_Multiline_Input(argX, argY, argW, argH, labelCPtr)
        {}

    public:
        void bind(CommandWindow *winPtr)
        {
            m_window = winPtr;
            m_worker = std::make_unique<threadPool>(1);
        }

    public:
        int handle(int);

    public:
        const auto &getHistory() const
        {
            return m_inputList;
        }
};
