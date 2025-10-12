#   JavaScriptCore bindings

There is quite a selection of JavaScript environments these days:
node.js, Deno, Bun, Bare, and so on. The top issue with node-like
environments is *bloat*. What should be a simple scripting language
gradually became an elephantine monstruosity. Can we try to work
around the underlying forces that cause bloat? Why does it happen,
again and again? My working hypothesis is:

 1. Layered architecture,
 2. over-engineering, and
 3. market forces.

 The layering works like this:

 1. the JavaScript world,
 2. the JS VM world (V8, JavaScriptCore),
 3. the node.js/deno/etc world,
 4. the system-library layer (incl. libuv)
 5. the POSIX layer (also Windows).

Each of the layers tends to have its own programming language, 
its own memory management theory, I/O abstraction layer, its own 
package/dependency management. Each layer is a separate universe.
We will try to compact that into three layers:

 1. the JavaScript world,
 2. the system-library layer (incl libv8/libjavascriptcore),
 3. the POSIX layer.

Essentially, these bindings are the glue inbetween layers.
They hold no memory, use no special programming language,
and so on. The particular solutions used are listed below.
