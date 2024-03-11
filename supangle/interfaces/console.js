(() => {
    const times = [];

    console.log = function(...arguments) {
        for (const arg of arguments) {
            process.stdout.write(arg);
            process.stdout.write("\n");
        }
    };
    
    console.time = function(label) {
        times[label] = Date.now();
    };
      
      
    console.timeEnd = function(label) {
        const time = times[label];
        if (!time) {
          throw new Error('No such label: ' + label);
        }

        const duration = Date.now() - time;
        console.log(`${label}: ${duration}ms`)
      };
})()