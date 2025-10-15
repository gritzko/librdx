var i=1; 
io.timer(function(){
    io.log("tick"); 
    i*=2; 
    return i;
});
