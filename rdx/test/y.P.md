""`
This file tests tuple merge rules.
The rules are simple...
```

```
...between two tuples, the newer wins (i.e. higher id)
```

1@1:2
1@2:2:3
1@1:4
    ~
1@2:2:3


```
...for the same tuple (same id), elements in the same position get merged
```
1@1:2
1@1:2:3
1@1:4
    ~
1@1:4:3

1:2
2:3
    ~
2:3

1:2:3
1:4:5
    ~
1:4:5

1:<2,3>
1:<2,4>
1:<2,5>
    ~
1:<2,5>


```
...when merged with non-tuple elements, those are seen as 1-element tuples.
```
1 @1
<@2 2, 2>
    ~
<@2 2, 2>

<@1 1, 2>
2@2
    ~
2@2

