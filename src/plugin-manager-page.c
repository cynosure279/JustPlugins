/*
 * Copyright (C) 2026 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: JustPlugins Contributors
 */

#define G_LOG_DOMAIN "phosh-plugin-manager-page"

#include "phosh-config.h"
#include "plugin-manager-page.h"
#include "plugin-manager.h"
#include "plugin-info.h"

/**
 * SECTION:plugin-manager-page
 * @short_description: A GTK widget to list and toggle Phosh plugins
 *
 * #PhoshPluginManagerPage displays all discovered quick-setting plugins
 * in a scrollable list, each with a toggle switch. The user can enable
 * or disable plugins directly from this page.
 */

struct _PhoshPluginManagerPage {
  GtkBin              parent;

  PhoshPluginManager *manager;
  GtkListBox         *list_box;
  GtkStack           *stack;
  GtkLabel           *empty_label;
  GtkButton          *back_btn;
};

G_DEFINE_TYPE (PhoshPluginManagerPage, phosh_plugin_manager_page, GTK_TYPE_BIN)

enum {
  BACK_CLICKED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

/* -------------------------------------------------------------------------
 * Plugin row helpers
 * ------------------------------------------------------------------------- */

static GtkWidget *
create_plugin_row (PhoshPluginInfo *info)
{
  GtkWidget *row, *hbox, *vbox, *icon, *name_label, *desc_label, *switch_w;
  GtkWidget *spacer;

  row = gtk_list_box_row_new ();
  gtk_widget_set_margin_start (row, 12);
  gtk_widget_set_margin_end (row, 12);
  gtk_widget_set_margin_top (row, 6);
  gtk_widget_set_margin_bottom (row, 6);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_halign (hbox, GTK_ALIGN_FILL);
  gtk_container_add (GTK_CONTAINER (row), hbox);

  /* Plugin icon */
  if (info->icon && *info->icon) {
    icon = gtk_image_new_from_icon_name (info->icon, GTK_ICON_SIZE_LARGE_TOOLBAR);
  } else {
    icon = gtk_image_new_from_icon_name ("application-x-executable-symbolic",
                                         GTK_ICON_SIZE_LARGE_TOOLBAR);
  }
  gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

  /* Text area: name + description */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_hexpand (vbox, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  name_label = gtk_label_new (info->name);
  gtk_label_set_xalign (GTK_LABEL (name_label), 0.0);
  gtk_widget_set_halign (name_label, GTK_ALIGN_START);
  gtk_label_set_ellipsize (GTK_LABEL (name_label), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (vbox), name_label, FALSE, FALSE, 0);

  if (info->comment && *info->comment) {
    desc_label = gtk_label_new (info->comment);
    gtk_label_set_xalign (GTK_LABEL (desc_label), 0.0);
    gtk_widget_set_halign (desc_label, GTK_ALIGN_START);
    gtk_label_set_ellipsize (GTK_LABEL (desc_label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_opacity (desc_label, 0.7);
    gtk_label_set_max_width_chars (GTK_LABEL (desc_label), 40);
    gtk_box_pack_start (GTK_BOX (vbox), desc_label, FALSE, FALSE, 0);
  }

  /* Spacer to push switch to the right */
  spacer = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand (spacer, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spacer, TRUE, TRUE, 0);
  gtk_widget_set_no_show_all (spacer, TRUE);
  gtk_widget_set_visible (spacer, FALSE);

  /* Toggle switch */
  switch_w = gtk_switch_new ();
  gtk_switch_set_active (GTK_SWITCH (switch_w), info->enabled);
  gtk_widget_set_valign (switch_w, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (hbox), switch_w, FALSE, FALSE, 0);

  /* Store the plugin id as data on the switch */
  g_object_set_data_full (G_OBJECT (switch_w), "plugin-id", g_strdup (info->id), g_free);
  /* Store the plugin id on the row too for easy access */
  g_object_set_data_full (G_OBJECT (row), "plugin-id", g_strdup (info->id), g_free);

  gtk_widget_show_all (row);

  return row;
}

/* -------------------------------------------------------------------------
 * Signal handlers
 * ------------------------------------------------------------------------- */

static void
on_back_clicked (GtkButton *btn G_GNUC_UNUSED, PhoshPluginManagerPage *self)
{
  g_signal_emit (self, signals[BACK_CLICKED], 0);
}


static void
on_switch_toggled (GtkSwitch              *switch_w,
                   GParamSpec             *pspec G_GNUC_UNUSED,
                   PhoshPluginManagerPage *self)
{
  const char *plugin_id = g_object_get_data (G_OBJECT (switch_w), "plugin-id");
  gboolean active;

  g_return_if_fail (plugin_id != NULL);

  active = gtk_switch_get_active (switch_w);
  g_debug ("Plugin '%s' toggled %s", plugin_id, active ? "ON" : "OFF");

  if (!phosh_plugin_manager_set_enabled (self->manager, plugin_id, active)) {
    /* GSettings write failed — revert the switch */
    g_warning ("Failed to set plugin '%s' to %d, reverting switch", plugin_id, active);
    g_signal_handlers_block_by_func (switch_w, on_switch_toggled, self);
    gtk_switch_set_active (switch_w, !active);
    g_signal_handlers_unblock_by_func (switch_w, on_switch_toggled, self);
  }
}

/* -------------------------------------------------------------------------
 * Populate the list
 * ------------------------------------------------------------------------- */

static void
populate_plugin_list (PhoshPluginManagerPage *self)
{
  GPtrArray *plugins;
  GList *children, *l;

  g_return_if_fail (PHOSH_IS_PLUGIN_MANAGER_PAGE (self));
  g_return_if_fail (GTK_IS_LIST_BOX (self->list_box));

  /* Clear existing rows (GTK3 compatible) */
  children = gtk_container_get_children (GTK_CONTAINER (self->list_box));
  for (l = children; l != NULL; l = l->next)
    gtk_widget_destroy (GTK_WIDGET (l->data));
  g_list_free (children);

  plugins = phosh_plugin_manager_get_plugins (self->manager);

  if (plugins == NULL || plugins->len == 0) {
    gtk_stack_set_visible_child_name (self->stack, "empty");
    return;
  }

  for (guint i = 0; i < plugins->len; i++) {
    PhoshPluginInfo *info = g_ptr_array_index (plugins, i);
    GtkWidget *row = create_plugin_row (info);

    /* Connect the switch signal by finding the switch in the row */
    GList *row_children = gtk_container_get_children (GTK_CONTAINER (row));
    for (GList *rl = row_children; rl != NULL; rl = rl->next) {
      /* The hbox is the direct child */
      GtkWidget *hbox = GTK_WIDGET (rl->data);
      if (GTK_IS_BOX (hbox)) {
        GList *box_children = gtk_container_get_children (GTK_CONTAINER (hbox));
        for (GList *bl = box_children; bl != NULL; bl = bl->next) {
          if (GTK_IS_SWITCH (bl->data)) {
            g_signal_connect (bl->data, "notify::active",
                              G_CALLBACK (on_switch_toggled), self);
            break;
          }
        }
        g_list_free (box_children);
      }
    }
    g_list_free (row_children);

    gtk_container_add (GTK_CONTAINER (self->list_box), row);
  }

  gtk_stack_set_visible_child_name (self->stack, "list");
}

/* -------------------------------------------------------------------------
 * GObject boilerplate
 * ------------------------------------------------------------------------- */

static void
phosh_plugin_manager_page_dispose (GObject *object)
{
  PhoshPluginManagerPage *self = PHOSH_PLUGIN_MANAGER_PAGE (object);

  g_clear_object (&self->manager);

  G_OBJECT_CLASS (phosh_plugin_manager_page_parent_class)->dispose (object);
}

static void
phosh_plugin_manager_page_class_init (PhoshPluginManagerPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_plugin_manager_page_dispose;

  /**
   * PhoshPluginManagerPage::back-clicked:
   * @self: The plugin manager page
   *
   * Emitted when the user clicks the back button to return to the
   * quick settings panel.
   */
  signals[BACK_CLICKED] =
    g_signal_new ("back-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/ui/plugin-manager-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshPluginManagerPage, list_box);
  gtk_widget_class_bind_template_child (widget_class, PhoshPluginManagerPage, stack);
  gtk_widget_class_bind_template_child (widget_class, PhoshPluginManagerPage, empty_label);
  gtk_widget_class_bind_template_child (widget_class, PhoshPluginManagerPage, back_btn);

  gtk_widget_class_bind_template_callback (widget_class, on_back_clicked);
}

static void
phosh_plugin_manager_page_init (PhoshPluginManagerPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

GtkWidget *
phosh_plugin_manager_page_new (PhoshPluginManager *manager)
{
  PhoshPluginManagerPage *self;

  g_return_val_if_fail (PHOSH_IS_PLUGIN_MANAGER (manager), NULL);

  self = g_object_new (PHOSH_TYPE_PLUGIN_MANAGER_PAGE, NULL);
  self->manager = g_object_ref (manager);

  populate_plugin_list (self);

  return GTK_WIDGET (self);
}
