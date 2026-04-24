#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <algorithm>

struct dynamic_bitset {
    dynamic_bitset() = default;
    ~dynamic_bitset() = default;
    dynamic_bitset(const dynamic_bitset &) = default;
    dynamic_bitset &operator=(const dynamic_bitset &) = default;

    explicit dynamic_bitset(std::size_t n) { resize_bits(n); }
    explicit dynamic_bitset(const std::string &str) {
        nbits = str.size();
        blocks.assign((nbits + 63) / 64, 0ULL);
        for (std::size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '1') set_bit(i);
        }
    }

    bool operator[](std::size_t n) const {
        if (n >= nbits) return false;
        std::size_t idx = n >> 6, off = n & 63U;
        return (blocks[idx] >> off) & 1ULL;
    }

    dynamic_bitset &set(std::size_t n, bool val = true) {
        if (n >= nbits) return *this;
        std::size_t idx = n >> 6, off = n & 63U;
        if (val) blocks[idx] |= (1ULL << off);
        else blocks[idx] &= ~(1ULL << off);
        return *this;
    }

    dynamic_bitset &push_back(bool val) {
        std::size_t idx = nbits >> 6;
        std::size_t off = nbits & 63U;
        if (off == 0) blocks.push_back(0ULL);
        if (val) blocks[idx] |= (1ULL << off);
        ++nbits;
        return *this;
    }

    bool none() const {
        if (nbits == 0) return true;
        std::size_t full = nbits >> 6;
        for (std::size_t i = 0; i < full; ++i) if (blocks[i] != 0ULL) return false;
        std::size_t rem = nbits & 63U;
        if (rem == 0) return true;
        uint64_t mask = (1ULL << rem) - 1ULL;
        return (blocks[full] & mask) == 0ULL;
    }

    bool all() const {
        std::size_t full = nbits >> 6;
        for (std::size_t i = 0; i < full; ++i) if (blocks[i] != ~0ULL) return false;
        std::size_t rem = nbits & 63U;
        if (rem == 0) return true;
        uint64_t mask = (1ULL << rem) - 1ULL;
        return (blocks[full] & mask) == mask;
    }

    std::size_t size() const { return nbits; }

    dynamic_bitset &operator|=(const dynamic_bitset &other) {
        apply_bitop_partial(other, [](uint64_t a, uint64_t b) { return a | b; });
        return *this;
    }

    dynamic_bitset &operator&=(const dynamic_bitset &other) {
        apply_bitop_partial(other, [](uint64_t a, uint64_t b) { return a & b; });
        return *this;
    }

    dynamic_bitset &operator^=(const dynamic_bitset &other) {
        apply_bitop_partial(other, [](uint64_t a, uint64_t b) { return a ^ b; });
        return *this;
    }

    dynamic_bitset &operator<<=(std::size_t n) {
        if (nbits == 0 || n == 0) { nbits += n; ensure_blocks(); return *this; }
        std::vector<uint64_t> old = blocks;
        nbits += n;
        ensure_blocks();
        std::fill(blocks.begin(), blocks.end(), 0ULL);
        for (std::size_t i = 0; i < old.size(); ++i) {
            uint64_t v = old[i];
            if (v == 0ULL) continue;
            std::size_t dest = (i << 6) + n;
            std::size_t j = dest >> 6, r = dest & 63U;
            blocks[j] |= (v << r);
            if (r && j + 1 < blocks.size()) blocks[j + 1] |= (v >> (64 - r));
        }
        mask_tail_bits();
        return *this;
    }

    dynamic_bitset &operator>>=(std::size_t n) {
        if (n == 0) return *this;
        if (n >= nbits) { nbits = 0; blocks.clear(); return *this; }
        std::size_t new_bits = nbits - n;
        std::vector<uint64_t> newblk((new_bits + 63) / 64, 0ULL);
        for (std::size_t j = 0; j < newblk.size(); ++j) {
            std::size_t readPos = (j << 6) + n;
            std::size_t k = readPos >> 6;
            std::size_t r = readPos & 63U;
            uint64_t low = (k < blocks.size()) ? (blocks[k] >> r) : 0ULL;
            uint64_t high = (r && (k + 1 < blocks.size())) ? (blocks[k + 1] << (64 - r)) : 0ULL;
            newblk[j] = low | high;
        }
        blocks.swap(newblk);
        nbits = new_bits;
        mask_tail_bits();
        return *this;
    }

    dynamic_bitset &set() {
        std::size_t full = nbits >> 6;
        for (std::size_t i = 0; i < full; ++i) blocks[i] = ~0ULL;
        std::size_t rem = nbits & 63U;
        if (rem) {
            uint64_t mask = (1ULL << rem) - 1ULL;
            if (blocks.size() <= full) blocks.resize(full + 1, 0ULL);
            blocks[full] = mask;
        }
        return *this;
    }

    dynamic_bitset &flip() {
        for (auto &x : blocks) x = ~x;
        mask_tail_bits();
        return *this;
    }

    dynamic_bitset &reset() {
        for (auto &x : blocks) x = 0ULL;
        return *this;
    }

