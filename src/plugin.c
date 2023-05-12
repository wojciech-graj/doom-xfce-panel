/*  $Id$
 *
 *  Copyright (C) 2023 John Doo <john@foo.org>
 *  Copyright (C) 2023 Wojciech Graj
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "doomgeneric.h"
#include "doomkeys.h"
#include "i_system.h"

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>

/* Plugin */
static void construct(XfcePanelPlugin *plugin);
static void free_data(XfcePanelPlugin *plugin, gpointer user_data);
static void about(XfcePanelPlugin *plugin, gpointer user_data);

/* EventBox [DrawingArea] */
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

/* DrawingArea */
static void on_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data);
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data);

/* Wad File Selector */
static void on_wad_selection_changed(GtkFileChooser *chooser, gpointer user_data);

/* Utils */
static void free_image(void);
static void start_game(void);

enum Argv {
	ARGV_BIN = 0,
	ARGV_FLAG_IWAD,
	ARGV_IWAD,
	NARGV,
};

static char *argv[NARGV] = {
	[ARGV_BIN] = "./doom-plugin",
	[ARGV_FLAG_IWAD] = "-iwad",
	[ARGV_IWAD] = NULL,
};

static XfcePanelPlugin *xfce_plugin = NULL;
static GtkFrame *frame = NULL;
static GtkBox *wad_box = NULL;
static GtkDrawingArea *display = NULL;

static GDateTime *dt_start;

static guint width, height, dx, dy;
static guint32 *pixels = NULL;
static cairo_surface_t *image_surface = NULL;
static cairo_pattern_t *pattern = NULL;

static GArray *inputs;
static gboolean input_state[256] = { 0 };

XFCE_PANEL_PLUGIN_REGISTER(construct)

void free_image(void)
{
	cairo_pattern_destroy(pattern);
	cairo_surface_destroy(image_surface);
	g_free(pixels);
}

void free_data(XfcePanelPlugin *plugin, gpointer user_data)
{
	free_image();
	g_array_free(inputs, TRUE);
	g_date_time_unref(dt_start);
	g_free(argv[ARGV_IWAD]);
	(void)plugin;
	(void)user_data;
}

void about(XfcePanelPlugin *plugin, gpointer user_data)
{
	gtk_show_about_dialog(NULL,
		"logo-icon-name", "input-gaming",
		"license", xfce_get_license_text(XFCE_LICENSE_TEXT_GPL),
		"version", "0.1.0",
		"program-name", "DooM",
		"comments", _("DooM in the Xfce panel"),
		"website", "https://github.com/wojciech-graj/doom-xfce-panel",
		"copyright", "Copyright \302\251 1993-1996 Id Software, Inc.\nCopyright \302\251 2005-2014 Simon Howard\nCopyright \302\251 2023 Wojciech Graj",
		NULL);
	(void)plugin;
	(void)user_data;
}

void on_wad_selection_changed(GtkFileChooser *chooser, gpointer user_data)
{
	gchar *wad_fname = gtk_file_chooser_get_filename(chooser);
	if (!wad_fname || !(g_str_has_suffix(wad_fname, ".WAD") || g_str_has_suffix(wad_fname, ".wad")))
		return;
	g_free(argv[ARGV_IWAD]);
	argv[ARGV_IWAD] = wad_fname;
	start_game();
	(void)user_data;
}

void on_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
	width = MIN(DOOMGENERIC_RESX, allocation->width);
	height = MIN(DOOMGENERIC_RESY, allocation->height);
	dx = DOOMGENERIC_RESX / width;
	dy = DOOMGENERIC_RESY / height;

	free_image();
	pixels = g_malloc0(width * height * 4);
	image_surface = cairo_image_surface_create_for_data((unsigned char *)pixels, CAIRO_FORMAT_RGB24, width, height, width * 4);
	pattern = cairo_pattern_create_for_surface(image_surface);

	gtk_widget_queue_draw(GTK_WIDGET(display));
	(void)widget;
	(void)user_data;
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	cairo_set_source(cr, pattern);
	cairo_paint(cr);
	return FALSE;
	(void)widget;
	(void)user_data;
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	unsigned char doomKey;

	switch (event->keyval) {
	case GDK_KEY_Right:
		doomKey = KEY_RIGHTARROW;
		break;
	case GDK_KEY_Left:
		doomKey = KEY_LEFTARROW;
		break;
	case GDK_KEY_Up:
		doomKey = KEY_UPARROW;
		break;
	case GDK_KEY_Down:
		doomKey = KEY_DOWNARROW;
		break;
	case GDK_KEY_Escape:
		doomKey = KEY_ESCAPE;
		break;
	case GDK_KEY_Return:
		doomKey = KEY_ENTER;
		break;
	case GDK_KEY_Tab:
		doomKey = KEY_TAB;
		break;
	case GDK_KEY_BackSpace:
		doomKey = KEY_BACKSPACE;
		break;
	case GDK_KEY_Shift_L:
	case GDK_KEY_Shift_R:
		doomKey = KEY_RSHIFT;
		break;
	case GDK_KEY_Alt_L:
	case GDK_KEY_Alt_R:
		doomKey = KEY_RALT;
		break;
	case GDK_KEY_Control_L:
	case GDK_KEY_Control_R:
		doomKey = KEY_FIRE;
		break;
	case GDK_KEY_space:
		doomKey = KEY_USE;
		break;
	default:
		if (g_ascii_isgraph(event->keyval))
			doomKey = g_ascii_tolower(event->keyval);
		else
			return TRUE;
	}

	gboolean pressed = event->type == GDK_KEY_PRESS;
	if (pressed && input_state[doomKey])
		return TRUE;

	input_state[doomKey] = pressed;
	g_array_append_val(inputs, doomKey);
	return TRUE;
	(void)widget;
	(void)user_data;
}

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (event->button != 3)
		xfce_panel_plugin_focus_widget(xfce_plugin, widget);

	return FALSE;
	(void)user_data;
}

