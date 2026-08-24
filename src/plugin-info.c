/*
 * Copyright (C) 2026 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: JustPlugins Contributors
 */

#define G_LOG_DOMAIN "phosh-plugin-info"

#include "plugin-info.h"

/**
 * SECTION:plugin-info
 * @short_description: Parses .plugin metadata files
 *
 * #PhoshPluginInfo reads a .plugin keyfile and extracts the metadata
 * needed to display and manage a Phosh plugin.
 */

PhoshPluginInfo *
phosh_plugin_info_new (GKeyFile *keyfile, gboolean enabled)
{
  PhoshPluginInfo *info;
  g_autofree char *id = NULL;
  g_autofree char *name = NULL;
  g_autofree char *comment = NULL;
  g_autofree char *icon = NULL;
  g_autofree char *module = NULL;
  g_autofree char *prefs_plugin = NULL;
  g_auto (GStrv) types = NULL;

  g_return_val_if_fail (keyfile != NULL, NULL);

  id = g_key_file_get_string (keyfile, "Plugin", "Id", NULL);
  if (id == NULL || *id == '\0') {
    g_warning ("Plugin metadata missing 'Id' field, skipping");
    return NULL;
  }

  module = g_key_file_get_string (keyfile, "Plugin", "Plugin", NULL);
  if (module == NULL || *module == '\0') {
    g_warning ("Plugin '%s' missing 'Plugin' field (module path), skipping", id);
    return NULL;
  }

  name = g_key_file_get_locale_string (keyfile, "Plugin", "Name", NULL, NULL);
  if (name == NULL)
    name = g_strdup (id);

  comment = g_key_file_get_locale_string (keyfile, "Plugin", "Comment", NULL, NULL);
  icon = g_key_file_get_string (keyfile, "Plugin", "Icon", NULL);
  types = g_key_file_get_string_list (keyfile, "Plugin", "Types", NULL, NULL);

  /* First version only handles quick-setting plugins */
  if (types == NULL || !g_strv_contains ((const char * const *) types, "quick-setting")) {
    g_debug ("Plugin '%s' is not a quick-setting plugin, skipping", id);
    return NULL;
  }

  prefs_plugin = g_key_file_get_string (keyfile, "Prefs", "Plugin", NULL);

  info = g_new0 (PhoshPluginInfo, 1);
  info->id = g_steal_pointer (&id);
  info->name = g_steal_pointer (&name);
  info->comment = g_steal_pointer (&comment);
  info->icon = g_steal_pointer (&icon);
  info->module = g_steal_pointer (&module);
  info->enabled = enabled;
  info->has_prefs = (prefs_plugin != NULL && *prefs_plugin != '\0');

  return info;
}

void
phosh_plugin_info_free (PhoshPluginInfo *info)
{
  if (info == NULL)
    return;

  g_free (info->id);
  g_free (info->name);
  g_free (info->comment);
  g_free (info->icon);
  g_free (info->module);
  g_free (info);
}
