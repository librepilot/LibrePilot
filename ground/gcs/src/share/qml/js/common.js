/*
 * Copyright (C) 2016 The LibrePilot Project
 * Contact: http://www.librepilot.org
 *
 * This file is part of LibrePilot GCS.
 *
 * LibrePilot GCS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LibrePilot GCS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LibrePilot GCS.  If not, see <http://www.gnu.org/licenses/>.
 */

// ***********************
// Common javascript utils
// ***********************

// Get date
function getDateTime() {
    switch(pfdContext.timeMode) {
    case TimeMode.Local:
         return new Date();
    case TimeMode.Predefined:
         return pfdContext.dateTime;
    }
}

// Format time with two digits
function formatTime(time) {
    if (time === 0)
        return "00";
    if (time < 10)
        return "0" + time;
    else
        return time.toString();
}

// Returns a formated time value
// In: 119 (seconds)
// Out: 00:01:59
function formatFlightTime(timeValue) {
    var timeHour = 0;
    var timeMinutes = 0;
    var timeSeconds = 0;

    if (timeValue > 0) {
        timeHour = Math.floor(timeValue / 3600);
        timeMinutes = Math.floor((timeValue - timeHour * 3600) / 60);
        timeSeconds = Math.floor(timeValue - timeHour * 3600 - timeMinutes * 60);
    }

    return formatTime(timeHour) + ":" + formatTime(timeMinutes) + ":" + formatTime(timeSeconds);
}

// Returns a formated ETA
function estimatedTimeOfArrival(distance, velocity) {
    var etaValue = 0;
    var etaHour = 0;
    var etaMinutes = 0;
    var etaSeconds = 0;

    if ((distance > 0) && (velocity > 0)) {
        etaValue = Math.round(distance/velocity);
    }

    return formatFlightTime(etaValue);
}

