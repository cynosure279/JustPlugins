/*
 * Copyright (C) 2026 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: JustPlugins Contributors
 */

#define G_LOG_DOMAIN "phosh-plugin-manager"

#include "phosh-config.h"
#include "plugin-manager.h"

/**
 * SECTION:plugin-manager
 * @short_description: Discovers and manages Phosh plugins
 *
 * #PhoshPluginManager scans Phosh's plugin directories for .plugin
 * metadata files, reads their information, and manages the enabled
 * plugin list via GSettings.
 */

/* The GSettings schema and key that store the enabled quick-setting plugins */
#define PLUGIN_SCHEMA_ID  "sm.puri.phosh.plugins"
#define PLUGIN_SETTINGS_KEY "quick-settings"

struct _PhoshPluginManager {
  GObject     parent;

  GPtrArray  *plugins;   /* (element-type PhoshPluginInfo) */
  GSettings  *settings;
  GStrv       plugin_dirs;
};

G_DEFINE_TYPE (PhoshPluginManager, phosh_plugin_manager, G_TYPE_OBJECT)

/* -------------------------------------------------------------------------
 * Plugin directory scanning
 * ------------------------------------------------------------------------- */

static void
scan_plugin_dir (PhoshPluginManager *self, const char *dir_path)
{
  g_autoptr (GDir) dir = g_dir_open (dir_path, 0, NULL);
  const char *filename;
  g_auto (GStrv) enabled_plugins = NULL;

  if (dir == NULL) {
    g_debug ("Plugin directory '%s' does not exist, skipping", dir_path);
    return;
  }

  enabled_plugins = g_settings_get_strv (self->settings, PLUGIN_SETTINGS_KEY);
  g_debug ("Scanning plugin directory: %s", dir_path);

  while ((filename = g_dir_read_name (dir))) {
    g_autofree char *path = NULL;
    g_autoptr (GKeyFile) keyfile = g_key_file_new ();
    g_autoptr (GError) error = NULL;
    PhoshPluginInfo *info;
    gboolean enabled;

    /* Only process .plugin files */
    if (!g_str_has_suffix (filename, ".plugin"))
      continue;

    path = g_build_filename (dir_path, filename, NULL);

    if (!g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, &error)) {
      g_warning ("Failed to load plugin metadata '%s': %s", path, error->message);
      continue;
    }

    enabled = g_strv_contains ((const char * const *) enabled_plugins,
                               g_key_file_get_string (keyfile, "Plugin", "Id", NULL));

    info = phosh_plugin_info_new (keyfile, enabled);
    if (info == NULL)
      continue;

    g_debug ("Discovered plugin: %s (%s) enabled=%d", info->id, info->name, info->enabled);
    g_ptr_array_add (self->plugins, info);
  }
}

/* -------------------------------------------------------------------------
 * GObject boilerplate
 * ------------------------------------------------------------------------- */

static void
phosh_plugin_manager_finalize (GObject *object)
{
  PhoshPluginManager *self = PHOSH_PLUGIN_MANAGER (object);

  g_clear_object (&self->settings);
  g_strfreev (self->plugin_dirs);

  if (self->plugins) {
    g_ptr_array_set_free_func (self->plugins, (GDestroyNotify) phosh_plugin_info_free);
    g_ptr_array_free (self->plugins, TRUE);
  }

  G_OBJECT_CLASS (phosh_plugin_manager_parent_class)->finalize (object);
}

static void
phosh_plugin_manager_class_init (PhoshPluginManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = phosh_plugin_manager_finalize;
}

static void
phosh_plugin_manager_init (PhoshPluginManager *self)
{
  const char *default_dirs[] = { PHOSH_PLUGINS_DIR, NULL };

  self->plugins = g_ptr_array_new_with_free_func ((GDestroyNotify) phosh_plugin_info_free);
  self->settings = g_settings_new (PLUGIN_SCHEMA_ID);
  self->plugin_dirs = g_strdupv ((GStrv) default_dirs);

  /* Scan all plugin directories */
  for (int i = 0; self->plugin_dirs[i] != NULL; i++)
    scan_plugin_dir (self, self->plugin_dirs[i]);
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

PhoshPluginManager *
phosh_plugin_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_PLUGIN_MANAGER, NULL);
}

GPtrArray *
phosh_plugin_manager_get_plugins (PhoshPluginManager *self)
{
  g_return_val_if_fail (PHOSH_IS_PLUGIN_MANAGER (self), NULL);

  return self->plugins;
}

gboolean
phosh_plugin_manager_set_enabled (PhoshPluginManager *self,
                                  const char         *plugin_id,
                                  gboolean            enabled)
{
  g_auto (GStrv) current = NULL;
  g_autoptr (GStrvBuilder) builder = g_strv_builder_new ();
  gboolean found = FALSE;

  g_return_val_if_fail (PHOSH_IS_PLUGIN_MANAGER (self), FALSE);
  g_return_val_if_fail (plugin_id != NULL, FALSE);

  current = g_settings_get_strv (self->settings, PLUGIN_SETTINGS_KEY);

  if (enabled) {
    /* Append to the end, preserving existing order */
    /* First copy all existing entries */
    for (int i = 0; current[i] != NULL; i++) {
      g_strv_builder_add (builder, current[i]);
      if (g_strcmp0 (current[i], plugin_id) == 0)
        found = TRUE;
    }
    /* Only append if not already in the list */
    if (!found) {
      g_debug ("Enabling plugin: %s (appending to list)", plugin_id);
      g_strv_builder_add (builder, plugin_id);
    }
  } else {
    /* Remove from the list, preserving order */
    g_debug ("Disabling plugin: %s (removing from list)", plugin_id);
    for (int i = 0; current[i] != NULL; i++) {
      if (g_strcmp0 (current[i], plugin_id) == 0) {
        found = TRUE;
        continue;  /* Skip the one we're removing */
      }
      g_strv_builder_add (builder, current[i]);
    }
    if (!found) {
      g_debug ("Plugin '%s' was not in the enabled list", plugin_id);
    }
  }

  g_settings_set_strv (self->settings, PLUGIN_SETTINGS_KEY,
                       (const char * const *) g_strv_builder_end (builder));

  /* Update our cached enabled state */
  for (guint i = 0; i < self->plugins->len; i++) {
    PhoshPluginInfo *info = g_ptr_array_index (self->plugins, i);
    if (g_strcmp0 (info->id, plugin_id) == 0) {
      info->enabled = enabled;
      break;
    }
  }

  return TRUE;
}

gboolean
phosh_plugin_manager_is_enabled (PhoshPluginManager *self,
                                 const char         *plugin_id)
{
  g_return_val_if_fail (PHOSH_IS_PLUGIN_MANAGER (self), FALSE);

  for (guint i = 0; i < self->plugins->len; i++) {
    PhoshPluginInfo *info = g_ptr_array_index (self->plugins, i);
    if (g_strcmp0 (info->id, plugin_id) == 0)
      return info->enabled;
  }

  return FALSE;
}
