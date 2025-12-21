const char* EULERZ_TEST =
    "{"
    "(),"
    // ...that applies to arrays as well...
    "[ 1, 2, 3 ],"
    "[ @a-10 2, 3, 4, 5 ],"
    "[ @a-20 3 1,2,3 ],"
    // Eulerian collections: by their id.
    "{ 1, 2, 3,  },"
    "{ @e-10 1, 2, 3 },"
    "{ @e-20 },"
    // ...as well as to multiplexed collections.
    "< 1@a1ec-10, 2@b0b-1 >,"
    "< @b0b-0 1@a1ec-1, 2@b0b-1 >,"
    "<@b0b-10 ~@b0b-2, ~@a1ec-3 >,"
    "<@a1ec-10 ~@b0b-3, ~@a1ec-3 >,"
    // Empty tuple is the leastest
    // Floats get ordered numerically.
    "-0.123,"
    "1.23,"
    "1.24 @a1ec-1,"
    // Integers are like floats.
    "-3,"
    "-2,"
    "1,"
    "2,"
    "3 @b0b-1,"
    // A tuple is ordered the same as its key.
    "4:5,"
    // References get ordered in the "Lamport order" (seq, then src).
    "b0b-20,"  // 20
    "b0b-30 @b0b-4,"
    "b0b-40 @b0b-3,"
    "b0b-50 @a1ec-1,"
    "a1ec-70,"
    // Strings get ordered lexicographically (strcmp wise).
    "\"Alice\","
    "\"Bob\" @b0b-1,"
    "\"Bobby\","
    "\"Carol\","
    // Terms: same as strings.
    "false,"
    "once:twice,"  // 30
    "one:two:three,"
    "true,"
    "}";
