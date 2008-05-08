/*
 *  Copyright (C) 2004,2005 Marc Pavot <marc.pavot@gmail.com>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <glib.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkmessagedialog.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "covers/ario-cover.h"
#include "ario-mpd.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_cover_create_ario_cover_dir (void);

gchar *
ario_cover_make_ario_cover_path (const gchar *artist,
                                 const gchar *album,
                                 ArioCoverHomeCoversSize ario_cover_size)
{
        ARIO_LOG_FUNCTION_START
        char *ario_cover_path, *tmp;
        char *filename;
        const char *to_strip = "#/";

        if (!artist || !album)
                return NULL;

        /* There are 2 different files : a small one for the labums list
         * and a normal one for the album-cover widget */
        if (ario_cover_size == SMALL_COVER)
                filename = g_strdup_printf ("SMALL%s-%s.jpg", artist, album);
        else
                filename = g_strdup_printf ("%s-%s.jpg", artist, album);

        /* We replace some special characters with spaces. */
        for (tmp = filename; *tmp != '\0'; ++tmp) {
                if (strchr (to_strip, *tmp))
                        *tmp = ' ';
        }

        /* The returned path is ~/.config/ario/covers/filename */
        ario_cover_path = g_build_filename (ario_util_config_dir (), "covers", filename, NULL);
        g_free (filename);

        return ario_cover_path;
}

gboolean
ario_cover_cover_exists (const gchar *artist,
                         const gchar *album)
{
        ARIO_LOG_FUNCTION_START
        gchar *ario_cover_path, *small_ario_cover_path;
        gboolean result;

        /* The path for the normal cover */
        ario_cover_path = ario_cover_make_ario_cover_path (artist,
                                                           album,
                                                           NORMAL_COVER);

        /* The path for the small cover */
        small_ario_cover_path = ario_cover_make_ario_cover_path (artist,
                                                                 album,
                                                                 SMALL_COVER);

        /* We consider that the cover exists only if the normal and small covers exist */
        result = (ario_util_uri_exists (ario_cover_path) && ario_util_uri_exists (small_ario_cover_path));

        g_free (ario_cover_path);
        g_free (small_ario_cover_path);

        return result;
}

void
ario_cover_create_ario_cover_dir (void)
{
        ARIO_LOG_FUNCTION_START
        gchar *ario_cover_dir;

        ario_cover_dir = g_build_filename (ario_util_config_dir (), "covers", NULL);

        /* If the cover directory doesn't exist, we create it */
        if (!ario_util_uri_exists (ario_cover_dir))
                ario_util_mkdir (ario_cover_dir);
        g_free (ario_cover_dir);
}

gboolean
ario_cover_size_is_valid (int size)
{
        ARIO_LOG_FUNCTION_START
        /* return true if the cover isn't too big or too small (blank amazon image) */
        return (size < 1024 * 1024 * 10 && size > 900);
}

void
ario_cover_remove_cover (const gchar *artist,
                         const gchar *album)
{
        ARIO_LOG_FUNCTION_START
        gchar *small_ario_cover_path;
        gchar *ario_cover_path;

        if (!ario_cover_cover_exists (artist, album))
                return;

        /* Delete the small cover*/
        small_ario_cover_path = ario_cover_make_ario_cover_path (artist, album, SMALL_COVER);
        if (ario_util_uri_exists (small_ario_cover_path))
                ario_util_unlink_uri (small_ario_cover_path);
        g_free (small_ario_cover_path);

        /* Delete the normal cover*/
        ario_cover_path = ario_cover_make_ario_cover_path (artist, album, NORMAL_COVER);
        if (ario_util_uri_exists (ario_cover_path))
                ario_util_unlink_uri (ario_cover_path);
        g_free (ario_cover_path);
}

static gboolean
ario_cover_can_overwrite_cover (const gchar *artist,
                                const gchar *album,
                                ArioCoverOverwriteMode overwrite_mode)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *dialog;
        gint retval;

        /* If the cover already exists, we can ask, replace or skip depending of the overwrite_mode argument */
        if (ario_cover_cover_exists (artist, album)) {
                switch (overwrite_mode) {
                case OVERWRITE_MODE_ASK:
                        dialog = gtk_message_dialog_new (NULL,
                                                         GTK_DIALOG_MODAL,
                                                         GTK_MESSAGE_QUESTION,
                                                         GTK_BUTTONS_YES_NO,
                                                         _("The cover already exists. Do you want to replace it?"));

                        retval = gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);
                        /* If the user doesn't say "yes", we don't overwrite the cover */
                        if (retval != GTK_RESPONSE_YES)
                                return FALSE;
                        break;
                case OVERWRITE_MODE_REPLACE:
                        /* We overwrite the cover */
                        return TRUE;
                        break;
                case OVERWRITE_MODE_SKIP:
                        /* We don't overwrite the cover */
                        return FALSE;
                default:
                        /* By default, we don't overwrite the cover */
                        return FALSE;
                }
        }

        /*If the cover doesn't exists, we return TRUE */
        return TRUE;
}

gboolean
ario_cover_save_cover (const gchar *artist,
                       const gchar *album,
                       char *data,
                       int size,
                       ArioCoverOverwriteMode overwrite_mode)
{
        ARIO_LOG_FUNCTION_START
        gboolean ret;
        gchar *ario_cover_path, *small_ario_cover_path;
        GdkPixbufLoader *loader;
        GdkPixbuf *pixbuf, *small_pixbuf;
        int width, height;

        if (!artist || !album || !data)
                return FALSE;

        /* If the cover directory doesn't exist, we create it */
        ario_cover_create_ario_cover_dir ();

        /* If the cover already exists, can we overwrite it? */
        if (!ario_cover_can_overwrite_cover (artist, album, overwrite_mode))
                return TRUE;

        /* The path for the normal cover */
        ario_cover_path = ario_cover_make_ario_cover_path (artist,
                                                           album,
                                                           NORMAL_COVER);

        /* The path for the small cover */
        small_ario_cover_path = ario_cover_make_ario_cover_path (artist,
                                                                 album,
                                                                 SMALL_COVER);

        loader = gdk_pixbuf_loader_new ();

        /*By default, we return an error */
        ret = FALSE;

        if (gdk_pixbuf_loader_write (loader,
                                     (const guchar *)data,
                                     size,
                                     NULL)) {
                gdk_pixbuf_loader_close (loader, NULL);

                pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
                width = gdk_pixbuf_get_width (pixbuf);
                height = gdk_pixbuf_get_height (pixbuf);

                /* We resize the pixbuf to save the small cover.
                 * To do it, we keep the original aspect ratio and
                 * we limit max (height, width) by COVER_SIZE */
                if (width > height) {
                        small_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                                COVER_SIZE,
                                                                height * COVER_SIZE / width,
                                                                GDK_INTERP_BILINEAR);
                } else {
                        small_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                                width * COVER_SIZE / height,
                                                                COVER_SIZE,
                                                                GDK_INTERP_BILINEAR);
                }

                /* We save the normal and the small covers */
                if (gdk_pixbuf_save (pixbuf, ario_cover_path, "jpeg", NULL, NULL) &&
                    gdk_pixbuf_save (small_pixbuf, small_ario_cover_path, "jpeg", NULL, "quality", "95", NULL)) {
                        /* If we succeed in the 2 operations, we return OK */
                        ret = TRUE;
                }
                g_object_unref (G_OBJECT (pixbuf));
                g_object_unref (G_OBJECT (small_pixbuf));
        }

        g_free (ario_cover_path);
        g_free (small_ario_cover_path);

        return ret;
}
