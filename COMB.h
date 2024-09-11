#ifndef ABC_COMB_H
#define ABC_COMB_H

#define COMBsize (sizeof(size_t) * 4)

#define COMBinit(buf)         \
    {                         \
        u64** b = (u64**)buf; \
        b[0] += 4;            \
        b[1] = b[0];          \
        b[2] = b[0];          \
    }

const char* COMBmagic = ("ABCCOMB");

#define COMBsave(buf)            \
    {                            \
        u64* c = (u64*)buf[0];   \
        c -= 4;                  \
        memcpy(c, COMBmagic, 8); \
        c[1] = buf[1] - buf[0];  \
        c[2] = buf[2] - buf[1];  \
        c[3] = buf[3] - buf[2];  \
    }

#define COMBload(buf)                \
    {                                \
        u64* c = (u64*)buf[0];       \
        c -= 4;                      \
        u8** b = (u8**)buf;          \
        b[1] = (u8*)(buf[0] + c[1]); \
        b[2] = (u8*)(buf[1] + c[2]); \
    }

#endif  // ABC_COMB_H
