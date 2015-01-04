#ifndef GRAPRO_H
#define GRAPRO_H

#include "framework/mainwindow.h"

class GraPro
  : public framework::MainWindow
{
public:
    using MainWindow::MainWindow;

    virtual void render_scene() override;
    virtual void update_gui(double delta_t) override;
};

#endif // GRAPRO_H

