/*
 * Copyright (C) 2026 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * PhoshPluginInfo:
 * @id: Plugin identifier (e.g. "justapps")
 * @name: Human-readable name (e.g. "JustApps")
 * @comment: Description (e.g. "Show running tray applications")
 * @icon: Icon name (e.g. "justapps-symbolic")
 * @module: Path to the plugin .so module
 * @enabled: Whether the plugin is currently enabled
 * @has_prefs: Whether the plugin has a preferences dialog
 *
 * Information about a single Phosh plugin, parsed from a .plugin file.
 */
typedef struct {
  char     *id;
  char     *name;
  char     *comment;
  char     *icon;
  char     *module;
  gboolean  enabled;
  gboolean  has_prefs;
} PhoshPluginInfo;

/**
 * phosh_plugin_info_new:
 * @keyfile: A loaded #GKeyFile containing the .plugin metadata
 * @enabled: Whether the plugin is currently enabled
 *
 * Creates a new #PhoshPluginInfo from a .plugin keyfile.
 *
 * Returns: (transfer full): A new #PhoshPluginInfo, or %NULL on failure
 */
PhoshPluginInfo *phosh_plugin_info_new     (GKeyFile *keyfile, gboolean enabled);

/**
 * phosh_plugin_info_free:
 * @info: The #PhoshPluginInfo to free
 *
 * Frees a #PhoshPluginInfo.
 */
void             phosh_plugin_info_free    (PhoshPluginInfo *info);

G_END_DECLS
