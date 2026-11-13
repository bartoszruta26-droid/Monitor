#include "monitor/market_pricing.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
template<class F> void throws(F&& fn, const char* message) {
    try { fn(); } catch (const std::exception&) { return; }
    std::cerr << "FAIL: expected exception: " << message << '\n';
    std::exit(EXIT_FAILURE);
}
bool near(double a, double b, double tolerance = 1e-9) {
    return std::abs(a - b) <= tolerance;
}
}

int main() {
    using namespace monitor::market;
    const FxRateQuote eur_pln{"EUR", "PLN", 4.20, 4.24};
    const auto sell_eur = convert_currency(100.0, "EUR", "PLN", eur_pln, 1.0, 2.0);
    require(near(sell_eur.execution_rate, 4.20), "selling base currency uses bid");
    require(near(sell_eur.gross_output, 420.0), "direct conversion gross output");
    require(near(sell_eur.fee, 6.2), "percentage and fixed fee use output currency");
    require(near(sell_eur.net_output, 413.8), "direct conversion net output");
    require(near(sell_eur.mid_rate, 4.22), "direct mid rate");
    require(near(sell_eur.spread_basis_points, (0.04 / 4.22) * 10000.0),
            "spread is expressed in basis points");

    const auto buy_eur = convert_currency(422.0, "PLN", "EUR", eur_pln);
    require(near(buy_eur.execution_rate, 1.0 / 4.24), "inverse conversion uses reciprocal ask");
    require(near(buy_eur.gross_output, 422.0 / 4.24), "inverse conversion amount");
    require(near(buy_eur.mid_rate, 2.0 / 8.44), "inverse mid rate");

    const auto zero = convert_currency(0.0, "EUR", "PLN", eur_pln);
    require(near(zero.net_output, 0.0), "zero amount without fees");
    throws([&] { (void)convert_currency(1, "EUR", "USD", eur_pln); },
           "quote must match requested currencies");
    throws([&] { (void)convert_currency(-1, "EUR", "PLN", eur_pln); },
           "negative amount");
    throws([&] { (void)convert_currency(1, "EUR", "PLN", {"EUR", "PLN", 4.3, 4.2}); },
           "crossed quote");
    throws([&] { (void)convert_currency(1, "EUR", "PLN", eur_pln, 0, 500); },
           "fees exceeding gross output");
    throws([&] { (void)convert_currency(1, "EUR", "PLN", eur_pln, 101, 0); },
           "percentage fee above 100");
    throws([&] { (void)convert_currency(1, "PLN", "EUR",
                {"EUR", "PLN", 1e-320, 1e-320}); },
           "inverse quote reciprocal overflow");
    const auto wide_quote = convert_currency(1.0, "AAA", "BBB",
                {"AAA", "BBB", 1e308, 1.7e308});
    require(std::isfinite(wide_quote.mid_rate) &&
            std::isfinite(wide_quote.spread_basis_points),
            "large finite quote avoids intermediate sum overflow");

    const auto gold = value_gold(10.0, Unit::Gram, 300.0, "PLN", 0.585, 2.0, 1.0);
    require(near(gold.gross_mass_grams, 10.0), "gold mass in grams");
    require(near(gold.fine_gold_grams, 5.85), "gold purity applied to fine mass");
    require(near(gold.gross_value, 1755.0), "gold gross reference valuation");
    require(near(gold.fee, 36.1), "gold fees are explicit");
    require(near(gold.net_value, 1718.9), "gold net valuation");

    const auto ounce = value_gold(1.0, Unit::TroyOunce, 300.0, "USD");
    require(near(ounce.gross_mass_grams, 31.1034768), "troy ounce mass conversion");
    require(near(ounce.gross_value, 31.1034768 * 300.0), "ounce valuation");
    throws([&] { (void)value_gold(-1, Unit::Gram, 300, "PLN"); }, "negative gold quantity");
    throws([&] { (void)value_gold(1, Unit::Gram, 300, "PLN", 1.01); }, "purity above one");
    throws([&] { (void)value_gold(1, Unit::Barrel, 300, "PLN"); }, "non-mass unit");
    throws([&] { (void)value_gold(1, Unit::Gram, 300, "PL"); }, "invalid currency");
    std::cout << "All FX conversion and gold valuation tests passed.\n";
}
