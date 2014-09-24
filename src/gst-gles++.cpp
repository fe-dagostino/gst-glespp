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


#include "gst-gles++-video-sink.h"

GST_DEBUG_CATEGORY (gst_gles_sink_debug);
GST_DEBUG_CATEGORY_STATIC (GST_CAT_PERFORMANCE);

/**
 * Entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "glessink", GST_RANK_PRIMARY, GST_GLES_TYPE_SINK))
    return FALSE;
  
  GST_DEBUG_CATEGORY_INIT (gst_gles_sink_debug, "glessink", 0, "gles++ sink element");

  GST_DEBUG_CATEGORY_GET (GST_CAT_PERFORMANCE, "GST_PERFORMANCE");
   
  return TRUE;
}

/** 
 * gstreamer looks for this structure to register plugins
 *
 * exchange the string 'Template plugin' with your plugin description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    gles++sink,
    "libgles++ sink element",
    plugin_init,
    VERSION,
    "LGPL",
    PACKAGE,
    "http://gstreamer.net/"
)


