/*
    This file is part of gst-gles plus plus.

    gst-gles++ is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    gst-gles++ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with gst-gles++.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <GLTexture.h>

#include "gst-gles++-video-sink.h"

#include <gst/video/gstvideopool.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/video.h>

#include <gst/gstelement.h>
#include <gst/gstbuffer.h>

/* Debugging category */
#include <gst/gstinfo.h>



USING_NAMESPACE_FED


#define DEFAULT_TEXTURE_WIDTH   320
#define DEFAULT_TEXTURE_HEIGHT  240
#define DEFAULT_TEXTURE_FORMAT  GL_RGB
#define DEFAULT_TEXTURE_DTYPE   GL_UNSIGNED_BYTE

/* Default template - initiated with class struct to allow gst-register to work
   without X running */
static GstStaticPadTemplate gst_gles_sink_template_factory =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, format=RGB, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], " 
        "height = (int) [ 1, MAX ]; "
        "video/x-raw, format=RGBA, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], " 
        "height = (int) [ 1, MAX ]")
    );

GST_DEBUG_CATEGORY_STATIC (gst_gles_sink_debug);
#define GST_CAT_DEFAULT gst_gles_sink_debug    
    
enum
{
  PROP_0,
  PROP_TEXTURE,
  PROP_TEXTURE_WIDTH,
  PROP_TEXTURE_HEIGHT,
  PROP_TEXTURE_FORMAT,
  PROP_TEXTURE_DTYPE
};    

struct _GstGlesSinkPrivate 
{
  GMutex                 mtx_update;
  GLTexture              texture;
  GstClockTime           pts;
};



static void                  gst_gles_sink_dispose       (GObject * object);
static void                  gst_gles_sink_finalize      (GObject * object);
static void                  gst_gles_sink_set_property  (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void                  gst_gles_sink_get_property  (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
  
static GstCaps *             gst_gles_sink_getcaps       (GstBaseSink *bsink, GstCaps *filter);
static gboolean              gst_gles_sink_setcaps       (GstBaseSink *bsink, GstCaps *caps);

static GstStateChangeReturn  gst_gles_sink_change_state  (GstElement * element, GstStateChange transition);
static void                  gst_gles_sink_get_times     (GstBaseSink * bsink, GstBuffer * buf, GstClockTime * start, GstClockTime * end);

static gboolean              gst_gles_sink_event         (GstBaseSink * sink, GstEvent * event);

static GstFlowReturn         gst_gles_sink_show_frame    (GstVideoSink * video_sink, GstBuffer * buffer);

static void                  gst_gles_sink_init          (GstGlesSink      * glessink);
static void                  gst_gles_sink_base_init     (gpointer           g_class ); 
static void                  gst_gles_sink_class_init    (GstGlesSinkClass * klass   ); 

static gboolean              gst_gles_sink_get_buffer_details (GstPad * pad, gint * width, gint * height, guint * format);



static gpointer      gst_gles_sink_parent_class = NULL; 


GType gst_gles_sink_get_type (void) 
{ 
  static GType glessink_type = 0;

  if (!glessink_type) 
  {
    static const GTypeInfo glessink_info = {
      sizeof (GstGlesSinkClass),
      gst_gles_sink_base_init,
      NULL,
      (GClassInitFunc) gst_gles_sink_class_init,
      NULL,
      NULL,
      sizeof (GstGlesSink), 0, (GInstanceInitFunc) gst_gles_sink_init,
    };
    
    glessink_type = g_type_register_static (GST_TYPE_VIDEO_SINK, "GstGlesSink", &glessink_info, (GTypeFlags)0);

    /* register type and create class in a more safe place instead of at
     * runtime since the type registration and class creation is not
     * threadsafe. */
    //g_type_class_ref (gst_gles_buffer_get_type ());
  }

  return glessink_type;  
} /* closes gst_gles_sink_get_type() */

static void gst_gles_sink_init (GstGlesSink * glessink)
{
  
  glessink->priv              = new GstGlesSinkPrivate();
  
  g_mutex_init( &glessink->priv->mtx_update );
  
  glessink->priv->pts         = 0;
    
  glessink->fps_n             = 0;
  glessink->fps_d             = 1;
  
  glessink->width             = DEFAULT_TEXTURE_WIDTH; 
  glessink->height            = DEFAULT_TEXTURE_HEIGHT;
  glessink->format            = DEFAULT_TEXTURE_FORMAT;
  glessink->type              = DEFAULT_TEXTURE_DTYPE;
  glessink->texture           = NULL;
}

static void gst_gles_sink_base_init (gpointer g_class)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_static_metadata (
                                          gstelement_class,
                                          "Gles++ Sink",
                                          "Sink/Gles++", 
					  "A libgles++ sink",
                                          "Francesco Emanuele D'Agostino <fedagostino@gmail.com>"
					);

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&gst_gles_sink_template_factory));
}

