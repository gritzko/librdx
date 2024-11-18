  # COMB common buffer

Sometimes, a buffer has to be shared between processes
or its progress must be saved.
COMB reserves the first 32 bytes of the buffer to store
the lengths of its PAST, DATA and IDLE sections.
That way, processes may signal their progress to each
other or the state of a saved buffer can be restored.
