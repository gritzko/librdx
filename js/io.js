function something() {
    var TestGlobals = "test";
}

something();

io.stdin = io.StdIn()
io.stdout = io.StdOut()
io.stderr = io.StdErr()
