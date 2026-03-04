import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const bason = require('..');

describe('bason.parse / stringify roundtrip', () => {
  const cases = [
    '{"a":"b","c":"d"}',
    '[1,2,3]',
    '{"active":true,"addr":{"city":"NYC"},"age":30,"name":"Alice","scores":[100,200,300],"x":null}',
    '"hello"',
    '42',
    'true',
    'false',
    'null',
    '[]',
    '{}',
  ];
  for (const json of cases) {
    it(`roundtrips: ${json.slice(0, 40)}`, () => {
      const doc = bason.parse(json);
      const out = doc.toJSON();
      assert.equal(out, json);
      assert.equal(bason.stringify(doc), json);
    });
  }
});

describe('iteration', () => {
  it('iterates object children with for...of', () => {
    const doc = bason.parse('{"a":"hello","b":42,"c":true,"d":null}');
    const entries = [];
    for (const e of doc) {
      entries.push(e);
    }
    // Keys are sorted: a, b, c, d
    assert.equal(entries.length, 4);
    assert.equal(entries[0].key, 'a');
    assert.equal(entries[0].type, 's');
    assert.equal(entries[0].value, 'hello');

    assert.equal(entries[1].key, 'b');
    assert.equal(entries[1].type, 'n');
    assert.equal(entries[1].value, 42);

    assert.equal(entries[2].key, 'c');
    assert.equal(entries[2].type, 'b');
    assert.equal(entries[2].value, true);

    assert.equal(entries[3].key, 'd');
    assert.equal(entries[3].type, 'b');
    assert.equal(entries[3].value, null);
  });

  it('iterates array children', () => {
    const doc = bason.parse('[10,20,30]');
    const entries = [];
    for (const e of doc) {
      entries.push(e);
    }
    assert.equal(entries.length, 3);
    assert.equal(entries[0].type, 'n');
    assert.equal(entries[0].value, 10);
    assert.equal(entries[1].value, 20);
    assert.equal(entries[2].value, 30);
  });
});

describe('nested iteration', () => {
  it('iterates nested objects and arrays', () => {
    const doc = bason.parse('{"arr":[1,2],"obj":{"x":"y"}}');
    const entries = [];
    for (const e of doc) {
      if (e.type === 'a' || e.type === 'o') {
        const children = [];
        for (const child of e.value) {
          children.push(child);
        }
        entries.push({ key: e.key, type: e.type, children });
      } else {
        entries.push(e);
      }
    }
    // sorted: arr, obj
    assert.equal(entries.length, 2);
    assert.equal(entries[0].key, 'arr');
    assert.equal(entries[0].children.length, 2);
    assert.equal(entries[0].children[0].value, 1);
    assert.equal(entries[0].children[1].value, 2);

    assert.equal(entries[1].key, 'obj');
    assert.equal(entries[1].children.length, 1);
    assert.equal(entries[1].children[0].key, 'x');
    assert.equal(entries[1].children[0].value, 'y');
  });
});

describe('get()', () => {
  it('looks up keys', () => {
    const doc = bason.parse('{"name":"Alice","scores":[95,87]}');
    assert.equal(doc.get('name'), 'Alice');
  });

  it('returns undefined for missing keys', () => {
    const doc = bason.parse('{"a":1}');
    assert.equal(doc.get('z'), undefined);
  });
});

describe('merge', () => {
  it('right-wins merge', () => {
    const a = bason.parse('{"x":1,"y":2}');
    const b = bason.parse('{"y":3,"z":4}');
    const m = bason.merge(a, b);
    const json = m.toJSON();
    const obj = JSON.parse(json);
    assert.equal(obj.x, 1);
    assert.equal(obj.y, 3);
    assert.equal(obj.z, 4);
  });

  it('n-way merge', () => {
    const a = bason.parse('{"x":1}');
    const b = bason.parse('{"y":2}');
    const c = bason.parse('{"x":3,"z":4}');
    const m = bason.mergeN([a, b, c]);
    const obj = JSON.parse(m.toJSON());
    assert.equal(obj.x, 3);
    assert.equal(obj.y, 2);
    assert.equal(obj.z, 4);
  });
});

describe('diff', () => {
  it('diff + merge roundtrip', () => {
    const old = bason.parse('{"a":1,"b":2,"c":3}');
    const nw = bason.parse('{"a":1,"b":99,"d":4}');
    const patch = bason.diff(old, nw);
    const merged = bason.merge(old, patch);
    const obj = JSON.parse(merged.toJSON());
    assert.equal(obj.a, 1);
    assert.equal(obj.b, 99);
    assert.equal(obj.d, 4);
    assert.equal(obj.c, undefined);
  });
});

describe('from() / buffer', () => {
  it('roundtrips through buffer', () => {
    const doc = bason.parse('{"x":1}');
    const buf = doc.buffer;
    assert.ok(buf instanceof Uint8Array);
    assert.ok(buf.length > 0);

    const doc2 = bason.from(buf);
    assert.equal(doc2.toJSON(), '{"x":1}');
  });
});
