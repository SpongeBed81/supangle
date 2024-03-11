const setTimeout = function(callback, sleepTime) {
    return process.timeout(callback, sleepTime, 0);
}

const setInterval = function(callback, sleepTime) {
    return process.timeout(callback, 0, sleepTime);
}