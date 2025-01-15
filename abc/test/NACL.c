#include <NACL.h>
#include <TEST.h>

ok64 NACLtest() {
    sane(1);
    edpub256 publicKey;
    edsec512 secretKey;
    edpub256 publicKey2;
    edsec512 secretKey2;
    call(NACLed25519create, &publicKey, &secretKey);
    call(NACLed25519create, &publicKey2, &secretKey2);
    $u8c value = $u8str("Hello world!\n");
    sha256 hash = {};
    SHAsum(&hash, value);
    edsig512 sign;
    call(NACLed25519sign, &sign, &hash, &secretKey);
    call(NACLed25519verify, &sign, &hash, &publicKey);
    ok64 o = NACLed25519verify(&sign, &hash, &publicKey2);
    testeq(o, NACLbad);
    done;
}

TEST(NACLtest);
