/* GStreamer
 * Copyright (C) <2017> Philippe Renon <philippe_renon@yahoo.fr>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "cameraevent.hpp"

#include <opencv2/opencv.hpp>

#include <QDebug>
#include <QString>

/**
 * gst_video_event_new_still_frame:
 * @in_still: boolean value for the still-frame state of the event.
 *
 * Creates a new Still Frame event. If @in_still is %TRUE, then the event
 * represents the start of a still frame sequence. If it is %FALSE, then
 * the event ends a still frame sequence.
 *
 * To parse an event created by gst_video_event_new_still_frame() use
 * gst_video_event_parse_still_frame().
 *
 * Returns: The new GstEvent
 */
GstEvent *
gst_camera_event_new_calibrated (gchar * settings)
{
  GstEvent *calibrated_event;
  GstStructure *s;

  s = gst_structure_new (GST_CAMERA_EVENT_CALIBRATED_NAME,
          "undistort-settings", G_TYPE_STRING, g_strdup(settings), NULL);

  calibrated_event = gst_event_new_custom (GST_EVENT_CUSTOM_BOTH, s);

  return calibrated_event;
}

/**
 * gst_video_event_parse_still_frame:
 * @event: A #GstEvent to parse
 * @in_still: A boolean to receive the still-frame status from the event, or NULL
 *
 * Parse a #GstEvent, identify if it is a Still Frame event, and
 * return the still-frame state from the event if it is.
 * If the event represents the start of a still frame, the in_still
 * variable will be set to TRUE, otherwise FALSE. It is OK to pass NULL for the
 * in_still variable order to just check whether the event is a valid still-frame
 * event.
 *
 * Create a still frame event using gst_video_event_new_still_frame()
 *
 * Returns: %TRUE if the event is a valid still-frame event. %FALSE if not
 */
gboolean
gst_camera_event_parse_calibrated (GstEvent * event, gchar ** settings)
{
  const GstStructure *s;

  g_return_val_if_fail (event != NULL, FALSE);

  if (GST_EVENT_TYPE (event) != GST_EVENT_CUSTOM_BOTH)
      return FALSE;               /* Not a calibrated event */

  s = gst_event_get_structure (event);
  if (s == NULL
      || !gst_structure_has_name (s, GST_CAMERA_EVENT_CALIBRATED_NAME))
    return FALSE;               /* Not a calibrated event */

  const gchar *str = gst_structure_get_string(s, "undistort-settings");
  if (!str)
      return FALSE;               /* Not calibrated frame event */

  //qDebug() << "*** " << buf;//QString::fromStdString(buf);

  *settings = g_strdup (str);

  return TRUE;
}