static void gst_gles_sink_class_init (GstGlesSinkClass * klass)
{
  GObjectClass      *gobject_class;
  GstElementClass   *gstelement_class;
  GstBaseSinkClass  *gstbasesink_class;
  GstVideoSinkClass *videosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  videosink_class = (GstVideoSinkClass *) klass;

  gst_gles_sink_parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose      = gst_gles_sink_dispose;
  gobject_class->finalize     = gst_gles_sink_finalize;
  gobject_class->set_property = gst_gles_sink_set_property;
  gobject_class->get_property = gst_gles_sink_get_property;
  
  // Properties 
  g_object_class_install_property (gobject_class, PROP_TEXTURE, 
      g_param_spec_pointer ("texture", "texture", "Texture object"     , (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (gobject_class, PROP_TEXTURE_WIDTH, 
      g_param_spec_int     ("width"  , "width"  , "Texture width"      , 0, G_MAXINT, DEFAULT_TEXTURE_WIDTH , (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_TEXTURE_HEIGHT, 
      g_param_spec_int     ("height" , "height" , "Texture height"     , 0, G_MAXINT, DEFAULT_TEXTURE_HEIGHT, (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));
  
  g_object_class_install_property (gobject_class, PROP_TEXTURE_FORMAT, 
      g_param_spec_int     ("format" , "format" , "Texture pixels ftm" , 0, G_MAXINT, DEFAULT_TEXTURE_FORMAT, (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_TEXTURE_DTYPE, 
      g_param_spec_int     ("dtype"  , "dtype"  , "Texture pixels type", 0, G_MAXINT, DEFAULT_TEXTURE_DTYPE , (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));
  
  gstelement_class->change_state        = GST_DEBUG_FUNCPTR (gst_gles_sink_change_state);

  gstbasesink_class->get_caps           = GST_DEBUG_FUNCPTR (gst_gles_sink_getcaps);
  gstbasesink_class->set_caps           = GST_DEBUG_FUNCPTR (gst_gles_sink_setcaps);
  gstbasesink_class->get_times          = GST_DEBUG_FUNCPTR (gst_gles_sink_get_times);
  gstbasesink_class->event              = GST_DEBUG_FUNCPTR (gst_gles_sink_event);
  
  videosink_class->show_frame           = GST_DEBUG_FUNCPTR (gst_gles_sink_show_frame);
}

static void gst_gles_sink_dispose (GObject * object)
{
  GstGlesSink *glessink = GST_GLES_SINK (object);

  g_mutex_clear( &glessink->priv->mtx_update );
  
  if ( glessink->priv )
  {
    delete glessink->priv;
    glessink->priv = NULL;
  }
  
  G_OBJECT_CLASS (gst_gles_sink_parent_class)->dispose (object);
}

static void gst_gles_sink_finalize (GObject * object)
{
  G_OBJECT_CLASS (gst_gles_sink_parent_class)->finalize (object);
}

static void gst_gles_sink_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstGlesSink *sink = GST_GLES_SINK (object);

  switch (prop_id) 
  {
    case PROP_TEXTURE:
    {
      g_mutex_lock( &sink->priv->mtx_update   );
        sink->texture = g_value_get_pointer( value );
      g_mutex_unlock( &sink->priv->mtx_update );
    }; break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void gst_gles_sink_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstGlesSink *sink = GST_GLES_SINK (object);

  switch (prop_id) 
  {
    case PROP_TEXTURE:
    {
      GLTexture* pTexture = (GLTexture*)(sink->texture);
      if ( pTexture != NULL )
      {
        g_mutex_lock( &sink->priv->mtx_update   );
      
        pTexture->init( 
                         sink->priv->texture.getSize().width,
                         sink->priv->texture.getSize().height,
                         sink->priv->texture.getFormat(),
                         sink->priv->texture.getType(),
                         sink->priv->texture.getPixels()       
                      );

        g_mutex_unlock( &sink->priv->mtx_update );
      }
    }; break;
    case PROP_TEXTURE_WIDTH:
      g_mutex_lock( &sink->priv->mtx_update   );
        g_value_set_int ( value, sink->width  );
      g_mutex_unlock( &sink->priv->mtx_update );
    break;
    case PROP_TEXTURE_HEIGHT:
      g_mutex_lock( &sink->priv->mtx_update   );
        g_value_set_int ( value, sink->height );
      g_mutex_unlock( &sink->priv->mtx_update );
    break;
    case PROP_TEXTURE_FORMAT:
      g_mutex_lock( &sink->priv->mtx_update   );
        g_value_set_int ( value, sink->format );
      g_mutex_unlock( &sink->priv->mtx_update );
    break;
    case PROP_TEXTURE_DTYPE:
      g_mutex_lock( &sink->priv->mtx_update   );
        g_value_set_int ( value, sink->type   );
      g_mutex_unlock( &sink->priv->mtx_update );
    break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

/* Element stuff */


static GstCaps *  gst_gles_sink_getcaps (GstBaseSink *bsink, GstCaps *filter)
{
  GstGlesSink * glessink = GST_GLES_SINK(bsink);
  GstCaps *caps;
 
  /* get a template copy */
  caps = gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_SINK(glessink)->sinkpad));
  
  return caps;
}

static gboolean   gst_gles_sink_setcaps (GstBaseSink *bsink, GstCaps *caps)
{
  GstGlesSink * glessink = GST_GLES_SINK(bsink);
  gboolean ret = TRUE;
  GstVideoInfo    info;
  
  ret  = gst_video_info_from_caps( &info, caps );
  if (!ret)
    return FALSE;

  GST_VIDEO_SINK_WIDTH  (glessink) = info.width;
  GST_VIDEO_SINK_HEIGHT (glessink) = info.height;
  glessink->fps_n                  = info.fps_n;
  glessink->fps_d                  = info.fps_d;

  /* Creating our window and our image */
  if (GST_VIDEO_SINK_WIDTH (glessink) <= 0 || GST_VIDEO_SINK_HEIGHT (glessink) <= 0)
  {
    GST_ELEMENT_ERROR (glessink, CORE, NEGOTIATION, (NULL), ("Invalid image size."));
    return FALSE;
  }

  return TRUE;
}

static GstStateChangeReturn  gst_gles_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret      = GST_STATE_CHANGE_SUCCESS;
  GstGlesSink *        glessink = GST_GLES_SINK(element);
  
  ret = GST_ELEMENT_CLASS (gst_gles_sink_parent_class)->change_state (element, transition);

  switch (transition)
  {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
      glessink->fps_n = 0;
      glessink->fps_d = 1;
      GST_VIDEO_SINK_WIDTH  (glessink) = 0;
      GST_VIDEO_SINK_HEIGHT (glessink) = 0;
    }; break;
    case GST_STATE_CHANGE_READY_TO_NULL:
    {
    }; break;
    default:
    break;
  }
  
  return ret;
}
 
static void gst_gles_sink_get_times (GstBaseSink * bsink, GstBuffer * buf, GstClockTime * start, GstClockTime * end)
{
  GstGlesSink *glessink = GST_GLES_SINK (bsink);

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buf))
  {
    *start = GST_BUFFER_TIMESTAMP (buf);
    
    if (GST_BUFFER_DURATION_IS_VALID (buf))
    {
      *end = *start + GST_BUFFER_DURATION (buf);
    } 
    else
    {
      if (glessink->fps_n > 0)
      {
        *end = *start + gst_util_uint64_scale_int (GST_SECOND, glessink->fps_d, glessink->fps_n);
      }
    }
  }
}

static GstFlowReturn gst_gles_sink_show_frame (GstVideoSink * video_sink, GstBuffer * buffer)
{
  GstBaseSink *basesink = GST_BASE_SINK(video_sink);
  GstGlesSink *glessink = GST_GLES_SINK(video_sink);
  GstMapInfo   info;

  gst_buffer_map (buffer, &info, GST_MAP_READ);

  if ( (info.size > 0) && (info.data != NULL) ) 
  {
    if ( buffer->pts != glessink->priv->pts )
    {
      gint              width   = 0;
      gint              height  = 0;
      guint             ftm     = 0;
      
      if ( gst_gles_sink_get_buffer_details( basesink->sinkpad, &width, &height, &ftm ) == true )
      {
        g_mutex_lock( &glessink->priv->mtx_update );
          
          glessink->width  = width;
          glessink->height = height;
          glessink->format = ftm;
      
          glessink->priv->texture.init( glessink->width, glessink->height, glessink->format, glessink->type, info.data );
	  
          glessink->priv->pts = buffer->pts;

        g_mutex_unlock( &glessink->priv->mtx_update );
      }
    } // if ( buffer->pts != glessink->priv->pts )
  } // if ( (info.size > 0) && (info.data != NULL) )

  gst_buffer_unmap (buffer, &info);

  return GST_FLOW_OK;
}

/* handle events (search) */
static gboolean gst_gles_sink_event (GstBaseSink * sink, GstEvent * event)
{
  GstGlesSink  *glessink = GST_GLES_SINK  (sink );
  GstEventType  type     = GST_EVENT_TYPE (event);

  (void)glessink;
  (void)type;

  switch (type) 
  {
    case GST_EVENT_SEGMENT:
    {
      const GstSegment *segment = NULL;

      gst_event_parse_segment (event, &segment);      
    }; break;
    case GST_EVENT_FLUSH_STOP:
    {
    }; break;
    case GST_EVENT_EOS:
    {
    }; break;
    default:
    break;
  }

  return GST_BASE_SINK_CLASS (gst_gles_sink_parent_class)->event (sink, event);
}

static gboolean gst_gles_sink_get_buffer_details (GstPad * pad, gint * width, gint * height, guint * format)
{
  const GstCaps *caps = NULL;
  GstVideoInfo  info;
  gboolean ret;

  g_return_val_if_fail (pad    != NULL, FALSE);
  g_return_val_if_fail (width  != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  caps = gst_pad_get_current_caps(pad);

  if (caps == NULL) 
  {
    g_warning ("GstGlesSink: failed to get caps of pad %s:%s", GST_DEBUG_PAD_NAME (pad));
    return FALSE;
  }

  ret = gst_video_info_from_caps( &info, caps );
  if (!ret) 
  {
    g_warning ("GstGlesSink: failed to get buffer size properties on pad %s:%s", GST_DEBUG_PAD_NAME (pad));
    return FALSE;
  }

  *width  = info.width;
  *height = info.height;
  *format = 0;
  switch ( info.finfo->format )
  {
    case GST_VIDEO_FORMAT_RGB : *format = GL_RGB;  break;
    case GST_VIDEO_FORMAT_RGBA: *format = GL_RGBA; break;
    default:
    *format = 0; break;
  }
    
  GST_DEBUG ("size request on pad %s:%s: %dx%d", GST_DEBUG_PAD_NAME (pad), width ? *width : -1, height ? *height : -1);
  return TRUE;
}
