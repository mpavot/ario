/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __ARIO_SHELL_H
#define __ARIO_SHELL_H

#include <glib-object.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define TYPE_ARIO_SHELL         (ario_shell_get_type ())
#define ARIO_SHELL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_SHELL, ArioShell))
#define ARIO_SHELL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_SHELL, ArioShellClass))
#define IS_ARIO_SHELL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_SHELL))
#define IS_ARIO_SHELL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_SHELL))
#define ARIO_SHELL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_SHELL, ArioShellClass))

typedef struct ArioShellPrivate ArioShellPrivate;

typedef struct
{
        GObject parent;

        ArioShellPrivate *priv;
} ArioShell;

typedef struct
{
        GObjectClass parent_class;
} ArioShellClass;

typedef enum
{
        VISIBILITY_HIDDEN,
        VISIBILITY_VISIBLE,
        VISIBILITY_TOGGLE
}ArioVisibility;

GType           ario_shell_get_type             (void) G_GNUC_CONST;

ArioShell *     ario_shell_new                  (void);

void            ario_shell_construct            (ArioShell *shell,
                                                 gboolean minimized);
void            ario_shell_shutdown             (ArioShell *shell);

void            ario_shell_present              (ArioShell *shell);

void            ario_shell_set_visibility       (ArioShell *shell,
                                                 ArioVisibility state);
void            ario_shell_restore_main_window  (ArioShell *shell);

G_END_DECLS

#endif /* __ARIO_SHELL_H */
