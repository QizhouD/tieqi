// 与 tq::Rng 相同：PCG32 XSH-RR，initseq = 1，uniform 取高位之外的 24 位再收成 float32。
const MASK64 = (1n << 64n) - 1n;
const MULT = 6364136223846793005n;

export function makePcg(seed) {
    let state = 0n;
    const increment = 3n;
    function next() {
        const old = state;
        state = (old * MULT + increment) & MASK64;
        const mixed = (((old >> 18n) ^ old) >> 27n) & 0xffffffffn;
        const xorshifted = Number(mixed);
        const rotation = Number(old >> 59n) & 31;
        return rotr(xorshifted, rotation);
    }
    next();
    state = (state + BigInt(seed)) & MASK64;
    next();
    return {
        next,
        uniform() {
            return Math.fround((next() >>> 8) / 16777216);
        },
    };
}

function rotr(value, shift) {
    const x = value >>> 0;
    const bits = shift & 31;
    if (bits === 0) {
        return x;
    }
    return ((x >>> bits) | (x << (32 - bits))) >>> 0;
}
