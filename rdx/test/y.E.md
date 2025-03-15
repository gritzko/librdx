```
The merge rules for an Eulerian set are rather straightforward.
```
```
Versions of the same set get merged, element by element.
```
{@alices-A 1 2 4}
{@alices-A 1 3 5}
    ~
{@alices-A 1 2 3 4 5}
```
That applies to no-id sets as well: they count as the same set.
```
{1},
{1},
    ~,
{1},

{1 3},
{4 5},
    ~,
{1,3,4,5},

{4 5},
{1 3},
    ~,
{1,3,4,5},

{1:2},
{3:4},
~,
{1:2,3:4},

{1:2, 3:4},
{1:1, 3:5, 4:5},
~,
{1:2, 3:5, 4:5},

{1:2, 3:{4,5}},
{1:2, 3:6, 7:8},
~,
{1:2, 3:{4,5}, 7:8},

```
The rules apply recursively. When we merge two versions of a set,
versions of its elements also get merged.
```
{1:2, 3:{4,5}},
{1:2, 3:{4:10}, 7:8},
~,
{1:2, 3:{4:10,5}, 7:8},

```
The above rules applied recursively again:
```
{},
{{}},
{{{}}},
~,
{{{}}},

{1:<2:3>},
{1:<2:3:4>},
{2:<5:6>},
~,
{1:<2:3:4>,2:<5:6>},

{1:<2:3>},
{1:<2:3:4>, 2:<5:6>},
~,
{1:<2:3:4>,2:<5:6>},

{2:<5:6>},
{1:<2:3:4>},
{1:<2:3>},
~,
{1:<2:3:4>,2:<5:6>},

{2:<5:6>},
{1:<2:3>},
{1:<2:3:4>},
{1:<2:3:4>},
{2:<5:6>},
{1:<2:3>},
~,
{1:<2:3:4>,2:<5:6>},

```
Different sets (different ids) never get merged; it is either one or another.
```
{@alices-A 1 2 3}
{@bobs-B 4 5}
    ~
{@bobs-B 4 5}

```
In the example above, the bigger id wins.
In case we want to replace one set by another, we use a revision-envelope.

This operation is only necessary when our model is not a clean tree, but
we also need to include other objects by reference and to "redirect" those
references at times.
```
<{@bobs-B 4 5}>
<@2 {@alices-A 1 2 3}>
    ~
<@2 {@alices-A 1 2 3}>

```
Revision-envelopes with odd numbers count as tombstones.
So, set deletion looks like:
```
<{@alices-A 1 2 3}>
<@1 {@alices-A}>
    ~
<@1 {@alices-A}>

```
Correspondingly, here is undeletion:
```
<@1 {@alices-A 1 2 3}>
<@2 {@alices-A 1 2 3}>
    ~
<@2 {@alices-A 1 2 3}>

```
Again, enveloping is only relevant when our data model is not
a clean JSON-like tree, but we have to include arbitrary objects
by reference. If you want to keep things simple, use trees.
```

