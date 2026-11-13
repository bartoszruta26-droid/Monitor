#include "monitor/market_pricing.hpp"

#include <cmath>
#include <stdexcept>

namespace monitor::market {
namespace {
bool currency_code(const std::string& value) {
    if (value.size() != 3U) return false;
    for (const char c : value) if (c < 'A' || c > 'Z') return false;
    return true;
}
void validate_fees(double percent, double fixed_fee) {
    if (!std::isfinite(percent) || percent < 0.0 || percent > 100.0 ||
        !std::isfinite(fixed_fee) || fixed_fee < 0.0) {
        throw std::invalid_argument("Nieprawidłowa prowizja lub opłata stała.");
    }
}
double net_after_fees(double gross, double percent, double fixed_fee,
                      double& fee) {
    fee = gross * (percent / 100.0) + fixed_fee;
    if (!std::isfinite(fee) || fee > gross) {
        throw std::invalid_argument("Opłaty przekraczają wartość operacji.");
    }
    const double net = gross - fee;
    if (!std::isfinite(net)) throw std::overflow_error("Przekroczono zakres wyceny.");
    return net;
}
}

FxConversion convert_currency(double amount, const std::string& from,
                              const std::string& to, const FxRateQuote& q,
                              double fee_percent, double fixed_fee) {
    if (!std::isfinite(amount) || amount < 0.0)
        throw std::invalid_argument("Kwota wymiany musi być skończona i nieujemna.");
    if (!currency_code(from) || !currency_code(to) || from == to ||
        !currency_code(q.base_currency) || !currency_code(q.quote_currency) ||
        q.base_currency == q.quote_currency) {
        throw std::invalid_argument("Nieprawidłowa para walutowa.");
    }
    if (!std::isfinite(q.bid) || !std::isfinite(q.ask) ||
        q.bid <= 0.0 || q.ask <= 0.0 || q.bid > q.ask) {
        throw std::invalid_argument("Kwotowanie FX wymaga 0 < bid <= ask.");
    }
    validate_fees(fee_percent, fixed_fee);

    double execution_rate{};
    double mid_rate{};
    if (from == q.base_currency && to == q.quote_currency) {
        execution_rate = q.bid;
        mid_rate = (q.bid + q.ask) / 2.0;
    } else if (from == q.quote_currency && to == q.base_currency) {
        execution_rate = 1.0 / q.ask;
        mid_rate = 2.0 / (q.bid + q.ask);
    } else {
        throw std::invalid_argument("Kwotowanie nie obsługuje wskazanego kierunku wymiany.");
    }
    const double gross = amount * execution_rate;
    if (!std::isfinite(gross)) throw std::overflow_error("Przekroczono zakres wymiany.");
    double fee{};
    const double net = net_after_fees(gross, fee_percent, fixed_fee, fee);
    const double spread_bps = ((q.ask - q.bid) / ((q.ask + q.bid) / 2.0)) * 10000.0;
    return {from, to, amount, gross, fee, net, execution_rate, mid_rate, spread_bps};
}

GoldValuation value_gold(double quantity, Unit unit, double price_per_fine_gram,
                         const std::string& currency, double purity,
                         double fee_percent, double fixed_fee) {
    if (!std::isfinite(quantity) || quantity < 0.0 ||
        !std::isfinite(price_per_fine_gram) || price_per_fine_gram < 0.0 ||
        !std::isfinite(purity) || purity < 0.0 || purity > 1.0 ||
        !currency_code(currency)) {
        throw std::invalid_argument("Nieprawidłowe parametry wyceny złota.");
    }
    validate_fees(fee_percent, fixed_fee);
    const double grams = convert_quantity(quantity, unit, Unit::Gram);
    const double fine_grams = grams * purity;
    const double gross = fine_grams * price_per_fine_gram;
    if (!std::isfinite(fine_grams) || !std::isfinite(gross))
        throw std::overflow_error("Przekroczono zakres wyceny złota.");
    double fee{};
    const double net = net_after_fees(gross, fee_percent, fixed_fee, fee);
    return {currency, quantity, unit, grams, fine_grams, gross, fee, net};
}

} // namespace monitor::market
