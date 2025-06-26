#include "LedgerService.hpp"
#include "../net/Connection.hpp"
#include <unordered_map>

namespace CoopNet
{

struct LedgerKey
{
    Connection* conn;
    uint64_t nonce;
};
struct KeyHash
{
    size_t operator()(const LedgerKey& k) const noexcept
    {
        return reinterpret_cast<size_t>(k.conn) ^ std::hash<uint64_t>{}(k.nonce);
    }
};
struct KeyEq
{
    bool operator()(const LedgerKey& a, const LedgerKey& b) const noexcept
    {
        return a.conn == b.conn && a.nonce == b.nonce;
    }
};

static std::unordered_set<LedgerKey, KeyHash, KeyEq> g_processed;

bool Ledger_Transfer(Connection* conn, int64_t delta, uint64_t nonce, uint64_t& outBalance)
{
    LedgerKey key{conn, nonce};
    if (g_processed.find(key) != g_processed.end())
        return false;
    if (delta < 0 && conn->balance < static_cast<uint64_t>(-delta))
        return false;
    conn->balance += delta;
    outBalance = conn->balance;
    g_processed.insert(key);
    return true;
}

} // namespace CoopNet
