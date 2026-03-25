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

test.run = function() {
    var passed = 0;
    var failed = 0;

    function check(name, fn) {
        try {
            fn();
            passed++;
        } catch (e) {
            io.log("FAIL " + name + ": " + e);
            failed++;
        }
    }

    check("is: equal numbers", function() {
        test.is(1, 1);
    });

    check("is: equal strings", function() {
        test.is("abc", "abc");
    });

    check("is: equal objects", function() {
        test.is({a: 1, b: "x"}, {a: 1, b: "x"});
    });

    check("is: nested objects", function() {
        test.is({a: {b: 2}}, {a: {b: 2}});
    });

    check("is: null equality", function() {
        test.is(null, null);
    });

    check("utf8: encode", function() {
        var ta = utf8.Encode("hello");
        test.is(ta.byteLength, 6);
    });

    if (failed > 0) {
        io.log("FAILED: " + failed + "/" + (passed+failed));
        io.exit();
    }
    io.log("PASSED: " + passed + "/" + (passed+failed));
    io.exit();
}
