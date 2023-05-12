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

#include "plugin.h"
#include "doomkeys.h"
#include "i_system.h"
#include "doomgeneric.h"

static void construct(XfcePanelPlugin *plugin);
static void free_data(XfcePanelPlugin *plugin, gpointer data);
static void about(XfcePanelPlugin *plugin, gpointer data);

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

static void on_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data);
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

static void free_image(void);

static XfcePanelPlugin *xfce_plugin;
static GtkDrawingArea *display;

static GDateTime *dt_start;

static const char *argv[] = {"./doom-plugin", "-iwad", "/home/wojtek/Documents/code/doom-desktop/src/DOOM1.WAD"};

static guint width, height, dx, dy;
static guint32 *pixels = NULL;
static cairo_surface_t *image_surface = NULL;
static cairo_pattern_t *pattern = NULL;

static GArray *inputs;
static gboolean input_state[256] = {0};

XFCE_PANEL_PLUGIN_REGISTER(construct);

void free_data(XfcePanelPlugin *plugin, gpointer data)
{
	free_image();
	g_array_free(inputs, TRUE);
	g_date_time_unref(dt_start);
}

void about(XfcePanelPlugin *plugin, gpointer data)
{
}

void free_image(void)
{
	cairo_pattern_destroy(pattern);
	cairo_surface_destroy(image_surface);
	g_free(pixels);
}

void on_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
	width = allocation->width;
	height = allocation->height;
	dx = DOOMGENERIC_RESX / width;
	dy = DOOMGENERIC_RESY / height;

	free_image();
	pixels = g_malloc(width * height * 4);
	image_surface = cairo_image_surface_create_for_data((unsigned char*)pixels, CAIRO_FORMAT_RGB24, width, height, width * 4);
	pattern = cairo_pattern_create_for_surface(image_surface);
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	gtk_widget_grab_focus(widget);
	cairo_set_source(cr, pattern);
	cairo_paint(cr);
	return FALSE;
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	unsigned char doomKey;

	switch (event->keyval) {
	case GDK_KEY_Right: doomKey = KEY_RIGHTARROW; break;
	case GDK_KEY_Left: doomKey = KEY_LEFTARROW; break;
	case GDK_KEY_Up: doomKey = KEY_UPARROW; break;
	case GDK_KEY_Down: doomKey = KEY_DOWNARROW; break;
	case GDK_KEY_Escape: doomKey = KEY_ESCAPE; break;
	case GDK_KEY_Return: doomKey = KEY_ENTER; break;
	case GDK_KEY_Tab: doomKey = KEY_TAB; break;
	case GDK_KEY_BackSpace: doomKey = KEY_BACKSPACE; break;
	case GDK_KEY_Shift_L: case GDK_KEY_Shift_R: doomKey = KEY_RSHIFT; break;
	case GDK_KEY_Alt_L: case GDK_KEY_Alt_R: doomKey = KEY_RALT; break;
	case GDK_KEY_Control_L: case GDK_KEY_Control_R: doomKey = KEY_RCTRL; break;
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
}

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (event->button != 3)
		xfce_panel_plugin_focus_widget(xfce_plugin, GTK_WIDGET(display));

	return FALSE;
}

gboolean main_loop_iter(gpointer user_data)
{
	doomgeneric_Tick();
	return TRUE;
}

void construct(XfcePanelPlugin *plugin)
{
	xfce_plugin = plugin;

	GtkFrame *frame = GTK_FRAME(gtk_frame_new(NULL));
	gtk_widget_set_vexpand(GTK_WIDGET(frame), TRUE);

	gtk_container_add(GTK_CONTAINER(plugin), GTK_WIDGET(frame));
	xfce_panel_plugin_add_action_widget(plugin, GTK_WIDGET(frame));

	GtkWidget *event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(event_box));
	g_signal_connect(G_OBJECT(event_box), "button-press-event", G_CALLBACK(on_button_press), NULL);

	display = GTK_DRAWING_AREA(gtk_drawing_area_new());
	gtk_widget_set_can_focus(GTK_WIDGET(display), TRUE);
	gtk_container_add(GTK_CONTAINER(event_box), GTK_WIDGET(display));
	g_signal_connect(G_OBJECT(display), "key-press-event", G_CALLBACK(on_key_press), NULL);
	g_signal_connect(G_OBJECT(display), "key-release-event", G_CALLBACK(on_key_press), NULL);
	g_signal_connect(G_OBJECT(display), "draw", G_CALLBACK(on_draw), NULL);
	g_signal_connect(G_OBJECT(display), "size-allocate", G_CALLBACK(on_size_allocate), NULL);

	xfce_panel_plugin_menu_show_about(plugin);
	g_signal_connect(G_OBJECT(plugin), "about", G_CALLBACK(about), NULL);
	g_signal_connect(G_OBJECT(plugin), "free-data", G_CALLBACK(free_data), NULL);

	inputs = g_array_new(FALSE, FALSE, sizeof(unsigned char));

	gtk_widget_show_all(GTK_WIDGET(frame));

	doomgeneric_Create(3, (char**)argv);

	g_timeout_add(16, G_SOURCE_FUNC(main_loop_iter), NULL);
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

int DG_GetKey(int* pressed, unsigned char* doomKey)
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
