#ifndef LANDINGWINDOW_H
#define LANDINGWINDOW_H

class LandingWindow
{
private:
    GObject *window;
    GtkBuilder *builder;

    void initMenubar();

    static void consoleInput(GtkWidget *widget, gpointer data);

    static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer data);

public:
    int launch(int argc, char *args[]);
};

#endif