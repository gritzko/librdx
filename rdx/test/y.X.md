`` `
X collections merge "normally".
Namely, X elements get sorted by the author, elements by the same author get merged.
For two versions of an element, higher revision wins.
For two variants of an element <same version>, higher value wins.
Enveloping rules are standard for all PLEX collections <delete/undelete etc>.
```
<1>,<3>,<2, 4@5>, ~, <4@5>,

<1>,<3>,<2, 4@b0b-5>, ~, <3, 4@b0b-5>,

<0@b0b-1, 0@a1ec-4>,
<0@b0b-3, 0@a1ec-2>,
    ~,
<0@b0b-3, 0@a1ec-4>,

<0@b0b-3, 0@a1ec-2>,
<0@b0b-1, 0@a1ec-4>,
    ~,
<0@b0b-3, 0@a1ec-4>,

<1@be-1, -4@a1e-4>,
<3@be-3, -2@a1e-2>,
<2@be-2, -3@a1e-3>,
    ~,
<3@be-3, -4@a1e-4>,

<3@be-3, -4@a1-4>,
<2@be-2, -3@a1-3>,
    ~,
<3@be-3, -4@a1-4>,

<1@be-1, -4@a-4>,
<3@be-3, -3@a-3>,
    ~,
<3@be-3, -4@a-4>,

```
Right, higher version wins
```
<1@be-1, -4@alex-4>,
<3@be-3, -2@alex-2>,
<2@be-2, -3@alex-3>,
<1@be-1, -4@alex-4>,
<3@be-3, -2@alex-2>,
<2@be-2, -3@alex-3>,
    ~,
<3@be-3, -4@alex-4>,

```
Here we delete an element
```
<1@b0b-1, 1234@a1ec-0>,
<2@b0b-2, <@a1ec-1 1234>>,
    ~,
<2@b0b-2, <@a1ec-1 1234>>,

