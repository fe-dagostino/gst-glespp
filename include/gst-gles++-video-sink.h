/* GStreamer
 * Copyright (C) <2014> Francesco Emanuele D'Agostino <fedagostino@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_GLES_PLUGIN_H__
#define __GST_GLES_PLUGIN_H__

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>

G_BEGIN_DECLS

/* Version number of package */
#ifndef VERSION
#define VERSION "1.0.0"
#endif /* VERSION */
#ifndef PACKAGE
#define PACKAGE "gscore.plugin"
#endif /* PACKAGE */

/* #defines don't like whitespacey bits */
#define GST_GLES_TYPE_SINK gst_gles_sink_get_type()

#define GST_TYPE_GLES_SINK \
  (gst_gles_sink_get_type())
#define GST_GLES_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GLES_SINK,GstGlesSink))
#define GST_GLES_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GLES_SINK,GstGlesSinkClass))
#define GST_IS_GLES_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GLES_SINK))
#define GST_IS_GLES_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GLES_SINK))

  
typedef struct _GstGlesSink        GstGlesSink;
typedef struct _GstGlesSinkClass   GstGlesSinkClass;

typedef struct _GstGlesBuffer      GstGlesBuffer;

typedef struct _GstGlesSinkPrivate GstGlesSinkPrivate;

struct _GstGlesSink
{
  /* Our element stuff */
  GstVideoSink        parent;
  
  GstGlesSinkPrivate* priv;
  
  /*< public >*/

  /* Framerate numerator and denominator */
  gint     fps_n;
  gint     fps_d;

  gint     width;
  gint     height;
  guint    format;
  guint    type;
  gpointer texture;
};

struct _GstGlesSinkClass 
{
  /*< private >*/
  GstVideoSinkClass    parent_class;
};

/**
 * GstGlesBuffer:
 * Subclass of #GstBuffer containing additional information about an XImage.
 */
struct _GstGlesBuffer
{
  GstBuffer    buffer;

  /* Reference to the clessink we belong to */
  GstGlesSink *glessink;

  gint         width;
  gint         height;
  guint        format;
  guint        type;
  
  gsize        size;
  gpointer     pixels;
};

GType gst_gles_sink_get_type (void);

G_END_DECLS

#endif /* __GST_GLES_PLUGIN_H__ */

