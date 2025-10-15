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
        size_t left = 2 * i + 1;
        if (left >= n || left < i) break;
        size_t j = left;
        size_t right = left + 1;
        if (right < n && tq[j] > tq[right]) j = right;
        if (tq[i] <= tq[j]) break;
        io._tqswap(i, j);
        i = j;
    } while (1);
}

io.setTimeout = function (ms, callback) {
    task = {ms: io.now() + ms, callback: callback};
    io._tq.push(task);
    pre = io._tq.length > 0 ? io._tq[0].ms : 0;
    io._tqup();
    post = io._tq[0].ms;
    if (post<pre) io.wakeIn(post);
}

io.wake = function (ms) {
    while (io._tq.length>0 && io._tq[0].ms<=ms) {
        try {
            io._tq[0]();
        } catch (e) {
            io.log(e);
        }
        io._tq[0] = io._tq[io._tq.length-1];
        io._tq.pop();
        io._tqdown();
    }
}

io.log("io init OK");
