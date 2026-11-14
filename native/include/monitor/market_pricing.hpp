#pragma once

#include "monitor/market_data.hpp"

#include <string>

namespace monitor::market {

struct FxRateQuote {
    std::string base_currency;
    std::string quote_currency;
    double bid{}; // units of quote currency for one unit of base currency
    double ask{};
};

struct FxConversion {
    std::string from_currency;
    std::string to_currency;
    double input_amount{};
    double gross_output{};
    double fee{};
    double net_output{};
    double execution_rate{};
    double mid_rate{};
    double spread_basis_points{};
};

// Percentage fee is charged on gross output; fixed_fee is denominated in
// the output currency. Direct conversion sells the base at bid; inverse
// conversion buys the base at ask (reciprocal rate).
[[nodiscard]] FxConversion convert_currency(
    double amount, const std::string& from_currency,
    const std::string& to_currency, const FxRateQuote& quote,
    double fee_percent = 0.0, double fixed_fee = 0.0);

struct GoldValuation {
    std::string currency;
    double input_quantity{};
    Unit input_unit{Unit::Gram};
    double gross_mass_grams{};
    double fine_gold_grams{};
    double gross_value{};
    double fee{};
    double net_value{};
};

// price_per_fine_gram is the reference price of one gram of pure gold.
// Purity is a fraction in [0, 1], e.g. 0.585 for 585/1000.
[[nodiscard]] GoldValuation value_gold(
    double quantity, Unit quantity_unit, double price_per_fine_gram,
    const std::string& currency, double purity = 1.0,
    double fee_percent = 0.0, double fixed_fee = 0.0);

} // namespace monitor::market
