
/* Generated data (by glib-mkenums) */

#include "ario-enum-types.h"

/* enumerations from "servers/ario-server.h" */
#include "servers/ario-server.h"

GType
ario_server_type_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ ArioServerMpd,
			  "ArioServerMpd",
			  "mpd" },
			{ ArioServerXmms,
			  "ArioServerXmms",
			  "xmms" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("ArioServerType"),
				values);
	}
	return the_type;
}

GType
ario_server_action_type_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ ARIO_SERVER_ACTION_ADD,
			  "ARIO_SERVER_ACTION_ADD",
			  "add" },
			{ ARIO_SERVER_ACTION_DELETE_ID,
			  "ARIO_SERVER_ACTION_DELETE_ID",
			  "delete-id" },
			{ ARIO_SERVER_ACTION_DELETE_POS,
			  "ARIO_SERVER_ACTION_DELETE_POS",
			  "delete-pos" },
			{ ARIO_SERVER_ACTION_MOVE,
			  "ARIO_SERVER_ACTION_MOVE",
			  "move" },
			{ ARIO_SERVER_ACTION_MOVEID,
			  "ARIO_SERVER_ACTION_MOVEID",
			  "moveid" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("ArioServerActionType"),
				values);
	}
	return the_type;
}

/* enumerations from "shell/ario-shell.h" */
#include "shell/ario-shell.h"

GType
ario_visibility_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ VISIBILITY_HIDDEN,
			  "VISIBILITY_HIDDEN",
			  "hidden" },
			{ VISIBILITY_VISIBLE,
			  "VISIBILITY_VISIBLE",
			  "visible" },
			{ VISIBILITY_TOGGLE,
			  "VISIBILITY_TOGGLE",
			  "toggle" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("ArioVisibility"),
				values);
	}
	return the_type;
}


/* Generated data ends here */