gboolean main_loop_iter(gpointer user_data)
{
	doomgeneric_Tick();
	return TRUE;
	(void)user_data;
}

void start_game(void)
{
	if (!argv[ARGV_IWAD])
		return;

	gtk_container_remove(GTK_CONTAINER(frame), GTK_WIDGET(wad_box));
	wad_box = NULL;

	/* Create EventBox because DrawingArea does not receive button press events */
	GtkEventBox *event_box = GTK_EVENT_BOX(gtk_event_box_new());
	gtk_widget_set_can_focus(GTK_WIDGET(event_box), TRUE);
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(event_box));
	g_signal_connect(G_OBJECT(event_box), "button-press-event", G_CALLBACK(on_button_press), NULL);
	g_signal_connect(G_OBJECT(event_box), "key-press-event", G_CALLBACK(on_key_press), NULL);
	g_signal_connect(G_OBJECT(event_box), "key-release-event", G_CALLBACK(on_key_press), NULL);

	display = GTK_DRAWING_AREA(gtk_drawing_area_new());
	gtk_container_add(GTK_CONTAINER(event_box), GTK_WIDGET(display));
	g_signal_connect(G_OBJECT(display), "draw", G_CALLBACK(on_draw), NULL);
	g_signal_connect(G_OBJECT(display), "size-allocate", G_CALLBACK(on_size_allocate), NULL);

	gtk_widget_show_all(GTK_WIDGET(event_box));

	doomgeneric_Create(NARGV, (char **)argv);
	/* Run main loop iteration every 16ms (approx. 60fps) */
	g_timeout_add(16, G_SOURCE_FUNC(main_loop_iter), NULL);
}

void construct(XfcePanelPlugin *plugin)
{
	xfce_plugin = plugin;
	xfce_panel_plugin_menu_show_about(plugin);
	g_signal_connect(G_OBJECT(plugin), "about", G_CALLBACK(about), NULL);
	g_signal_connect(G_OBJECT(plugin), "free-data", G_CALLBACK(free_data), NULL);

	frame = GTK_FRAME(gtk_frame_new(NULL));
	gtk_widget_set_vexpand(GTK_WIDGET(frame), TRUE);
	gtk_container_add(GTK_CONTAINER(plugin), GTK_WIDGET(frame));
	xfce_panel_plugin_add_action_widget(plugin, GTK_WIDGET(frame));

	wad_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(wad_box));

	GtkLabel *wad_label = GTK_LABEL(gtk_label_new("WAD: "));
	gtk_container_add(GTK_CONTAINER(wad_box), GTK_WIDGET(wad_label));

	GtkFileChooserButton *wad_file_chooser_button = GTK_FILE_CHOOSER_BUTTON(gtk_file_chooser_button_new("WAD", GTK_FILE_CHOOSER_ACTION_OPEN));
	gtk_container_add(GTK_CONTAINER(wad_box), GTK_WIDGET(wad_file_chooser_button));
	g_signal_connect(G_OBJECT(wad_file_chooser_button), "selection-changed", G_CALLBACK(on_wad_selection_changed), NULL);

	gtk_widget_show_all(GTK_WIDGET(frame));

	inputs = g_array_new(FALSE, FALSE, sizeof(unsigned char));
}

void DG_Init()
{
	dt_start = g_date_time_new_now_utc();
}

void DG_DrawFrame()
{
	guint x, y;
	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			pixels[y * width + x] = DG_ScreenBuffer[y * dy * DOOMGENERIC_RESX + x * dx];
	gtk_widget_queue_draw(GTK_WIDGET(display));
}

void DG_SleepMs(uint32_t ms)
{
	(void)ms;
}

uint32_t DG_GetTicksMs()
{
	GDateTime *dt_now = g_date_time_new_now_utc();
	GTimeSpan diff = g_date_time_difference(dt_now, dt_start);
	g_date_time_unref(dt_now);
	return diff / 1000LL;
}

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
	if (!inputs->len)
		return 0;
	*doomKey = g_array_index(inputs, unsigned char, inputs->len - 1);
	*pressed = input_state[*doomKey];
	g_array_remove_index(inputs, inputs->len - 1);
	return 1;
}

void DG_SetWindowTitle(const char *title)
{
	(void)title;
}
