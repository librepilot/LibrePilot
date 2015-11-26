// ***********************
// common.js
//
// Common javascript utils
//
// Librepilot
// ***********************
//
// qml/js treats qint8 as a char, necessary to convert it back to integer value
function toInt(enum_value) {
    if (Object.prototype.toString.call(enum_value) == "[object String]") {
        return enum_value.charCodeAt(0);
    }
    return enum_value;
}

// Format time
function formatTime(time) {
    if (time === 0)
        return "00"
    if (time < 10)
        return "0" + time;
    else
        return time.toString();
}

