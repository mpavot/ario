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

#include <gtk/gtkwindow.h>

G_BEGIN_DECLS

/**
 * ArioShell represents the main window of Ario.
 * It is also in charge of starting the different services at startup
 * and of stoping them on exit
 */

#define ARIO_TYPE_SHELL         (ario_shell_get_type ())
#define ARIO_SHELL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ARIO_TYPE_SHELL, ArioShell))
#define ARIO_SHELL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ARIO_TYPE_SHELL, ArioShellClass))
#define IS_ARIO_SHELL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), ARIO_TYPE_SHELL))
#define IS_ARIO_SHELL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ARIO_TYPE_SHELL))
#define ARIO_SHELL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ARIO_TYPE_SHELL, ArioShellClass))

typedef struct ArioShellPrivate ArioShellPrivate;

typedef struct
{
        GtkWindow parent;

        ArioShellPrivate *priv;
} ArioShell;

typedef struct
{
        GtkWindowClass parent_class;
} ArioShellClass;

/* Main Window visibility */
typedef enum
{
        VISIBILITY_HIDDEN,
        VISIBILITY_VISIBLE,
        VISIBILITY_TOGGLE
} ArioVisibility;

GType           ario_shell_get_type             (void) G_GNUC_CONST;

ArioShell *     ario_shell_new                  (void);

void            ario_shell_construct            (ArioShell *shell,
                                                 gboolean minimized);
void            ario_shell_shutdown             (ArioShell *shell);

void            ario_shell_present              (ArioShell *shell);

void            ario_shell_set_visibility       (ArioShell *shell,
                                                 ArioVisibility state);

G_END_DECLS

#endif /* __ARIO_SHELL_H */