private:
    std::vector<uint64_t> blocks;
    std::size_t nbits{0};

    void resize_bits(std::size_t n) {
        nbits = n;
        blocks.assign((nbits + 63) / 64, 0ULL);
    }

    void ensure_blocks() {
        std::size_t need = (nbits + 63) / 64;
        if (blocks.size() < need) blocks.resize(need, 0ULL);
    }

    void set_bit(std::size_t n) {
        std::size_t idx = n >> 6, off = n & 63U;
        if (idx >= blocks.size()) blocks.resize(idx + 1, 0ULL);
        blocks[idx] |= (1ULL << off);
    }

    void mask_tail_bits() {
        std::size_t rem = nbits & 63U;
        if (rem == 0) return;
        std::size_t idx = nbits >> 6;
        if (idx < blocks.size()) {
            uint64_t mask = (1ULL << rem) - 1ULL;
            blocks[idx] &= mask;
        }
    }

    template <typename F>
    void apply_bitop_partial(const dynamic_bitset &other, F op) {
        std::size_t min_bits = std::min(nbits, other.nbits);
        if (min_bits == 0) return;
        std::size_t full = min_bits >> 6;
        for (std::size_t i = 0; i < full; ++i) blocks[i] = op(blocks[i], other.blocks[i]);
        std::size_t rem = min_bits & 63U;
        if (rem) {
            uint64_t mask = (1ULL << rem) - 1ULL;
            uint64_t a = blocks[full] & mask;
            uint64_t b = (other.blocks.size() > full ? other.blocks[full] : 0ULL) & mask;
            uint64_t res = op(a, b);
            blocks[full] = (blocks[full] & ~mask) | (res & mask);
        }
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int Q; if (!(std::cin >> Q)) return 0;
    dynamic_bitset bs;
    while (Q--) {
        std::string op; std::cin >> op;
        if (op == "init") {
            size_t n; std::cin >> n; bs = dynamic_bitset(n);
        } else if (op == "init_str") {
            std::string s; std::cin >> s; bs = dynamic_bitset(s);
        } else if (op == "get") {
            size_t i; std::cin >> i; std::cout << (bs[i] ? 1 : 0) << '\n';
        } else if (op == "set") {
            size_t i; int v; std::cin >> i >> v; bs.set(i, v != 0);
        } else if (op == "push") {
            int v; std::cin >> v; bs.push_back(v != 0);
        } else if (op == "none") {
            std::cout << (bs.none() ? 1 : 0) << '\n';
        } else if (op == "all") {
            std::cout << (bs.all() ? 1 : 0) << '\n';
        } else if (op == "size") {
            std::cout << bs.size() << '\n';
        } else if (op == "or") {
            std::string s; std::cin >> s; dynamic_bitset b(s); bs |= b;
        } else if (op == "and") {
            std::string s; std::cin >> s; dynamic_bitset b(s); bs &= b;
        } else if (op == "xor") {
            std::string s; std::cin >> s; dynamic_bitset b(s); bs ^= b;
        } else if (op == "shl") {
            size_t n; std::cin >> n; bs <<= n;
        } else if (op == "shr") {
            size_t n; std::cin >> n; bs >>= n;
        } else if (op == "setall") {
            bs.set();
        } else if (op == "flipall") {
            bs.flip();
        } else if (op == "resetall") {
            bs.reset();
        }
    }
    return 0;
}
