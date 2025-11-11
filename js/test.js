function is_inner(av, bv, path) {
        var at = typeof av;
        var bt = typeof bv;
        if (at!=bt) 
            throw path+" "+at+" vs "+bt;
        if (at!="object") {
            if (av!=bv) throw path+" "+av+" vs "+bv;
            return;
        }
        if (av==null || bv==null) {
            if (av==null && bv==null) return true;
            throw path+" "+av+" vs "+bv;
        }
        is_impl(av, bv, path, 0)
}

function is_impl(a, b, path, t) {
    for(const key in a) {
        var av = a[key]
        var bv = b[key]
        is_inner(av, bv, path + "/" + key)
    }
    if (t==0)
        is_impl(a, b, path, 1)
}

test.is = function(a, b) {
    return is_inner(a, b, "")
}
