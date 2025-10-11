io.connect("127.0.0.1:8888", function(rw){
    io.log("CALLBACK: "+rw)
    if (rw=="w") {
        this.write("Hello");
    } else if (rw=="r") {
        var ta = new Uint8Array(16);
        this.read(ta);
        io.log(ta);
    }
});
