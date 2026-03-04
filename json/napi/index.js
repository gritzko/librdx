const path = require('path');
const dir = __dirname;
let addon;
try {
    addon = require(path.join(dir, 'build', 'Release', 'bason.node'));
} catch (e1) {
    try {
        addon = require(path.join(dir, 'build-manual', 'bason.node'));
    } catch (e2) {
        addon = require(path.join(dir, 'build', 'bason.node'));
    }
}
module.exports = addon;
