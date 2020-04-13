#include "Common.h"
#include "gui/LandingWindow.h"

int LandingWindow::launch(int argc, char *args[])
{
    gtk_init(&argc, &args);

    // Create builder instance and load UI description
    GtkBuilder *builder = gtk_builder_new();
    GError *error = NULL;

    if (gtk_builder_add_from_file(builder, "../ui/landing.ui", &error) == 0)
    {
        g_printerr("Error loading from file: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // Get window object and add kill event
    window = gtk_builder_get_object(builder, "landingWindow");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    //initMenubar();

    // Add test render event to GL Area
    //GObject *drawArea = gtk_builder_get_object(builder, "drawArea");
    //g_signal_connect(drawArea, "draw", G_CALLBACK(draw), NULL);

    GObject *consoleView = gtk_builder_get_object(builder, "consoleView");

    GObject *consoleEntry = gtk_builder_get_object(builder, "consoleEntry");
    g_signal_connect(consoleEntry, "activate", G_CALLBACK(consoleInput), consoleView);

    srand(time(NULL));

    gtk_main();
    return 0;
}

void LandingWindow::consoleInput(GtkWidget *widget, gpointer data)
{
    GtkWidget *consoleView = (GtkWidget *)data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(consoleView));
    
    const gchar *toInsert = gtk_entry_get_text (GTK_ENTRY(widget));
    gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(buffer), toInsert, -1);
    gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(buffer), "\n", -1);
    gtk_entry_set_text(GTK_ENTRY(widget), "");
}

gboolean LandingWindow::draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    guint width, height;
    GdkRGBA color;
    GtkStyleContext *context;

    context = gtk_widget_get_style_context (widget);

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    gtk_render_background (context, cr, 0, 0, width, height);

    cairo_arc (cr,
        width / 2.0, height / 2.0,
        MIN (width, height) / 2.0,
        0, 2 * G_PI);

    gtk_style_context_get_color (context, gtk_style_context_get_state (context), &color);
    gdk_cairo_set_source_rgba (cr, &color);

    cairo_fill (cr);
    return FALSE;
}

void LandingWindow::initMenubar()
{
        GObject *item;

        // File menu
        //item = gtk_builder_get_object(builder, "menuFile_open");
        //g_signal_connect(item, "activate", G_CALLBACK(_openFileDialog), NULL);
}