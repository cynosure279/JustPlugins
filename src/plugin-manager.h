/*
 * Copyright (C) 2026 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "plugin-info.h"

G_BEGIN_DECLS

#define PHOSH_TYPE_PLUGIN_MANAGER phosh_plugin_manager_get_type ()
G_DECLARE_FINAL_TYPE (PhoshPluginManager, phosh_plugin_manager, PHOSH, PLUGIN_MANAGER, GObject)

/**
 * phosh_plugin_manager_new:
 *
 * Creates a new #PhoshPluginManager that scans for Phosh plugins
 * and manages their enable state via GSettings.
 *
 * Returns: (transfer full): A new #PhoshPluginManager
 */
PhoshPluginManager *phosh_plugin_manager_new (void);

/**
 * phosh_plugin_manager_get_plugins:
 * @self: The plugin manager
 *
 * Returns: (transfer none) (element-type PhoshPluginInfo): The list of discovered plugins
 */
GPtrArray *phosh_plugin_manager_get_plugins (PhoshPluginManager *self);

/**
 * phosh_plugin_manager_set_enabled:
 * @self: The plugin manager
 * @plugin_id: The plugin identifier
 * @enabled: Whether to enable or disable the plugin
 *
 * Enables or disables a plugin by updating the GSettings list.
 * Returns %TRUE on success, %FALSE if the GSettings write failed.
 */
gboolean phosh_plugin_manager_set_enabled  (PhoshPluginManager *self,
                                            const char         *plugin_id,
                                            gboolean            enabled);

/**
 * phosh_plugin_manager_is_enabled:
 * @self: The plugin manager
 * @plugin_id: The plugin identifier
 *
 * Checks whether a plugin is currently enabled.
 *
 * Returns: %TRUE if the plugin is in the enabled list
 */
gboolean phosh_plugin_manager_is_enabled   (PhoshPluginManager *self,
                                            const char         *plugin_id);

G_END_DECLS
