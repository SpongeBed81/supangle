console.log('running');

require('module.js');

const timeout = setTimeout(() => {
    console.log('hello from timeout');
}, 2000);

// clearTimeout(timeout);