/*
 * Copyright (C) 2026 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "plugin-manager.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_PLUGIN_MANAGER_PAGE phosh_plugin_manager_page_get_type ()
G_DECLARE_FINAL_TYPE (PhoshPluginManagerPage, phosh_plugin_manager_page, PHOSH, PLUGIN_MANAGER_PAGE, GtkBin)

/**
 * phosh_plugin_manager_page_new:
 * @manager: (transfer none): The #PhoshPluginManager to use
 *
 * Creates a new plugin manager page widget.
 *
 * Returns: (transfer floating): A new #PhoshPluginManagerPage
 */
GtkWidget *phosh_plugin_manager_page_new (PhoshPluginManager *manager);

G_END_DECLS
