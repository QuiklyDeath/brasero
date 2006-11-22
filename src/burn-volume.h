/***************************************************************************
 *            burn-volume.h
 *
 *  mer nov 15 09:44:34 2006
 *  Copyright  2006  Philippe Rouquier
 *  bonfire-app@wanadoo.fr
 ***************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>

#ifndef BURN_VOLUME_H
#define BURN_VOLUME_H

G_BEGIN_DECLS

struct _BraseroVolDesc {
	guchar type;
	gchar id			[5];
	guchar version;
};
typedef struct _BraseroVolDesc BraseroVolDesc;

typedef struct _BraseroVolFile BraseroVolFile;
struct _BraseroVolFile {
	BraseroVolFile *parent;

	gchar *name;

	union {
	struct {
		gint address_block;
		gint size_bytes;
	} file;

		GList *children;
	} specific;

	gint isdir:1;
};

void
brasero_volume_file_free (BraseroVolFile *file);

gboolean
brasero_volume_is_valid (const gchar *path,
			 GError **error);

gboolean
brasero_volume_is_iso9660 (const gchar *path,
			   GError **error);

gboolean
brasero_volume_get_size (const gchar *path,
			 gint64 *nb_blocks,
			 GError **error);
gboolean
brasero_volume_get_label (const gchar *path,
			  gchar **label,
			  GError **error);
BraseroVolFile *
brasero_volume_get_files (const gchar *path,
			  gchar **label,
			  gint64 *nb_blocks,
			  GError **error);



G_END_DECLS

#endif /* BURN_VOLUME_H */
