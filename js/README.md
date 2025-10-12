#   JABC: JavaScriptCore bindings

<img align=right width="25%" src="./logo.jpg"/>
There is quite a selection of JavaScript environments these days:
node.js, Deno, Bun, Bare, and so on. The top issue with node-like
environments is *bloat*. What should be a simple scripting language
gradually became an elephantine monstrosity. Can we try to work
around the underlying forces that cause bloat? Why does it happen,
again and again? My working hypothesis is that

  - layered architecture and
  - over-engineering, combined with
  - the market forces, cause bloat.

 The layering works like this:

 1. the JavaScript world,
 2. the JS VM world (V8, JavaScriptCore),
 3. the node.js/deno/etc bindings world,
 4. the system-library layer (incl. `libuv`),
 5. the POSIX layer (also Windows).

Each of the layers tends to have its own programming language, 
its own memory management theory, I/O abstraction layer, its own
package/dependency management. Each layer is a separate universe.
We will try to compact that into three layers:

 1. the JavaScript world,
 2. the system-library layer (incl libv8/libjavascriptcore),
 3. the POSIX layer.

Essentially, the bindings become the glue in between the layers.
They hold no memory, use no special programming language,
and so on. The particular solutions used are listed below.

 1. Use the stock libjavascriptcore, do not bundle your own.
    Linuses and Apples all have it. Do not make the Electron
    mistake. Building and shipping your own copy is a lot of
    work and a lot of overhead. Advanced users would likely
    make their own build anyway.
 2. `poll()` is probably enough. As a historical note, I was 
    using a node.js-like technology at least a year before
    node.js was released in 2009. I needed it to crawl large
    BitTorrent swarms for scientific purposes. For thousands
    of low-traffic connections that was really necessary.
    The perceived need to manage more than a thousand conns 
    per a thread requires the use of platform-specific APIs.
    The resulting `libuv` layer allocates and tries to manage 
    buffers, being unable to track their entire life cycle.
    That is a source of many problems. Also, our `poll()` use is
    covered by a C sugar wrapper, so `epoll()` is no biggie.
 3. Do not toss buffers across layers. That is a safety hazard
    and voids the warranty on your layers. In 2025, JavaScript
    has ArrayBuffer/TypedArrays. In 2009 it had not. We can
    let JavaScript manage the buffers, no shared custody.
 4. Minimize the number of JS object references held in the
    C land to the minimum: only keep the bootstrap points.
    Do not stash the callbacks, values and suchlike.
    This clear separation between layers improves many things,
    including security.
 5. Running threads for disk reads the `libuv` way is likely
    no longer necessary. `libuv` quotes Arvid Norberg on that. 
    Again, that was the 2008/2010 era and Arvid authored a
    high-performance low-overhead BitTorrent client. People
    had HDDs back in those days. These days we all have NVMe 
    and our data quite likely fits in memory anyway.
 6. Because of the latter fact, let JavaScript use mmap.
    Asynchrony in file system access is a huge stumble point.
 7. The UTF8/UTF16 mismatch is a pain.

 This list will be appended as the story develops.
