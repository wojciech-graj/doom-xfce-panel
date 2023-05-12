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

static void construct(XfcePanelPlugin *plugin);
static void orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, struct Plugin *sample_py);
static void free_data(XfcePanelPlugin *plugin, struct Plugin *sample_py);
static void about(XfcePanelPlugin *plugin, struct Plugin *sample_py);

XFCE_PANEL_PLUGIN_REGISTER(construct);

void orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, struct Plugin *sample_py)
{
}

void free_data(XfcePanelPlugin *plugin, struct Plugin *sample_py)
{
    g_slice_free(struct Plugin, sample_py);
}

void about(XfcePanelPlugin *plugin, struct Plugin *sample_py)
{
}

void construct(XfcePanelPlugin *plugin)
{
    struct Plugin *sample_py;
    GObject *widget;

    sample_py = g_slice_new0(struct Plugin);
    sample_py->plugin = plugin;

    widget = G_OBJECT(gtk_label_new("Sample"));

    gtk_widget_show_all(GTK_WIDGET(widget));

    gtk_container_add(GTK_CONTAINER(plugin), GTK_WIDGET(widget));

    xfce_panel_plugin_add_action_widget(plugin, GTK_WIDGET(widget));

    xfce_panel_plugin_menu_show_about(plugin);
    g_signal_connect(G_OBJECT(plugin), "about",
                     G_CALLBACK(about), sample_py);

    g_signal_connect(G_OBJECT(plugin), "free-data",
                     G_CALLBACK(free_data), sample_py);

    g_signal_connect(G_OBJECT(plugin), "orientation-changed",
                     G_CALLBACK(orientation_changed), sample_py);
}
