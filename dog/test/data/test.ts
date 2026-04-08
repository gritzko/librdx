interface Logger {
    info(msg: string): void;
    error(msg: string): void;
}

class ConsoleLogger implements Logger {
    private prefix: string;

    constructor(prefix: string) {
        this.prefix = prefix;
    }

    info(msg: string): void {
        console.log(`[${this.prefix}] ${msg}`);
    }

    error(msg: string): void {
        console.error(`[${this.prefix}] ERROR: ${msg}`);
    }
}

const DEFAULT_TIMEOUT = 5000;
let requestCount = 0;

function createLogger(name: string): Logger {
    return new ConsoleLogger(name);
}

const identity = <T>(x: T): T => x;

async function fetchJSON<T>(url: string): Promise<T> {
    const response = await fetch(url);
    if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
    }
    return response.json() as Promise<T>;
}

class Cache<K, V> {
    private store: Map<K, V>;

    constructor() {
        this.store = new Map();
    }

    get(key: K): V | undefined {
        return this.store.get(key);
    }

    set(key: K, value: V): void {
        this.store.set(key, value);
    }
}

function main(): void {
    const log = createLogger("app");
    log.info("started");
    const cache = new Cache<string, number>();
    cache.set("x", 42);
    log.info(`cached: ${cache.get("x")}`);
}
