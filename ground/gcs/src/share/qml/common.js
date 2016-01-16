// ***********************
// common.js
//
// Common javascript utils
//
// Librepilot
// ***********************

// Format time
function formatTime(time) {
    if (time === 0)
        return "00"
    if (time < 10)
        return "0" + time;
    else
        return time.toString();
}
