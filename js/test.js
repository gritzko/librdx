function is_inner(av, bv, path, key) {
        var at = typeof av;
        var bt = typeof bv;
        if (at!=bt) 
            throw path+" "+key+": "+at+" vs "+bt;
        if (at!="object" && av!=bv)
            throw path+" "+key+": "+av+" vs "+bv;
        if (av==null || bv==null) {
            if (av==null && bv==null) return true;
            throw path+" "+key+": "+av+" vs "+bv;
        }
        is_impl(av, bv, path + "/" + key, 0)
}

function is_impl(a, b, path, t) {
    for(const key in a) {
        var av = a[key]
        var bv = b[key]
        is_inner(av, bv, path, key)
    }
    if (t==0)
        is_impl(a, b, path, 1)
}

test.is = function(a, b) {

    return is_inner(a, b, "", "values")
}
