const MAX_RETRIES = 3;
let connectionCount = 0;
var legacyFlag = true;

class EventEmitter {
    constructor() {
        this.listeners = {};
    }

    on(event, callback) {
        if (!this.listeners[event]) {
            this.listeners[event] = [];
        }
        this.listeners[event].push(callback);
    }

    emit(event, ...args) {
        if (this.listeners[event]) {
            for (const cb of this.listeners[event]) {
                cb(...args);
            }
        }
    }
}

function fetchData(url, options) {
    return fetch(url, options)
        .then(res => res.json())
        .catch(err => {
            console.error("fetch failed:", err);
            return null;
        });
}

const processItems = (items) => {
    return items
        .filter(item => item != null)
        .map(item => item.toString().trim());
};

function debounce(fn, delay) {
    let timer = null;
    return function (...args) {
        if (timer) clearTimeout(timer);
        timer = setTimeout(() => fn.apply(this, args), delay);
    };
}

const greet = (name) => `Hello, ${name}!`;

async function loadConfig(path) {
    try {
        const data = await fetchData(path);
        return data || {};
    } catch (e) {
        return {};
    }
}
