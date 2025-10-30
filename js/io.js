// io module initialization
//
io.stdin = io.stdIn();
io.stdout = io.stdOut();
io.stderr = io.stdErr();

io._tq = [];

io._tqswap = function(a, b) {
    var t = io._tq[a];
    io._tq[a] = io._tq[b];
    io._tq[b] = t;
}

io._tqup = function() {
    var tq = io._tq;
    var a = tq.length;
    if (a == 0) return;
    a--;
    while (a>0) {
        var b = (a - 1) / 2;
        if (tq[b].ms <= tq[a].ms) break;
        io._tqswap(a, b);
        a = b;
    }
}

io._tqdown = function () {
    var tq = io._tq;
    var n = tq.length;
    var i = 0;
    do {
        var left = 2 * i + 1;
        if (left >= n || left < i) break;
        var j = left;
        var right = left + 1;
        if (right < n && tq[j].ms > tq[right].ms) j = right;
        if (tq[i].ms <= tq[j].ms) break;
        io._tqswap(i, j);
        i = j;
    } while (1);
}

io.timer( function (now) {
    console.log("TICK "+now);
    var tq = io._tq;
    while (tq.length>0 && tq[0].ms<=now) {
        try {
            tq[0].callback();
        } catch (e) {
            io.log(e);
        }
        tq[0] = tq[io._tq.length-1];
        tq.pop();
        io._tqdown();
    }
    if (tq.length>0) {
        return tq[0].ms - now
    } else {
        return 1000*60*60*24*31; // month or "never"
    }
} );

io.setTimeout = function (ms, callback) {
    io.log("set timeout "+ms)
    var tq = io._tq;
    task = {ms: io.now() + ms, callback: callback};
    tq.push(task);
    io._tqup();
    io.wakeIn(ms);
}

io.log("io init OK, incl timeouts");
