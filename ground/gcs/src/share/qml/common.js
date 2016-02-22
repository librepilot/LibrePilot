// ***********************
// common.js
//
// Common javascript utils
//
// Librepilot
// ***********************


// Get date
function getDateTime() {
    switch(qmlWidget.timeMode) {
    case TimeMode.Local:
         return new Date();
    case TimeMode.Predefined:
         return qmlWidget.dateTime;
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

